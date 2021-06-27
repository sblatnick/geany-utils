#include <geanyplugin.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include <stdlib.h>

GeanyPlugin *geany_plugin;
GeanyData *geany_data;

static GRegex *trim_file, *trim_regex;
static GtkWidget *entry, *panel, *label, *scrollable_table, *tree, *check_case;
static GtkTreeStore *list;
static GtkTreeIter row;
static gint row_pos;
static gint first_run = 1;
static const gchar *base_directory;
static GtkWidget *treebrowser_entry = NULL;

const gchar *conf;
gchar *executable;
static GKeyFile *config;
const gchar *DEFAULT_EXECUTABLE = "/usr/bin/ack-grep";
GtkWidget *executable_entry;

enum
{
  KB_QUICK_FIND,
  KB_GROUP
};

static GtkWidget* find_entry(GtkContainer *container)
{
  GtkWidget *entry = NULL;
  GList *node;
  GList *children = gtk_container_get_children(container);
  for(node = children; !entry && node; node = node->next) {
    if(GTK_IS_ENTRY(node->data) && strcmp(gtk_widget_get_tooltip_text(GTK_WIDGET(node->data)), "Addressbar for example '/projects/my-project'") == 0) {
      entry = node->data;
    }
    else if(GTK_IS_CONTAINER(node->data)) {
      entry = find_entry(node->data);
    }
  }
  g_list_free(children);
  return entry;
}

static void get_path()
{
  if(first_run == 1) {
    for(gint i = 0; i < gtk_notebook_get_n_pages(GTK_NOTEBOOK(geany_plugin->geany_data->main_widgets->sidebar_notebook)); i++) {
      GtkWidget *page;
      const gchar *page_name;

      page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(geany_plugin->geany_data->main_widgets->sidebar_notebook), i);
      page_name = gtk_notebook_get_tab_label_text(GTK_NOTEBOOK(geany_plugin->geany_data->main_widgets->sidebar_notebook), page);
      if(page_name && strcmp(page_name, "Tree Browser") == 0) {
        treebrowser_entry = find_entry(GTK_CONTAINER(page));
        break;
      }
    }
  }
  
  if(treebrowser_entry == NULL) {
    GeanyProject *project = geany_plugin->geany_data->app->project;
    if(project) {
      base_directory = project->base_path;
    }
    else {
      base_directory = geany->prefs->default_open_path;
    }  
  }
  else {
    base_directory = gtk_entry_get_text(GTK_ENTRY(treebrowser_entry));
  }
}

static void cell_data(GtkTreeViewColumn *tree_column, GtkCellRenderer *render, GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
  gchar *file;
  gtk_tree_model_get(model, iter, 2, &file, -1);
  gchar *clean_file = g_regex_replace(trim_file, file, -1, 0, "", 0, NULL);
  g_object_set(render, "text", clean_file, NULL);
  g_free(file);
  g_free(clean_file);
}

/*
static gboolean panel_focus_tab(GtkWidget *widget, GdkEvent *event, gpointer data)
{
  GeanyKeyBinding *kb = keybindings_lookup_item(GEANY_KEY_GROUP_FOCUS, GEANY_KEYS_FOCUS_SIDEBAR);
  if (kb != NULL && event->key.keyval == kb->key && (event->key.state & gtk_accelerator_get_default_mod_mask()) == kb->mods) {
    gint current = gtk_notebook_get_current_page(GTK_NOTEBOOK(geany_plugin->geany_data->main_widgets->sidebar_notebook));
    gint tab = gtk_notebook_page_num(GTK_NOTEBOOK(geany_plugin->geany_data->main_widgets->sidebar_notebook), panel);
    if(current == tab) {
      gtk_widget_grab_focus(entry);
      //TODO: or grab table entries, whatever was last focused
    }
  }
  return FALSE;
}
*/

