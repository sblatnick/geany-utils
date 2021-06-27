#include <geanyplugin.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>

extern GtkWidget *scrollable_table;
static GtkTreeStore *table;
static GtkWidget *tree;
static gboolean init;

void table_prepare()
{
  printf("TABLE_PREPARE\n");
  init = TRUE;
}

void table_print(gchar *text, const gchar *tag)
{
  printf("TABLE_PRINT\n");
  if(init) {
    init = FALSE;
    
    if(tree != NULL) {
      gtk_container_remove(GTK_CONTAINER(scrollable_table), tree);
      //G_OBJECT_FREE(tree);
    }

    table = gtk_tree_store_new(1, G_TYPE_STRING);
    tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(table));
    GtkCellRenderer *render = gtk_cell_renderer_text_new();

    GtkTreeViewColumn* column = gtk_tree_view_column_new_with_attributes(text, render, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

    gtk_container_add(GTK_CONTAINER(scrollable_table), tree);
    gtk_widget_show_all(scrollable_table);
  }
  else {
    GtkTreeIter row;
    gtk_tree_store_append(table, &row, NULL);
    gtk_tree_store_set(table, &row, 0, text, -1);
  }
}
