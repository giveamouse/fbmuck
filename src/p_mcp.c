/* Primitives package */

#include "copyright.h"
#include "config.h"

#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include "db.h"
#include "mcp.h"
#include "inst.h"
#include "externs.h"
#include "match.h"
#include "interface.h"
#include "params.h"
#include "tune.h"
#include "strings.h"
#include "interp.h"
#include "mcpgui.h"
#include "array.h"
#include "mufevent.h"

static struct inst *oper1, *oper2, *oper3, *oper4;
static int tmp, result;
static dbref ref;




void
muf_mcp_callback(McpFrame * mfr, McpMesg * mesg, McpVer version, void *context)
{
	dbref obj = (dbref) context;
	struct mcp_binding *ptr;
	McpArg *arg;
	char *pkgname = mesg->package;
	char *msgname = mesg->mesgname;
	dbref user;
	int descr;

	descr = mcpframe_to_descr(mfr);
	user = mcpframe_to_user(mfr);

	for (ptr = PROGRAM_MCPBINDS(obj); ptr; ptr = ptr->next) {
		if (ptr->pkgname && ptr->msgname) {
			if (!string_compare(ptr->pkgname, pkgname)) {
				if (!string_compare(ptr->msgname, msgname)) {
					break;
				}
			}
		}
	}

	if (ptr) {
		struct frame *tmpfr;
		stk_array *argarr;
		struct inst argname, argval, argpart;

		tmpfr = interp(descr, user, getloc(user), obj, -1, PREEMPT, STD_REGUID);
		if (tmpfr) {
			oper1 = tmpfr->argument.st + --tmpfr->argument.top;
			CLEAR(oper1);
			argarr = new_array_dictionary();
			for (arg = mesg->args; arg; arg = arg->next) {
				if (!arg->value) {
					argval.type = PROG_STRING;
					argval.data.string = NULL;
				} else if (!arg->value->next) {
					argval.type = PROG_STRING;
					argval.data.string = alloc_prog_string(arg->value->value);
				} else {
					McpArgPart *partptr;
					int count = 0;

					argname.type = PROG_INTEGER;
					argval.type = PROG_ARRAY;
					argval.data.array =
							new_array_packed(mcp_mesg_arg_linecount(mesg, arg->name));

					for (partptr = arg->value; partptr; partptr = partptr->next) {
						argname.data.number = count++;
						argpart.type = PROG_STRING;
						argpart.data.string = alloc_prog_string(partptr->value);
						array_setitem(&argval.data.array, &argname, &argpart);
						CLEAR(&argpart);
					}
				}
				argname.type = PROG_STRING;
				argname.data.string = alloc_prog_string(arg->name);
				array_setitem(&argarr, &argname, &argval);
				CLEAR(&argname);
				CLEAR(&argval);
			}
			push(tmpfr->argument.st, &(tmpfr->argument.top), PROG_INTEGER, MIPSCAST & descr);
			push(tmpfr->argument.st, &(tmpfr->argument.top), PROG_ARRAY, MIPSCAST argarr);
			tmpfr->pc = ptr->addr;
			interp_loop(user, obj, tmpfr, 0);
		}
	}
	/* mcp_mesg_clear(mesg); */
}


