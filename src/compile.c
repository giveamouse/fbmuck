/*
 *  Compile.c   (This is really a tokenizer, not a compiler)
 */

#include "copyright.h"
#include "config.h"

#include "db.h"
#include "props.h"
#include "interface.h"
#include "inst.h"
#include "externs.h"
#include "params.h"
#include "tune.h"
#include "match.h"
#include "interp.h"
#include <ctype.h>
#include <time.h>

/* This file contains code for doing "byte-compilation" of
   mud-forth programs.  As such, it contains many internal
   data structures and other such which are not found in other
   parts of TinyMUCK.                                       */

/* The IF_STACK is a stack for holding previous IF statements.
   Everytime a THEN is encountered, the next address is inserted
   into the code before the most recent IF.  */

#define CTYPE_IF    1
#define CTYPE_ELSE  2
#define CTYPE_BEGIN 3
#define CTYPE_FOR   4			/* Get it?  CTYPE_FOUR!!  HAHAHAHAHA  -Fre'ta */
					  /* C-4?  *BOOM!*  -Revar */
#define CTYPE_WHILE 5
#define CTYPE_TRY   6			/* reserved for exception handling */
#define CTYPE_CATCH 7			/* reserved for exception handling */


/* These would be constants, but their value isn't known until runtime. */
static int IN_FORITER;
static int IN_FOREACH;
static int IN_FORPOP;
static int IN_FOR;


static hash_tab primitive_list[COMP_HASH_SIZE];

struct IF_STACK {
	short type;
	struct INTERMEDIATE *place;
	struct IF_STACK *next;
};

/* This structure is an association list that contains both a procedure
   name and the place in the code that it belongs.  A lookup to the procedure
   will see both it's name and it's number and so we can generate a
   reference to it.  Since I want to disallow co-recursion,  I will not allow
   forward referencing.
   */

struct PROC_LIST {
	const char *name;
	struct INTERMEDIATE *code;
	struct PROC_LIST *next;
};

/* The intermediate code is code generated as a linked list
   when there is no hint or notion of how much code there
   will be, and to help resolve all references.
   There is always a pointer to the current word that is
   being compiled kept.                                   */

struct INTERMEDIATE {
	int no;						/* which number instruction this is */
	struct inst in;				/* instruction itself */
	short line;					/* line number of instruction */
	struct INTERMEDIATE *next;	/* next instruction */
};


/* The state structure for a compile. */
typedef struct COMPILE_STATE_T {
	struct IF_STACK *if_stack;
	struct IF_STACK *loop_stack;	/* WORK: Need to merge if_stack and loop_stack. */
	struct PROC_LIST *procs;

	int nowords;				/* number of words compiled */
	struct INTERMEDIATE *curr_word;	/* word being compiled */
	struct INTERMEDIATE *first_word;	/* first word of the list */
	struct INTERMEDIATE *curr_proc;	/* first word of curr. proc. */
	struct INTERMEDIATE *curr_decl;	/* DECLVAR for curr proc. */
	struct publics *currpubs;
	int nested_fors;

	/* variable names.  The index into cstat->variables give you what position
	 * the variable holds.
	 */
	const char *variables[MAX_VAR];
	const char *localvars[MAX_VAR];
	const char *scopedvars[MAX_VAR];

	struct line *curr_line;		/* current line */
	int lineno;					/* current line number */
	const char *next_char;		/* next char * */
	dbref player, program;		/* player and program for this compile */

	int compile_err;			/* 1 if error occured */

	char *line_copy;
	int macrosubs;				/* Safeguard for macro-subst. infinite loops */
	int descr;					/* the descriptor that initiated compiling */
	int force_err_display;		/* If true, always show compiler errors. */
	struct INTERMEDIATE *nextinst;
	hash_tab defhash[DEFHASHSIZE];
} COMPSTATE;


int primitive(const char *s);	/* returns primitive_number if

								 * primitive */
void free_prog(dbref);
const char *next_token(COMPSTATE *);
const char *next_token_raw(COMPSTATE *);
struct INTERMEDIATE *next_word(COMPSTATE *, const char *);
struct INTERMEDIATE *process_special(COMPSTATE *, const char *);
struct INTERMEDIATE *primitive_word(COMPSTATE *, const char *);
struct INTERMEDIATE *string_word(COMPSTATE *, const char *);
struct INTERMEDIATE *number_word(COMPSTATE *, const char *);
struct INTERMEDIATE *float_word(COMPSTATE *, const char *);
struct INTERMEDIATE *object_word(COMPSTATE *, const char *);
struct INTERMEDIATE *quoted_word(COMPSTATE *, const char *);
struct INTERMEDIATE *call_word(COMPSTATE *, const char *);
struct INTERMEDIATE *var_word(COMPSTATE *, const char *);
struct INTERMEDIATE *lvar_word(COMPSTATE *, const char *);
struct INTERMEDIATE *svar_word(COMPSTATE *, const char *);
const char *do_string(COMPSTATE *);
void do_comment(COMPSTATE *);
void do_directive(COMPSTATE *, char *direct);
struct prog_addr *alloc_addr(COMPSTATE *, int, struct inst *);
struct INTERMEDIATE *new_inst(COMPSTATE *);
struct INTERMEDIATE *locate_if(COMPSTATE *);
struct INTERMEDIATE *find_if(COMPSTATE *);
struct INTERMEDIATE *find_else(COMPSTATE *);
struct INTERMEDIATE *locate_begin(COMPSTATE *);
struct INTERMEDIATE *locate_for(COMPSTATE *);
struct INTERMEDIATE *find_begin(COMPSTATE *);
struct INTERMEDIATE *find_while(COMPSTATE *);
void cleanpubs(struct publics *mypub);
void clean_mcpbinds(struct mcp_binding *mcpbinds);
void cleanup(COMPSTATE *);
void add_proc(COMPSTATE *, const char *, struct INTERMEDIATE *);
void addif(COMPSTATE *, struct INTERMEDIATE *);
void addelse(COMPSTATE *, struct INTERMEDIATE *);
void addbegin(COMPSTATE *, struct INTERMEDIATE *);
void addfor(COMPSTATE *, struct INTERMEDIATE *);
void addwhile(COMPSTATE *, struct INTERMEDIATE *);
void resolve_loop_addrs(COMPSTATE *, int where);
int add_variable(COMPSTATE *, const char *);
int add_localvar(COMPSTATE *, const char *);
int add_scopedvar(COMPSTATE *, const char *);
int special(const char *);
int call(COMPSTATE *, const char *);
int quoted(COMPSTATE *, const char *);
int object(const char *);
int string(const char *);
int variable(COMPSTATE *, const char *);
int localvar(COMPSTATE *, const char *);
int scopedvar(COMPSTATE *, const char *);
void copy_program(COMPSTATE *);
void set_start(COMPSTATE *);

/* Character defines */
#define BEGINCOMMENT '('
#define ENDCOMMENT ')'
#define BEGINSTRING '"'
#define ENDSTRING '"'
#define BEGINMACRO '.'
#define BEGINDIRECTIVE '$'
#define BEGINESCAPE '\\'

#define SUBSTITUTIONS 20		/* How many nested macros will we allow? */

void
do_abort_compile(COMPSTATE * cstat, const char *c)
{
	static char _buf[BUFFER_LEN];

	sprintf(_buf, "Error in line %d: %s", cstat->lineno, c);
	if (cstat->line_copy) {
		free((void *) cstat->line_copy);
		cstat->line_copy = NULL;
	}
	if (((FLAGS(cstat->player) & INTERACTIVE) && !(FLAGS(cstat->player) & READMODE)) ||
		cstat->force_err_display) {
		notify_nolisten(cstat->player, _buf, 1);
	} else {
		log_muf("MUF compiler warning in program %d:\n%s\n", cstat->program, _buf);
	}
	cstat->compile_err++;
	if (cstat->compile_err > 1) {
		return;
	}
	if (cstat->nextinst) {
		free(cstat->nextinst);
		cstat->nextinst = NULL;
	}
	cleanup(cstat);
	cleanpubs(cstat->currpubs);
	cstat->currpubs = NULL;
	free_prog(cstat->program);
	cleanpubs(PROGRAM_PUBS(cstat->program));
	PROGRAM_SET_PUBS(cstat->program, NULL);
	clean_mcpbinds(PROGRAM_MCPBINDS(cstat->program));
	PROGRAM_SET_MCPBINDS(cstat->program, NULL);
}

/* abort compile macro */
#define abort_compile(ST,C) { do_abort_compile(ST,C); return 0; }

