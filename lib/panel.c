#include <geanyplugin.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>

#define foreach_slist_free(node, list) for (node = list, list = NULL; g_slist_free_1(list), node != NULL; list = node, node = node->next)

extern GeanyPlugin *geany_plugin;
extern GeanyData *geany_data;

extern const gchar *tools;

static GtkWidget *panel; //All contents of the panel
static GtkWidget *label;
static GtkWidget *text_view;
static GtkWidget *scrollable_text, *scrollable_table;

typedef struct
{
  const gchar *text;
  GRegex *regex;
} RegexSetting;
static RegexSetting fileRegexSetting = {"[\\w./-]+\\.[\\w./-]+(:\\d+)?", NULL};

gboolean panel_goto_line(GeanyEditor *editor, gint line_no)
  {
  gint pos;

  g_return_val_if_fail(editor, FALSE);
  if (line_no < 0 || line_no >= sci_get_line_count(editor->sci))
    return FALSE;

  pos = sci_get_position_from_line(editor->sci, line_no);
  return editor_goto_pos(editor, pos, TRUE);
}


static void open_path(gchar *path, gchar *line)
{
  GeanyDocument *doc = document_open_file(path, FALSE, NULL, NULL);
  if(line != NULL) {
    panel_goto_line(doc->editor, atoi(line) - 1);
  }
}

static gboolean find_files(gchar *base, const gchar *file, gchar* line, gboolean open)
{
  gchar *path = g_build_path(G_DIR_SEPARATOR_S, base, file, NULL);
  if(g_file_test(path, G_FILE_TEST_IS_REGULAR)) {
    if(open) {
      open_path(path, line);
    }
    g_free(path);
    return TRUE;
  }
  g_free(path);
  
  GDir *dir;
  gchar const *file_name;
  dir = g_dir_open(base, 0, NULL);
  
  foreach_dir(file_name, dir)
  {
    gchar *path = g_build_path(G_DIR_SEPARATOR_S, base, file_name, NULL);
    if(g_file_test(path, G_FILE_TEST_IS_DIR)) {
      if(find_files(path, file, line, open)) {
        g_dir_close(dir);
        g_free(path);
        return TRUE;
      }
    }
    g_free(path);
  }
  g_dir_close(dir);
  return FALSE;
}

static gboolean file_found(gchar *file_path, gboolean open)
{
  gchar **column = g_strsplit(file_path, ":", 2);
  gchar *file = column[0];
  gchar *line = column[1];

  //Find in abs path
  if(g_path_is_absolute(file) && g_file_test(file, G_FILE_TEST_IS_REGULAR)) {
    if(open) {
      open_path(file, line);
    }
    g_strfreev(column);
    return TRUE;
  }

  //Find in current doc path:
  GeanyDocument *doc = document_get_current();
  gchar *current = g_path_get_dirname(doc->file_name);
  if(find_files(current, file, line, open)) {
    g_free(current);
    g_strfreev(column);
    return TRUE;
  }
  g_free(current);

  //Find in Project Directory
  gchar *base_directory;
  GeanyProject *project = geany_plugin->geany_data->app->project;
  if(project) {
    base_directory = project->base_path;
  }
  else {
    base_directory = geany->prefs->default_open_path;
  }
  if(find_files(base_directory, file, line, open)) {
    g_strfreev(column);
    return TRUE;
  }
  g_strfreev(column);
  return FALSE;
}

static void goto_link(gchar *url)
{
  if(strcmp("", url) != 0) {
    file_found(url, TRUE);
  }
}

static gchar* get_link_at_iter(GtkTextIter in)
{
  GtkTextIter iter = in;
  gchar *text = (gpointer)"";
  GSList *list, *node;
  list = gtk_text_iter_get_tags(&iter);
  if(list != NULL) {
    foreach_slist_free(node, list) {
      GtkTextTag *tag = node->data;
      gchar *name;
      g_object_get(G_OBJECT(tag), "name", &name, NULL);
      if(strcmp(name, "link") == 0) {
        GtkTextIter end = iter;
        gtk_text_iter_backward_to_tag_toggle(&iter, tag);
        gtk_text_iter_forward_to_tag_toggle(&end, tag);
        text = gtk_text_iter_get_text(&iter, &end);
        g_free(name);
        break;
      }
      g_free(name);
    }
  }
  return text;
}

static gboolean on_click(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  GtkTextIter iter;
  gint x, y;
  gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(text_view), GTK_TEXT_WINDOW_TEXT, event->x, event->y, &x, &y);
  gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(text_view), &iter, x, y);
  
  gchar *text = get_link_at_iter(iter);
  goto_link(text);
  return FALSE;
}

static gboolean on_keypress(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
  if(event->keyval == GDK_Return) {
    GtkTextIter iter;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    GtkTextMark *cursor = gtk_text_buffer_get_mark(buffer, "insert");
    gtk_text_buffer_get_iter_at_mark(buffer, &iter, cursor);
    gchar *link = get_link_at_iter(iter);
    goto_link(link);
  }
  return FALSE;
}