int
stuff_dict_in_mesg(stk_array* arr, McpMesg* msg)
{
	struct inst argname, *argval;
	char buf[64];
	int result;
	
	result = array_first(arr, &argname);
	while (result) {
		if (argname.type != PROG_STRING) {
			CLEAR(&argname);
			return -1;
		}
		if (!argname.data.string || !*argname.data.string->data) {
			CLEAR(&argname);
			return -2;
		}
		argval = array_getitem(arr, &argname);
		switch (argval->type) {
		case PROG_ARRAY:{
				struct inst subname, *subval;
				int contd = array_first(argval->data.array, &subname);

				mcp_mesg_arg_remove(msg, argname.data.string->data);
				while (contd) {
					subval = array_getitem(argval->data.array, &subname);
					switch (subval->type) {
					case PROG_STRING:
						mcp_mesg_arg_append(msg, argname.data.string->data,
											DoNullInd(subval->data.string));
						break;
					case PROG_INTEGER:
						sprintf(buf, "%d", subval->data.number);
						mcp_mesg_arg_append(msg, argname.data.string->data, buf);
						break;
					case PROG_OBJECT:
						sprintf(buf, "#%d", subval->data.number);
						mcp_mesg_arg_append(msg, argname.data.string->data, buf);
						break;
					case PROG_FLOAT:
						sprintf(buf, "%g", subval->data.fnumber);
						mcp_mesg_arg_append(msg, argname.data.string->data, buf);
						break;
					default:
						CLEAR(&argname);
						return -3;
					}
					contd = array_next(argval->data.array, &subname);
				}
				CLEAR(&subname);
				break;
			}
		case PROG_STRING:
			mcp_mesg_arg_remove(msg, argname.data.string->data);
			mcp_mesg_arg_append(msg, argname.data.string->data,
								DoNullInd(argval->data.string));
			break;
		case PROG_INTEGER:
			sprintf(buf, "%d", argval->data.number);
			mcp_mesg_arg_remove(msg, argname.data.string->data);
			mcp_mesg_arg_append(msg, argname.data.string->data, buf);
			break;
		case PROG_OBJECT:
			sprintf(buf, "#%d", argval->data.number);
			mcp_mesg_arg_remove(msg, argname.data.string->data);
			mcp_mesg_arg_append(msg, argname.data.string->data, buf);
			break;
		case PROG_FLOAT:
			sprintf(buf, "%g", argval->data.fnumber);
			mcp_mesg_arg_remove(msg, argname.data.string->data);
			mcp_mesg_arg_append(msg, argname.data.string->data, buf);
			break;
		default:
			CLEAR(&argname);
			return -4;
		}
		result = array_next(arr, &argname);
	}
	return 0;
}


void
prim_mcp_register(PRIM_PROTOTYPE)
{
	char *pkgname;
	McpVer vermin, vermax;

	CHECKOP(3);
	oper3 = POP();
	oper2 = POP();
	oper1 = POP();

	/* oper1  oper2   oper3  */
	/* pkgstr minver  maxver */

	if (mlev < 3)
		abort_interp("Requires Wizbit.");
	if (oper1->type != PROG_STRING)
		abort_interp("Package name string expected. (1)");
	if (oper2->type != PROG_FLOAT)
		abort_interp("Floating point minimum version number expected. (2)");
	if (oper3->type != PROG_FLOAT)
		abort_interp("Floating point maximum version number expected. (3)");
	if (!oper1->data.string || !*oper1->data.string->data)
		abort_interp("Package name cannot be a null string. (1)");

	pkgname = oper1->data.string->data;
	vermin.vermajor = (int) oper2->data.fnumber;
	vermin.verminor = (int) (oper2->data.fnumber * 1000) % 1000;
	vermax.vermajor = (int) oper3->data.fnumber;
	vermax.verminor = (int) (oper3->data.fnumber * 1000) % 1000;

	mcp_package_register(pkgname, vermin, vermax, muf_mcp_callback, (void *) program);

	CLEAR(oper1);
	CLEAR(oper2);
	CLEAR(oper3);
}



void
prim_mcp_supports(PRIM_PROTOTYPE)
{
	char *pkgname;
	int descr;
	McpVer ver;
	McpFrame *mfr;
	float fver;

	CHECKOP(2);
	oper2 = POP();
	oper1 = POP();

	/* oper1 oper2  */
	/* descr pkgstr */

	if (oper1->type != PROG_INTEGER)
		abort_interp("Integer descriptor number expected. (1)");
	if (oper2->type != PROG_STRING)
		abort_interp("Package name string expected. (2)");
	if (!oper2->data.string || !*oper2->data.string->data)
		abort_interp("Package name cannot be a null string. (2)");

	pkgname = oper2->data.string->data;
	descr = oper1->data.number;

	fver = 0.0;
	mfr = descr_mcpframe(descr);
	if (mfr) {
		ver = mcp_frame_package_supported(mfr, pkgname);
		fver = ver.vermajor + (ver.verminor / 1000.0);
	}

	CLEAR(oper1);
	CLEAR(oper2);

	PushFloat(fver);
}