/* abort compile for void functions */
#define v_abort_compile(ST,C) { do_abort_compile(ST,C); return; }


void
fixpubs(struct publics *mypubs, struct inst *offset)
{
	while (mypubs) {
		mypubs->addr.ptr = offset + mypubs->addr.no;
		mypubs = mypubs->next;
	}
}


int
size_pubs(struct publics *mypubs)
{
	int bytes = 0;

	while (mypubs) {
		bytes += sizeof(*mypubs);
		mypubs = mypubs->next;
	}
	return bytes;
}



char *
expand_def(COMPSTATE * cstat, const char *defname)
{
	hash_data *exp = find_hash(defname, cstat->defhash, DEFHASHSIZE);

	if (!exp) {
		if (*defname == BEGINMACRO) {
			return (macro_expansion(macrotop, &defname[1]));
		} else {
			return (NULL);
		}
	}
	return (string_dup((char *) exp->pval));
}


void
kill_def(COMPSTATE * cstat, const char *defname)
{
	hash_data *exp = find_hash(defname, cstat->defhash, DEFHASHSIZE);

	if (exp) {
		free(exp->pval);
		(void) free_hash(defname, cstat->defhash, DEFHASHSIZE);
	}
}


void
insert_def(COMPSTATE * cstat, const char *defname, const char *deff)
{
	hash_data hd;

	(void) kill_def(cstat, defname);
	hd.pval = (void *) string_dup(deff);
	(void) add_hash(defname, hd, cstat->defhash, DEFHASHSIZE);
}


void
insert_intdef(COMPSTATE * cstat, const char *defname, int deff)
{
	char buf[BUFFER_LEN];

	sprintf(buf, "%d", deff);
	insert_def(cstat, defname, buf);
}


void
purge_defs(COMPSTATE * cstat)
{
	kill_hash(cstat->defhash, DEFHASHSIZE, 1);
}


void
include_defs(COMPSTATE * cstat, dbref i)
{
	char dirname[BUFFER_LEN];
	char temp[BUFFER_LEN];
	const char *tmpptr;
	PropPtr j, pptr;

	strcpy(dirname, "/_defs/");
	j = first_prop(i, dirname, &pptr, temp);
	while (j) {
		strcpy(dirname, "/_defs/");
		strcat(dirname, temp);
		tmpptr = uncompress(get_property_class(i, dirname));
		if (tmpptr && *tmpptr)
			insert_def(cstat, temp, (char *) tmpptr);
		j = next_prop(pptr, j, temp);
	}
}


void
init_defs(COMPSTATE * cstat)
{
	/* initialize hash table */
	int i;

	for (i = 0; i < DEFHASHSIZE; i++) {
		cstat->defhash[i] = NULL;
	}

	/* Create standard server defines */
	insert_def(cstat, "__version", VERSION);
	insert_def(cstat, "__muckname", tp_muckname);
	insert_def(cstat, "strip", "striplead striptail");
	insert_def(cstat, "instring", "tolower swap tolower swap instr");
	insert_def(cstat, "rinstring", "tolower swap tolower swap rinstr");
	insert_intdef(cstat, "bg_mode", BACKGROUND);
	insert_intdef(cstat, "fg_mode", FOREGROUND);
	insert_intdef(cstat, "pr_mode", PREEMPT);
	insert_intdef(cstat, "max_variable_count", MAX_VAR);

	/* make defines for compatability to removed primitives */
	insert_def(cstat, "desc", "\"_/de\" getpropstr");
	insert_def(cstat, "succ", "\"_/sc\" getpropstr");
	insert_def(cstat, "fail", "\"_/fl\" getpropstr");
	insert_def(cstat, "drop", "\"_/dr\" getpropstr");
	insert_def(cstat, "osucc", "\"_/osc\" getpropstr");
	insert_def(cstat, "ofail", "\"_/ofl\" getpropstr");
	insert_def(cstat, "odrop", "\"_/odr\" getpropstr");
	insert_def(cstat, "setdesc", "\"_/de\" swap 0 addprop");
	insert_def(cstat, "setsucc", "\"_/sc\" swap 0 addprop");
	insert_def(cstat, "setfail", "\"_/fl\" swap 0 addprop");
	insert_def(cstat, "setdrop", "\"_/dr\" swap 0 addprop");
	insert_def(cstat, "setosucc", "\"_/osc\" swap 0 addprop");
	insert_def(cstat, "setofail", "\"_/ofl\" swap 0 addprop");
	insert_def(cstat, "setodrop", "\"_/odr\" swap 0 addprop");
	insert_def(cstat, "preempt", "pr_mode setmode");
	insert_def(cstat, "background", "bg_mode setmode");
	insert_def(cstat, "foreground", "fg_mode setmode");
	insert_def(cstat, "notify_except", "1 swap notify_exclude");

	/* MUF Error defines */
	insert_def(cstat, "err_divzero?", "0 is_set?");
	insert_def(cstat, "err_nan?", "1 is_set?");
	insert_def(cstat, "err_imaginary?", "2 is_set?");
	insert_def(cstat, "err_fbounds?", "3 is_set?");
	insert_def(cstat, "err_ibounds?", "4 is_set?");

	/* array convenience defines */
	insert_def(cstat, "}list", "} array_make");
	insert_def(cstat, "}dict", "}  2 / array_make_dict");
	insert_def(cstat, "[]", "array_getitem");
	insert_def(cstat, "[..]", "array_getrange");
	insert_def(cstat, "array_diff", "2 array_ndiff");
	insert_def(cstat, "array_union", "2 array_nunion");
	insert_def(cstat, "array_intersect", "2 array_nintersect");

	/* GUI control types */
	insert_def(cstat, "c_datum", "\"datum\"");
	insert_def(cstat, "c_label", "\"text\"");
	insert_def(cstat, "c_hrule", "\"hrule\"");
	insert_def(cstat, "c_vrule", "\"vrule\"");
	insert_def(cstat, "c_button", "\"button\"");
	insert_def(cstat, "c_checkbox", "\"checkbox\"");
	insert_def(cstat, "c_edit", "\"edit\"");
	insert_def(cstat, "c_multiedit", "\"multiedit\"");
	insert_def(cstat, "c_combobox", "\"combobox\"");
	insert_def(cstat, "c_spinner", "\"spinner\"");
	insert_def(cstat, "c_scale", "\"scale\"");
	insert_def(cstat, "c_listbox", "\"listbox\"");
	insert_def(cstat, "c_frame", "\"frame\"");
	insert_def(cstat, "c_notebook", "\"notebook\"");
	/* insert_def(cstat, "ctrl_image",     "\"image\""); */
	/* insert_def(cstat, "ctrl_radiobtn",  "\"radio\""); */

	/* include any defines set in #0's _defs/ propdir. */
	include_defs(cstat, (dbref) 0);

	/* include any defines set in program owner's _defs/ propdir. */
	include_defs(cstat, OWNER(cstat->program));
}


void
uncompile_program(dbref i)
{
	/* free program */
	(void) dequeue_prog(i, 1);
	free_prog(i);
	cleanpubs(PROGRAM_PUBS(i));
	PROGRAM_SET_PUBS(i, NULL);
	clean_mcpbinds(PROGRAM_MCPBINDS(i));
	PROGRAM_SET_MCPBINDS(i, NULL);
	PROGRAM_SET_CODE(i, NULL);
	PROGRAM_SET_SIZ(i, 0);
	PROGRAM_SET_START(i, NULL);
}


void
do_uncompile(dbref player)
{
	dbref i;

	if (!Wizard(OWNER(player))) {
		notify_nolisten(player, "Permission denied.", 1);
		return;
	}
	for (i = 0; i < db_top; i++) {
		if (Typeof(i) == TYPE_PROGRAM) {
			uncompile_program(i);
		}
	}
	notify_nolisten(player, "All programs decompiled.", 1);
}


void
free_unused_programs()
{
	dbref i;
	time_t now = time(NULL);

	for (i = 0; i < db_top; i++) {
		if ((Typeof(i) == TYPE_PROGRAM) && !(FLAGS(i) & (ABODE | INTERNAL)) &&
			(now - DBFETCH(i)->ts.lastused > tp_clean_interval) && (PROGRAM_INSTANCES(i) == 0)) {
			uncompile_program(i);
		}
	}
}


/* overall control code.  Does piece-meal tokenization parsing and
   backward checking.                                            */
