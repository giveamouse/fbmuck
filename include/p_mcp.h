extern void prim_mcp_register (PRIM_PROTOTYPE);
extern void prim_mcp_bind (PRIM_PROTOTYPE);
extern void prim_mcp_supports (PRIM_PROTOTYPE);
extern void prim_mcp_send (PRIM_PROTOTYPE);
extern void prim_gui_available (PRIM_PROTOTYPE);
extern void prim_gui_dlog_simple (PRIM_PROTOTYPE);
extern void prim_gui_dlog_tabbed (PRIM_PROTOTYPE);
extern void prim_gui_dlog_helper (PRIM_PROTOTYPE);
extern void prim_gui_dlog_show (PRIM_PROTOTYPE);
extern void prim_gui_dlog_close (PRIM_PROTOTYPE);
extern void prim_gui_value_set (PRIM_PROTOTYPE);
extern void prim_gui_values_get (PRIM_PROTOTYPE);
extern void prim_gui_ctrl_create (PRIM_PROTOTYPE);


#define PRIMS_MCP_FUNCS prim_mcp_register, prim_mcp_bind, prim_mcp_supports, \
		prim_mcp_send, prim_gui_available, prim_gui_dlog_simple, \
		prim_gui_dlog_tabbed, prim_gui_dlog_helper, prim_gui_dlog_show, \
		prim_gui_dlog_close, prim_gui_values_get, prim_gui_value_set, \
		prim_gui_ctrl_create

#define PRIMS_MCP_NAMES "MCP_REGISTER", "MCP_BIND", "MCP_SUPPORTS", \
		"MCP_SEND", "GUI_AVAILABLE", "GUI_DLOG_SIMPLE", \
		"GUI_DLOG_TABBED", "GUI_DLOG_HELPER", "GUI_DLOG_SHOW", \
		"GUI_DLOG_CLOSE", "GUI_VALUES_GET", "GUI_VALUE_SET", \
		"GUI_CTRL_CREATE"


#define PRIMS_MCP_CNT 13