void
prim_mcp_bind(PRIM_PROTOTYPE)
{
	char *pkgname, *msgname;
	struct mcp_binding *ptr;

	CHECKOP(3);
	oper3 = POP();
	oper2 = POP();
	oper1 = POP();

	/* oper1  oper2  oper3 */
	/* pkgstr msgstr address */

	if (mlev < 3)
		abort_interp("Requires at least Mucker Level 3.");
	if (oper1->type != PROG_STRING)
		abort_interp("Package name string expected. (1)");
	if (oper2->type != PROG_STRING)
		abort_interp("Message name string expected. (2)");
	if (oper3->type != PROG_ADD)
		abort_interp("Function address expected. (3)");
	if (!oper1->data.string || !*oper1->data.string->data)
		abort_interp("Package name cannot be a null string. (1)");
	if (oper3->data.addr->progref > db_top ||
		oper3->data.addr->progref < 0 || (Typeof(oper3->data.addr->progref) != TYPE_PROGRAM)) {
		abort_interp("Invalid address. (3)");
	}
	if (program != oper3->data.addr->progref) {
		abort_interp("Destination address outside current program. (3)");
	}

	pkgname = oper1->data.string->data;
	msgname = oper2->data.string ? oper2->data.string->data : "";

	for (ptr = PROGRAM_MCPBINDS(program); ptr; ptr = ptr->next) {
		if (ptr->pkgname && ptr->msgname) {
			if (!string_compare(ptr->pkgname, pkgname)) {
				if (!string_compare(ptr->msgname, msgname)) {
					break;
				}
			}
		}
	}
	if (!ptr) {
		ptr = (struct mcp_binding *) malloc(sizeof(struct mcp_binding));

		ptr->pkgname = (char *) malloc(strlen(pkgname) + 1);
		strcpy(ptr->pkgname, pkgname);
		ptr->msgname = (char *) malloc(strlen(msgname) + 1);
		strcpy(ptr->msgname, msgname);

		ptr->next = PROGRAM_MCPBINDS(program);
		PROGRAM_SET_MCPBINDS(program, ptr);
	}
	ptr->addr = oper3->data.addr->data;

	CLEAR(oper1);
	CLEAR(oper2);
	CLEAR(oper3);
}


void
prim_mcp_send(PRIM_PROTOTYPE)
{
	char *pkgname, *msgname;
	char buf[BUFFER_LEN];
	int descr;
	McpFrame *mfr;
	stk_array *arr;

	CHECKOP(4);
	oper3 = POP();
	oper2 = POP();
	oper1 = POP();
	oper4 = POP();

	/* oper4 oper1  oper2  oper3 */
	/* descr pkgstr msgstr argsarray */

	if (oper4->type != PROG_INTEGER)
		abort_interp("Integer descriptor number expected. (1)");
	if (oper1->type != PROG_STRING)
		abort_interp("Package name string expected. (2)");
	if (!oper1->data.string || !*oper1->data.string->data)
		abort_interp("Package name cannot be a null string. (2)");
	if (oper2->type != PROG_STRING)
		abort_interp("Message name string expected. (3)");
	if (oper3->type != PROG_ARRAY)
		abort_interp("Arguments dictionary expected. (4)");

	pkgname = oper1->data.string->data;
	msgname = oper2->data.string ? oper2->data.string->data : "";
	arr = oper3->data.array;
	descr = oper4->data.number;

	mfr = descr_mcpframe(descr);
	if (mfr) {
		McpMesg msg;

		mcp_mesg_init(&msg, pkgname, msgname);

		result = stuff_dict_in_mesg(arr, &msg);
		if (result) {
			mcp_mesg_clear(&msg);
			switch (result) {
				case -1:
					abort_interp("Args dictionary can only have string keys. (4)");
					break;
				case -2:
					abort_interp("Args dictionary cannot have a null string key. (4)");
					break;
				case -3:
					abort_interp("Unsupported value type in list value. (4)");
					break;
				case -4:
					abort_interp("Unsupported value type in args dictionary. (4)");
					break;
			}
		}
		
		mcp_frame_output_mesg(mfr, &msg);
		mcp_mesg_clear(&msg);
	}

	CLEAR(oper1);
	CLEAR(oper2);
}