void
do_compile(int descr, dbref player_in, dbref program_in, int force_err_display)
{
	const char *token;
	struct INTERMEDIATE *new_word;
	int i;
	COMPSTATE cstat;

	/* set all compile state variables */
	cstat.force_err_display = force_err_display;
	cstat.descr = descr;
	cstat.if_stack = 0;
	cstat.loop_stack = 0;
	cstat.procs = 0;
	cstat.nowords = 0;
	cstat.curr_word = cstat.first_word = NULL;
	cstat.curr_proc = cstat.curr_decl = NULL;
	cstat.currpubs = NULL;
	cstat.nested_fors = 0;
	for (i = 0; i < MAX_VAR; i++) {
		cstat.variables[i] = NULL;
		cstat.localvars[i] = NULL;
		cstat.scopedvars[i] = NULL;
	}
	cstat.curr_line = PROGRAM_FIRST(program_in);
	cstat.lineno = 1;
	cstat.next_char = NULL;
	if (cstat.curr_line)
		cstat.next_char = cstat.curr_line->this_line;
	cstat.player = player_in;
	cstat.program = program_in;
	cstat.compile_err = 0;
	cstat.line_copy = NULL;
	cstat.macrosubs = 0;
	cstat.nextinst = NULL;
	init_defs(&cstat);

	cstat.variables[0] = "ME";
	cstat.variables[1] = "LOC";
	cstat.variables[2] = "TRIGGER";
	cstat.variables[3] = "COMMAND";

	/* free old stuff */
	(void) dequeue_prog(cstat.program, 1);
	free_prog(cstat.program);
	cleanpubs(PROGRAM_PUBS(cstat.program));
	PROGRAM_SET_PUBS(cstat.program, NULL);
	clean_mcpbinds(PROGRAM_MCPBINDS(cstat.program));
	PROGRAM_SET_MCPBINDS(cstat.program, NULL);

	if (!cstat.curr_line)
		v_abort_compile(&cstat, "Missing program text.");

	/* do compilation */
	while ((token = next_token(&cstat))) {
		new_word = next_word(&cstat, token);

		/* test for errors */
		if (cstat.compile_err)
			return;

		if (new_word) {
			if (!cstat.first_word)
				cstat.first_word = cstat.curr_word = new_word;
			else {
				cstat.curr_word->next = new_word;
				cstat.curr_word = cstat.curr_word->next;
			}
		}
		while (cstat.curr_word && cstat.curr_word->next)
			cstat.curr_word = cstat.curr_word->next;

		free((void *) token);
	}

	if (cstat.curr_proc)
		v_abort_compile(&cstat, "Unexpected end of file.");

	if (!cstat.procs)
		v_abort_compile(&cstat, "Missing procedure definition.");

	/* do copying over */
	copy_program(&cstat);
	fixpubs(cstat.currpubs, PROGRAM_CODE(cstat.program));
	PROGRAM_SET_PUBS(cstat.program, cstat.currpubs);

	if (cstat.nextinst) {
		free(cstat.nextinst);
		cstat.nextinst = NULL;
	}
	if (cstat.compile_err)
		return;

	set_start(&cstat);
	cleanup(&cstat);

	/* restart AUTOSTART program. */
	if ((FLAGS(cstat.program) & ABODE) && TrueWizard(OWNER(cstat.program)))
		add_muf_queue_event(-1, OWNER(cstat.program), NOTHING, NOTHING,
							cstat.program, "Startup", "Queued Event.", 0);

	if (force_err_display)
		notify_nolisten(cstat.player, "Program compiled successfully.", 1);
}

struct INTERMEDIATE *
next_word(COMPSTATE * cstat, const char *token)
{
	struct INTERMEDIATE *new_word;
	static char buf[BUFFER_LEN];

	if (!token)
		return 0;

	if (call(cstat, token))
		new_word = call_word(cstat, token);
	else if (scopedvar(cstat, token))
		new_word = svar_word(cstat, token);
	else if (localvar(cstat, token))
		new_word = lvar_word(cstat, token);
	else if (variable(cstat, token))
		new_word = var_word(cstat, token);
	else if (special(token))
		new_word = process_special(cstat, token);
	else if (primitive(token))
		new_word = primitive_word(cstat, token);
	else if (string(token))
		new_word = string_word(cstat, token + 1);
	else if (number(token))
		new_word = number_word(cstat, token);
	else if (ifloat(token))
		new_word = float_word(cstat, token);
	else if (object(token))
		new_word = object_word(cstat, token);
	else if (quoted(cstat, token))
		new_word = quoted_word(cstat, token + 1);
	else {
		sprintf(buf, "Unrecognized word %s.", token);
		abort_compile(cstat, buf);
	}
	return new_word;
}



/* Little routine to do the line_copy handling right */
void
advance_line(COMPSTATE * cstat)
{
	cstat->curr_line = cstat->curr_line->next;
	cstat->lineno++;
	cstat->macrosubs = 0;
	if (cstat->line_copy) {
		free((void *) cstat->line_copy);
		cstat->line_copy = NULL;
	}
	if (cstat->curr_line)
		cstat->next_char = (cstat->line_copy = alloc_string(cstat->curr_line->this_line));
	else
		cstat->next_char = (cstat->line_copy = NULL);
}

/* Skips comments, grabs strings, returns NULL when no more tokens to grab. */
const char *
next_token_raw(COMPSTATE * cstat)
{
	static char buf[BUFFER_LEN];
	int i;

	if (!cstat->curr_line)
		return (char *) 0;

	if (!cstat->next_char)
		return (char *) 0;

	/* skip white space */
	while (*cstat->next_char && isspace(*cstat->next_char))
		cstat->next_char++;

	if (!(*cstat->next_char)) {
		advance_line(cstat);
		return next_token_raw(cstat);
	}
	/* take care of comments */
	if (*cstat->next_char == BEGINCOMMENT) {
		do_comment(cstat);
		return next_token_raw(cstat);
	}
	if (*cstat->next_char == BEGINSTRING)
		return do_string(cstat);

	for (i = 0; *cstat->next_char && !isspace(*cstat->next_char); i++) {
		buf[i] = *cstat->next_char;
		cstat->next_char++;
	}
	buf[i] = '\0';
	return alloc_string(buf);
}


const char *
next_token(COMPSTATE * cstat)
{
	char *expansion, *temp;

	temp = (char *) next_token_raw(cstat);
	if (!temp)
		return NULL;

	if (temp[0] == BEGINDIRECTIVE) {
		do_directive(cstat, temp);
		free(temp);
		return next_token(cstat);
	}
	if (temp[0] == BEGINESCAPE) {
		if (temp[1]) {
			expansion = temp;
			temp = (char *) malloc(strlen(expansion));
			strcpy(temp, (expansion + 1));
			free(expansion);
		}
		return (temp);
	}
	if ((expansion = expand_def(cstat, temp))) {
		free(temp);
		if (++cstat->macrosubs > SUBSTITUTIONS) {
			abort_compile(cstat, "Too many macro substitutions.");
		} else {
			temp = (char *) malloc(strlen(cstat->next_char) + strlen(expansion) + 21);
			strcpy(temp, expansion);
			strcat(temp, cstat->next_char);
			free((void *) expansion);
			if (cstat->line_copy) {
				free((void *) cstat->line_copy);
			}
			cstat->next_char = cstat->line_copy = temp;
			return next_token(cstat);
		}
	} else {
		return (temp);
	}
}



/* skip comments */
void
do_comment(COMPSTATE * cstat)
{
	while (*cstat->next_char && *cstat->next_char != ENDCOMMENT)
		cstat->next_char++;
	if (!(*cstat->next_char)) {
		advance_line(cstat);
		if (!cstat->curr_line) {
			v_abort_compile(cstat, "Unterminated comment.");
		}
		do_comment(cstat);
	} else {
		cstat->next_char++;
		if (!(*cstat->next_char))
			advance_line(cstat);
	}
}

