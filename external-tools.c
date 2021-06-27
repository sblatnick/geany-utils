#include <geanyplugin.h>
#include <gdk/gdkkeysyms.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include "lib/panel.c"
#include "lib/table.c"
#include "lib/tool.c"
#include "lib/dialog.c"

GeanyPlugin *geany_plugin;
GeanyData *geany_data;

const gchar *path, *conf, *tools, *home;
GRegex *name_regex, *path_regex, *match_regex;

static GtkWidget *tool_menu_item = NULL;

static void menu_callback(GtkMenuItem *menuitem, gpointer gdata)
{
  plugin_show_configure(geany_plugin);
}

static gboolean et_init(GeanyPlugin *plugin, gpointer pdata)
{
  geany_plugin = plugin;

  path_regex = g_regex_new("^\\.|^build$", G_REGEX_OPTIMIZE | G_REGEX_CASELESS, 0, NULL);
  name_regex = g_regex_new("^\\.|\\.(o|so|exe|class|pyc)$", G_REGEX_OPTIMIZE | G_REGEX_CASELESS, 0, NULL);
  match_regex = g_regex_new(g_strconcat("[^", G_DIR_SEPARATOR_S, "]+\\.[^\\s]+", NULL), G_REGEX_OPTIMIZE | G_REGEX_CASELESS, 0, NULL);

  home = g_getenv("HOME");
  if (!home) {
    home = g_get_home_dir();
  }
  path = g_build_path(G_DIR_SEPARATOR_S, geany_plugin->geany_data->app->configdir, "plugins", "external-tools", NULL);
  conf = g_build_path(G_DIR_SEPARATOR_S, path, "external-tools.conf", NULL);
  tools = g_build_path(G_DIR_SEPARATOR_S, path, "tools", NULL);
  g_mkdir_with_parents(tools, S_IRUSR | S_IWUSR | S_IXUSR);

  reload_tools();

  tool_menu_item = gtk_menu_item_new_with_mnemonic("External Tools...");
  gtk_widget_show(tool_menu_item);
  gtk_container_add(GTK_CONTAINER(geany_plugin->geany_data->main_widgets->tools_menu), tool_menu_item);
  g_signal_connect(tool_menu_item, "activate", G_CALLBACK(menu_callback), NULL);
  panel_init();
}

static void cleanup(GeanyPlugin *plugin, gpointer pdata)
{
  g_regex_unref(path_regex);
  g_regex_unref(name_regex);
  g_regex_unref(match_regex);
  clean_tools();
  gtk_container_remove(GTK_CONTAINER(geany_plugin->geany_data->main_widgets->tools_menu), tool_menu_item);
  panel_cleanup();
}

G_MODULE_EXPORT
void geany_load_module(GeanyPlugin *plugin)
{
  main_locale_init("share/locale", "external-tools");
  plugin->info->name = _("External Tools");
  plugin->info->description = _("Allow external tools to be integrated into many common actions.");
  plugin->info->version = "0.2";
  plugin->info->author = "Steven Blatnick <steve8track@yahoo.com>";
  plugin->funcs->init = et_init;
  plugin->funcs->cleanup = cleanup;
  plugin->funcs->configure = configure;
  GEANY_PLUGIN_REGISTER(plugin, 225);
}
