/*
 * Headers for MCP GUI support code.
 */

#ifndef MCPGUI_H
#define MCPGUI_H

#include <stdarg.h>


/*
 * Error results.
 */
#define EGUINOSUPPORT -1                    /* This connection doesn't support the GUI MCP package. */
#define EGUINODLOG    -2                    /* No dialog exists with the given ID. */


/* The MCP package name. */
#define GUI_PACKAGE   "org-fuzzball-gui"


/* Used in list related commands to refer to the end of a list. */
#define GUI_LIST_END  -1

/*
 * Defines for specifying control layout
 */
#define GUI_STICKY_MASK  0xF
#define GUI_N     0x1
#define GUI_S     0x2
#define GUI_E     0x4
#define GUI_W     0x8
#define GUI_NS (GUI_N | GUI_S)
#define GUI_NW (GUI_N | GUI_W)
#define GUI_NE (GUI_N | GUI_E)
#define GUI_SE (GUI_E | GUI_S)
#define GUI_SW (GUI_W | GUI_S)
#define GUI_EW (GUI_E | GUI_W)
#define GUI_NSE (GUI_NS | GUI_E)
#define GUI_NSW (GUI_NS | GUI_W)
#define GUI_NEW (GUI_N | GUI_EW)
#define GUI_SEW (GUI_S | GUI_EW)
#define GUI_NSEW (GUI_NS | GUI_EW)

#define GUI_COLSPAN_MASK 0xF0
#define COLSPAN(val) (((val-1) & 0xF) << 4)
#define GET_COLSPAN(val) (((val & GUI_COLSPAN_MASK) >> 4) +1)

#define GUI_ROWSPAN_MASK 0xF00
#define ROWSPAN(val) (((val-1) & 0xF) << 8)
#define GET_ROWSPAN(val) (((val & GUI_ROWSPAN_MASK) >> 8) +1)

#define GUI_COLSKIP_MASK 0xF000
#define COLSKIP(val) ((val & 0xF) << 12)
#define GET_COLSKIP(val) ((val & GUI_COLSKIP_MASK) >> 12)

#define GUI_LEFTPAD_MASK 0xF0000
#define LEFTPAD(val) (((val>>1) & 0xF) << 16)
#define GET_LEFTPAD(val) ((val & GUI_LEFTPAD_MASK) >> 15)

#define GUI_TOPPAD_MASK 0xF00000
#define TOPPAD(val) (((val>>1) & 0xF) << 20)
#define GET_TOPPAD(val) ((val & GUI_TOPPAD_MASK) >> 19)

#define GUI_NONL     0x1000000
#define GUI_HEXP     0x2000000
#define GUI_VEXP     0x4000000
#define GUI_REPORT   0x8000000
#define GUI_REQUIRED 0x8000000


/*
 * Defines the callback arguments, etc.
 */
#define GUI_EVENT_CB_ARGS \
    int   descr,          \
    char* dlogid,         \
    char* id,             \
    char* event,          \
    int   did_dismiss,    \
    void* context

typedef void (*Gui_CB) (GUI_EVENT_CB_ARGS);




/*
 * Do this when the muck itself starts.
 */
void gui_initialize();

/*
 * First you check to see if the MCP GUI package is supported.
 */
McpVer GuiVersion(int descr);
int GuiSupported(int descr);

/*
 * Second, create a dialog window.
 */
char* GuiSimple(int descr, char *title, Gui_CB callback, void* context);
char* GuiTabbed(int descr, char *title, int pagecount, char** pagenames, char** pageids, Gui_CB callback, void* context);
char* GuiHelper(int descr, char *title, int pagecount, char** pagenames, char** pageids, Gui_CB callback, void* context);

/*
 * Once you have a dialog, you can add menu items with these functions.
 */
int gui_menu_item(char* dlogid, char* id, char* type, char* name, char** args);
int GuiMenuCmd(char* dlogid, char* id, char* name);
int GuiMenuCheckBtn(char* dlogid, char* id, char* name, char** args);


/*
 * You use these commands to add controls to a dialog.
 */
int gui_ctrl_make_v(char* dlogid, char* type, char* pane, char* id, char* text, char* value, int layout, char** args);
int gui_ctrl_make_l(char* dlogid, char* type, char* pane, char* id, char* text, char* value, int layout, ...);


/*
 * You can use these as shortcuts for creating controls.
 */
int GuiHRule(char* dlogid, char* pane, char* id, int height, int layout);
int GuiVRule(char* dlogid, char* pane, char* id, int thickness, int layout);
int GuiText(char* dlogid, char* pane, char* id, char* value, int width, int layout);
int GuiButton(char* dlogid, char* pane, char* id, char* text, int width, int dismiss, int layout);
int GuiEdit(char* dlogid, char* pane, char* id, char* text, char* value, int width, int layout);
int GuiMulti(char* dlogid, char* pane, char* id, char* value, int width, int height, int fixed, int layout);

/* CHECKBOX */
/* SCALE */

int GuiSpinner(char* dlogid, char* pane, char* id, char* text, int value, int width, int min, int max, int layout);
int GuiCombo(char* dlogid, char* pane, char* id, char* text, char* value, int width, int editable, int layout);

/* LISTBOX */

int GuiFrame(char* dlogid, char* pane, char* id, int layout);
int GuiGroupBox(char* dlogid, char* pane, char* id, char* text, int collapsible, int collapsed, int layout);


/*
 * You can set or retrieve the values of various widgets with these calls.
 */
char* GuiValueFirst(char* dlogid);
char* GuiValueNext(char* dlogid, char* prev);
int gui_value_linecount(char* dlogid, char* id);
char* gui_value_get(char* dlogid, char* id, int line);
int GuiSetVal(char* dlogid, char* id, int lines, char** value);


/*
 * Listbox and combobox widgets also have lists associated with them.
 * You can use these calls to get or set items in these lists.
 */
int GuiListInsert(char* dlogid, char* id, int after, int lines, char** value);
int GuiListDelete(char* dlogid, char* id, int from, int to);


/*
 *  When you are done laying out the dialog, you display it to the user.
 */
int GuiShow(char* id);


/*
 * When you are done with the dialog, you can force it to close.
 * You should make sure to free up the dialog resources when you are done.
 */
int GuiClose(char* id);
int GuiFree(char* id);
int gui_dlog_freeall_descr(int descr);


/*
 * This might be useful for callbacks.
 */
int gui_dlog_get_descr(char* dlogid);


/* Testing framework.  WORK: THIS SHOULD BE REMOVED BEFORE RELEASE */
void do_post_dlog(int descr, char* text);


/* internal support for MUF */
void muf_dlog_add(struct frame*fr, char* dlogid);
void muf_dlog_remove(struct frame*fr, char* dlogid);
void muf_dlog_purge(struct frame *fr);

#endif /* MCPGUI_H */



