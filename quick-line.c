#include <geanyplugin.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>

GeanyPlugin *geany_plugin;
GeanyData *geany_data;

static GtkWidget *dialog, *entry;
static gulong handler;
static const gchar *text = "";

enum
{
  KB_QUICK_LINE,
  KB_GROUP
};

static void quick_line(G_GNUC_UNUSED guint key_id)
{
  gint ox, oy, x, y;
  gdk_window_get_origin(gtk_widget_get_window(geany_plugin->geany_data->main_widgets->window), &ox, &oy);
  gtk_widget_translate_coordinates(geany_plugin->geany_data->main_widgets->notebook, geany_plugin->geany_data->main_widgets->window, 0, 0, &x, &y);
  gtk_window_move(GTK_WINDOW(dialog), ox + x, oy + y);

  gtk_entry_set_text(GTK_ENTRY(entry), "");
  gtk_widget_show_all(dialog);
}

static gboolean on_out(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
  g_signal_handler_disconnect(entry, handler);
  gtk_widget_hide(dialog);
  return FALSE;
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

static gboolean on_activate(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
  text = gtk_entry_get_text(GTK_ENTRY(entry));
  GeanyDocument *doc = document_get_current();
  quick_goto_line(doc->editor, atoi(text) - 1);
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
    text = gtk_entry_get_text(GTK_ENTRY(entry));
    //
  }
}

static gboolean init(GeanyPlugin *plugin, gpointer pdata)
{
  geany_plugin = plugin;
  dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_decorated(GTK_WINDOW(dialog), FALSE);
  g_signal_connect(G_OBJECT(dialog), "show", G_CALLBACK(on_in), NULL);
  gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(geany_plugin->geany_data->main_widgets->window));

  entry = gtk_entry_new();
  gtk_entry_set_icon_from_icon_name(GTK_ENTRY(entry), GTK_ENTRY_ICON_SECONDARY, "go-jump");
  gtk_widget_grab_focus(GTK_WIDGET(entry));

  gtk_container_add(GTK_CONTAINER(dialog), entry);
  g_signal_connect(entry, "key-release-event", G_CALLBACK(on_key), NULL);
  g_signal_connect(entry, "activate", G_CALLBACK(on_activate), NULL);

  GeanyKeyGroup *key_group;
  
  key_group = plugin_set_key_group(geany_plugin, "quick_line_keyboard_shortcut", KB_GROUP, NULL);
  keybindings_set_item(key_group, KB_QUICK_LINE, quick_line, 0, 0,
    "quick_line", _("Quick Line..."), NULL);
}

static void cleanup(GeanyPlugin *plugin, gpointer pdata)
{
  gtk_widget_destroy(dialog);
}

G_MODULE_EXPORT
void geany_load_module(GeanyPlugin *plugin)
{
  main_locale_init("share/locale", "quick-search");
  plugin->info->name = _("Quick Line");
  plugin->info->description = _("Quickly go to the line entered.");
  plugin->info->version = "0.2";
  plugin->info->author = "Steven Blatnick <steve8track@yahoo.com>";
  plugin->funcs->init = init;
  plugin->funcs->cleanup = cleanup;
  GEANY_PLUGIN_REGISTER(plugin, 225);
}