void
fbgui_muf_event_cb(GUI_EVENT_CB_ARGS)
{
	char buf[BUFFER_LEN];
	struct frame *fr = (struct frame *) context;
	struct inst temp;

	temp.type = PROG_ARRAY;
	temp.data.array = new_array_dictionary();

	array_set_strkey_intval(&temp.data.array, "dismissed", did_dismiss);
	array_set_strkey_intval(&temp.data.array, "descr", descr);
	array_set_strkey_strval(&temp.data.array, "dlogid", dlogid);
	array_set_strkey_strval(&temp.data.array, "id", id);
	array_set_strkey_strval(&temp.data.array, "event", event);

	if (did_dismiss) {
		muf_dlog_remove(fr, dlogid);
	}

	sprintf(buf, "GUI.%s", dlogid);
	muf_event_add(fr, buf, &temp);
	CLEAR(&temp);
}



void
prim_gui_available(PRIM_PROTOTYPE)
{
	McpVer ver;
	float fver;

	CHECKOP(1);
	oper1 = POP();				/* descr */

	if (oper1->type != PROG_INTEGER)
		abort_interp("Integer descriptor number expected. (1)");

	ver = GuiVersion(oper1->data.number);
	fver = ver.vermajor + (ver.verminor / 1000.0);

	CLEAR(oper1);

	PushFloat(fver);
}



void
prim_gui_dlog_create(PRIM_PROTOTYPE)
{
	char *dlogid = NULL;
	char *title = NULL;
	char *wintype = NULL;
	stk_array *arr = NULL;
	McpMesg msg;
	McpFrame *mfr;

	CHECKOP(4);
	oper4 = POP();				/* arr  args */
	oper3 = POP();				/* str  title */
	oper2 = POP();				/* str  type */
	oper1 = POP();				/* int  descr */

	if (oper1->type != PROG_INTEGER)
		abort_interp("Integer descriptor number expected. (1)");
	if (!GuiSupported(oper1->data.number))
		abort_interp("The MCP GUI package is not supported for this connection.");
	if (oper2->type != PROG_STRING)
		abort_interp("Window type string expected. (3)");
	if (oper3->type != PROG_STRING)
		abort_interp("Window title string expected. (3)");
	if (oper4->type != PROG_ARRAY)
		abort_interp("Window options array expected. (4)");

	title = oper3->data.string ? oper3->data.string->data : "";
	wintype = oper2->data.string ? oper2->data.string->data : "simple";
	arr = oper4->data.array;
	mfr = descr_mcpframe(oper1->data.number);

	if (!mfr)
		abort_interp("Invalid descriptor number. (1)");

	dlogid = gui_dlog_alloc(oper1->data.number, fbgui_muf_event_cb, fr);
	mcp_mesg_init(&msg, GUI_PACKAGE, "dlog-create");
	mcp_mesg_arg_append(&msg, "title", title);

	result = stuff_dict_in_mesg(arr, &msg);
	if (result) {
		mcp_mesg_clear(&msg);
		switch (result) {
			case -1:
				abort_interp("Args dictionary can only have string keys. (4)");
				break;
			case -2:
				abort_interp("Args dictionary cannot have a null string key. (4)");
				break;
			case -3:
				abort_interp("Unsupported value type in list value. (4)");
				break;
			case -4:
				abort_interp("Unsupported value type in args dictionary. (4)");
				break;
		}
	}
	
	mcp_mesg_arg_remove(&msg, "type");
	mcp_mesg_arg_append(&msg, "type", wintype);
	mcp_mesg_arg_remove(&msg, "dlogid");
	mcp_mesg_arg_append(&msg, "dlogid", dlogid);

	mcp_frame_output_mesg(mfr, &msg);
	mcp_mesg_clear(&msg);
	muf_dlog_add(fr, dlogid);

	CLEAR(oper1);
	CLEAR(oper2);
	CLEAR(oper3);
	CLEAR(oper4);

	PushString(dlogid);
}



void
prim_gui_dlog_show(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();				/* str  dlogid */

	if (oper1->type != PROG_STRING)
		abort_interp("String dialog ID expected.");
	if (!oper1->data.string || !oper1->data.string->data[0])
		abort_interp("Invalid dialog ID.");

	result = GuiShow(oper1->data.string->data);
	if (result == EGUINOSUPPORT)
		abort_interp("GUI not available.  Internal error.");
	if (result == EGUINODLOG)
		abort_interp("Invalid dialog ID.");

	CLEAR(oper1);
}