/* handle compiler directives */
void
do_directive(COMPSTATE * cstat, char *direct)
{
	char temp[BUFFER_LEN];
	char *tmpname;
	char *tmpptr;
	int i = 0;
	int j;

	strcpy(temp, ++direct);

	if (!(temp[0])) {
		v_abort_compile(cstat, "I don't understand that compiler directive!");
	}
	if (!string_compare(temp, "define")) {
		tmpname = (char *) next_token_raw(cstat);
		if (!tmpname)
			v_abort_compile(cstat, "Unexpected end of file looking for $define name.");
		i = 0;
		while ((tmpptr = (char *) next_token_raw(cstat)) &&
			   (string_compare(tmpptr, "$enddef"))) {
			char *cp;

			for (cp = tmpptr; i < (BUFFER_LEN / 2) && *cp;) {
				if (*tmpptr == BEGINSTRING && cp != tmpptr &&
					(*cp == ENDSTRING || *cp == BEGINESCAPE)) {
					temp[i++] = BEGINESCAPE;
				}
				temp[i++] = *cp++;
			}
			if (*tmpptr == BEGINSTRING)
				temp[i++] = ENDSTRING;
			temp[i++] = ' ';
			free(tmpptr);
			if (i > (BUFFER_LEN / 2))
				v_abort_compile(cstat, "$define definition too long.");
		}
		if (i)
			i--;
		temp[i] = '\0';
		if (!tmpptr)
			v_abort_compile(cstat, "Unexpected end of file in $define definition.");
		free(tmpptr);
		(void) insert_def(cstat, tmpname, temp);
		free(tmpname);

	} else if (!string_compare(temp, "enddef")) {
		v_abort_compile(cstat, "$enddef without a previous matching $define.");

	} else if (!string_compare(temp, "def")) {
		tmpname = (char *) next_token_raw(cstat);
		if (!tmpname)
			v_abort_compile(cstat, "Unexpected end of file looking for $def name.");
		(void) insert_def(cstat, tmpname, cstat->next_char);
		while (*cstat->next_char)
			cstat->next_char++;
		advance_line(cstat);
		free(tmpname);

	} else if (!string_compare(temp, "include")) {
		struct match_data md;

		tmpname = (char *) next_token_raw(cstat);
		if (!tmpname)
			v_abort_compile(cstat, "Unexpected end of file while doing $include.");
		{
			char tempa[BUFFER_LEN], tempb[BUFFER_LEN];

			strcpy(tempa, match_args);
			strcpy(tempb, match_cmdname);
			init_match(cstat->descr, cstat->player, tmpname, NOTYPE, &md);
			match_registered(&md);
			match_absolute(&md);
			match_me(&md);
			i = (int) match_result(&md);
			strcpy(match_args, tempa);
			strcpy(match_cmdname, tempb);
		}
		free(tmpname);
		if (((dbref) i == NOTHING) || (i < 0) || (i > db_top)
			|| (Typeof(i) == TYPE_GARBAGE))
			v_abort_compile(cstat, "I don't understand what object you want to $include.");
		include_defs(cstat, (dbref) i);

	} else if (!string_compare(temp, "undef")) {
		tmpname = (char *) next_token_raw(cstat);
		if (!tmpname)
			v_abort_compile(cstat, "Unexpected end of file looking for name to $undef.");
		kill_def(cstat, tmpname);
		free(tmpname);

	} else if (!string_compare(temp, "echo")) {
		notify_nolisten(cstat->player, cstat->next_char, 1);
		while (*cstat->next_char)
			cstat->next_char++;
		advance_line(cstat);

	} else if (!string_compare(temp, "ifdef")) {
		tmpname = (char *) next_token_raw(cstat);
		if (!tmpname)
			v_abort_compile(cstat, "Unexpected end of file looking for $ifdef condition.");
		strcpy(temp, tmpname);
		free(tmpname);
		for (i = 1; temp[i] && (temp[i] != '=') && (temp[i] != '>') && (temp[i] != '<'); i++) ;
		tmpname = &(temp[i]);
		i = (temp[i] == '>') ? 1 : ((temp[i] == '=') ? 0 : ((temp[i] == '<') ? -1 : -2));
		*tmpname = '\0';
		tmpname++;
		tmpptr = (char *) expand_def(cstat, temp);
		if (i == -2) {
			j = (!tmpptr);
			if (tmpptr)
				free(tmpptr);
		} else {
			if (!tmpptr) {
				j = 1;
			} else {
				j = string_compare(tmpptr, tmpname);
				j = !((!i && !j) || ((i * j) > 0));
				free(tmpptr);
			}
		}
		if (j) {
			i = 0;
			while ((tmpptr = (char *) next_token_raw(cstat)) &&
				   (i || ((string_compare(tmpptr, "$else"))
						  && (string_compare(tmpptr, "$endif"))))) {
				if (!string_compare(tmpptr, "$ifdef"))
					i++;
				else if (!string_compare(tmpptr, "$ifndef"))
					i++;
				else if (!string_compare(tmpptr, "$endif"))
					i--;
				free(tmpptr);
			}
			if (!tmpptr) {
				v_abort_compile(cstat, "Unexpected end of file in $ifdef clause.");
			}
			free(tmpptr);
		}
	} else if (!string_compare(temp, "ifndef")) {
		tmpname = (char *) next_token_raw(cstat);
		if (!tmpname) {
			v_abort_compile(cstat, "Unexpected end of file looking for $ifndef condition.");
		}
		strcpy(temp, tmpname);
		free(tmpname);
		for (i = 1; temp[i] && (temp[i] != '=') && (temp[i] != '>') && (temp[i] != '<'); i++) ;
		tmpname = &(temp[i]);
		i = (temp[i] == '>') ? 1 : ((temp[i] == '=') ? 0 : ((temp[i] == '<') ? -1 : -2));
		*tmpname = '\0';
		tmpname++;
		tmpptr = (char *) expand_def(cstat, temp);
		if (i == -2) {
			j = (!tmpptr);
			if (tmpptr)
				free(tmpptr);
		} else {
			if (!tmpptr) {
				j = 1;
			} else {
				j = string_compare(tmpptr, tmpname);
				j = !((!i && !j) || ((i * j) > 0));
				free(tmpptr);
			}
		}
		if (!j) {
			i = 0;
			while ((tmpptr = (char *) next_token_raw(cstat)) &&
				   (i || ((string_compare(tmpptr, "$else"))
						  && (string_compare(tmpptr, "$endif"))))) {
				if (!string_compare(tmpptr, "$ifdef"))
					i++;
				else if (!string_compare(tmpptr, "$ifndef"))
					i++;
				else if (!string_compare(tmpptr, "$endif"))
					i--;
				free(tmpptr);
			}
			if (!tmpptr) {
				v_abort_compile(cstat, "Unexpected end of file in $ifndef clause.");
			}
			free(tmpptr);
		}
	} else if (!string_compare(temp, "else")) {
		i = 0;
		while ((tmpptr = (char *) next_token_raw(cstat)) &&
			   (i || (string_compare(tmpptr, "$endif")))) {
			if (!string_compare(tmpptr, "$ifdef"))
				i++;
			else if (!string_compare(tmpptr, "$ifndef"))
				i++;
			else if (!string_compare(tmpptr, "$endif"))
				i--;
			free(tmpptr);
		}
		if (!tmpptr) {
			v_abort_compile(cstat, "Unexpected end of file in $else clause.");
		}
		free(tmpptr);

	} else if (!string_compare(temp, "endif")) {

	} else {
		v_abort_compile(cstat, "Unrecognized compiler directive.");
	}
}


/* return string */
const char *
do_string(COMPSTATE * cstat)
{
	static char buf[BUFFER_LEN];
	int i = 0, quoted = 0;

	buf[i] = *cstat->next_char;
	cstat->next_char++;
	i++;
	while ((quoted || *cstat->next_char != ENDSTRING) && *cstat->next_char) {
		if (*cstat->next_char == '\\' && !quoted) {
			quoted++;
			cstat->next_char++;
		} else if (*cstat->next_char == 'r' && quoted) {
			buf[i++] = '\r';
			cstat->next_char++;
			quoted = 0;
		} else if (*cstat->next_char == '[' && quoted) {
			buf[i++] = ESCAPE_CHAR;
			cstat->next_char++;
			quoted = 0;
		} else {
			buf[i] = *cstat->next_char;
			i++;
			cstat->next_char++;
			quoted = 0;
		}
	}
	if (!*cstat->next_char) {
		abort_compile(cstat, "Unterminated string found at end of line.");
	}
	cstat->next_char++;
	buf[i] = '\0';
	return alloc_string(buf);
}



/* process special.  Performs special processing.
   It sets up FOR and IF structures.  Remember --- for those,
   we've got to set aside an extra argument space.         */