static gboolean output_out(GIOChannel *channel, GIOCondition cond, gpointer type)
{
  if(cond == G_IO_HUP)
  {
    g_io_channel_unref(channel);
    return FALSE;
  }

  gchar *string, **column;
  GeanyDocument *doc = document_get_current();

  if((g_io_channel_read_line(channel, &string, NULL, NULL, NULL)) == G_IO_STATUS_NORMAL && string) {
    column = g_strsplit(string, ":", 3);
    gchar *code = column[2];
    column[2] = g_regex_replace(trim_regex, column[2], -1, 0, "", 0, NULL);
    g_free(code);
    gtk_tree_store_append(list, &row, NULL);
    gtk_tree_store_set(list, &row, 0, row_pos, 1, column[1], 2, column[0], 3, g_strstrip(column[2]), -1);
    g_strfreev(column);
    row_pos++;
  }
  g_free(string);

  return TRUE;
}

static void quick_find()
{
  gtk_tree_store_clear(list);
  const gchar *text = gtk_entry_get_text(GTK_ENTRY(entry));
  if(strcmp(text, "") == 0) {
    return;
  }
  
  get_path();
  row_pos = 1;
  
  gboolean case_sensitive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_case));
  
  GError *error = NULL;
  gint std_out, std_err;

  gchar **cmd;
  gchar *command = g_strconcat(executable, " ", case_sensitive ? "" : "-i ", g_shell_quote(text), NULL);
  if(!g_shell_parse_argv(command, NULL, &cmd, &error)) {
    ui_set_statusbar(TRUE, _("quick-find failed: %s (%s)"), error->message, command);
    g_error_free(error);
    g_free(command);
    return;
  }
  g_free(command);

  gchar **env = utils_copy_environment(
    NULL,
    "GEAN", "running from geany",
    NULL
  );

  if(g_spawn_async_with_pipes(
    base_directory,
    cmd,
    env,
    0, NULL, NULL, NULL, NULL,
    &std_out,
    &std_err,
    &error
  ))
  {
    #ifdef G_OS_WIN32
      GIOChannel *err_channel = g_io_channel_win32_new_fd(std_err);
      GIOChannel *out_channel = g_io_channel_win32_new_fd(std_out);
    #else
      GIOChannel *err_channel = g_io_channel_unix_new(std_err);
      GIOChannel *out_channel = g_io_channel_unix_new(std_out);
    #endif

    g_io_channel_set_encoding(out_channel, NULL, NULL);
    g_io_add_watch(out_channel, G_IO_IN | G_IO_HUP, (GIOFunc)output_out, GUINT_TO_POINTER(0));

    g_io_channel_set_encoding(err_channel, NULL, NULL);
    g_io_add_watch(err_channel, G_IO_IN | G_IO_HUP, (GIOFunc)output_out, GUINT_TO_POINTER(1));
  }
  else {
    printf("quick-find ERROR %s: %s\n", cmd[0], error->message);
    ui_set_statusbar(TRUE, _("quick-find ERROR %s: %s"), cmd[0], error->message);
    g_error_free(error);
  }
  g_free(cmd);
  g_free(env);
}

static void entry_focus(G_GNUC_UNUSED guint key_id)
{
  gtk_notebook_set_current_page(
    GTK_NOTEBOOK(geany_plugin->geany_data->main_widgets->sidebar_notebook),
    gtk_notebook_page_num(GTK_NOTEBOOK(geany_plugin->geany_data->main_widgets->sidebar_notebook), panel)
  );
  gtk_widget_grab_focus(entry);
}

static gboolean on_activate(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
  quick_find();
  return FALSE;
}

static void on_click(GtkButton* button, gpointer data)
{
  quick_find();
}

gboolean quick_goto_line(GeanyEditor *editor, gint line_no)
{
  gint pos;

  g_return_val_if_fail(editor, FALSE);
  if (line_no < 0 || line_no >= sci_get_line_count(editor->sci))
    return FALSE;

  pos = sci_get_position_from_line(editor->sci, line_no);
  return editor_goto_pos(editor, pos, TRUE);
}

static void selected_row(GtkTreeSelection *selected, gpointer data)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  if(gtk_tree_selection_get_selected(selected, &model, &iter))
  {
    gchar *line, *file;
    gtk_tree_model_get(model, &iter, 1, &line, 2, &file, -1);
    file = g_build_path(G_DIR_SEPARATOR_S, base_directory, file, NULL);
    GeanyDocument *doc = document_open_file(file, FALSE, NULL, NULL);
    quick_goto_line(doc->editor, atoi(line) - 1);

    g_free(line);
    g_free(file);
  }
}

