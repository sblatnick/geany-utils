#include <geanyplugin.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include <stdlib.h>

GeanyPlugin *geany_plugin;
GeanyData *geany_data;

static GtkWidget *panel, *editor, *docs, *tree, *hpaned, *label;
static GtkTreeModel *model;

enum
{
  KB_DOC_PANEL_NEXT,
  KB_DOC_PANEL_PREV,
  KB_GROUP
};

static void next_focus(G_GNUC_UNUSED guint key_id)
{
  GtkTreeIter selected, next, parent;
  if (gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(tree)), NULL, &selected))
  {
    next = selected;
    if(gtk_tree_model_iter_next(model, &next))
    {
      gtk_tree_view_set_cursor(GTK_TREE_VIEW(tree), gtk_tree_model_get_path(model, &next), NULL, FALSE);
      g_signal_emit_by_name(tree, "row-activated");
    }
    else if(gtk_tree_model_iter_parent(model, &parent, &selected)) {
      if(gtk_tree_model_iter_next(model, &parent)) {
        if(gtk_tree_model_iter_children(model, &selected, &parent)) {
          gtk_tree_view_set_cursor(GTK_TREE_VIEW(tree), gtk_tree_model_get_path(model, &selected), NULL, FALSE);
          g_signal_emit_by_name(tree, "row-activated");
        }
      }
    }
  }
}

static void prev_focus(G_GNUC_UNUSED guint key_id)
{
  GtkTreeIter selected, next, parent;
  if (gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(tree)), NULL, &selected))
  {
    next = selected;
    if(gtk_tree_model_iter_previous(model, &next))
    {
      gtk_tree_view_set_cursor(GTK_TREE_VIEW(tree), gtk_tree_model_get_path(model, &next), NULL, FALSE);
      g_signal_emit_by_name(tree, "row-activated");
    }
    else if(gtk_tree_model_iter_parent(model, &parent, &selected)) {
      if(gtk_tree_model_iter_previous(model, &parent)) {
        if(gtk_tree_model_iter_nth_child(model, &selected, &parent, gtk_tree_model_iter_n_children(model, &parent) - 1)) {
          gtk_tree_view_set_cursor(GTK_TREE_VIEW(tree), gtk_tree_model_get_path(model, &selected), NULL, FALSE);
          g_signal_emit_by_name(tree, "row-activated");
        }
      }
    }
  }
}

static gboolean init(GeanyPlugin *plugin, gpointer pdata)
{
  geany_plugin = plugin;

  docs = ui_lookup_widget(geany_plugin->geany_data->main_widgets->window, "scrolledwindow7");
  tree = ui_lookup_widget(geany_plugin->geany_data->main_widgets->window, "treeview6");
  model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
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

  gtk_paned_set_position(GTK_PANED(hpaned), wnat.width - 250);
  
  GeanyKeyGroup *key_group;
  key_group = plugin_set_key_group(geany_plugin, "quick_find_keyboard_shortcut", KB_GROUP, NULL);
  keybindings_set_item(key_group, KB_DOC_PANEL_NEXT, next_focus, 0, 0, "doc_panel_next", _("Next Document"), NULL);
  keybindings_set_item(key_group, KB_DOC_PANEL_PREV, prev_focus, 0, 0, "doc_panel_prev", _("Prev Document"), NULL);
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