struct INTERMEDIATE *
process_special(COMPSTATE * cstat, const char *token)
{
	static char buf[BUFFER_LEN];
	const char *tok;
	struct INTERMEDIATE *new;

	if (!string_compare(token, ":")) {
		const char *proc_name;
		struct INTERMEDIATE *new2;

		if (cstat->curr_proc)
			abort_compile(cstat, "Definition within definition.");
		proc_name = next_token(cstat);
		if (!proc_name)
			abort_compile(cstat, "Unexpected end of file within procedure.");

		new = new_inst(cstat);
		new->no = cstat->nowords++;
		new->in.type = PROG_FUNCTION;
		new->in.line = cstat->lineno;
		new->in.data.string = alloc_prog_string(proc_name);

		new2 = (new->next = new_inst(cstat));
		new2->no = cstat->nowords++;
		new2->in.type = PROG_DECLVAR;
		new2->in.line = cstat->lineno;
		new2->in.data.number = 0;

		cstat->curr_proc = new;
		cstat->curr_decl = new2;
		add_proc(cstat, proc_name, new);

		if (proc_name)
			free((void *) proc_name);

		return new;
	} else if (!string_compare(token, ";")) {
		int i;

		if (cstat->if_stack || cstat->loop_stack)
			abort_compile(cstat, "Unexpected end of procedure definition.");
		if (!cstat->curr_proc)
			abort_compile(cstat, "Procedure end without body.");
		cstat->curr_proc = 0;
		new = new_inst(cstat);
		new->no = cstat->nowords++;
		new->in.type = PROG_PRIMITIVE;
		new->in.line = cstat->lineno;
		new->in.data.number = IN_RET;
		for (i = 0; i < MAX_VAR && cstat->scopedvars[i]; i++) {
			free((void *) cstat->scopedvars[i]);
			cstat->scopedvars[i] = 0;
		}
		return new;
	} else if (!string_compare(token, "IF")) {
		new = new_inst(cstat);
		new->no = cstat->nowords++;
		new->in.type = PROG_IF;
		new->in.line = cstat->lineno;
		new->in.data.call = 0;
		addif(cstat, new);
		return new;
	} else if (!string_compare(token, "ELSE")) {
		struct INTERMEDIATE *eef;

		eef = find_if(cstat);
		if (!eef)
			abort_compile(cstat, "ELSE without IF.");
		new = new_inst(cstat);
		new->no = cstat->nowords++;
		new->in.type = PROG_JMP;
		new->in.line = cstat->lineno;
		new->in.data.call = 0;
		addelse(cstat, new);

		eef->in.data.number = cstat->nowords;
		return new;
	} else if (!string_compare(token, "THEN")) {
		/* can't use 'if' because it's a reserved word */
		struct INTERMEDIATE *eef;

		eef = find_else(cstat);
		if (!eef)
			abort_compile(cstat, "THEN without IF.");

		eef->in.data.number = cstat->nowords;
		return NULL;
	} else if (!string_compare(token, "BEGIN")) {
		addbegin(cstat, cstat->nextinst);
		return NULL;
	} else if (!string_compare(token, "FOR")) {
		struct INTERMEDIATE *new2, *new3;

		new = new_inst(cstat);
		new->no = cstat->nowords++;
		new->in.type = PROG_PRIMITIVE;
		new->in.line = cstat->lineno;
		new->in.data.number = IN_FOR;
		new2 = (new->next = new_inst(cstat));
		new2->no = cstat->nowords++;
		new2->in.type = PROG_PRIMITIVE;
		new2->in.line = cstat->lineno;
		new2->in.data.number = IN_FORITER;
		new3 = (new2->next = new_inst(cstat));
		new3->no = cstat->nowords++;
		new3->in.line = cstat->lineno;
		new3->in.type = PROG_IF;
		new3->in.data.number = 0;

		addfor(cstat, new2);
		return new;
	} else if (!string_compare(token, "FOREACH")) {
		struct INTERMEDIATE *new2, *new3;

		new = new_inst(cstat);
		new->no = cstat->nowords++;
		new->in.type = PROG_PRIMITIVE;
		new->in.line = cstat->lineno;
		new->in.data.number = IN_FOREACH;
		new2 = (new->next = new_inst(cstat));
		new2->no = cstat->nowords++;
		new2->in.type = PROG_PRIMITIVE;
		new2->in.line = cstat->lineno;
		new2->in.data.number = IN_FORITER;
		new3 = (new2->next = new_inst(cstat));
		new3->no = cstat->nowords++;
		new3->in.line = cstat->lineno;
		new3->in.type = PROG_IF;
		new3->in.data.number = 0;

		addfor(cstat, new2);
		return new;
	} else if (!string_compare(token, "UNTIL")) {
		/* can't use 'if' because it's a reserved word */
		struct INTERMEDIATE *eef;
		struct INTERMEDIATE *beef;
		struct INTERMEDIATE *curr;

		resolve_loop_addrs(cstat, cstat->nowords + 1);
		curr = locate_for(cstat);
		eef = find_begin(cstat);
		if (!eef)
			abort_compile(cstat, "UNTIL without BEGIN.");
		beef = locate_if(cstat);
		if (beef && (beef->no > eef->no))
			abort_compile(cstat, "Unexpected end of loop.");
		new = new_inst(cstat);
		new->no = cstat->nowords++;
		new->in.type = PROG_IF;
		new->in.line = cstat->lineno;
		new->in.data.number = eef->no;
		if (curr) {
			curr = (new->next = new_inst(cstat));
			curr->no = cstat->nowords++;
			curr->in.type = PROG_PRIMITIVE;
			curr->in.line = cstat->lineno;
			curr->in.data.number = IN_FORPOP;
		}
		return new;
	} else if (!string_compare(token, "WHILE")) {
		/* can't use 'if' because it's a reserved word */
		struct INTERMEDIATE *eef;

		eef = locate_begin(cstat);
		if (!eef)
			abort_compile(cstat, "Can't have a WHILE outside of a loop.");
		new = new_inst(cstat);
		new->no = cstat->nowords++;
		new->in.type = PROG_IF;
		new->in.line = cstat->lineno;
		new->in.data.number = 0;

		addwhile(cstat, new);
		return new;
	} else if (!string_compare(token, "BREAK")) {
		/* can't use 'if' because it's a reserved word */
		struct INTERMEDIATE *eef;

		eef = locate_begin(cstat);
		if (!eef)
			abort_compile(cstat, "Can't have a BREAK outside of a loop.");

		new = new_inst(cstat);
		new->no = cstat->nowords++;
		new->in.type = PROG_JMP;
		new->in.line = cstat->lineno;
		new->in.data.number = 0;

		addwhile(cstat, new);
		return new;
	} else if (!string_compare(token, "CONTINUE")) {
		/* can't use 'if' because it's a reserved word */
		struct INTERMEDIATE *beef;

		beef = locate_begin(cstat);
		if (!beef)
			abort_compile(cstat, "Can't CONTINUE outside of a loop.");
		new = new_inst(cstat);
		new->no = cstat->nowords++;
		new->in.type = PROG_JMP;
		new->in.line = cstat->lineno;
		new->in.data.number = beef->no;

		return new;
	} else if (!string_compare(token, "REPEAT")) {
		/* can't use 'if' because it's a reserved word */
		struct INTERMEDIATE *eef;
		struct INTERMEDIATE *beef;
		struct INTERMEDIATE *curr;

		resolve_loop_addrs(cstat, cstat->nowords + 1);
		curr = locate_for(cstat);
		eef = find_begin(cstat);
		if (!eef)
			abort_compile(cstat, "REPEAT without BEGIN.");
		beef = locate_if(cstat);
		if (beef && (beef->no > eef->no))
			abort_compile(cstat, "Unexpected end of loop.");
		new = new_inst(cstat);
		new->no = cstat->nowords++;
		new->in.type = PROG_JMP;
		new->in.line = cstat->lineno;
		new->in.data.number = eef->no;

		if (curr) {
			curr = (new->next = new_inst(cstat));
			curr->no = cstat->nowords++;
			curr->in.type = PROG_PRIMITIVE;
			curr->in.line = cstat->lineno;
			curr->in.data.number = IN_FORPOP;
		}

		return new;
	} else if (!string_compare(token, "CALL")) {
		new = new_inst(cstat);
		new->no = cstat->nowords++;
		new->in.type = PROG_PRIMITIVE;
		new->in.line = cstat->lineno;
		new->in.data.number = IN_CALL;
		return new;
	} else if (!string_compare(token, "WIZCALL") || !string_compare(token, "PUBLIC")) {
		struct PROC_LIST *p;
		struct publics *pub;
		int wizflag = 0;

		if (!string_compare(token, "WIZCALL"))
			wizflag = 1;
		if (cstat->curr_proc)
			abort_compile(cstat, "PUBLIC  or WIZCALL declaration within procedure.");
		tok = next_token(cstat);
		if ((!tok) || !call(cstat, tok))
			abort_compile(cstat, "Subroutine unknown in PUBLIC or WIZCALL declaration.");
		for (p = cstat->procs; p; p = p->next)
			if (!string_compare(p->name, tok))
				break;
		if (!p)
			abort_compile(cstat, "Subroutine unknown in PUBLIC or WIZCALL declaration.");
		if (!cstat->currpubs) {
			cstat->currpubs = (struct publics *) malloc(sizeof(struct publics));

			cstat->currpubs->next = NULL;
			cstat->currpubs->subname = (char *) string_dup(tok);
			if (tok)
				free((void *) tok);
			cstat->currpubs->addr.no = p->code->no;
			cstat->currpubs->mlev = wizflag ? 4 : 1;
			return 0;
		}
		for (pub = cstat->currpubs; pub;) {
			if (!string_compare(tok, pub->subname)) {
				abort_compile(cstat, "Function already declared public.");
			} else {
				if (pub->next) {
					pub = pub->next;
				} else {
					pub->next = (struct publics *) malloc(sizeof(struct publics));

					pub = pub->next;
					pub->next = NULL;
					pub->subname = (char *) string_dup(tok);
					if (tok)
						free((void *) tok);
					pub->addr.no = p->code->no;
					pub->mlev = wizflag ? 4 : 1;
					pub = NULL;
				}
			}
		}
		return 0;
	} else if (!string_compare(token, "VAR")) {
		if (cstat->curr_proc) {
			tok = next_token(cstat);
			if (!tok)
				abort_compile(cstat, "Unexpected end of program.");
			if (add_scopedvar(cstat, tok) < 0)
				abort_compile(cstat, "Variable limit exceeded.");
			if (tok)
				free((void *) tok);
			cstat->curr_decl->in.data.number++;
		} else {
			tok = next_token(cstat);
			if (!tok)
				abort_compile(cstat, "Unexpected end of program.");
			if (!add_variable(cstat, tok))
				abort_compile(cstat, "Variable limit exceeded.");
			if (tok)
				free((void *) tok);
		}
		return 0;
	} else if (!string_compare(token, "LVAR")) {
		if (cstat->curr_proc)
			abort_compile(cstat, "Local variable declared within procedure.");
		tok = next_token(cstat);
		if (!tok || (add_localvar(cstat, tok) == -1))
			abort_compile(cstat, "Local variable limit exceeded.");
		if (tok)
			free((void *) tok);
		return 0;
	} else {
		sprintf(buf, "Unrecognized special form %s found. (%d)", token, cstat->lineno);
		abort_compile(cstat, buf);
	}
}