void
prim_gui_dlog_close(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();				/* str  dlogid */

	if (oper1->type != PROG_STRING)
		abort_interp("String dialog ID expected.");
	if (!oper1->data.string || !oper1->data.string->data[0])
		abort_interp("Invalid dialog ID.");

	result = GuiClose(oper1->data.string->data);
	if (result == EGUINOSUPPORT)
		abort_interp("Internal error: GUI not available.");
	if (result == EGUINODLOG)
		abort_interp("Invalid dialog ID.");

	muf_dlog_remove(fr, oper1->data.string->data);

	CLEAR(oper1);
}



void
prim_gui_ctrl_create(PRIM_PROTOTYPE)
{
	int vallines = 0;
	char **vallist = NULL;
	char *dlogid = NULL;
	char *ctrlid = NULL;
	char *ctrltype = NULL;
	stk_array *arr;
	McpMesg msg;
	McpFrame *mfr;
	int descr;
	int i;
	char cmdname[64];

	CHECKOP(4);
	oper4 = POP();				/* dict args */
	oper3 = POP();				/* str  ctrlid */
	oper2 = POP();				/* str  type */
	oper1 = POP();				/* str  dlogid */

	if (oper1->type != PROG_STRING)
		abort_interp("Dialog ID string expected. (1)");
	if (!oper1->data.string || !oper1->data.string->data[0])
		abort_interp("Invalid dialog ID. (1)");

	if (oper2->type != PROG_STRING)
		abort_interp("Control type string expected. (2)");
	if (!oper2->data.string || !oper2->data.string->data[0])
		abort_interp("Invalid control type. (2)");

	if (oper3->type != PROG_STRING)
		abort_interp("Control ID string expected. (3)");

	if (oper4->type != PROG_ARRAY)
		abort_interp("Dictionary of arguments expected. (4)");

	dlogid = oper1->data.string->data;
	ctrltype = oper2->data.string->data;
	ctrlid = oper3->data.string ? oper3->data.string->data : NULL;
	arr = oper4->data.array;

	descr = gui_dlog_get_descr(dlogid);
	mfr = descr_mcpframe(descr);

	if (!mfr)
		abort_interp("No such dialog currently exists. (1)");

	if (!GuiSupported(descr))
		abort_interp("Internal error: The given dialog's descriptor doesn't support the GUI package. (1)");

	sprintf(cmdname, "ctrl-%.55s", ctrltype);
	mcp_mesg_init(&msg, GUI_PACKAGE, cmdname);

	result = stuff_dict_in_mesg(arr, &msg);
	if (result) {
		mcp_mesg_clear(&msg);
		switch (result) {
			case -1:
				abort_interp("Args dictionary can only have string keys. (4)");
				break;
			case -2:
				abort_interp("Args dictionary cannot have a null string key. (4)");
				break;
			case -3:
				abort_interp("Unsupported value type in list value. (4)");
				break;
			case -4:
				abort_interp("Unsupported value type in args dictionary. (4)");
				break;
		}
	}
	
	vallines = mcp_mesg_arg_linecount(&msg, "value");
	if (ctrlid && vallines > 0) {
		vallist = (char**)malloc(sizeof(char*) * vallines);
		for (i = 0; i < vallines; i++)
			vallist[i] = mcp_mesg_arg_getline(&msg, "value", i);
		gui_value_set_local(dlogid, ctrlid, vallines, vallist);
		free(vallist);
	}

	mcp_mesg_arg_remove(&msg, "dlogid");
	mcp_mesg_arg_append(&msg, "dlogid", dlogid);

	mcp_mesg_arg_remove(&msg, "id");
	mcp_mesg_arg_append(&msg, "id", ctrlid);

	mcp_frame_output_mesg(mfr, &msg);
	mcp_mesg_clear(&msg);

	CLEAR(oper1);
	CLEAR(oper2);
	CLEAR(oper3);
	CLEAR(oper4);
}


