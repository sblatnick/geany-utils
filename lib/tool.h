#include <geanyplugin.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>

enum
{
	SAVE = 0,
	MENU,
	SHORTCUT
};

enum
{
	TOOL_OUTPUT_NONE = 0,
	TOOL_OUTPUT_MESSAGE_TEXT,
	TOOL_OUTPUT_MESSAGE_TABLE,
	TOOL_OUTPUT_REPLACE_SELECTED,
	TOOL_OUTPUT_REPLACE_LINE,
	TOOL_OUTPUT_REPLACE_WORD,
	TOOL_OUTPUT_APPEND_CURRENT_DOCUMENT,
	TOOL_OUTPUT_NEW_DOCUMENT
};

typedef struct
{
	gchar *id;
	gchar *name;
	gint output;

	gboolean save;
	gboolean menu;
	gboolean shortcut;
} Tool;
static Tool **shortcut_tools;
static GtkWidget **menu_tools;