/* return primitive word. */
struct INTERMEDIATE *
primitive_word(COMPSTATE * cstat, const char *token)
{
	struct INTERMEDIATE *new, *cur;
	int pnum, loop;

	pnum = get_primitive(token);
	cur = new = new_inst(cstat);
	if (pnum == IN_RET || pnum == IN_JMP) {
		for (loop = 0; loop < cstat->nested_fors; loop++) {
			cur->no = cstat->nowords++;
			cur->in.type = PROG_PRIMITIVE;
			cur->in.line = cstat->lineno;
			cur->in.data.number = IN_FORPOP;
			cur->next = new_inst(cstat);
			cur = cur->next;
		}
	}

	cur->no = cstat->nowords++;
	cur->in.type = PROG_PRIMITIVE;
	cur->in.line = cstat->lineno;
	cur->in.data.number = pnum;

	return new;
}

/* return self pushing word (string) */
struct INTERMEDIATE *
string_word(COMPSTATE * cstat, const char *token)
{
	struct INTERMEDIATE *new;

	new = new_inst(cstat);
	new->no = cstat->nowords++;
	new->in.type = PROG_STRING;
	new->in.line = cstat->lineno;
	new->in.data.string = alloc_prog_string(token);
	return new;
}

/* return self pushing word (float) */
struct INTERMEDIATE *
float_word(COMPSTATE * cstat, const char *token)
{
	struct INTERMEDIATE *new;

	new = new_inst(cstat);
	new->no = cstat->nowords++;
	new->in.type = PROG_FLOAT;
	new->in.line = cstat->lineno;
	sscanf(token, "%g", &(new->in.data.fnumber));
	return new;
}

/* return self pushing word (number) */
struct INTERMEDIATE *
number_word(COMPSTATE * cstat, const char *token)
{
	struct INTERMEDIATE *new;

	new = new_inst(cstat);
	new->no = cstat->nowords++;
	new->in.type = PROG_INTEGER;
	new->in.line = cstat->lineno;
	new->in.data.number = atoi(token);
	return new;
}

/* do a subroutine call --- push address onto stack, then make a primitive
   CALL.
   */
struct INTERMEDIATE *
call_word(COMPSTATE * cstat, const char *token)
{
	struct INTERMEDIATE *new;
	struct PROC_LIST *p;

	new = new_inst(cstat);
	new->no = cstat->nowords++;
	new->in.type = PROG_EXEC;
	new->in.line = cstat->lineno;
	for (p = cstat->procs; p; p = p->next)
		if (!string_compare(p->name, token))
			break;

	new->in.data.number = p->code->no;
	return new;
}

struct INTERMEDIATE *
quoted_word(COMPSTATE * cstat, const char *token)
{
	struct INTERMEDIATE *new;
	struct PROC_LIST *p;

	new = new_inst(cstat);
	new->no = cstat->nowords++;
	new->in.type = PROG_ADD;
	new->in.line = cstat->lineno;
	for (p = cstat->procs; p; p = p->next)
		if (!string_compare(p->name, token))
			break;

	new->in.data.number = p->code->no;
	return new;
}

/* returns number corresponding to variable number.
   We assume that it DOES exist */
struct INTERMEDIATE *
var_word(COMPSTATE * cstat, const char *token)
{
	struct INTERMEDIATE *new;
	int i, var_no;

	new = new_inst(cstat);
	new->no = cstat->nowords++;
	new->in.type = PROG_VAR;
	new->in.line = cstat->lineno;
	for (var_no = i = 0; i < MAX_VAR; i++) {
		if (!cstat->variables[i])
			break;
		if (!string_compare(token, cstat->variables[i]))
			var_no = i;
	}
	new->in.data.number = var_no;

	return new;
}

struct INTERMEDIATE *
svar_word(COMPSTATE * cstat, const char *token)
{
	struct INTERMEDIATE *new;
	int i, var_no;

	new = new_inst(cstat);
	new->no = cstat->nowords++;
	new->in.type = PROG_SVAR;
	new->in.line = cstat->lineno;
	for (i = var_no = 0; i < MAX_VAR; i++) {
		if (!cstat->scopedvars[i])
			break;
		if (!string_compare(token, cstat->scopedvars[i]))
			var_no = i;
	}
	new->in.data.number = var_no;

	return new;
}

struct INTERMEDIATE *
lvar_word(COMPSTATE * cstat, const char *token)
{
	struct INTERMEDIATE *new;
	int i, var_no;

	new = new_inst(cstat);
	new->no = cstat->nowords++;
	new->in.type = PROG_LVAR;
	new->in.line = cstat->lineno;
	for (i = var_no = 0; i < MAX_VAR; i++) {
		if (!cstat->localvars[i])
			break;
		if (!string_compare(token, cstat->localvars[i]))
			var_no = i;
	}
	new->in.data.number = var_no;

	return new;
}

/* check if object is in database before putting it in */
struct INTERMEDIATE *
object_word(COMPSTATE * cstat, const char *token)
{
	struct INTERMEDIATE *new;
	int objno;

	objno = atol(token + 1);
	new = new_inst(cstat);
	new->no = cstat->nowords++;
	new->in.type = PROG_OBJECT;
	new->in.line = cstat->lineno;
	new->in.data.objref = objno;
	return new;
}



/* support routines for internal data structures. */

/* add procedure to procedures list */
void
add_proc(COMPSTATE * cstat, const char *proc_name, struct INTERMEDIATE *place)
{
	struct PROC_LIST *new;

	new = (struct PROC_LIST *) malloc(sizeof(struct PROC_LIST));

	new->name = alloc_string(proc_name);
	new->code = place;
	new->next = cstat->procs;
	cstat->procs = new;
}

/* add if to if stack */
void
addif(COMPSTATE * cstat, struct INTERMEDIATE *place)
{
	struct IF_STACK *new;

	new = (struct IF_STACK *) malloc(sizeof(struct IF_STACK));

	new->place = place;
	new->type = CTYPE_IF;
	new->next = cstat->if_stack;
	cstat->if_stack = new;
}

