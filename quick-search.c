#include <geanyplugin.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>

GeanyPlugin *geany_plugin;
GeanyData *geany_data;

static gint QUICK_SEARCH_INDICATOR = 20;
static gint SELECTED_SEARCH_INDICATOR = 21;

static GtkWidget *main_menu_item = NULL;
static GtkWidget *dialog, *entry;
static gulong handler;
static const gchar *text = "";
static const gchar *old;
static gboolean skip = FALSE;

static gint mark_all(GeanyDocument *doc, const gchar *search_text, gint indicator);

enum
{
  KB_QUICK_SEARCH,
  KB_QUICK_SEARCH_NEXT,
  KB_QUICK_SEARCH_PREV,
  KB_GROUP
};

typedef struct MatchInfo
{
  gint start, end;
}
MatchInfo;

static gchar* replace_all(const gchar *haystack, const gchar *needle, const gchar *replace)
{
  gchar *result;
  GString *str = g_string_new(haystack);
  utils_string_replace_all(str, needle, replace);
  result = utils_get_utf8_from_locale(str->str);
  g_string_free(str, TRUE);
  return result;
}

static gchar* escape(gchar *source)
{
  source = replace_all(source, "\n", "\\n");
  source = replace_all(source, "\r", "\\r");
  source = replace_all(source, "\t", "\\t");
  return source;
}

static const gchar* unescape(const gchar *source)
{
  source = replace_all(source, "\\n", "\n");
  source = replace_all(source, "\\r", "\r");
  source = replace_all(source, "\\t", "\t");
  return source;
}

static void quick_search(G_GNUC_UNUSED guint key_id)
{
  gint ox, oy, x, y;
  gdk_window_get_origin(gtk_widget_get_window(geany_plugin->geany_data->main_widgets->window), &ox, &oy);
  gtk_widget_translate_coordinates(geany_plugin->geany_data->main_widgets->notebook, geany_plugin->geany_data->main_widgets->window, 0, 0, &x, &y);
  gtk_window_move(GTK_WINDOW(dialog), ox + x, oy + y);

  gtk_widget_show_all(dialog);
  
  GeanyDocument *doc = document_get_current();
  
  if(sci_has_selection(doc->editor->sci)) {
    gchar *selected;
    //selected = g_malloc(sci_get_selected_text_length(doc->editor->sci) + 1);
    selected = sci_get_selection_contents(doc->editor->sci);

    sci_set_current_position(doc->editor->sci, sci_get_selection_start(doc->editor->sci), TRUE);
    scintilla_send_message(doc->editor->sci, SCI_SEARCHANCHOR, 0, 0);

    old = selected;
    selected = escape(selected);
    mark_all(doc, selected, QUICK_SEARCH_INDICATOR);
    gtk_entry_set_text(GTK_ENTRY(entry), selected);
    g_free(selected);
  }
  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1);
}

static void quick_next(G_GNUC_UNUSED guint key_id)
{
  GeanyDocument *doc = document_get_current();
  sci_set_current_position(doc->editor->sci, sci_get_selection_end(doc->editor->sci), TRUE);
  scintilla_send_message(doc->editor->sci, SCI_SEARCHANCHOR, 0, 0);
  scintilla_send_message(doc->editor->sci, SCI_SEARCHNEXT, (uptr_t) 0, (sptr_t) text);
  if(scintilla_send_message(doc->editor->sci, SCI_SEARCHNEXT, (uptr_t) 0, (sptr_t) text) == -1) {
    sci_set_current_position(doc->editor->sci, 0, TRUE);
    scintilla_send_message(doc->editor->sci, SCI_SEARCHANCHOR, 0, 0);
    scintilla_send_message(doc->editor->sci, SCI_SEARCHNEXT, (uptr_t) 0, (sptr_t) text);
  }
  skip = TRUE;
  doc->editor->scroll_percent = 0.3F;
}

