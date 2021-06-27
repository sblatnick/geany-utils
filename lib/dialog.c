#include <geanyplugin.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>

extern const gchar *tools;

static GtkWidget *scrollable, *tree, *title, *saveCheckbox, *menuCheckbox,
  *shortcutCheckbox, *outputCombo, *editButton, *shortcutButton;
static GtkTreeStore *list, *outputs;
static GtkTreeIter row;
static GtkDialog *configDialog;

static Tool* get_active_tool()
{
  GtkTreeSelection *selected = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
  GtkTreeIter iter;
  GtkTreeModel *model;
  if(gtk_tree_selection_get_selected(selected, &model, &iter)) {
    Tool *tool;
    gtk_tree_model_get(model, &iter, 0, &tool, -1);
    return tool;
  }
  else {
    return NULL;
  }
}

static void selected_changed(GtkTreeView *view, gpointer data)
{
  Tool* tool = get_active_tool();
  if(tool == NULL) {
    gtk_widget_set_sensitive(GTK_WIDGET(title), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(saveCheckbox), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuCheckbox), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(shortcutCheckbox), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(shortcutCheckbox), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(outputCombo), FALSE);
    gtk_widget_set_sensitive(GTK_WIDGET(editButton), FALSE);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(saveCheckbox), FALSE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(menuCheckbox), FALSE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(shortcutCheckbox), FALSE);
    gtk_entry_set_text(GTK_ENTRY(title), "");
  }
  else {
    gtk_widget_set_sensitive(GTK_WIDGET(title), TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(saveCheckbox), TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(menuCheckbox), TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(shortcutCheckbox), TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(shortcutCheckbox), TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(outputCombo), TRUE);
    gtk_widget_set_sensitive(GTK_WIDGET(editButton), TRUE);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(saveCheckbox), tool->save);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(menuCheckbox), tool->menu);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(shortcutCheckbox), tool->shortcut);
    gtk_combo_box_set_active(GTK_COMBO_BOX(outputCombo), tool->output == -1 ? 0 : tool->output);
    gtk_entry_set_text(GTK_ENTRY(title), tool->name);
  }
}

static void combo_changed(GtkComboBox *combo, gpointer data)
{
  Tool* tool = get_active_tool();
  if(tool != NULL) {
    GtkTreeIter iter;
    gint value;
    GtkTreeModel *model;

    if(gtk_combo_box_get_active_iter(combo, &iter)) {
      model = gtk_combo_box_get_model(combo);
      gtk_tree_model_get(model, &iter, 1, &value, -1);
    }
    tool->output = value;
  }
}

static int add_tool(Tool *tool)
{
  gtk_tree_store_append(list, &row, NULL);
  gtk_tree_store_set(list, &row, 0, tool, -1);
}

static int add_output(const gchar *label, gint value)
{
  gtk_tree_store_append(outputs, &row, NULL);
  gtk_tree_store_set(outputs, &row, 0, label, 1, value, -1);
}

static void delete_tool(GtkButton *button, gpointer data)
{
  GtkTreeSelection *selected = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
  GtkTreeIter iter;
  GtkTreeModel *model;
  if(gtk_tree_selection_get_selected(selected, &model, &iter))
  {
    Tool *tool;
    gtk_tree_model_get(model, &iter, 0, &tool, -1);
    g_remove(tool->id);
    free_tool(tool, TRUE);
    gtk_tree_selection_unselect_iter(selected, &iter);
    gtk_tree_store_remove(list, &iter);
  }
}

static void new_tool_entry(GtkButton *button, gpointer data)
{
  Tool *tool = new_tool();
  add_tool(tool);
}

static void cell_data(GtkTreeViewColumn *tree_column, GtkCellRenderer *render,
  GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
  Tool *tool;
  gtk_tree_model_get(model, iter, 0, &tool, -1);
  g_object_set(render, "text", tool->name, NULL);
}

static void toggle_checkbox(GtkToggleButton *checkbox, gpointer data)
{
  Tool* tool = get_active_tool();
  if(tool != NULL) {
    gboolean value = gtk_toggle_button_get_active(checkbox);
    gint property = GPOINTER_TO_INT(data);
    switch(property) {
      case SAVE:
        tool->save = value;
        break;
      case MENU:
        tool->menu = value;
        break;
      case SHORTCUT:
        tool->shortcut = value;
        break;
    }
  }
}

static GtkWidget* checkbox(const gchar *label, const gchar *tooltip, gchar *key)
{
  GtkWidget *check = gtk_check_button_new_with_label(_(label));
  gtk_widget_set_tooltip_text(check, _(tooltip));
  g_signal_connect(G_OBJECT(check), "toggled", G_CALLBACK(toggle_checkbox), key);
  return check;
}

static void dialog_response(GtkDialog *dialog, gint response, gpointer user_data)
{
  GtkTreeIter iter;
  gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(list), &iter);

  while(valid)
  {
    Tool *tool;
    gtk_tree_model_get(GTK_TREE_MODEL(list), &iter, 0, &tool, -1);
    save_tool(tool);
    valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(list), &iter);
  }

  //Remove all shortcuts to tools from the UI for a refresh:
  clean_tools();
  //We need to re-establish all of the tools in the UI:
  reload_tools();
}

static gboolean on_change(GtkWidget *entry, GdkEventKey *event, gpointer user_data)
{
  Tool* tool = get_active_tool();
  if(tool != NULL) {
    g_free(tool->name);
    tool->name = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
  }
  return FALSE;
}

static void on_edit(GtkButton* button, gpointer data)
{
  Tool* tool = get_active_tool();
  gtk_dialog_response(configDialog, GTK_RESPONSE_OK);
  document_open_file(tool->id, FALSE, NULL, NULL);
}