void
prim_gui_value_set(PRIM_PROTOTYPE)
{
	int count;
	int i;
	char buf[BUFFER_LEN];
	char *name;
	char *dlogid;
	char *value;
	char **valarray;
	struct inst temp1;
	array_data *temp2;

	CHECKOP(2);
	oper3 = POP();				/* str value  */
	oper2 = POP();				/* str ctrlid */
	oper1 = POP();				/* str dlogid */

	if (oper3->type != PROG_STRING && oper3->type != PROG_ARRAY)
		abort_interp("String or string list control value expected. (3)");

	if (oper2->type != PROG_STRING)
		abort_interp("String control ID expected. (2)");
	if (!oper2->data.string || !oper2->data.string->data[0])
		abort_interp("Invalid dialog ID. (2)");

	if (oper1->type != PROG_STRING)
		abort_interp("String dialog ID expected. (1)");
	if (!oper1->data.string || !oper1->data.string->data[0])
		abort_interp("Invalid dialog ID. (1)");

	dlogid = oper1->data.string->data;
	name = oper2->data.string->data;

	if (oper3->type == PROG_STRING) {
		count = 1;
		valarray = (char **) malloc(sizeof(char *) * count);

		value = oper3->data.string ? oper3->data.string->data : "";
		valarray[0] = (char *) malloc(sizeof(char) * strlen(value) + 1);

		strcpy(valarray[0], value);
	} else {
		count = array_count(oper3->data.array);
		valarray = (char **) malloc(sizeof(char *) * count);

		for (i = 0; i < count; i++) {
			temp1.type = PROG_INTEGER;
			temp1.data.number = i;

			temp2 = array_getitem(oper3->data.array, &temp1);
			if (!temp2) {
				break;
			}
			switch (temp2->type) {
			case PROG_STRING:
				value = DoNullInd(temp2->data.string);
				break;
			case PROG_INTEGER:
				sprintf(buf, "%d", temp2->data.number);
				value = buf;
				break;
			case PROG_OBJECT:
				sprintf(buf, "#%d", temp2->data.number);
				value = buf;
				break;
			case PROG_FLOAT:
				sprintf(buf, "%g", temp2->data.fnumber);
				value = buf;
				break;
			default:
				while (i-- > 0) {
					free(valarray[i]);
				}
				free(valarray);
				abort_interp("Unsupported value type in list value. (3)");
			}
			valarray[i] = (char *) malloc(sizeof(char) * strlen(value) + 1);

			strcpy(valarray[i], value);
		}
	}

	GuiSetVal(dlogid, name, count, valarray);

	while (count-- > 0) {
		free(valarray[count]);
	}
	free(valarray);

	CLEAR(oper1);
	CLEAR(oper2);
	CLEAR(oper3);
}


void
prim_gui_values_get(PRIM_PROTOTYPE)
{
	char *name;
	char *value;
	char *dlogid;
	stk_array *new;
	struct inst temp1;
	array_data temp2;

	CHECKOP(1);
	oper1 = POP();				/* str  dlogid */

	if (oper1->type != PROG_STRING)
		abort_interp("String dialog ID expected.");
	if (!oper1->data.string || !oper1->data.string->data[0])
		abort_interp("Invalid dialog ID.");

	dlogid = oper1->data.string->data;
	new = new_array_dictionary();
	name = GuiValueFirst(oper1->data.string->data);
	while (name) {
		int i;
		int lines = gui_value_linecount(dlogid, name);

		temp1.type = PROG_STRING;
		temp1.data.string = alloc_prog_string(name);

		temp2.type = PROG_ARRAY;
		temp2.data.array = new_array_packed(lines);

		for (i = 0; i < lines; i++) {
			struct inst temp3;
			array_data temp4;

			temp3.type = PROG_INTEGER;
			temp3.data.number = i;

			temp4.type = PROG_STRING;
			temp4.data.string = alloc_prog_string(gui_value_get(dlogid, name, i));

			array_setitem(&temp2.data.array, &temp3, &temp4);
			CLEAR(&temp3);
			CLEAR(&temp4);
		}
		array_setitem(&new, &temp1, &temp2);
		CLEAR(&temp1);
		CLEAR(&temp2);

		name = GuiValueNext(dlogid, name);
	}

	CLEAR(oper1);
	PushArrayRaw(new);
}