/* add else to if stack */
void
addelse(COMPSTATE * cstat, struct INTERMEDIATE *place)
{
	struct IF_STACK *new;

	new = (struct IF_STACK *) malloc(sizeof(struct IF_STACK));

	new->place = place;
	new->type = CTYPE_ELSE;
	new->next = cstat->if_stack;
	cstat->if_stack = new;
}

/* add begin to if stack */
void
addbegin(COMPSTATE * cstat, struct INTERMEDIATE *place)
{
	struct IF_STACK *new;

	new = (struct IF_STACK *) malloc(sizeof(struct IF_STACK));

	new->place = place;
	new->type = CTYPE_BEGIN;
	new->next = cstat->loop_stack;
	cstat->loop_stack = new;
}

/* add for to LOOP stack, get it right */
void
addfor(COMPSTATE * cstat, struct INTERMEDIATE *place)
{
	struct IF_STACK *new;

	new = (struct IF_STACK *) malloc(sizeof(struct IF_STACK));

	new->place = place;
	new->type = CTYPE_FOR;
	new->next = cstat->loop_stack;
	cstat->loop_stack = new;
	cstat->nested_fors++;
}

/* add while to if stack */
void
addwhile(COMPSTATE * cstat, struct INTERMEDIATE *place)
{
	struct IF_STACK *new;

	new = (struct IF_STACK *) malloc(sizeof(struct IF_STACK));

	new->place = place;
	new->type = CTYPE_WHILE;
	new->next = cstat->loop_stack;
	cstat->loop_stack = new;
}

/* finds topmost else or if off the stack */
struct INTERMEDIATE *
locate_if(COMPSTATE * cstat)
{
	if (!cstat->if_stack)
		return 0;
	if ((cstat->if_stack->type != CTYPE_IF) && (cstat->if_stack->type != CTYPE_ELSE))
		return 0;

	return (cstat->if_stack->place);
}

/* pops topmost if off the stack */
struct INTERMEDIATE *
find_if(COMPSTATE * cstat)
{
	struct INTERMEDIATE *temp;
	struct IF_STACK *tofree;

	if (!cstat->if_stack)
		return 0;
	if (cstat->if_stack->type != CTYPE_IF)
		return 0;

	temp = cstat->if_stack->place;
	tofree = cstat->if_stack;
	cstat->if_stack = cstat->if_stack->next;
	free((void *) tofree);
	return temp;
}

/* pops topmost else or if off the stack */
struct INTERMEDIATE *
find_else(COMPSTATE * cstat)
{
	struct INTERMEDIATE *temp;
	struct IF_STACK *tofree;

	if (!cstat->if_stack)
		return 0;
	if ((cstat->if_stack->type != CTYPE_IF) && (cstat->if_stack->type != CTYPE_ELSE))
		return 0;

	temp = cstat->if_stack->place;
	tofree = cstat->if_stack;
	cstat->if_stack = cstat->if_stack->next;
	free((void *) tofree);
	return temp;
}

/* returns topmost begin or for off the stack */
struct INTERMEDIATE *
locate_begin(COMPSTATE * cstat)
{
	struct IF_STACK *tempeef;

	tempeef = cstat->loop_stack;
	while (tempeef && (tempeef->type == CTYPE_WHILE))
		tempeef = tempeef->next;
	if (!tempeef)
		return 0;
	if (tempeef->type != CTYPE_BEGIN && tempeef->type != CTYPE_FOR)
		return 0;

	return tempeef->place;
}

/* checks if topmost loop stack item is a for */
struct INTERMEDIATE *
locate_for(COMPSTATE * cstat)
{
	struct IF_STACK *tempeef;

	tempeef = cstat->loop_stack;
	while (tempeef && (tempeef->type == CTYPE_WHILE))
		tempeef = tempeef->next;
	if (!tempeef)
		return 0;
	if (tempeef->type != CTYPE_FOR)
		return 0;

	return tempeef->place;
}

/* pops topmost begin or for off the stack */
struct INTERMEDIATE *
find_begin(COMPSTATE * cstat)
{
	struct INTERMEDIATE *temp;
	struct IF_STACK *tofree;

	if (!cstat->loop_stack)
		return 0;
	if (cstat->loop_stack->type != CTYPE_BEGIN && cstat->loop_stack->type != CTYPE_FOR)
		return 0;

	if (cstat->loop_stack->type == CTYPE_FOR)
		cstat->nested_fors--;

	temp = cstat->loop_stack->place;
	tofree = cstat->loop_stack;
	cstat->loop_stack = cstat->loop_stack->next;
	free((void *) tofree);
	return temp;
}

/* pops topmost while off the stack */
struct INTERMEDIATE *
find_while(COMPSTATE * cstat)
{
	struct INTERMEDIATE *temp;
	struct IF_STACK *tofree;

	if (!cstat->loop_stack)
		return 0;
	if (cstat->loop_stack->type != CTYPE_WHILE)
		return 0;

	temp = cstat->loop_stack->place;
	tofree = cstat->loop_stack;
	cstat->loop_stack = cstat->loop_stack->next;
	free((void *) tofree);
	return temp;
}

void
resolve_loop_addrs(COMPSTATE * cstat, int where)
{
	struct INTERMEDIATE *eef;

	while ((eef = find_while(cstat)))
		eef->in.data.number = where;
	if ((eef = locate_for(cstat))) {
		eef->next->in.data.number = where;
	}
}

/* adds variable.  Return 0 if no space left */
int
add_variable(COMPSTATE * cstat, const char *varname)
{
	int i;

	for (i = RES_VAR; i < MAX_VAR; i++)
		if (!cstat->variables[i])
			break;

	if (i == MAX_VAR)
		return 0;

	cstat->variables[i] = alloc_string(varname);
	return i;
}


/* adds local variable.  Return 0 if no space left */
int
add_scopedvar(COMPSTATE * cstat, const char *varname)
{
	int i;

	for (i = 0; i < MAX_VAR; i++)
		if (!cstat->scopedvars[i])
			break;

	if (i == MAX_VAR)
		return -1;

	cstat->scopedvars[i] = alloc_string(varname);
	return i;
}


/* adds local variable.  Return 0 if no space left */
int
add_localvar(COMPSTATE * cstat, const char *varname)
{
	int i;

	for (i = 0; i < MAX_VAR; i++)
		if (!cstat->localvars[i])
			break;

	if (i == MAX_VAR)
		return -1;

	cstat->localvars[i] = alloc_string(varname);
	return i;
}


/* predicates for procedure calls */
int
special(const char *token)
{
	return (token && !(string_compare(token, ":")
					   && string_compare(token, ";")
					   && string_compare(token, "IF")
					   && string_compare(token, "ELSE")
					   && string_compare(token, "THEN")
					   && string_compare(token, "BEGIN")
					   && string_compare(token, "FOR")
					   && string_compare(token, "FOREACH")
					   && string_compare(token, "UNTIL")
					   && string_compare(token, "WHILE")
					   && string_compare(token, "BREAK")
					   && string_compare(token, "CONTINUE")
					   && string_compare(token, "REPEAT")
					   && string_compare(token, "CALL")
					   && string_compare(token, "PUBLIC")
					   && string_compare(token, "WIZCALL")
					   && string_compare(token, "LVAR")
					   && string_compare(token, "VAR")));
}

/* see if procedure call */
int
call(COMPSTATE * cstat, const char *token)
{
	struct PROC_LIST *i;

	for (i = cstat->procs; i; i = i->next)
		if (!string_compare(i->name, token))
			return 1;

	return 0;
}

/* see if it's a quoted procedure name */
int
quoted(COMPSTATE * cstat, const char *token)
{
	return (*token == '\'' && call(cstat, token + 1));
}

/* see if it's an object # */
int
object(const char *token)
{
	if (*token == '#' && number(token + 1))
		return 1;
	else
		return 0;
}

/* see if string */
int
string(const char *token)
{
	return (token[0] == '"');
}

int
variable(COMPSTATE * cstat, const char *token)
{
	int i;

	for (i = 0; i < MAX_VAR && cstat->variables[i]; i++)
		if (!string_compare(token, cstat->variables[i]))
			return 1;

	return 0;
}

int
scopedvar(COMPSTATE * cstat, const char *token)
{
	int i;

	for (i = 0; i < MAX_VAR && cstat->scopedvars[i]; i++)
		if (!string_compare(token, cstat->scopedvars[i]))
			return 1;

	return 0;
}