static void on_shortcut(GtkButton* button, gpointer data)
{
  Tool* tool = get_active_tool();
  gtk_dialog_response(configDialog, GTK_RESPONSE_OK);
  //TODO: keybindings_dialog_show_prefs_scroll("External Tools");
}

static GtkWidget* configure(GeanyPlugin *plugin, GtkDialog *dialog, gpointer pdata)
{
  configDialog = dialog;
  GtkCellRenderer *render;
  GtkWidget* vbox = gtk_vbox_new(FALSE, 6);
  GtkWidget* hbox = gtk_hbox_new(FALSE, 6);
  GtkWidget* hboxButtons = gtk_hbox_new(FALSE, 6);
  GtkWidget* settingsBox = gtk_vbox_new(FALSE, 6);
  GtkWidget* buttonBox = gtk_hbox_new(FALSE, 6);

  //Tools (left half):
  list = gtk_tree_store_new(1, G_TYPE_POINTER);
  tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list));
  render = gtk_cell_renderer_text_new();

  GtkTreeViewColumn* column = gtk_tree_view_column_new_with_attributes("Tool Name", render, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), TRUE);

  gtk_tree_view_column_set_cell_data_func(column, render,
    (GtkTreeCellDataFunc)cell_data, NULL, NULL);

  scrollable = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollable),
    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(scrollable), tree);

  GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
  gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);
  g_signal_connect(G_OBJECT(select), "changed",
    G_CALLBACK(selected_changed), NULL);

  GtkWidget* delete = gtk_button_new_with_label("Delete");
  GtkWidget* new = gtk_button_new_with_label("New");
  g_signal_connect(delete, "clicked", G_CALLBACK(delete_tool), NULL);
  g_signal_connect(new, "clicked", G_CALLBACK(new_tool_entry), NULL);

  gtk_box_pack_start(GTK_BOX(buttonBox), delete, FALSE, FALSE, 10);
  gtk_box_pack_end(GTK_BOX(buttonBox), new, FALSE, FALSE, 10);

  gtk_box_pack_start(GTK_BOX(vbox), scrollable, TRUE, TRUE, 10);
  gtk_box_pack_start(GTK_BOX(vbox), buttonBox, FALSE, FALSE, 10);

  //Properties (right half):
  gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 10);
  
  outputs = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_INT);
  outputCombo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(outputs));
  GtkCellRenderer *cell = gtk_cell_renderer_text_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(outputCombo), cell, TRUE );
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(outputCombo), cell, "text", 0, NULL );
  add_output(_("None"), TOOL_OUTPUT_NONE);
  add_output(_("Message Text"), TOOL_OUTPUT_MESSAGE_TEXT);
  add_output(_("Message Table"), TOOL_OUTPUT_MESSAGE_TABLE);
  add_output(_("Replace Selected"), TOOL_OUTPUT_REPLACE_SELECTED);
  add_output(_("Replace Line"), TOOL_OUTPUT_REPLACE_LINE);
  add_output(_("Replace Word"), TOOL_OUTPUT_REPLACE_WORD);
  add_output(_("Append Current Document"), TOOL_OUTPUT_APPEND_CURRENT_DOCUMENT);
  add_output(_("New Document"), TOOL_OUTPUT_NEW_DOCUMENT);

  g_signal_connect(G_OBJECT(outputCombo), "changed", G_CALLBACK(combo_changed), NULL);

  saveCheckbox = checkbox("Save", "Should the active document be saved when this tool is run?", GINT_TO_POINTER(SAVE));
  menuCheckbox = checkbox("Menu", "Should there be a menu for this tool?", GINT_TO_POINTER(MENU));
  shortcutCheckbox = checkbox("Shortcut", "Should there be a keyboard shortcut for this tool?", GINT_TO_POINTER(SHORTCUT));

  title = gtk_entry_new();
  g_signal_connect(title, "focus-out-event", G_CALLBACK(on_change), NULL);
  gtk_box_pack_start(GTK_BOX(settingsBox), title, FALSE, FALSE, 2);

  gtk_box_pack_start(GTK_BOX(settingsBox), saveCheckbox, FALSE, FALSE, 10);
  gtk_box_pack_start(GTK_BOX(settingsBox), menuCheckbox, FALSE, FALSE, 10);
  gtk_box_pack_start(GTK_BOX(settingsBox), shortcutCheckbox, FALSE, FALSE, 10);
  gtk_box_pack_start(GTK_BOX(settingsBox), outputCombo, FALSE, FALSE, 10);

  editButton = gtk_button_new_with_label(_("Edit"));
  g_signal_connect(editButton, "clicked", G_CALLBACK(on_edit), NULL);
  gtk_box_pack_start(GTK_BOX(hboxButtons), editButton, TRUE, TRUE, 2);

  shortcutButton = gtk_button_new_with_label(_("Shortcut"));
  g_signal_connect(shortcutButton, "clicked", G_CALLBACK(on_shortcut), NULL);
  gtk_box_pack_start(GTK_BOX(hboxButtons), shortcutButton, TRUE, TRUE, 2);

  gtk_box_pack_start(GTK_BOX(settingsBox), hboxButtons, FALSE, FALSE, 10);
  gtk_box_pack_end(GTK_BOX(hbox), settingsBox, TRUE, TRUE, 10);
  gtk_widget_show_all(hbox);

  g_signal_connect(dialog, "response", G_CALLBACK(dialog_response), NULL);

  load_tools(add_tool);

  gtk_widget_set_sensitive(GTK_WIDGET(title), FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(saveCheckbox), FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(menuCheckbox), FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(shortcutCheckbox), FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(shortcutCheckbox), FALSE);
  gtk_widget_set_sensitive(GTK_WIDGET(editButton), FALSE);

  return hbox;
}
