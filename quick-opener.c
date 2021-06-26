#include <geanyplugin.h>

static void item_activate_cb(GtkMenuItem *menuitem, gpointer user_data)
{
    dialogs_show_msgbox(GTK_MESSAGE_INFO, "Hello World");
}
static gboolean init(GeanyPlugin *plugin, gpointer pdata)
{
    GtkWidget *main_menu_item;
    // Create a new menu item and show it
    main_menu_item = gtk_menu_item_new_with_mnemonic("Hello World");
    gtk_widget_show(main_menu_item);
    gtk_container_add(GTK_CONTAINER(plugin->geany_data->main_widgets->tools_menu),
            main_menu_item);
    g_signal_connect(main_menu_item, "activate",
            G_CALLBACK(item_activate_cb), NULL);
    geany_plugin_set_data(plugin, main_menu_item, NULL);
    return TRUE;
}
static void cleanup(GeanyPlugin *plugin, gpointer pdata)
{
    GtkWidget *main_menu_item = (GtkWidget *) pdata;
    gtk_widget_destroy(main_menu_item);
}

G_MODULE_EXPORT
void geany_load_module(GeanyPlugin *plugin)
{
  plugin->info->name = "Quick Opener";
  plugin->info->description = "Search filenames while typing";
  plugin->info->version = "0.2";
  plugin->info->author = "Steven Blatnick <steve8track@yahoo.com>";
  plugin->funcs->init = init;
  plugin->funcs->cleanup = cleanup;
  GEANY_PLUGIN_REGISTER(plugin, 225);
}

