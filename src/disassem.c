/* $Header$ */

/*
 * $Log: disassem.c,v $
 * Revision 1.8  2000/12/03 12:31:14  revar
 * Added TRY-CATCH-ENDCATCH exception handling.  (Major changes.)
 *
 * Revision 1.7  2000/07/18 23:44:13  winged
 * Removed ptrdiff_t cast from disassemble()'s sprintf()s
 *
 * Revision 1.6  2000/07/18 18:15:02  winged
 * Various fixes to support warning-free compiling under -Wall -Wstrict-prototypes -Wno-format -- not all warnings fixed or found yet
 *
 * Revision 1.5  2000/07/02 23:19:00  revar
 * Added support for the planned C-like language to muf-bytecode compiler.
 * Changed compiler to use a linking lookup table for address resolution.
 * Added PROG_SVAR_BANG and PROG_SVAR_AT support, for one-instr var get/set.
 *
 * Revision 1.4  2000/05/12 03:22:12  revar
 * Optimized CLEAR() and RCLEAR()
 * Optimized copyinst()
 * Optimized muf function header trilogy into one inst.
 *
 * Revision 1.3  2000/05/08 08:21:17  revar
 * Added VAR! directive to the muf compiler.
 * Added procedural argument variable declaration.  ie: : myfunc[ foo bar baz ]
 * Added MCP_REGISTER_EVENT for muf event based MCP handling.
 * Changed MCP_SEND to error out on non-support of MCP on the given connection.
 * Fixed Mucker Level error messages in MCP_REGISTER.
 *
 * Revision 1.2  2000/03/29 12:21:02  revar
 * Reformatted all code into consistent format.
 * 	Tabs are 4 spaces.
 * 	Indents are one tab.
 * 	Braces are generally K&R style.
 * Added ARRAY_DIFF, ARRAY_INTERSECT and ARRAY_UNION to man.txt.
 * Rewrote restart script as a bourne shell script.
 *
 * Revision 1.1.1.1  1999/12/16 03:23:29  revar
 * Initial Sourceforge checkin, fb6.00a29
 *
 * Revision 1.1.1.1  1999/12/12 07:27:43  foxen
 * Initial FB6 CVS checkin.
 *
 * Revision 1.2  1996/07/03 22:35:46  foxen
 * 5.55 checkpoint.
 *
 * Revision 1.1  1996/06/12 02:19:11  foxen
 * Initial revision
 *
 * Revision 5.3  1994/03/14  12:20:58  foxen
 * Fb5.20 release checkpoint.
 *
 * Revision 5.2  1994/01/18  20:52:20  foxen
 * Version 5.15 release.
 *
 * Revision 5.1  1993/12/17  00:07:33  foxen
 * initial revision.
 *
 * Revision 1.4  90/09/28  12:20:26  rearl
 * Added shared program strings.
 *
 * Revision 1.3  90/09/16  04:42:01  rearl
 * Preparation code added for disk-based MUCK.
 *
 * Revision 1.2  90/07/29  17:31:43  rearl
 * Added debugging patches from lpb@csadfa.cs.adfa.oz.au.
 *
 * Revision 1.1  90/07/19  23:03:29  casie
 * Initial revision
 *
 *
 */

#include "copyright.h"
#include "config.h"

#include "db.h"
#include "props.h"
#include "externs.h"
#include "interface.h"
#include "interp.h"
#include "inst.h"

void
disassemble(dbref player, dbref program)
{
	struct inst *curr;
	struct inst *codestart;
	int i;
	char buf[BUFFER_LEN];

	codestart = curr = PROGRAM_CODE(program);
	if (!PROGRAM_SIZ(program)) {
		notify(player, "Nothing to disassemble!");
		return;
	}
	for (i = 0; i < PROGRAM_SIZ(program); i++, curr++) {
		switch (curr->type) {
		case PROG_PRIMITIVE:
			if (curr->data.number >= BASE_MIN && curr->data.number <= BASE_MAX)
				sprintf(buf, "%d: (line %d) PRIMITIVE: %s", i,
						curr->line, base_inst[curr->data.number - BASE_MIN]);
			else
				sprintf(buf, "%d: (line %d) PRIMITIVE: %d", i, curr->line, curr->data.number);
			break;
		case PROG_MARK:
			sprintf(buf, "%d: (line %d) MARK", i, curr->line);
			break;
		case PROG_STRING:
			sprintf(buf, "%d: (line %d) STRING: \"%s\"", i, curr->line,
					curr->data.string ? curr->data.string->data : "");
			break;
		case PROG_ARRAY:
			sprintf(buf, "%d: (line %d) ARRAY: %d items", i, curr->line,
					curr->data.array ? curr->data.array->items : 0);
			break;
		case PROG_FUNCTION:
			sprintf(buf, "%d: (line %d) FUNCTION: %s, VARS: %d, ARGS: %d", i, curr->line,
					curr->data.mufproc->procname ? curr->data.mufproc->procname : "",
					curr->data.mufproc->vars, curr->data.mufproc->args);
			break;
		case PROG_LOCK:
			sprintf(buf, "%d: (line %d) LOCK: [%s]", i, curr->line,
					curr->data.lock == TRUE_BOOLEXP ? "TRUE_BOOLEXP" :
					unparse_boolexp(0, curr->data.lock, 0));
			break;
		case PROG_INTEGER:
			sprintf(buf, "%d: (line %d) INTEGER: %d", i, curr->line, curr->data.number);
			break;
		case PROG_FLOAT:
			sprintf(buf, " %d: (line %d) FLOAT: %g", i, curr->line, curr->data.fnumber);
			break;
		case PROG_ADD:
			sprintf(buf, "%d: (line %d) ADDRESS: %d", i,
					curr->line, (curr->data.addr->data - codestart));
			break;
		case PROG_TRY:
			sprintf(buf, "%d: (line %d) TRY: %d", i, curr->line, (curr->data.call - codestart));
			break;
		case PROG_IF:
			sprintf(buf, "%d: (line %d) IF: %d", i, curr->line, (curr->data.call - codestart));
			break;
		case PROG_JMP:
			sprintf(buf, "%d: (line %d) JMP: %d", i, curr->line, (curr->data.call - codestart));
			break;
		case PROG_EXEC:
			sprintf(buf, "%d: (line %d) EXEC: %d", i, curr->line, (curr->data.call - codestart));
			break;
		case PROG_OBJECT:
			sprintf(buf, "%d: (line %d) OBJECT REF: %d", i, curr->line, curr->data.number);
			break;
		case PROG_VAR:
			sprintf(buf, "%d: (line %d) VARIABLE: %d", i, curr->line, curr->data.number);
			break;
		case PROG_SVAR:
			sprintf(buf, "%d: (line %d) SCOPEDVAR: %d", i, curr->line, curr->data.number);
			break;
		case PROG_SVAR_AT:
			sprintf(buf, "%d: (line %d) GET SCOPEDVAR: %d", i, curr->line, curr->data.number);
			break;
		case PROG_SVAR_BANG:
			sprintf(buf, "%d: (line %d) SET SCOPEDVAR: %d", i, curr->line, curr->data.number);
			break;
		case PROG_LVAR:
			sprintf(buf, "%d: (line %d) LOCALVAR: %d", i, curr->line, curr->data.number);
			break;
		case PROG_CLEARED:
			sprintf(buf, "%d: (line ???) CLEARED INST AT %s:%d", i, (char *) curr->data.addr,
					curr->line);
		default:
			sprintf(buf, "%d: (line ???) UNKNOWN INST", i);
		}
		notify(player, buf);
	}
}