/*
static gboolean panel_focus_tab(GtkWidget *widget, GdkEvent *event, gpointer data)
{
  GeanyKeyBinding *kb = keybindings_lookup_item(GEANY_KEY_GROUP_FOCUS, GEANY_KEYS_FOCUS_MESSAGE_WINDOW);
  if (kb != NULL && event->key.keyval == kb->key && (event->key.state & gtk_accelerator_get_default_mod_mask()) == kb->mods) {
    gint current = gtk_notebook_get_current_page(GTK_NOTEBOOK(geany_plugin->geany_data->main_widgets->message_window_notebook));
    gint tab = gtk_notebook_page_num(GTK_NOTEBOOK(geany_plugin->geany_data->main_widgets->message_window_notebook), panel);
    if(current == tab) {
      gtk_widget_grab_focus(scrollable_text);
    }
  }
  return FALSE;
}
*/

void panel_init()
{
  label = gtk_label_new(_("Tools"));
  panel = gtk_vbox_new(FALSE, 6);
  text_view = gtk_text_view_new();
  ui_widget_modify_font_from_string(GTK_WIDGET(text_view), "Monospace");
  gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
  scrollable_text = gtk_scrolled_window_new(NULL, NULL);
  scrollable_table = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollable_text), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollable_table), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  gtk_container_add(GTK_CONTAINER(scrollable_text), text_view);
  gtk_box_pack_start(GTK_BOX(panel), scrollable_table, TRUE, TRUE, 2);
  gtk_box_pack_start(GTK_BOX(panel), scrollable_text, TRUE, TRUE, 2);

  gtk_notebook_append_page(
    GTK_NOTEBOOK(geany_plugin->geany_data->main_widgets->message_window_notebook),
    panel,
    label
  );

  gtk_widget_show(label);
  gtk_widget_show(panel);
  gtk_widget_show_all(scrollable_text);
  gtk_container_set_focus_child(GTK_CONTAINER(panel), scrollable_text);

  //g_signal_connect(geany_plugin->geany_data->main_widgets->window, "key-release-event", G_CALLBACK(panel_focus_tab), NULL);
  g_signal_connect(text_view, "button-press-event", G_CALLBACK(on_click), NULL);
  g_signal_connect(text_view, "key-press-event", G_CALLBACK(on_keypress), NULL);

  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
  gtk_text_buffer_create_tag(buffer, "error", "foreground", "#ff0000", NULL);
  gtk_text_buffer_create_tag(buffer, "link", "foreground", "#0000ff", "underline", PANGO_UNDERLINE_SINGLE, NULL);
  
  fileRegexSetting.regex = g_regex_new(fileRegexSetting.text, G_REGEX_OPTIMIZE | G_REGEX_CASELESS, 0, NULL);
}

void panel_cleanup()
{
  gtk_notebook_remove_page(
    GTK_NOTEBOOK(geany_plugin->geany_data->main_widgets->message_window_notebook),
    gtk_notebook_page_num(GTK_NOTEBOOK(geany_plugin->geany_data->main_widgets->message_window_notebook), panel)
  );

  g_regex_unref(fileRegexSetting.regex);
}

void panel_prepare()
{
  gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view)), "", 0);
  gtk_widget_hide(scrollable_table);
}

void panel_print(gchar *text, const gchar *tag)
{
  GtkTextIter iter, start, end;
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
  gtk_text_buffer_get_end_iter(buffer, &iter);
  if(tag == NULL) {
    gtk_text_buffer_insert(buffer, &iter, text, -1);
  }
  else {
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, text, -1, tag, NULL);
  }
  end = iter;
  gtk_text_iter_backward_line(&iter);
  start = iter;

  gchar *line_text = gtk_text_iter_get_text(&start, &end);
  GMatchInfo *info;
  if(g_regex_match(fileRegexSetting.regex, line_text, 0, &info)) {
    while(g_match_info_matches(info))
    {
      gchar *file_path = g_match_info_fetch(info, 0);
      if(file_found(file_path, FALSE)) {
        gint start_pos, end_pos;
        g_match_info_fetch_pos(info, 0, &start_pos, &end_pos);
        GtkTextIter start = iter;
        GtkTextIter end = iter;
        gtk_text_iter_forward_chars(&start, start_pos);
        gtk_text_iter_forward_chars(&end, end_pos);
        gtk_text_buffer_apply_tag_by_name(buffer, "link", &start, &end);
      }
      g_free(file_path);
      g_match_info_next(info, NULL);
    }
  }  
  g_match_info_free(info);
  g_free(line_text);

  //Scroll to bottom:
  GtkTextMark *mark;
  mark = gtk_text_buffer_get_insert(buffer);
  gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(text_view), mark, 0.0, FALSE, 0.0, 0.0);
}