static void quick_prev(G_GNUC_UNUSED guint key_id)
{
  GeanyDocument *doc = document_get_current();
  sci_set_current_position(doc->editor->sci, sci_get_selection_start(doc->editor->sci), TRUE);
  scintilla_send_message(doc->editor->sci, SCI_SEARCHANCHOR, 0, 0);
  scintilla_send_message(doc->editor->sci, SCI_SEARCHPREV, (uptr_t) 0, (sptr_t) text);
  if(scintilla_send_message(doc->editor->sci, SCI_SEARCHPREV, (uptr_t) 0, (sptr_t) text) == -1) {
    sci_set_current_position(doc->editor->sci, sci_get_length(doc->editor->sci), TRUE);
    scintilla_send_message(doc->editor->sci, SCI_SEARCHANCHOR, 0, 0);
    scintilla_send_message(doc->editor->sci, SCI_SEARCHPREV, (uptr_t) 0, (sptr_t) text);
  }
  skip = TRUE;
  doc->editor->scroll_percent = 0.3F;
}

static gboolean on_out(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
  g_signal_handler_disconnect(entry, handler);
  gtk_widget_hide(dialog);
  return FALSE;
}

static gboolean on_activate(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
  text = unescape(gtk_entry_get_text(GTK_ENTRY(entry)));
  on_out(NULL, NULL, NULL);
  return FALSE;
}

static void on_in(GtkWidget *widget, gpointer user_data) {
  handler = g_signal_connect(entry, "grab-notify", G_CALLBACK(on_out), NULL);

  GdkDisplay *display;
  GdkCursor *cursor;
  GdkGrabStatus status;

  display = gdk_display_get_default();
  cursor = gdk_cursor_new_from_name(display, "crosshair");
  status = gdk_seat_grab(
    gdk_display_get_default_seat(display),
    gtk_widget_get_window(widget),
    GDK_SEAT_CAPABILITY_ALL_POINTING, TRUE,
    cursor, NULL, NULL, NULL);

  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1);
}

static gboolean on_key(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
  if(event->keyval == GDK_Escape) {
    on_out(NULL, NULL, NULL);
  }
  else {
    text = unescape(gtk_entry_get_text(GTK_ENTRY(entry)));
    if(old == NULL || g_ascii_strcasecmp(old, text) != 0) {
      old = text;
      GeanyDocument *doc = document_get_current();
      mark_all(doc, old, QUICK_SEARCH_INDICATOR);
      scintilla_send_message(doc->editor->sci, SCI_SEARCHNEXT, (uptr_t) 0, (sptr_t) old);
      doc->editor->scroll_percent = 0.3F;
    }
  }
  return TRUE;
}

static gboolean init(GeanyPlugin *plugin, gpointer pdata)
{
  geany_plugin = plugin;
  dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_decorated(GTK_WINDOW(dialog), FALSE);
  g_signal_connect(G_OBJECT(dialog), "show", G_CALLBACK(on_in), NULL);
  gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(geany_plugin->geany_data->main_widgets->window));

  entry = gtk_entry_new();
  gtk_entry_set_icon_from_icon_name(GTK_ENTRY(entry), GTK_ENTRY_ICON_SECONDARY, "edit-find");
  gtk_widget_grab_focus(GTK_WIDGET(entry));

  gtk_container_add(GTK_CONTAINER(dialog), entry);
  g_signal_connect(entry, "key-release-event", G_CALLBACK(on_key), NULL);
  g_signal_connect(entry, "activate", G_CALLBACK(on_activate), NULL);

  GeanyKeyGroup *key_group;
  
  key_group = plugin_set_key_group(geany_plugin, "quick_search_keyboard_shortcut", KB_GROUP, NULL);
  keybindings_set_item(key_group, KB_QUICK_SEARCH, quick_search, 0, 0,
    "quick_search", _("Quick Search..."), NULL);
  keybindings_set_item(key_group, KB_QUICK_SEARCH_NEXT, quick_next, 0, 0,
    "quick_search_next", _("Go Next"), NULL);
  keybindings_set_item(key_group, KB_QUICK_SEARCH_PREV, quick_prev, 0, 0,
    "quick_search_prev", _("Go Previous"), NULL);
}

static void cleanup(GeanyPlugin *plugin, gpointer pdata)
{
  gtk_widget_destroy(dialog);
}

static MatchInfo *match_info_new(gint start, gint end)
{
  MatchInfo *info = g_slice_alloc(sizeof *info);

  info->start = start;
  info->end = end;

  return info;
}

void match_info_free(MatchInfo *info)
{
  g_slice_free1(sizeof *info, info);
}

