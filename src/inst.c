
#include "copyright.h"
#include "config.h"

#include "db.h"
#include "tune.h"
#include "props.h"
#include "inst.h"
#include "externs.h"
#include "interface.h"
#include "strings.h"
#include "interp.h"

#undef DEBUGARRAYS

/* these arrays MUST agree with what's in inst.h */
const char *base_inst[] = {
	"JMP", "READ", "TREAD", "SLEEP", "CALL", "EXECUTE", "EXIT", "EVENT_WAIT",
	PRIMS_CONNECTS_NAMES,
	PRIMS_DB_NAMES,
	PRIMS_MATH_NAMES,
	PRIMS_MISC_NAMES,
	PRIMS_PROPS_NAMES,
	PRIMS_STACK_NAMES,
	PRIMS_STRINGS_NAMES,
	PRIMS_ARRAY_NAMES,
	PRIMS_FLOAT_NAMES,
	PRIMS_ERROR_NAMES,
	PRIMS_MCP_NAMES,
	PRIMS_INTERNAL_NAMES
};

/* converts an instruction into a printable string, stores the string in
   an internal buffer and returns a pointer to it */
char *
insttotext(struct inst *theinst, char *buffer, int buflen, int strmax, dbref program)
{
	char buf2[BUFFER_LEN];
	struct inst temp1;
	struct inst *oper2;
	int length;
	int firstflag = 1;

	switch (theinst->type) {
	case PROG_PRIMITIVE:
		if (theinst->data.number >= BASE_MIN && theinst->data.number <= BASE_MAX)
			strcpy(buffer, base_inst[theinst->data.number - BASE_MIN]);
		else
			strcpy(buffer, "???");
		break;
	case PROG_STRING:
		if (!theinst->data.string) {
			strcpy(buffer, "\"\"");
			break;
		}
		sprintf(buffer, "\"%1.*s", (strmax - 1), theinst->data.string->data);
		if (theinst->data.string->length <= strmax)
			strcat(buffer, "\"");
		else
			strcat(buffer, "\"_");
		break;
	case PROG_MARK:
		sprintf(buffer, "MARK");
		break;
	case PROG_ARRAY:
		if (!theinst->data.array) {
			strcpy(buffer, "1:0{}");
			break;
		}
		if (tp_expanded_debug) {
#ifdef DEBUGARRAYS
			sprintf(buffer, "R%dC%d{", theinst->data.array->links, theinst->data.array->items);
#else
			sprintf(buffer, "%d{", theinst->data.array->items);
#endif
			length = buflen - 10;
			firstflag = 1;
			if (array_first(theinst->data.array, &temp1)) {
				do {
					char *inststr;

					if (!firstflag) {
						strcat(buffer, " ");
						length--;
					}
					firstflag = 0;
					oper2 = array_getitem(theinst->data.array, &temp1);

					inststr = insttotext(&temp1, buf2, length, strmax, program);
					strcat(buffer, inststr);
					strcat(buffer, ":");
					length -= strlen(inststr) + 1;

					inststr = insttotext(oper2, buf2, length, strmax, program);
					strcat(buffer, inststr);
					length -= strlen(inststr);

					if (length < 1) {
						strcat(buffer, "_");
						length--;
						break;
					}
				} while (array_next(theinst->data.array, &temp1));
			}
			strcat(buffer, "}");
		} else {
			sprintf(buffer, "{%d item array}", theinst->data.array->items);
		}
		break;
	case PROG_INTEGER:
		sprintf(buffer, "%d", theinst->data.number);
		break;
	case PROG_FLOAT:
		sprintf(buffer, "%#g", theinst->data.fnumber);
		break;
	case PROG_ADD:
		if (theinst->data.addr->data->type == PROG_FUNCTION) {
			if (theinst->data.addr->progref != program)
				sprintf(buffer, "'#%d'%s", theinst->data.addr->progref,
						theinst->data.addr->data->data.string->data);
			else
				sprintf(buffer, "'%s", theinst->data.addr->data->data.string->data);
		} else {
			sprintf(buffer, "'line%d?", theinst->data.addr->data->line);
		}
		break;
	case PROG_DECLVAR:
		sprintf(buffer, "DECLVAR(%dvars)", theinst->data.number);
		break;
	case PROG_IF:
		sprintf(buffer, "IF->line%d", theinst->data.call->line);
		break;
	case PROG_EXEC:
		if (theinst->data.call->type == PROG_FUNCTION) {
			sprintf(buffer, "EXEC->%s", theinst->data.call->data.string->data);
		} else {
			sprintf(buffer, "EXEC->line%d", theinst->data.call->line);
		}
		break;
	case PROG_JMP:
		if (theinst->data.call->type == PROG_FUNCTION) {
			sprintf(buffer, "JMP->%s", theinst->data.call->data.string->data);
		} else {
			sprintf(buffer, "JMP->line%d", theinst->data.call->line);
		}
		break;
	case PROG_OBJECT:
		sprintf(buffer, "#%d", theinst->data.objref);
		break;
	case PROG_VAR:
		sprintf(buffer, "V%d", theinst->data.number);
		break;
	case PROG_SVAR:
		sprintf(buffer, "SV%d", theinst->data.number);
		break;
	case PROG_LVAR:
		sprintf(buffer, "LV%d", theinst->data.number);
		break;
	case PROG_FUNCTION:
		sprintf(buffer, "(%s)", theinst->data.string->data);
		break;
	case PROG_LOCK:
		if (theinst->data.lock == TRUE_BOOLEXP) {
			strcpy(buffer, "[TRUE_BOOLEXP]");
			break;
		}
		sprintf(buffer, "[%1.*s]", (strmax - 1), unparse_boolexp(0, theinst->data.lock, 0));
		break;
	case PROG_CLEARED:
		sprintf(buffer, "???<%s:%d>", (char *) theinst->data.addr, theinst->line);
		break;
	default:
		strcpy(buffer, "???");
		break;
	}
	return buffer;
}

/* produce one line summary of current state.  Note that sp is the next
   space on the stack -- 0..sp-1 is the current contents. */
char *
debug_inst(struct inst *pc, struct inst *stack, char *buffer, int buflen, int sp,
		   dbref program)
{
	char buf2[BUFFER_LEN];
	int count;

#ifdef DEBUGARRAYS
	sprintf(buffer, "Debug> %d #%d %d (", PROGRAM_INSTANCES(2), program, pc->line);
#else
	sprintf(buffer, "Debug> #%d %d (", program, pc->line);
#endif

	if (sp > 8)
		strcat(buffer, "..., ");
	count = (sp > 8) ? sp - 8 : 0;
	while (count < sp) {
		strcat(buffer, insttotext(stack + count, buf2, buflen, 30, program));
		if (++count < sp)
			strcat(buffer, ", ");
	}
	strcat(buffer, ") ");
	strcat(buffer, insttotext(pc, buf2, buflen, 30, program));
	return (buffer);
}