static gboolean init(GeanyPlugin *plugin, gpointer pdata)
{
  geany_plugin = plugin;

  conf = g_build_path(G_DIR_SEPARATOR_S, geany_plugin->geany_data->app->configdir, "plugins", "quick-find.conf", NULL);
  config = g_key_file_new();
  g_key_file_load_from_file(config, conf, G_KEY_FILE_NONE, NULL);
  executable = utils_get_setting_string(config, "main", "executable", DEFAULT_EXECUTABLE);

  trim_file = g_regex_new(g_strconcat("^.*", G_DIR_SEPARATOR_S, NULL), G_REGEX_OPTIMIZE | G_REGEX_CASELESS, 0, NULL);
  trim_regex = g_regex_new("\n$", G_REGEX_OPTIMIZE | G_REGEX_CASELESS, 0, NULL);
  
  label = gtk_label_new(_("Find"));
  panel = gtk_vbox_new(FALSE, 6);
  scrollable_table = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollable_table), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  gtk_box_pack_start(GTK_BOX(panel), scrollable_table, TRUE, TRUE, 0);
  gtk_notebook_append_page(GTK_NOTEBOOK(geany_plugin->geany_data->main_widgets->sidebar_notebook), panel, label);
  
  entry = gtk_entry_new();
  gtk_entry_set_icon_from_icon_name(GTK_ENTRY(entry), GTK_ENTRY_ICON_SECONDARY, "edit-find");
  g_signal_connect(entry, "activate", G_CALLBACK(on_activate), NULL);
  
  GtkWidget *button_box = gtk_hbox_new(FALSE, 6);
  gtk_box_pack_start(GTK_BOX(button_box), entry, TRUE, TRUE, 0);
  
  GtkWidget *button = gtk_button_new_with_label(_("Find"));
  g_signal_connect(button, "clicked", G_CALLBACK(on_click), NULL);
  gtk_box_pack_end(GTK_BOX(button_box), button, FALSE, TRUE, 0);
  
  check_case = gtk_check_button_new_with_label(_("Case Sensitive"));
  gtk_widget_set_tooltip_text(check_case, _("Perform a case-sensitive search."));
  
  gtk_box_pack_end(GTK_BOX(panel), check_case, FALSE, TRUE, 0);
  gtk_box_pack_end(GTK_BOX(panel), button_box, FALSE, TRUE, 0);
  gtk_container_set_focus_child(GTK_CONTAINER(panel), entry);

  GtkTreeViewColumn *number_column, *line_column, *file_column, *text_column;
  GtkCellRenderer *render;
  
  list = gtk_tree_store_new(4, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
  tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list));

  render = gtk_cell_renderer_text_new();
  number_column = gtk_tree_view_column_new_with_attributes("#", render, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tree), number_column);
  gtk_tree_view_column_set_alignment(number_column, 1.0);
  gtk_cell_renderer_set_alignment(render, 1.0, 0.0);
  gtk_tree_view_column_add_attribute(number_column, render, "text", 0);
  
  render = gtk_cell_renderer_text_new();
  line_column = gtk_tree_view_column_new_with_attributes("Line", render, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tree), line_column);
  gtk_tree_view_column_set_alignment(line_column, 1.0);
  gtk_cell_renderer_set_alignment(render, 1.0, 0.0);
  gtk_tree_view_column_add_attribute(line_column, render, "text", 1);
  
  render = gtk_cell_renderer_text_new();
  file_column = gtk_tree_view_column_new_with_attributes("File", render, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tree), file_column);
  gtk_tree_view_column_add_attribute(file_column, render, "text", 2);
  gtk_tree_view_column_set_cell_data_func(file_column, render, (GtkTreeCellDataFunc)cell_data, NULL, NULL);
  
  render = gtk_cell_renderer_text_new();
  text_column = gtk_tree_view_column_new_with_attributes("Text", render, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tree), text_column);
  gtk_tree_view_column_add_attribute(text_column, render, "text", 3);

  g_object_unref(GTK_TREE_MODEL(list));
  GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
  g_signal_connect(select, "changed", G_CALLBACK(selected_row), NULL);
  
  gtk_container_add(GTK_CONTAINER(scrollable_table), tree);
  gtk_widget_show(label);
  gtk_widget_show_all(panel);
  
  //TODO: g_signal_connect(geany_plugin->geany_data->main_widgets->window, "key-release-event", G_CALLBACK(panel_focus_tab), NULL);

  GeanyKeyGroup *key_group;
  key_group = plugin_set_key_group(geany_plugin, "quick_find_keyboard_shortcut", KB_GROUP, NULL);
  keybindings_set_item(key_group, KB_QUICK_FIND, entry_focus, 0, 0, "quick_find", _("Quick Find..."), NULL);
}