static GSList *find_range(ScintillaObject *sci, gint flags, struct Sci_TextToFind *ttf)
{
  GSList *matches = NULL;
  MatchInfo *info;

  g_return_val_if_fail(sci != NULL && ttf->lpstrText != NULL, NULL);
  if (! *ttf->lpstrText)
    return NULL;

  while(sci_find_text(sci, 0, ttf) != -1) {
    info = match_info_new(ttf->chrgText.cpMin, ttf->chrgText.cpMax);
    if (ttf->chrgText.cpMax > ttf->chrg.cpMax) {
      match_info_free(info);
      break;
    }

    matches = g_slist_prepend(matches, info);
    ttf->chrg.cpMin = ttf->chrgText.cpMax;

    if (ttf->chrgText.cpMax == ttf->chrgText.cpMin) {
      ttf->chrg.cpMin ++;
    }
  }

  return g_slist_reverse(matches);
}

gint mark_all(GeanyDocument *doc, const gchar *search_text, gint indicator)
{
  scintilla_send_message(doc->editor->sci, SCI_INDICSETSTYLE, QUICK_SEARCH_INDICATOR, INDIC_ROUNDBOX);
  scintilla_send_message(doc->editor->sci, SCI_INDICSETFORE, QUICK_SEARCH_INDICATOR, 0x88fe88); //weird: 0xBBGGRR
  scintilla_send_message(doc->editor->sci, SCI_INDICSETALPHA, QUICK_SEARCH_INDICATOR, 70);

  scintilla_send_message(doc->editor->sci, SCI_INDICSETSTYLE, SELECTED_SEARCH_INDICATOR, INDIC_ROUNDBOX);
  scintilla_send_message(doc->editor->sci, SCI_INDICSETFORE, SELECTED_SEARCH_INDICATOR, 0xfefe88);
  scintilla_send_message(doc->editor->sci, SCI_INDICSETALPHA, SELECTED_SEARCH_INDICATOR, 70);

  gint count = 0;
  struct Sci_TextToFind ttf;
  GSList *match, *matches;

  g_return_val_if_fail(doc != NULL, 0);

  editor_indicator_clear(doc->editor, indicator);

  if (G_UNLIKELY(! NZV(search_text)))
    return 0;

  ttf.chrg.cpMin = 0;
  ttf.chrg.cpMax = sci_get_length(doc->editor->sci);
  ttf.lpstrText = (gchar *)search_text;

  matches = find_range(doc->editor->sci, 0, &ttf);
  foreach_slist (match, matches)
  {
    MatchInfo *info = match->data;

    if (info->end != info->start)
      editor_indicator_set_on_range(doc->editor, indicator, info->start, info->end);
    count++;

    g_slice_free1(sizeof *info, info);
  }
  g_slist_free(matches);

  return count;
}

gboolean editor_notify_cb(GObject *object, GeanyEditor *editor, SCNotification *nt, gpointer data)
{
  if (nt->updated & SC_UPDATE_SELECTION && sci_has_selection(editor->sci)) {
    if(skip) {
      skip = FALSE;
      return FALSE;
    }
    gchar *selected;
    //selected = g_malloc(sci_get_selected_text_length(editor->sci) + 1);
    selected = sci_get_selection_contents(editor->sci);
    GeanyDocument *doc = document_get_current();
    mark_all(doc, selected, SELECTED_SEARCH_INDICATOR);
  }
  return FALSE;
}

PluginCallback plugin_callbacks[] = {
  {"editor-notify", (GCallback) &editor_notify_cb, TRUE, NULL},
  { NULL, NULL, FALSE, NULL }
};

G_MODULE_EXPORT
void geany_load_module(GeanyPlugin *plugin)
{
  main_locale_init("share/locale", "quick-search");
  plugin->info->name = _("Quick Search");
  plugin->info->description = _("Do a case-insensitive search on the current document while highlighting all results.  Also highlight all selected text.");
  plugin->info->version = "0.2";
  plugin->info->author = "Steven Blatnick <steve8track@yahoo.com>";
  plugin->funcs->init = init;
  plugin->funcs->cleanup = cleanup;
  GEANY_PLUGIN_REGISTER(plugin, 225);
}

