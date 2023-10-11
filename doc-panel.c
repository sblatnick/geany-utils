#include <geanyplugin.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include <stdlib.h>

GeanyPlugin *geany_plugin;
GeanyData *geany_data;

static GtkWidget *panel, *editor, *docs, *hpaned, *label;

static gboolean init(GeanyPlugin *plugin, gpointer pdata)
{
  geany_plugin = plugin;

  docs = ui_lookup_widget(geany_plugin->geany_data->main_widgets->window, "scrolledwindow7");
  label = gtk_notebook_get_tab_label(
    GTK_NOTEBOOK(geany_plugin->geany_data->main_widgets->sidebar_notebook),
    docs
  );
  gtk_notebook_remove_page(
    GTK_NOTEBOOK(geany_plugin->geany_data->main_widgets->sidebar_notebook),
    gtk_notebook_page_num(GTK_NOTEBOOK(geany_plugin->geany_data->main_widgets->sidebar_notebook), docs)
  );
  gtk_container_remove(GTK_CONTAINER(geany_plugin->geany_data->main_widgets->sidebar_notebook), GTK_WIDGET(docs));
  
  hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
  panel = ui_lookup_widget(geany_plugin->geany_data->main_widgets->window, "hpaned1");
  editor = gtk_paned_get_child2(GTK_PANED(panel));

  gtk_container_remove(GTK_CONTAINER(panel), GTK_WIDGET(editor));
  gtk_paned_pack2(GTK_PANED(panel), GTK_WIDGET(hpaned), TRUE, TRUE);

  gtk_paned_pack1(GTK_PANED(hpaned), GTK_WIDGET(editor), TRUE, TRUE);

  gtk_paned_pack2(GTK_PANED(hpaned), GTK_WIDGET(docs), TRUE, TRUE);
  
  gtk_widget_show_all(hpaned);
  
  gtk_widget_realize(geany_plugin->geany_data->main_widgets->window);
  GtkRequisition wmin, wnat;
  gtk_widget_get_preferred_size(GTK_WIDGET(geany_plugin->geany_data->main_widgets->window), &wmin, &wnat);
  printf("wnat width: %d\n", wnat.width);
  printf("min width: %d\n", wmin.width);
  printf("nat height: %d\n", wnat.height);
  printf("min height: %d\n", wmin.height);

  GtkRequisition smin, snat;
  gtk_widget_get_preferred_size(GTK_WIDGET(geany_plugin->geany_data->main_widgets->sidebar_notebook), &smin, &snat);
  printf("snat width: %d\n", snat.width);
  printf("min width: %d\n", smin.width);
  printf("nat height: %d\n", snat.height);
  printf("min height: %d\n", smin.height);

  gtk_paned_set_position(GTK_PANED(hpaned), wnat.width - 250);
}


static void cleanup(GeanyPlugin *plugin, gpointer pdata)
{
  gtk_container_remove(GTK_CONTAINER(hpaned), GTK_WIDGET(editor));
  gtk_container_remove(GTK_CONTAINER(hpaned), GTK_WIDGET(docs));
  gtk_container_remove(GTK_CONTAINER(panel), GTK_WIDGET(hpaned));
  gtk_paned_pack2(GTK_PANED(panel), GTK_WIDGET(editor), TRUE, TRUE);
  gtk_notebook_append_page(
    GTK_NOTEBOOK(geany_plugin->geany_data->main_widgets->sidebar_notebook),
    docs,
    label
  );
}

G_MODULE_EXPORT
void geany_load_module(GeanyPlugin *plugin)
{
  main_locale_init("share/locale", "doc-panel");
  plugin->info->name = _("Documents Panel");
  plugin->info->description = _("Move the Documents from the sidebar to the other side.");
  plugin->info->version = "0.2";
  plugin->info->author = "Steven Blatnick <steve8track@yahoo.com>";
  plugin->funcs->init = init;
  plugin->funcs->cleanup = cleanup;
  GEANY_PLUGIN_REGISTER(plugin, 225);
}