int
localvar(COMPSTATE * cstat, const char *token)
{
	int i;

	for (i = 0; i < MAX_VAR && cstat->localvars[i]; i++)
		if (!string_compare(token, cstat->localvars[i]))
			return 1;

	return 0;
}

/* see if token is primitive */
int
primitive(const char *token)
{
	int primnum;

	primnum = get_primitive(token);
	return (primnum && primnum <= BASE_MAX - PRIMS_INTERNAL_CNT);
}

/* return primitive instruction */
int
get_primitive(const char *token)
{
	hash_data *hd;

	if ((hd = find_hash(token, primitive_list, COMP_HASH_SIZE)) == NULL)
		return 0;
	else {
		return (hd->ival);
	}
}



/* clean up as nicely as we can. */

void
cleanpubs(struct publics *mypub)
{
	struct publics *tmppub;

	while (mypub) {
		tmppub = mypub->next;
		free(mypub->subname);
		free(mypub);
		mypub = tmppub;
	}
}

void
clean_mcpbinds(struct mcp_binding *mypub)
{
	struct mcp_binding *tmppub;

	while (mypub) {
		tmppub = mypub->next;
		free(mypub->pkgname);
		free(mypub->msgname);
		free(mypub);
		mypub = tmppub;
	}
}

void
cleanup(COMPSTATE * cstat)
{
	struct INTERMEDIATE *wd, *tempword;
	struct IF_STACK *eef, *tempif;
	struct PROC_LIST *p, *tempp;
	int i;

	for (wd = cstat->first_word; wd; wd = tempword) {
		tempword = wd->next;
		if (wd->in.type == PROG_STRING || wd->in.type == PROG_FUNCTION)
			if (wd->in.data.string)
				free((void *) wd->in.data.string);
		free((void *) wd);
	}
	cstat->first_word = 0;

	for (eef = cstat->if_stack; eef; eef = tempif) {
		tempif = eef->next;
		free((void *) eef);
	}
	cstat->if_stack = 0;

	for (eef = cstat->loop_stack; eef; eef = tempif) {
		tempif = eef->next;
		free((void *) eef);
	}
	cstat->loop_stack = 0;

	for (p = cstat->procs; p; p = tempp) {
		tempp = p->next;
		free((void *) p->name);
		free((void *) p);
	}
	cstat->procs = 0;

	purge_defs(cstat);

	for (i = RES_VAR; i < MAX_VAR && cstat->variables[i]; i++) {
		free((void *) cstat->variables[i]);
		cstat->variables[i] = 0;
	}

	for (i = 0; i < MAX_VAR && cstat->scopedvars[i]; i++) {
		free((void *) cstat->scopedvars[i]);
		cstat->scopedvars[i] = 0;
	}

	for (i = 0; i < MAX_VAR && cstat->localvars[i]; i++) {
		free((void *) cstat->localvars[i]);
		cstat->localvars[i] = 0;
	}
}



/* copy program to an array */
void
copy_program(COMPSTATE * cstat)
{
	/*
	 * Everything should be peachy keen now, so we don't do any error checking
	 */
	struct INTERMEDIATE *curr;
	struct inst *code;
	int i;

	if (!cstat->first_word)
		v_abort_compile(cstat, "Nothing to compile.");

	code = (struct inst *) malloc(sizeof(struct inst) * (cstat->nowords + 1));

	i = 0;
	for (curr = cstat->first_word; curr; curr = curr->next) {
		code[i].type = curr->in.type;
		code[i].line = curr->in.line;
		switch (code[i].type) {
		case PROG_PRIMITIVE:
		case PROG_INTEGER:
		case PROG_DECLVAR:
		case PROG_SVAR:
		case PROG_LVAR:
		case PROG_VAR:
			code[i].data.number = curr->in.data.number;
			break;
		case PROG_FLOAT:
			code[i].data.fnumber = curr->in.data.fnumber;
			break;
		case PROG_STRING:
		case PROG_FUNCTION:
			code[i].data.string = curr->in.data.string ?
					alloc_prog_string(curr->in.data.string->data) : 0;
			break;
		case PROG_OBJECT:
			code[i].data.objref = curr->in.data.objref;
			break;
		case PROG_ADD:
			code[i].data.addr = alloc_addr(cstat, curr->in.data.number, code);
			break;
		case PROG_IF:
		case PROG_JMP:
		case PROG_EXEC:
			code[i].data.call = code + curr->in.data.number;
			break;
		default:
			v_abort_compile(cstat, "Unknown type compile!  Internal error.");
			break;
		}
		i++;
	}
	PROGRAM_SET_CODE(cstat->program, code);
}

void
set_start(COMPSTATE * cstat)
{
	PROGRAM_SET_SIZ(cstat->program, cstat->nowords);
	PROGRAM_SET_START(cstat->program, (PROGRAM_CODE(cstat->program) + cstat->procs->code->no));
}


/* allocate and initialize data linked structure. */
struct INTERMEDIATE *
alloc_inst()
{
	struct INTERMEDIATE *new;

	new = (struct INTERMEDIATE *) malloc(sizeof(struct INTERMEDIATE));

	new->next = 0;
	new->no = 0;
	new->in.type = 0;
	new->in.line = 0;
	new->in.data.number = 0;
	return new;
}

struct INTERMEDIATE *
new_inst(COMPSTATE * cstat)
{
	struct INTERMEDIATE *new;

	new = cstat->nextinst;
	if (!new) {
		new = alloc_inst();
	}
	cstat->nextinst = alloc_inst();

	return new;
}

/* allocate an address */
struct prog_addr *
alloc_addr(COMPSTATE * cstat, int offset, struct inst *codestart)
{
	struct prog_addr *new;

	new = (struct prog_addr *) malloc(sizeof(struct prog_addr));

	new->links = 1;
	new->progref = cstat->program;
	new->data = codestart + offset;
	return new;
}

void
free_prog(dbref prog)
{
	int i;
	struct inst *c = PROGRAM_CODE(prog);
	int siz = PROGRAM_SIZ(prog);

	if (c) {
		if (PROGRAM_INSTANCES(prog)) {
			fprintf(stderr, "Freeing program %s with %d instances reported\n",
					unparse_object(GOD, prog), PROGRAM_INSTANCES(prog));
		}
		i = scan_instances(prog);
		if (i) {
			fprintf(stderr, "Freeing program %s with %d instances found\n",
					unparse_object(GOD, prog), i);
		}
		for (i = 0; i < siz; i++) {
			if (c[i].type == PROG_STRING || c[i].type == PROG_FUNCTION)
				CLEAR(c + i);
			if (c[i].type == PROG_ADD) {
				if (c[i].data.addr->links != 1) {
					fprintf(stderr, "Freeing address in %s with link count %d\n",
							unparse_object(GOD, prog), c[i].data.addr->links);
				}
				free(c[i].data.addr);
			}
		}
		free((void *) c);
	}
	PROGRAM_SET_CODE(prog, 0);
	PROGRAM_SET_SIZ(prog, 0);
	PROGRAM_SET_START(prog, 0);
}

long
size_prog(dbref prog)
{
	struct inst *c;
	long i, siz, byts = 0;

	c = PROGRAM_CODE(prog);
	if (!c)
		return 0;
	siz = PROGRAM_SIZ(prog);
	for (i = 0L; i < siz; i++) {
		byts += sizeof(*c);
		if ((c[i].type == PROG_STRING || c[i].type == PROG_FUNCTION) && c[i].data.string)
			byts += strlen(c[i].data.string->data) + 1;
		if ((c[i].type == PROG_ADD))
			byts += sizeof(struct prog_addr);
	}
	byts += size_pubs(PROGRAM_PUBS(prog));
	return byts;
}

static void
add_primitive(int val)
{
	hash_data hd;

	hd.ival = val;
	if (add_hash(base_inst[val - BASE_MIN], hd, primitive_list, COMP_HASH_SIZE) == NULL)
		panic("Out of memory");
	else
		return;
}

void
clear_primitives(void)
{
	kill_hash(primitive_list, COMP_HASH_SIZE, 0);
	return;
}

void
init_primitives(void)
{
	int i;

	clear_primitives();
	for (i = BASE_MIN; i <= BASE_MAX; i++) {
		add_primitive(i);
	}
	IN_FORPOP = get_primitive(" FORPOP");
	IN_FORITER = get_primitive(" FORITER");
	IN_FOR = get_primitive(" FOR");
	IN_FOREACH = get_primitive(" FOREACH");
}
