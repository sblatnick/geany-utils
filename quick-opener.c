#include <geanyplugin.h>

static void quick_open(GtkMenuItem *menuitem, gpointer user_data)
{
    dialogs_show_msgbox(GTK_MESSAGE_INFO, _("Hello World"));
}
static gboolean init(GeanyPlugin *plugin, gpointer pdata)
{
    GtkWidget *menu_entry;
    menu_entry = gtk_menu_item_new_with_mnemonic(_("Quick Open..."));
    gtk_widget_show(menu_entry);
    gtk_container_add(
      GTK_CONTAINER(
        plugin->geany_data->main_widgets->tools_menu
      ),
      menu_entry
    );
    g_signal_connect(
      menu_entry,
      "activate",
      G_CALLBACK(quick_open),
      NULL
    );
    geany_plugin_set_data(plugin, menu_entry, NULL);
    return TRUE;
}
static void cleanup(GeanyPlugin *plugin, gpointer pdata)
{
    GtkWidget *menu_entry = (GtkWidget *) pdata;
    gtk_widget_destroy(menu_entry);
}

G_MODULE_EXPORT
void geany_load_module(GeanyPlugin *plugin)
{
  main_locale_init("share/locale", "quick-opener");
  plugin->info->name = _("Quick Opener");
  plugin->info->description = _("Search filenames while typing");
  plugin->info->version = "0.2";
  plugin->info->author = "Steven Blatnick <steve8track@yahoo.com>";
  plugin->funcs->init = init;
  plugin->funcs->cleanup = cleanup;
  GEANY_PLUGIN_REGISTER(plugin, 225);
}