static void dialog_response(GtkDialog *configure, gint response, gpointer user_data)
{
  if(response == GTK_RESPONSE_OK || response == GTK_RESPONSE_APPLY) {
    g_free(executable);
    executable = g_strdup(gtk_entry_get_text(GTK_ENTRY(executable_entry)));
    g_key_file_set_string(config, "main", "executable", executable);
  }
}

static void set_text(GtkButton* button, gchar* text)
{
  gtk_entry_set_text(GTK_ENTRY(executable_entry), text);
}

static GtkWidget* configure(GeanyPlugin *plugin, GtkDialog *configure, gpointer pdata)
{
  g_signal_connect(configure, "response", G_CALLBACK(dialog_response), NULL);
  GtkWidget *vbox = gtk_vbox_new(FALSE, 6);
  
  GtkWidget *label = gtk_label_new(_("Command line program for searching:"));
  gtk_label_set_xalign(GTK_LABEL(label), 0);
  gtk_label_set_yalign(GTK_LABEL(label), 0);
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 2);

  executable_entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(executable_entry), executable);
  GtkWidget *hbox = gtk_hbox_new(FALSE, 6);
  GtkWidget *button_grep = gtk_button_new_with_label(_("grep (Good)"));
  GtkWidget *button_ack = gtk_button_new_with_label(_("ack-grep (Better)"));
  GtkWidget *button_ag = gtk_button_new_with_label(_("silversearcher-ag (Best)"));
  g_signal_connect(button_grep, "clicked", G_CALLBACK(set_text), (gpointer)"/bin/grep -Rn");
  g_signal_connect(button_ack, "clicked", G_CALLBACK(set_text), (gpointer)"/usr/bin/ack-grep");
  g_signal_connect(button_ag, "clicked", G_CALLBACK(set_text), (gpointer)"/usr/bin/ag");
  gtk_box_pack_start(GTK_BOX(vbox), executable_entry, FALSE, FALSE, 2);
  gtk_box_pack_start(GTK_BOX(hbox), button_grep, FALSE, FALSE, 2);
  gtk_box_pack_start(GTK_BOX(hbox), button_ack, FALSE, FALSE, 2);
  gtk_box_pack_start(GTK_BOX(hbox), button_ag, FALSE, FALSE, 2);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);

  gtk_widget_show_all(vbox);

  return vbox;
}

static void cleanup(GeanyPlugin *plugin, gpointer pdata)
{
  gchar *data = g_key_file_to_data(config, NULL, NULL);
  utils_write_file(conf, data);
  g_free(data);
  g_key_file_free(config);

  gtk_notebook_remove_page(
    GTK_NOTEBOOK(geany_plugin->geany_data->main_widgets->sidebar_notebook),
    gtk_notebook_page_num(GTK_NOTEBOOK(geany_plugin->geany_data->main_widgets->sidebar_notebook), panel)
  );
  g_regex_unref(trim_file);
  g_regex_unref(trim_regex);
}

G_MODULE_EXPORT
void geany_load_module(GeanyPlugin *plugin)
{
  main_locale_init("share/locale", "quick-find");
  plugin->info->name = _("Quick Find");
  plugin->info->description = _("Quickly search documents based on the treebrowser root or project root using the_silver_searcher or ack-grep.");
  plugin->info->version = "0.2";
  plugin->info->author = "Steven Blatnick <steve8track@yahoo.com>";
  plugin->funcs->init = init;
  plugin->funcs->cleanup = cleanup;
  GEANY_PLUGIN_REGISTER(plugin, 225);
}
