/* Primitives package */

#include "copyright.h"
#include "config.h"
#include "params.h"

#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include "db.h"
#include "tune.h"
#include "props.h"
#include "inst.h"
#include "externs.h"
#include "match.h"
#include "interface.h"
#include "strings.h"
#include "dbsearch.h"
#include "interp.h"

static struct inst *oper1, *oper2, *oper3, *oper4;
static int tmp, result;
static dbref ref;
static char buf[BUFFER_LEN];


void
copyobj(dbref player, dbref old, dbref nu)
{
	struct object *newp = DBFETCH(nu);

	NAME(nu) = alloc_string(NAME(old));
	if (Typeof(old) == TYPE_THING) {
		ALLOC_THING_SP(nu);
		THING_SET_HOME(nu, player);
		THING_SET_VALUE(nu, 1);
	}
	newp->properties = copy_prop(old);
	newp->exits = NOTHING;
	newp->contents = NOTHING;
	newp->next = NOTHING;
	newp->location = NOTHING;
	moveto(nu, player);

#ifdef DISKBASE
	newp->propsfpos = 0;
	newp->propsmode = PROPS_UNLOADED;
	newp->propstime = 0;
	newp->nextold = NOTHING;
	newp->prevold = NOTHING;
	dirtyprops(nu);
#endif

	DBDIRTY(nu);
}



void
prim_addpennies(PRIM_PROTOTYPE)
{
	CHECKOP(2);
	oper1 = POP();
	oper2 = POP();
	if (mlev < 2)
		abort_interp("Requires Mucker Level 2 or better.");
	if (!valid_object(oper2))
		abort_interp("Invalid object.");
	if (oper1->type != PROG_INTEGER)
		abort_interp("Non-integer argument (2)");
	ref = oper2->data.objref;
	if (Typeof(ref) == TYPE_PLAYER) {
		result = PLAYER_PENNIES(ref);
		if (mlev < 4) {
			if (oper1->data.number > 0) {
				if (result > (result + oper1->data.number))
					abort_interp("Would roll over player's score.");
				if ((result + oper1->data.number) > tp_max_pennies)
					abort_interp("Would exceed MAX_PENNIES.");
			} else {
				if (result < (result + oper1->data.number))
					abort_interp("Would roll over player's score.");
				if ((result + oper1->data.number) < 0)
					abort_interp("Result would be negative.");
			}
		}
		result += oper1->data.number;
		PLAYER_ADD_PENNIES(ref, oper1->data.number);
		DBDIRTY(ref);
	} else if (Typeof(ref) == TYPE_THING) {
		if (mlev < 4)
			abort_interp("Permission denied.");
		result = THING_VALUE(ref) + oper1->data.number;
		if (result < 1)
			abort_interp("Result must be positive.");
		THING_SET_VALUE(ref, (THING_VALUE(ref) + oper1->data.number));
		DBDIRTY(ref);
	} else {
		abort_interp("Invalid object type.");
	}
	CLEAR(oper1);
	CLEAR(oper2);
}

void
prim_moveto(PRIM_PROTOTYPE)
{
	struct inst *oper1=NULL, *oper2=NULL, *oper3=NULL, *oper4=NULL;

	CHECKOP(2);
	oper1 = POP();
	oper2 = POP();
	if (fr->level > 8)
		abort_interp("Interp call loops not allowed.");
	if (!(valid_object(oper1) && valid_object(oper2)) && !is_home(oper1))
		abort_interp("Non-object argument.");
	{
		dbref victim, dest;

		victim = oper2->data.objref;
		dest = oper1->data.objref;

		if (Typeof(dest) == TYPE_EXIT)
			abort_interp("Destination argument is an exit.");
		if (Typeof(victim) == TYPE_EXIT && (mlev < 3))
			abort_interp("Permission denied.");
		if (!(FLAGS(victim) & JUMP_OK)
			&& !permissions(ProgUID, victim) && (mlev < 3))
			abort_interp("Object can't be moved.");
		switch (Typeof(victim)) {
		case TYPE_PLAYER:
			if (Typeof(dest) != TYPE_ROOM &&
				!(Typeof(dest) == TYPE_THING && (FLAGS(dest) & VEHICLE)))
						abort_interp("Bad destination.");
			/* Check permissions */
			if (parent_loop_check(victim, dest))
				abort_interp("Things can't contain themselves.");
			if ((mlev < 3)) {
				if (!(FLAGS(DBFETCH(victim)->location) & JUMP_OK)
					&& !permissions(ProgUID, DBFETCH(victim)->location))
					abort_interp("Source not JUMP_OK.");
				if (!is_home(oper1) && !(FLAGS(dest) & JUMP_OK)
					&& !permissions(ProgUID, dest))
					abort_interp("Destination not JUMP_OK.");
				if (Typeof(dest) == TYPE_THING && getloc(victim) != getloc(dest))
					abort_interp("Not in same location as vehicle.");
			}
			enter_room(fr->descr, victim, dest, program);
			break;
		case TYPE_THING:
			if (parent_loop_check(victim, dest))
				abort_interp("A thing cannot contain itself.");
			if (mlev < 3 && (FLAGS(victim) & VEHICLE) &&
				(FLAGS(dest) & VEHICLE) && Typeof(dest) != TYPE_THING)
				abort_interp("Destination doesn't accept vehicles.");
			if (mlev < 3 && (FLAGS(victim) & ZOMBIE) &&
				(FLAGS(dest) & ZOMBIE) && Typeof(dest) != TYPE_THING)
				abort_interp("Destination doesn't accept zombies.");
			ts_lastuseobject(victim);
		case TYPE_PROGRAM:
			{
				dbref matchroom = NOTHING;

				if (Typeof(dest) != TYPE_ROOM && Typeof(dest) != TYPE_PLAYER
					&& Typeof(dest) != TYPE_THING) abort_interp("Bad destination.");
				if ((mlev < 3)) {
					if (permissions(ProgUID, dest))
						matchroom = dest;
					if (permissions(ProgUID, DBFETCH(victim)->location))
						matchroom = DBFETCH(victim)->location;
					if (matchroom != NOTHING && !(FLAGS(matchroom) & JUMP_OK)
						&& !permissions(ProgUID, victim))
						abort_interp("Permission denied.");
				}
			}
			if (Typeof(victim) == TYPE_THING &&
				(tp_thing_movement || (FLAGS(victim) & ZOMBIE))) {
				enter_room(fr->descr, victim, dest, program);
			} else {
				moveto(victim, dest);
			}
			break;
		case TYPE_EXIT:
			if ((mlev < 3) && (!permissions(ProgUID, victim)
							   || !permissions(ProgUID, dest)))
				abort_interp("Permission denied.");
			if (Typeof(dest) != TYPE_ROOM && Typeof(dest) != TYPE_THING &&
				Typeof(dest) != TYPE_PLAYER) abort_interp("Bad destination object.");
			if (!unset_source(ProgUID, getloc(player), victim))
				break;
			set_source(ProgUID, victim, dest);
			SetMLevel(victim, 0);
			break;
		case TYPE_ROOM:
			if (!tp_thing_movement && Typeof(dest) != TYPE_ROOM)
				abort_interp("Bad destination.");
			if (victim == GLOBAL_ENVIRONMENT)
				abort_interp("Permission denied.");
			if (dest == HOME) {
				dest = GLOBAL_ENVIRONMENT;
			} else {
				if ((mlev < 3) && (!permissions(ProgUID, victim)
								   || !can_link_to(ProgUID, NOTYPE, dest)))
					abort_interp("Permission denied.");
				if (parent_loop_check(victim, dest)) {
					abort_interp("Parent room would create a loop.");
				}
			}
			ts_lastuseobject(victim);
			moveto(victim, dest);
			break;
		default:
			abort_interp("Invalid object type (1)");
		}
	}
	CLEAR(oper1);
	CLEAR(oper2);
}

void
prim_pennies(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
	if (!valid_object(oper1))
		abort_interp("Invalid argument.");
	CHECKREMOTE(oper1->data.objref);
	switch (Typeof(oper1->data.objref)) {
	case TYPE_PLAYER:
		result = PLAYER_PENNIES(oper1->data.objref);
		break;
	case TYPE_THING:
		result = THING_VALUE(oper1->data.objref);
		break;
	default:
		abort_interp("Invalid argument.");
	}
	CLEAR(oper1);
	PushInt(result);
}


void
prim_dbcomp(PRIM_PROTOTYPE)
{
	CHECKOP(2);
	oper1 = POP();
	oper2 = POP();
	if (oper1->type != PROG_OBJECT || oper2->type != PROG_OBJECT)
		abort_interp("Invalid argument type.");
	result = oper1->data.objref == oper2->data.objref;
	CLEAR(oper1);
	CLEAR(oper2);
	PushInt(result);
}

void
prim_dbref(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
	if (oper1->type != PROG_INTEGER)
		abort_interp("Non-integer argument.");
	ref = (dbref) oper1->data.number;
	CLEAR(oper1);
	PushObject(ref);
}

void
prim_contents(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
	if (!valid_object(oper1))
		abort_interp("Invalid argument type.");
	CHECKREMOTE(oper1->data.objref);
	ref = DBFETCH(oper1->data.objref)->contents;
	while (mlev < 2 && ref != NOTHING && (FLAGS(ref) & DARK) && !controls(ProgUID, ref))
		ref = DBFETCH(ref)->next;
	if (Typeof(oper1->data.objref) != TYPE_PLAYER &&
		Typeof(oper1->data.objref) != TYPE_PROGRAM) ts_lastuseobject(oper1->data.objref);
	CLEAR(oper1);
	PushObject(ref);
}

void
prim_exits(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
	if (!valid_object(oper1))
		abort_interp("Invalid object.");
	ref = oper1->data.objref;
	CHECKREMOTE(ref);
	if ((mlev < 3) && !permissions(ProgUID, ref))
		abort_interp("Permission denied.");
	switch (Typeof(ref)) {
	case TYPE_ROOM:
	case TYPE_THING:
		ts_lastuseobject(ref);
	case TYPE_PLAYER:
		ref = DBFETCH(ref)->exits;
		break;
	default:
		abort_interp("Invalid object.");
	}
	CLEAR(oper1);
	PushObject(ref);
}


void
prim_next(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
	if (!valid_object(oper1))
		abort_interp("Invalid object.");
	CHECKREMOTE(oper1->data.objref);
	ref = DBFETCH(oper1->data.objref)->next;
	while (mlev < 2 && ref != NOTHING && Typeof(ref) != TYPE_EXIT &&
		   ((FLAGS(ref) & DARK) || Typeof(ref) == TYPE_ROOM) && !controls(ProgUID, ref))
		ref = DBFETCH(ref)->next;
	CLEAR(oper1);
	PushObject(ref);
}


void
prim_nextowned(PRIM_PROTOTYPE)
{
	dbref ownr;

	CHECKOP(1);
	oper1 = POP();
	if (!valid_object(oper1))
		abort_interp("Invalid object.");
	if (mlev < 2)
		abort_interp("Permission denied.");
	ref = oper1->data.objref;
	CHECKREMOTE(ref);

	ownr = OWNER(ref);
	if (Typeof(ref) == TYPE_PLAYER) {
		ref = 0;
	} else {
		ref++;
	}
	while (ref < db_top && (OWNER(ref) != ownr || ref == ownr))
		ref++;

	if (ref >= db_top) {
		ref = NOTHING;
	}
	CLEAR(oper1);
	PushObject(ref);
}


void
prim_truename(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
	if (oper1->type != PROG_OBJECT)
		abort_interp("Invalid argument type.");

	ref = oper1->data.objref;
	if (ref < 0 || ref >= db_top)
		abort_interp("Invalid object.");

	if (Typeof(ref) == TYPE_GARBAGE) {
		strcpy(buf, "<garbage>");
	} else {
		CHECKREMOTE(ref);
		if ((Typeof(ref) != TYPE_PLAYER) && (Typeof(ref) != TYPE_PROGRAM))
			ts_lastuseobject(ref);
		if (NAME(ref)) {
			strcpy(buf, NAME(ref));
		} else {
			buf[0] = '\0';
		}
	}
	CLEAR(oper1);
	PushString(buf);
}


void
prim_name(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
	if (oper1->type != PROG_OBJECT)
		abort_interp("Invalid argument type.");

	ref = oper1->data.objref;
	if (ref < 0 || ref >= db_top)
		abort_interp("Invalid object.");

	if (Typeof(ref) == TYPE_GARBAGE) {
		strcpy(buf, "<garbage>");
	} else {
		CHECKREMOTE(ref);
		if ((Typeof(ref) != TYPE_PLAYER) && (Typeof(ref) != TYPE_PROGRAM))
			ts_lastuseobject(ref);
		if (NAME(ref)) {
			strcpy(buf, PNAME(ref));
		} else {
			buf[0] = '\0';
		}
	}
	CLEAR(oper1);
	PushString(buf);
}

void
prim_setname(PRIM_PROTOTYPE)
{
	char *password;

	CHECKOP(2);
	oper1 = POP();
	oper2 = POP();
	if (!valid_object(oper2))
		abort_interp("Invalid argument type (1)");
	if (oper1->type != PROG_STRING)
		abort_interp("Non-string argument (2)");
	ref = oper2->data.objref;
	if ((mlev < 4) && !permissions(ProgUID, ref))
		abort_interp("Permission denied.");
	{
		const char *b = DoNullInd(oper1->data.string);

		if (Typeof(ref) == TYPE_PLAYER) {
			strcpy(buf, b);
			b = buf;
			if (mlev < 4) {
				abort_interp("Permission denied.");
			}

			/* split off password */
			for (password = buf; *password && !isspace(*password); password++) ;

			/* eat whitespace */
			if (*password) {
				*password++ = '\0';	/* terminate name */
				while (*password && isspace(*password)) {
					password++;
				}
			}

			/* check for null password */
			if (!*password) {
				abort_interp("Player namechange requires password.");
			} else if (strcmp(password, DoNull(PLAYER_PASSWORD(ref)))) {
				abort_interp("Incorrect password.");
			} else if (string_compare(b, NAME(ref)) && !ok_player_name(b)) {
				abort_interp("You can't give a player that name.");
			}

			/* everything ok, notify */
			log_status("NAME CHANGE (MUF): %s(#%d) to %s\n", NAME(ref), ref, b);
			delete_player(ref);
			if (NAME(ref)) {
				free((void *) NAME(ref));
			}
			NAME(ref) = alloc_string(b);
			add_player(ref);
			ts_modifyobject(ref);
		} else {
			if (!ok_name(b)) {
				abort_interp("Invalid name.");
			}
			if (NAME(ref)) {
				free((void *) NAME(ref));
			}
			NAME(ref) = alloc_string(b);
			ts_modifyobject(ref);
			if (MLevRaw(ref)) {
				SetMLevel(ref, 0);
			}
		}
	}
	CLEAR(oper1);
	CLEAR(oper2);
}

void
prim_pmatch(PRIM_PROTOTYPE)
{
	dbref ref;

	CHECKOP(1);
	oper1 = POP();
	if (oper1->type != PROG_STRING)
		abort_interp("Non-string argument.");
	if (!oper1->data.string)
		abort_interp("Empty string argument.");
	ref = lookup_player(oper1->data.string->data);
	CLEAR(oper1);
	PushObject(ref);
}


void
prim_match(PRIM_PROTOTYPE)
{
	struct inst *oper1=NULL, *oper2=NULL, *oper3=NULL, *oper4=NULL;
	dbref ref;

	CHECKOP(1);
	oper1 = POP();
	if (oper1->type != PROG_STRING)
		abort_interp("Non-string argument.");
	if (!oper1->data.string)
		abort_interp("Empty string argument.");
	{
		char tmppp[BUFFER_LEN];
		struct match_data md;

		(void) strcpy(buf, match_args);
		(void) strcpy(tmppp, match_cmdname);
		init_match(fr->descr, player, oper1->data.string->data, NOTYPE, &md);
		if (oper1->data.string->data[0] == REGISTERED_TOKEN) {
			match_registered(&md);
		} else {
			match_all_exits(&md);
			match_neighbor(&md);
			match_possession(&md);
			match_me(&md);
			match_here(&md);
			match_home(&md);
		}
		if (Wizard(ProgUID) || (mlev >= 4)) {
			match_absolute(&md);
			match_player(&md);
		}
		ref = match_result(&md);
		(void) strcpy(match_args, buf);
		(void) strcpy(match_cmdname, tmppp);
	}
	CLEAR(oper1);
	PushObject(ref);
}


void
prim_rmatch(PRIM_PROTOTYPE)
{
	struct inst *oper1=NULL, *oper2=NULL, *oper3=NULL, *oper4=NULL;
	dbref ref;

	CHECKOP(2);
	oper1 = POP();
	oper2 = POP();
	if (oper1->type != PROG_STRING)
		abort_interp("Invalid argument (2)");
	if (oper2->type != PROG_OBJECT
		|| oper2->data.objref < 0
		|| oper2->data.objref >= db_top
		|| Typeof(oper2->data.objref) == TYPE_PROGRAM
		|| Typeof(oper2->data.objref) == TYPE_EXIT) abort_interp("Invalid argument (1)");
	CHECKREMOTE(oper2->data.objref);
	{
		char tmppp[BUFFER_LEN];
		struct match_data md;

		(void) strcpy(buf, match_args);
		(void) strcpy(tmppp, match_cmdname);
		init_match(fr->descr, player, DoNullInd(oper1->data.string), TYPE_THING, &md);
		match_rmatch(oper2->data.objref, &md);
		ref = match_result(&md);
		(void) strcpy(match_args, buf);
		(void) strcpy(match_cmdname, tmppp);
	}
	CLEAR(oper1);
	CLEAR(oper2);
	PushObject(ref);
}


void
prim_copyobj(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
	if (!valid_object(oper1))
		abort_interp("Invalid object.");
	CHECKREMOTE(oper1->data.objref);
	if ((mlev < 3) && (fr->already_created))
		abort_interp("Can't create any more objects.");
	ref = oper1->data.objref;
	if (Typeof(ref) != TYPE_THING)
		abort_interp("Invalid object type.");
	if ((mlev < 3) && !permissions(ProgUID, ref))
		abort_interp("Permission denied.");
	fr->already_created++;
	{
		dbref newobj;

		newobj = new_object();
		*DBFETCH(newobj) = *DBFETCH(ref);
		copyobj(player, ref, newobj);
		CLEAR(oper1);
		PushObject(newobj);
	}
}


void
prim_set(PRIM_PROTOTYPE)
/* SET */
{
	CHECKOP(2);
	oper1 = POP();
	oper2 = POP();
	if (oper1->type != PROG_STRING)
		abort_interp("Invalid argument type (2)");
	if (!(oper1->data.string))
		abort_interp("Empty string argument (2)");
	if (!valid_object(oper2))
		abort_interp("Invalid object.");
	ref = oper2->data.objref;
	CHECKREMOTE(ref);
	tmp = 0;
	result = (*oper1->data.string->data == '!');
	{
		char *flag = oper1->data.string->data;

		if (result)
			flag++;

		if (string_prefix("dark", flag)
			|| string_prefix("debug", flag))
			tmp = DARK;
		else if (string_prefix("abode", flag)
				 || string_prefix("autostart", flag)
				 || string_prefix("abate", flag))
			tmp = ABODE;
		else if (string_prefix("chown_ok", flag)
				 || string_prefix("color", flag))
			tmp = CHOWN_OK;
		else if (string_prefix("haven", flag)
				 || string_prefix("harduid", flag))
			tmp = HAVEN;
		else if (string_prefix("jump_ok", flag))
			tmp = JUMP_OK;
		else if (string_prefix("link_ok", flag))
			tmp = LINK_OK;

		else if (string_prefix("kill_ok", flag))
			tmp = KILL_OK;

		else if (string_prefix("builder", flag))
			tmp = BUILDER;
		else if (string_prefix("mucker", flag))
			tmp = MUCKER;
		else if (string_prefix("nucker", flag))
			tmp = SMUCKER;
		else if (string_prefix("interactive", flag))
			tmp = INTERACTIVE;
		else if (string_prefix("sticky", flag)
				 || string_prefix("silent", flag))
			tmp = STICKY;
		else if (string_prefix("wizard", flag))
			tmp = WIZARD;
		else if (string_prefix("truewizard", flag))
			tmp = WIZARD;
		else if (string_prefix("xforcible", flag))
			tmp = XFORCIBLE;
		else if (string_prefix("zombie", flag))
			tmp = ZOMBIE;
		else if (string_prefix("vehicle", flag))
			tmp = VEHICLE;
		else if (string_prefix("quell", flag))
			tmp = QUELL;
	}
	if (!tmp)
		abort_interp("Unrecognized flag.");
	if ((mlev < 4) && !permissions(ProgUID, ref))
		abort_interp("Permission denied.");

	if (((mlev < 4) && ((tmp == DARK && ((Typeof(ref) == TYPE_PLAYER)
										 || (!tp_exit_darking && Typeof(ref) == TYPE_EXIT)
										 || (!tp_thing_darking && Typeof(ref) == TYPE_THING)
						 )
						)
						|| ((tmp == ZOMBIE) && (Typeof(ref) == TYPE_THING)
							&& (FLAGS(ProgUID) & ZOMBIE))
						|| ((tmp == ZOMBIE) && (Typeof(ref) == TYPE_PLAYER))
						|| (tmp == BUILDER)
		 )
		)
		|| (tmp == WIZARD) || (tmp == QUELL) || (tmp == INTERACTIVE)
		|| ((tmp == ABODE) && (Typeof(ref) == TYPE_PROGRAM))
		|| (tmp == MUCKER) || (tmp == SMUCKER) || (tmp == XFORCIBLE)
			)
		abort_interp("Permission denied.");
	if (result && Typeof(ref) == TYPE_THING && tmp == VEHICLE) {
		dbref obj = DBFETCH(ref)->contents;

		for (; obj != NOTHING; obj = DBFETCH(obj)->next) {
			if (Typeof(obj) == TYPE_PLAYER) {
				abort_interp("That vehicle still has players in it!");
			}
		}
	}
	if (!result) {
		FLAGS(ref) |= tmp;
		DBDIRTY(ref);
	} else {
		FLAGS(ref) &= ~tmp;
		DBDIRTY(ref);
	}
	CLEAR(oper1);
	CLEAR(oper2);
}

void
prim_mlevel(PRIM_PROTOTYPE)
/* MLEVEL */
{
	CHECKOP(1);
	oper1 = POP();
	if (!valid_object(oper1))
		abort_interp("Invalid object.");
	ref = oper1->data.objref;
	CHECKREMOTE(ref);
	result = MLevRaw(ref);
	CLEAR(oper1);
	PushInt(result);
}

void
prim_flagp(PRIM_PROTOTYPE)
/* FLAG? */
{
	int truwiz = 0;

	CHECKOP(2);
	oper1 = POP();
	oper2 = POP();
	if (oper1->type != PROG_STRING)
		abort_interp("Invalid argument type (2)");
	if (!(oper1->data.string))
		abort_interp("Empty string argument (2)");
	if (!valid_object(oper2))
		abort_interp("Invalid object.");
	ref = oper2->data.objref;
	CHECKREMOTE(ref);
	tmp = 0;
	result = 0;
	{
		char *flag = oper1->data.string->data;

		while (*flag == '!') {
			flag++;
			result = (!result);
		}
		if (!*flag)
			abort_interp("Unknown flag.");

		if (string_prefix("dark", flag)
			|| string_prefix("debug", flag))
			tmp = DARK;
		else if (string_prefix("abode", flag)
				 || string_prefix("autostart", flag)
				 || string_prefix("abate", flag))
			tmp = ABODE;
		else if (string_prefix("chown_ok", flag)
				 || string_prefix("color", flag))
			tmp = CHOWN_OK;
		else if (string_prefix("haven", flag)
				 || string_prefix("harduid", flag))
			tmp = HAVEN;
		else if (string_prefix("jump_ok", flag))
			tmp = JUMP_OK;
		else if (string_prefix("link_ok", flag))
			tmp = LINK_OK;

		else if (string_prefix("kill_ok", flag))
			tmp = KILL_OK;

		else if (string_prefix("builder", flag))
			tmp = BUILDER;
		else if (string_prefix("mucker", flag))
			tmp = MUCKER;
		else if (string_prefix("nucker", flag))
			tmp = SMUCKER;
		else if (string_prefix("interactive", flag))
			tmp = INTERACTIVE;
		else if (string_prefix("sticky", flag)
				 || string_prefix("silent", flag))
			tmp = STICKY;
		else if (string_prefix("wizard", flag))
			tmp = WIZARD;
		else if (string_prefix("truewizard", flag)) {
			tmp = WIZARD;
			truwiz = 1;
		} else if (string_prefix("zombie", flag))
			tmp = ZOMBIE;
		else if (string_prefix("xforcible", flag))
			tmp = XFORCIBLE;
		else if (string_prefix("vehicle", flag))
			tmp = VEHICLE;
		else if (string_prefix("quell", flag))
			tmp = QUELL;
	}
	if (result) {
		if ((!truwiz) && (tmp == WIZARD)) {
			result = (!Wizard(ref));
		} else {
			result = (tmp && ((FLAGS(ref) & tmp) == 0));
		}
	} else {
		if ((!truwiz) && (tmp == WIZARD)) {
			result = Wizard(ref);
		} else {
			result = (tmp && ((FLAGS(ref) & tmp) != 0));
		}
	}
	CLEAR(oper1);
	CLEAR(oper2);
	PushInt(result);
}


void
prim_playerp(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
	if (oper1->type != PROG_OBJECT)
		abort_interp("Invalid argument type.");
	if (!valid_object(oper1) && !is_home(oper1)) {
		result = 0;
	} else {
		ref = oper1->data.objref;
		CHECKREMOTE(ref);
		result = (Typeof(ref) == TYPE_PLAYER);
	}
	PushInt(result);
}


void
prim_thingp(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
	if (oper1->type != PROG_OBJECT)
		abort_interp("Invalid argument type.");
	if (!valid_object(oper1) && !is_home(oper1)) {
		result = 0;
	} else {
		ref = oper1->data.objref;
		CHECKREMOTE(ref);
		result = (Typeof(ref) == TYPE_THING);
	}
	PushInt(result);
}


void
prim_roomp(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
	if (oper1->type != PROG_OBJECT)
		abort_interp("Invalid argument type.");
	if (!valid_object(oper1) && !is_home(oper1)) {
		result = 0;
	} else {
		ref = oper1->data.objref;
		CHECKREMOTE(ref);
		result = (Typeof(ref) == TYPE_ROOM);
	}
	PushInt(result);
}


void
prim_programp(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
	if (oper1->type != PROG_OBJECT)
		abort_interp("Invalid argument type.");
	if (!valid_object(oper1) && !is_home(oper1)) {
		result = 0;
	} else {
		ref = oper1->data.objref;
		CHECKREMOTE(ref);
		result = (Typeof(ref) == TYPE_PROGRAM);
	}
	PushInt(result);
}


void
prim_exitp(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
	if (oper1->type != PROG_OBJECT)
		abort_interp("Invalid argument type.");
	if (!valid_object(oper1) && !is_home(oper1)) {
		result = 0;
	} else {
		ref = oper1->data.objref;
		CHECKREMOTE(ref);
		result = (Typeof(ref) == TYPE_EXIT);
	}
	PushInt(result);
}


void
prim_okp(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
	result = (valid_object(oper1));
	CLEAR(oper1);
	PushInt(result);
}

void
prim_location(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
	if (!valid_object(oper1))
		abort_interp("Invalid object.");
	CHECKREMOTE(oper1->data.objref);
	ref = DBFETCH(oper1->data.objref)->location;
	CLEAR(oper1);
	PushObject(ref);
}

void
prim_owner(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
	if (!valid_object(oper1))
		abort_interp("Invalid object.");
	CHECKREMOTE(oper1->data.objref);
	ref = OWNER(oper1->data.objref);
	CLEAR(oper1);
	PushObject(ref);
}

void
prim_controls(PRIM_PROTOTYPE)
{
	CHECKOP(2);
	oper1 = POP();
	oper2 = POP();
	if (!valid_object(oper1))
		abort_interp("Invalid object. (2)");
	if (!valid_object(oper2))
		abort_interp("Invalid object. (1)");
	CHECKREMOTE(oper1->data.objref);
	result = controls(oper2->data.objref, oper1->data.objref);
	CLEAR(oper1);
	PushInt(result);
}

void
prim_getlink(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
	if (!valid_object(oper1))
		abort_interp("Invalid object.");
	CHECKREMOTE(oper1->data.objref);
	if (Typeof(oper1->data.objref) == TYPE_PROGRAM)
		abort_interp("Illegal object referenced.");
	switch (Typeof(oper1->data.objref)) {
	case TYPE_EXIT:
		ref = (DBFETCH(oper1->data.objref)->sp.exit.ndest) ?
				(DBFETCH(oper1->data.objref)->sp.exit.dest)[0] : NOTHING;
		break;
	case TYPE_PLAYER:
		ref = PLAYER_HOME(oper1->data.objref);
		break;
	case TYPE_THING:
		ref = THING_HOME(oper1->data.objref);
		break;
	case TYPE_ROOM:
		ref = DBFETCH(oper1->data.objref)->sp.room.dropto;
		break;
	default:
		ref = NOTHING;
		break;
	}
	CLEAR(oper1);
	PushObject(ref);
}

void
prim_getlinks(PRIM_PROTOTYPE)
{
	int i, count;

	CHECKOP(1);
	oper1 = POP();
	if (!valid_object(oper1))
		abort_interp("Invalid object.");
	CHECKREMOTE(oper1->data.objref);
	if (Typeof(oper1->data.objref) == TYPE_PROGRAM)
		abort_interp("Illegal object referenced.");
	CLEAR(oper1);
	switch (Typeof(oper1->data.objref)) {
	case TYPE_EXIT:
		count = DBFETCH(oper1->data.objref)->sp.exit.ndest;
		for (i = 0; i < count; i++) {
			PushObject((DBFETCH(oper1->data.objref)->sp.exit.dest)[i]);
		}
		PushInt(count);
		break;
	case TYPE_PLAYER:
		ref = PLAYER_HOME(oper1->data.objref);
		count = 1;
		PushObject(ref);
		PushInt(count);
		break;
	case TYPE_THING:
		ref = THING_HOME(oper1->data.objref);
		count = 1;
		PushObject(ref);
		PushInt(count);
		break;
	case TYPE_ROOM:
		ref = DBFETCH(oper1->data.objref)->sp.room.dropto;
		if (ref != NOTHING) {
			count = 0;
			PushInt(count);
		} else {
			count = 1;
			PushObject(ref);
			PushInt(count);
		}
		break;
	default:
		count = 0;
		PushInt(count);
		break;
	}
}

int
prog_can_link_to(int mlev, dbref who, object_flag_type what_type, dbref where)
{
	if (where == HOME)
		return 1;
	if (where < 0 || where > db_top)
		return 0;
	switch (what_type) {
	case TYPE_EXIT:
		return (mlev > 3 || permissions(who, where) || (FLAGS(where) & LINK_OK));
		/* NOTREACHED */
		break;
	case TYPE_PLAYER:
		return (Typeof(where) == TYPE_ROOM && (mlev > 3 || permissions(who, where)
											   || Linkable(where)));
		/* NOTREACHED */
		break;
	case TYPE_ROOM:
		return ((Typeof(where) == TYPE_ROOM)
				&& (mlev > 3 || permissions(who, where) || Linkable(where)));
		/* NOTREACHED */
		break;
	case TYPE_THING:
		return ((Typeof(where) == TYPE_ROOM || Typeof(where) == TYPE_PLAYER)
				&& (mlev > 3 || permissions(who, where) || Linkable(where)));
		/* NOTREACHED */
		break;
	case NOTYPE:
		return (mlev > 3 || permissions(who, where) || (FLAGS(where) & LINK_OK) ||
				(Typeof(where) != TYPE_THING && (FLAGS(where) & ABODE)));
		/* NOTREACHED */
		break;
	}
	return 0;
}


void
prim_setlink(PRIM_PROTOTYPE)
{
	CHECKOP(2);
	oper1 = POP();				/* dbref: destination */
	oper2 = POP();				/* dbref: source */
	if ((oper1->type != PROG_OBJECT) || (oper2->type != PROG_OBJECT))
		abort_interp("setlink requires two dbrefs.");
	if (!valid_object(oper2))
		abort_interp("Invalid object. (1)");
	ref = oper2->data.objref;
	if (oper1->data.objref == NOTHING) {
		if ((mlev < 4) && !permissions(ProgUID, ref))
			abort_interp("Permission denied.");
		switch (Typeof(ref)) {
		case TYPE_EXIT:
			DBSTORE(ref, sp.exit.ndest, 0);
			if (DBFETCH(ref)->sp.exit.dest) {
				free((void *) DBFETCH(ref)->sp.exit.dest);
				DBSTORE(ref, sp.exit.dest, NULL);
			}
			if (MLevRaw(ref))
				SetMLevel(ref, 0);
			break;
		case TYPE_ROOM:
			DBSTORE(ref, sp.room.dropto, NOTHING);
			break;
		default:
			abort_interp("Invalid object. (1)");
		}
	} else {
		if (oper1->data.objref != HOME && !valid_object(oper1))
			abort_interp("Invalid object. (2)");
		if (Typeof(ref) == TYPE_PROGRAM)
			abort_interp("Program objects are not linkable. (1)");
		if (!prog_can_link_to(mlev, ProgUID, Typeof(ref), oper1->data.objref))
			abort_interp("Can't link source to destination.");
		switch (Typeof(ref)) {
		case TYPE_EXIT:
			if ((mlev < 4) && !permissions(ProgUID, ref))
				abort_interp("Permission denied.");
			if (DBFETCH(ref)->sp.exit.ndest != 0)
				abort_interp("Exit is already linked.");
			if (exit_loop_check(ref, oper1->data.objref))
				abort_interp("Link would cause a loop.");
			DBFETCH(ref)->sp.exit.ndest = 1;
			DBFETCH(ref)->sp.exit.dest = (dbref *) malloc(sizeof(dbref));
			(DBFETCH(ref)->sp.exit.dest)[0] = oper1->data.objref;
			DBDIRTY(ref);
			break;
		case TYPE_PLAYER:
			if ((mlev < 4) && !permissions(ProgUID, ref))
				abort_interp("Permission denied.");
			if (oper1->data.objref == HOME)
				abort_interp("Cannot link player to HOME.");
			PLAYER_SET_HOME(ref, oper1->data.objref);
			DBDIRTY(ref);
			break;
		case TYPE_THING:
			if ((mlev < 4) && !permissions(ProgUID, ref))
				abort_interp("Permission denied.");
			if (oper1->data.objref == HOME)
				abort_interp("Cannot link thing to HOME.");
			if (parent_loop_check(ref, oper1->data.objref))
				abort_interp("That would cause a parent paradox.");
			THING_SET_HOME(ref, oper1->data.objref);
			DBDIRTY(ref);
			break;
		case TYPE_ROOM:
			if ((mlev < 4) && !permissions(ProgUID, ref))
				abort_interp("Permission denied.");
			DBFETCH(ref)->sp.room.dropto = oper1->data.objref;
			DBDIRTY(ref);
			break;
		}
	}
	CLEAR(oper1);
	CLEAR(oper2);
}

void
prim_setown(PRIM_PROTOTYPE)
{
	CHECKOP(2);
	oper1 = POP();				/* dbref: new owner */
	oper2 = POP();				/* dbref: what */
	if (!valid_object(oper2))
		abort_interp("Invalid argument (1)");
	if (!valid_player(oper1))
		abort_interp("Invalid argument (2)");
	ref = oper2->data.objref;
	if ((mlev < 4) && oper1->data.objref != player)
		abort_interp("Permission denied. (2)");
	if ((mlev < 4) && (!(FLAGS(ref) & CHOWN_OK) ||
					   !test_lock(fr->descr, player, ref, "_/chlk")))
				abort_interp("Permission denied. (1)");
	switch (Typeof(ref)) {
	case TYPE_ROOM:
		if ((mlev < 4) && DBFETCH(player)->location != ref)
			abort_interp("Permission denied: not in room. (1)");
		break;
	case TYPE_THING:
		if ((mlev < 4) && DBFETCH(ref)->location != player)
			abort_interp("Permission denied: object not carried. (1)");
		break;
	case TYPE_PLAYER:
		abort_interp("Permission denied: cannot set owner of player. (1)");
	case TYPE_EXIT:
	case TYPE_PROGRAM:
		break;
	case TYPE_GARBAGE:
		abort_interp("Permission denied: who would want to own garbage? (1)");
	}
	OWNER(ref) = OWNER(oper1->data.objref);
	DBDIRTY(ref);
}

void
prim_newobject(PRIM_PROTOTYPE)
{
	CHECKOP(2);
	oper1 = POP();				/* string: name */
	oper2 = POP();				/* dbref: location */
	if ((mlev < 3) && (fr->already_created))
		abort_interp("An object was already created this program run.");
	CHECKOFLOW(1);
	ref = oper2->data.objref;
	if (!valid_object(oper2) || (!valid_player(oper2) && (Typeof(ref) != TYPE_ROOM)))
		abort_interp("Invalid argument (1)");
	if (oper1->type != PROG_STRING)
		abort_interp("Invalid argument (2)");
	CHECKREMOTE(ref);
	if ((mlev < 3) && !permissions(ProgUID, ref))
		abort_interp("Permission denied.");
	{
		const char *b = DoNullInd(oper1->data.string);
		dbref loc;

		if (!ok_name(b))
			abort_interp("Invalid name. (2)");

		ref = new_object();

		/* initialize everything */
		NAME(ref) = alloc_string(b);
		ALLOC_THING_SP(ref);
		DBFETCH(ref)->location = oper2->data.objref;
		OWNER(ref) = OWNER(ProgUID);
		THING_SET_VALUE(ref, 1);
		DBFETCH(ref)->exits = NOTHING;
		FLAGS(ref) = TYPE_THING;

		if ((loc = DBFETCH(player)->location) != NOTHING && controls(player, loc)) {
			THING_SET_HOME(ref, loc);	/* home */
		} else {
			THING_SET_HOME(ref, PLAYER_HOME(player));
			/* set to player's home instead */
		}
	}

	/* link it in */
	PUSH(ref, DBFETCH(oper2->data.objref)->contents);
	DBDIRTY(ref);
	DBDIRTY(oper2->data.objref);

	CLEAR(oper1);
	CLEAR(oper2);
	PushObject(ref);
}

void
prim_newroom(PRIM_PROTOTYPE)
{
	CHECKOP(2);
	oper1 = POP();				/* string: name */
	oper2 = POP();				/* dbref: location */
	if ((mlev < 3) && (fr->already_created))
		abort_interp("An object was already created this program run.");
	CHECKOFLOW(1);
	ref = oper2->data.objref;
	if (!valid_object(oper2) || (Typeof(ref) != TYPE_ROOM))
		abort_interp("Invalid argument (1)");
	if (oper1->type != PROG_STRING)
		abort_interp("Invalid argument (2)");
	if ((mlev < 3) && !permissions(ProgUID, ref))
		abort_interp("Permission denied.");
	{
		const char *b = DoNullInd(oper1->data.string);

		if (!ok_name(b))
			abort_interp("Invalid name. (2)");

		ref = new_object();

		/* Initialize everything */
		NAME(ref) = alloc_string(b);
		DBFETCH(ref)->location = oper2->data.objref;
		OWNER(ref) = OWNER(ProgUID);
		DBFETCH(ref)->exits = NOTHING;
		DBFETCH(ref)->sp.room.dropto = NOTHING;
		FLAGS(ref) = TYPE_ROOM | (FLAGS(player) & JUMP_OK);
		PUSH(ref, DBFETCH(oper2->data.objref)->contents);
		DBDIRTY(ref);
		DBDIRTY(oper2->data.objref);

		CLEAR(oper1);
		CLEAR(oper2);
		PushObject(ref);
	}
}

void
prim_newexit(PRIM_PROTOTYPE)
{
	CHECKOP(2);
	oper1 = POP();				/* string: name */
	oper2 = POP();				/* dbref: location */
	if (mlev < 3)
		abort_interp("Requires Mucker Level 3.");
	CHECKOFLOW(1);
	ref = oper2->data.objref;
	if (!valid_object(oper2) ||
		((!valid_player(oper2)) && (Typeof(ref) != TYPE_ROOM) && (Typeof(ref) != TYPE_THING)))
		abort_interp("Invalid argument (1)");
	if (oper1->type != PROG_STRING)
		abort_interp("Invalid argument (2)");
	CHECKREMOTE(ref);
	if ((mlev < 4) && !permissions(ProgUID, ref))
		abort_interp("Permission denied.");
	{
		const char *b = DoNullInd(oper1->data.string);

		if (!ok_name(b))
			abort_interp("Invalid name. (2)");

		ref = new_object();

		/* initialize everything */
		NAME(ref) = alloc_string(oper1->data.string->data);
		DBFETCH(ref)->location = oper2->data.objref;
		OWNER(ref) = OWNER(ProgUID);
		FLAGS(ref) = TYPE_EXIT;
		DBFETCH(ref)->sp.exit.ndest = 0;
		DBFETCH(ref)->sp.exit.dest = NULL;

		/* link it in */
		PUSH(ref, DBFETCH(oper2->data.objref)->exits);
		DBDIRTY(oper2->data.objref);

		CLEAR(oper1);
		CLEAR(oper2);
		PushObject(ref);
	}
}


void
prim_lockedp(PRIM_PROTOTYPE)
{
	struct inst *oper1, *oper2;

	/* d d - i */
	CHECKOP(2);
	oper1 = POP();				/* objdbref */
	oper2 = POP();				/* player dbref */
	if (fr->level > 8)
		abort_interp("Interp call loops not allowed.");
	if (!valid_object(oper2))
		abort_interp("invalid object (1).");
	if (!valid_player(oper2) && Typeof(oper2->data.objref) != TYPE_THING)
		abort_interp("Non-player argument (1).");
	CHECKREMOTE(oper2->data.objref);
	if (!valid_object(oper1))
		abort_interp("invalid object (2).");
	CHECKREMOTE(oper1->data.objref);
	result = !could_doit(fr->descr, oper2->data.objref, oper1->data.objref);
	CLEAR(oper1);
	CLEAR(oper2);
	PushInt(result);
}


void
prim_recycle(PRIM_PROTOTYPE)
{
	/* d -- */
	CHECKOP(1);
	oper1 = POP();				/* object dbref to recycle */
	if (oper1->type != PROG_OBJECT)
		abort_interp("Non-object argument (1).");
	if (!valid_object(oper1))
		abort_interp("Invalid object (1).");
	result = oper1->data.objref;
	if ((mlev < 3) || ((mlev < 4) && !permissions(ProgUID, result)))
		abort_interp("Permission denied.");
	if ((result == tp_player_start) || (result == GLOBAL_ENVIRONMENT))
		abort_interp("Cannot recycle that room.");
	if (Typeof(result) == TYPE_PLAYER)
		abort_interp("Cannot recycle a player.");
	if (result == program)
		abort_interp("Cannot recycle currently running program.");
	{
		int ii;

		for (ii = 0; ii < fr->caller.top; ii++)
			if (fr->caller.st[ii] == result)
				abort_interp("Cannot recycle active program.");
	}
	if (Typeof(result) == TYPE_EXIT)
		if (!unset_source(player, DBFETCH(player)->location, result))
			abort_interp("Cannot recycle old style exits.");
	CLEAR(oper1);
	recycle(fr->descr, player, result);
}


void
prim_setlockstr(PRIM_PROTOTYPE)
{
	CHECKOP(2);
	oper1 = POP();
	oper2 = POP();
	if (!valid_object(oper2))
		abort_interp("Invalid argument type (1)");
	if (oper1->type != PROG_STRING)
		abort_interp("Non-string argument (2)");
	ref = oper2->data.objref;
	if ((mlev < 4) && !permissions(ProgUID, ref))
		abort_interp("Permission denied.");
	result = setlockstr(fr->descr, player, ref,
						oper1->data.string ? oper1->data.string->data : (char *) "");
	CLEAR(oper1);
	CLEAR(oper2);
	PushInt(result);
}


void
prim_getlockstr(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();
	if (!valid_object(oper1))
		abort_interp("Invalid argument type");
	ref = oper1->data.objref;
	CHECKREMOTE(ref);
	if ((mlev < 3) && !permissions(ProgUID, ref))
		abort_interp("Permission denied.");
	{
		char *tmpstr;

		tmpstr = (char *) unparse_boolexp(player, GETLOCK(ref), 0);
		CLEAR(oper1);
		PushString(tmpstr);
	}
}


void
prim_part_pmatch(PRIM_PROTOTYPE)
{
	dbref ref;

	CHECKOP(1);
	oper1 = POP();
	if (oper1->type != PROG_STRING)
		abort_interp("Non-string argument.");
	if (!oper1->data.string)
		abort_interp("Empty string argument.");
	if (mlev < 3)
		abort_interp("Permission denied.  Requires Mucker Level 3.");
	ref = partial_pmatch(oper1->data.string->data);
	CLEAR(oper1);
	PushObject(ref);
}


void
prim_checkpassword(PRIM_PROTOTYPE)
{
	char *ptr;

	CHECKOP(2);
	oper2 = POP();
	oper1 = POP();

	if (mlev < 4)
		abort_interp("Permission denied.  Requires Wizbit.");
	if (oper1->type != PROG_OBJECT)
		abort_interp("Player dbref expected. (1)");
	ref = oper1->data.objref;
	if ((ref != NOTHING && !valid_player(oper1)) || ref == NOTHING)
		abort_interp("Player dbref expected. (1)");
	if (oper2->type != PROG_STRING)
		abort_interp("Password string expected. (2)");
	ptr = oper2->data.string ? oper2->data.string->data : "";
	if (ref != NOTHING) {
		const char *passwd = PLAYER_PASSWORD(ref);
		result = 1;
		if (!passwd) {
			if (*ptr)
				result = 0;
		} else {
			if (strcmp(ptr, PLAYER_PASSWORD(ref)))
				result = 0;
		}
	} else
		result = 0;

	CLEAR(oper1);
	CLEAR(oper2);
	PushInt(result);
}

void
prim_movepennies(PRIM_PROTOTYPE)
{
	int result2;
	dbref ref2;

	CHECKOP(3);
	oper1 = POP();
	oper2 = POP();
	oper3 = POP();
	if (mlev < 2)
		abort_interp("Requires Mucker Level 2 or better.");
	if (!valid_object(oper3))
		abort_interp("Invalid object. (1)");
	if (!valid_object(oper2))
		abort_interp("Invalid object. (2)");
	if (oper1->type != PROG_INTEGER)
		abort_interp("Non-integer argument (3)");
	if (oper1->data.number < 0)
		abort_interp("Invalid argument. (3)");
	ref = oper3->data.objref;
	ref2 = oper2->data.objref;
	if (Typeof(ref) == TYPE_PLAYER) {
		result = PLAYER_PENNIES(ref);
		if (Typeof(ref2) == TYPE_PLAYER) {
			result2 = PLAYER_PENNIES(ref2);
			if (mlev < 4) {
				if (result < (result - oper1->data.number))
					abort_interp("Would roll over player's score. (1)");
				if ((result - oper1->data.number) < 0)
					abort_interp("Result would be negative. (1)");
				if (result2 > (result2 + oper1->data.number))
					abort_interp("Would roll over player's score. (2)");
				if ((result2 + oper1->data.number) > tp_max_pennies)
					abort_interp("Would exceed MAX_PENNIES. (2)");
			}
			result2 += oper1->data.number;
			PLAYER_ADD_PENNIES(ref, -(oper1->data.number));
			PLAYER_ADD_PENNIES(ref2, oper1->data.number);
			DBDIRTY(ref);
			DBDIRTY(ref2);
		} else if (Typeof(ref2) == TYPE_THING) {
			if (mlev < 4)
				abort_interp("Permission denied. (2)");
			result2 = THING_VALUE(ref2) + oper1->data.number;
			if (result < (result - oper1->data.number))
				abort_interp("Would roll over player's score. (1)");
			if ((result - oper1->data.number) < 0)
				abort_interp("Result would be negative. (1)");
			PLAYER_ADD_PENNIES(ref, -(oper1->data.number));
			THING_SET_VALUE(ref2, (THING_VALUE(ref2) + oper1->data.number));
			DBDIRTY(ref);
			DBDIRTY(ref2);
		} else {
			abort_interp("Invalid object type. (2)");
		}
	} else if (Typeof(ref) == TYPE_THING) {
		if (mlev < 4)
			abort_interp("Permission denied. (1)");
		result = THING_VALUE(ref) - oper1->data.number;
		if (result < 1)
			abort_interp("Result must be positive. (1)");
		if (Typeof(ref2) == TYPE_PLAYER) {
			result2 = PLAYER_PENNIES(ref2);
			if (result2 > (result2 + oper1->data.number))
				abort_interp("Would roll over player's score. (2)");
			if ((result2 + oper1->data.number) > tp_max_pennies)
				abort_interp("Would exceed MAX_PENNIES. (2)");
			THING_SET_VALUE(ref, (THING_VALUE(ref) - oper1->data.number));
			PLAYER_ADD_PENNIES(ref2, oper1->data.number);
			DBDIRTY(ref);
			DBDIRTY(ref2);
		} else if (Typeof(ref2) == TYPE_THING) {
			THING_SET_VALUE(ref, (THING_VALUE(ref) - oper1->data.number));
			THING_SET_VALUE(ref2, (THING_VALUE(ref2) + oper1->data.number));
			DBDIRTY(ref);
			DBDIRTY(ref2);
		} else {
			abort_interp("Invalid object type. (2)");
		}
	} else {
		abort_interp("Invalid object type. (1)");
	}
	CLEAR(oper1);
	CLEAR(oper2);
	CLEAR(oper3);
}


void
prim_findnext(PRIM_PROTOTYPE)
{
	struct flgchkdat check;
	dbref who, item, ref, i;
	const char* name;

	CHECKOP(4);
	oper4 = POP(); /* str:flags */
	oper3 = POP(); /* str:namepattern */
	oper2 = POP(); /* ref:owner */
	oper1 = POP(); /* ref:currobj */

	if (oper4->type != PROG_STRING)
		abort_interp("Expected string argument. (4)");
	if (oper3->type != PROG_STRING)
		abort_interp("Expected string argument. (3)");
	if (oper2->type != PROG_OBJECT)
		abort_interp("Expected dbref argument. (2)");
	if (oper2->data.objref < NOTHING || oper2->data.objref >= db_top)
		abort_interp("Bad object. (2)");
	if (Typeof(oper2->data.objref) == TYPE_GARBAGE)
		abort_interp("Garbage object. (2)");
	if (oper1->type != PROG_OBJECT)
		abort_interp("Expected dbref argument. (1)");
	if (oper1->data.objref < NOTHING || oper1->data.objref >= db_top)
		abort_interp("Bad object. (1)");
	if (Typeof(oper1->data.objref) == TYPE_GARBAGE)
		abort_interp("Garbage object. (1)");

	item = oper1->data.objref;
	who = oper2->data.objref;
	name = DoNullInd(oper3->data.string);

	if (mlev < 2)
		abort_interp("Permission denied.  Requires at least Mucker Level 2.");

	if (mlev < 3) {
		if (who == NOTHING) {
			abort_interp("Permission denied.  Owner inspecific searches require Mucker Level 3.");
		} else if (who != ProgUID) {
			abort_interp("Permission denied.  Searching for other people's stuff requires Mucker Level 3.");
		}
	}

	if (item == NOTHING) {
		item = 0;
	} else {
		item++;
	}
	strcpy(buf, name);

	ref = NOTHING;
	init_checkflags(player, DoNullInd(oper4->data.string), &check);
	for (i = item; i < db_top; i++) {
		if ((who == NOTHING || OWNER(i) == who) &&
			checkflags(i, check) && NAME(i) &&
			(!*name || equalstr(buf, (char *) NAME(i))))
		{
			ref = i;
			break;
		}
	}

	CLEAR(oper1);
	CLEAR(oper2);
	CLEAR(oper3);
	CLEAR(oper4);

	PushObject(ref);
}


/* ============================ */
/* = More ProtoMuck prims     = */
/* ============================ */

void
prim_nextentrance(PRIM_PROTOTYPE)
{
	dbref linkref, ref;
	int foundref = 0;
	int i, count;

	if (mlev < 3)
		abort_interp("Permission denied.  Requires Mucker Level 3.");
	oper2 = POP();
	oper1 = POP();
	linkref = oper1->data.objref;
	ref = oper2->data.objref;
	if (!valid_object(oper1) && (linkref != NOTHING) && (linkref != HOME))
		abort_interp("Invalid link reference object (2)");
	if (!valid_object(oper2) && ref != NOTHING)
		abort_interp("Invalid reference object (1)");
	if (linkref == HOME)
		linkref = PLAYER_HOME(player);
	(void) ref++;
	for (; ref < db_top; ref++) {
		oper2->data.objref = ref;
		if (valid_object(oper2)) {
			switch(Typeof(ref)) {
				case TYPE_PLAYER:
					if (PLAYER_HOME(ref) == linkref)
						foundref = 1;
					break;
				case TYPE_ROOM:
					if (DBFETCH(ref)->sp.room.dropto == linkref)
						foundref = 1;
					break;
				case TYPE_THING:
					if (THING_HOME(ref) == linkref)
						foundref = 1;
					break;
				case TYPE_EXIT:
					count = DBFETCH(ref)->sp.exit.ndest;
					for (i = 0; i < count; i++) {
						if (DBFETCH(ref)->sp.exit.dest[i] == linkref)
							foundref = 1;
					}
					break;
			}
            if (foundref)
				break;
		}
	}
	if (!foundref)
		ref = NOTHING;
	CLEAR(oper1);
	CLEAR(oper2);
	PushObject(ref);
}

void
prim_newplayer(PRIM_PROTOTYPE)
{
	dbref newplayer;
	char *name, *password;
	struct object *newp;

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if (mlev < 4)
		abort_interp("Permission denied.  Requires Wizbit.");
    if (oper1->type != PROG_STRING)
		abort_interp("Non-string argument. (1)");
    if (!oper1->data.string)
		abort_interp("Empty string argument. (1)");
    if (oper2->type != PROG_STRING)
		abort_interp("Non-string argument. (2)");
    if (!oper2->data.string)
		abort_interp("Empty string argument. (2)");

    name = oper2->data.string->data;
    password = oper1->data.string->data;

    if (!ok_player_name(name))
       abort_interp("Invalid player name. (1)");
    if (!ok_password(password))
       abort_interp("Invalid password. (1)");

    /* else he doesn't already exist, create him */
    newplayer = create_player(name, password);

    log_status("PCREATED[MUF]: %s(%d) by %s(%d)\n",
        NAME(newplayer), (int) newplayer, NAME(player), (int) player);
    
    CLEAR(oper1);
    CLEAR(oper2);
    PushObject(newplayer);
}

void
prim_copyplayer(PRIM_PROTOTYPE)
{
   dbref newplayer, ref;
   char *name, *password;
   struct object *newp;

    CHECKOP(3);
    oper1 = POP();
    oper2 = POP();
    oper3 = POP();

    if (mlev < 4)
		abort_interp("Permission denied.  Requires Wizbit.");
    if (oper1->type != PROG_STRING)
		abort_interp("Non-string argument. (3)");
    if (!oper1->data.string)
		abort_interp("Empty string argument. (3)");
    if (oper2->type != PROG_STRING)
		abort_interp("Non-string argument. (2)");
    if (!oper2->data.string)
	abort_interp("Empty string argument. (2)");
    ref = oper3->data.objref;
    if ((ref != NOTHING && !valid_player(oper3)) || ref == NOTHING)
       abort_interp("Player dbref expected. (1)");
    CHECKREMOTE(ref);

    name = oper2->data.string->data;
    password = oper1->data.string->data;

    if (!ok_player_name(name))
       abort_interp("Invalid player name. (2)");
    if (!ok_password(password))
       abort_interp("Invalid password. (2)");

    /* else he doesn't already exist, create him */
    newplayer = create_player(name, password);

    /* initialize everything */
	FLAGS(newplayer) = FLAGS(ref);

    newp = DBFETCH(newplayer);
	newp->properties = copy_prop(ref);
	newp->exits = NOTHING;
	newp->contents = NOTHING;
	newp->next = NOTHING;
#ifdef DISKBASE
	newp->propsfpos = 0;
	newp->propsmode = PROPS_UNLOADED;
	newp->propstime = 0;
	newp->nextold = NOTHING;
	newp->prevold = NOTHING;
	dirtyprops(newplayer);
#endif

    PLAYER_SET_HOME(newplayer, PLAYER_HOME(ref));
	PLAYER_ADD_PENNIES(newplayer, PLAYER_PENNIES(ref));
	moveto(newplayer, PLAYER_HOME(ref));

    /* link him to player_start */
    log_status("PCREATE[MUF]: %s(%d) by %s(%d)\n",
        NAME(newplayer), (int) newplayer, NAME(player), (int) player);
    
    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
    PushObject(newplayer);
}

void
prim_toadplayer(PRIM_PROTOTYPE)
{
    dbref   victim;
    dbref   recipient;
    dbref   stuff;
    char    buf[BUFFER_LEN];

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    victim = oper1->data.objref;
    if (mlev < 4)
		abort_interp("Permission denied.  Requires Wizbit.");
    if ((ref != NOTHING && !valid_player(oper1)) || ref == NOTHING)
		abort_interp("Player dbref expected for player to be toaded (1)");
    recipient = oper2->data.objref;
    if ((recipient != NOTHING && !valid_player(oper2)) || ref == NOTHING)
		abort_interp("Player dbref expected for recipient (2)");
    CHECKREMOTE(ref);
    CHECKREMOTE(recipient);

    if (Typeof(victim) != TYPE_PLAYER) {
		abort_interp("You can only toad players.");
		return;
    }
    if (Typeof(recipient) != TYPE_PLAYER) {
		abort_interp("Only players can receive the objects.");
		return;
    }
    if (get_property_class( victim, "@/precious" )) {
		abort_interp("That player is precious.");
		return;
    }
    if ((FLAGS(victim) & WIZARD)) {
		abort_interp("You can't toad a wizard.");
		return;
    }

    /* we're ok, do it */
	send_contents(fr->descr, player, HOME);
	for (stuff = 0; stuff < db_top; stuff++) {
	    if (OWNER(stuff) == victim) {
			switch (Typeof(stuff)) {
				case TYPE_PROGRAM:
					dequeue_prog(stuff, 0);  /* dequeue player's progs */
					FLAGS(stuff) &= ~(ABODE | WIZARD);
					SetMLevel(stuff,0);
				case TYPE_ROOM:
				case TYPE_THING:
				case TYPE_EXIT:
					OWNER(stuff) = recipient;
					DBDIRTY(stuff);
				break;
			}
	    }
	    if (Typeof(stuff) == TYPE_THING && THING_HOME(stuff) == victim) {
			THING_SET_HOME(stuff, tp_player_start);
	    }
	}
	if (PLAYER_PASSWORD(victim)) {
	    free((void *) PLAYER_PASSWORD(victim));
		PLAYER_SET_PASSWORD(victim, 0);
	}
	dequeue_prog(victim, 0);  /* dequeue progs that player's running */

	log_status("TOADED[MUF]: %s(%d) by %s(%d)\n", NAME(victim),
		   victim, NAME(player), player);

	delete_player(victim);
	sprintf(buf, "A slimy toad named %s", unmangle(victim, PNAME(victim)));
	free((void *) NAME(victim));
	NAME(victim) = alloc_string(buf);
	DBDIRTY(victim);
	boot_player_off(victim);

	FREE_PLAYER_SP(victim);
	ALLOC_THING_SP(victim);
	THING_SET_HOME(victim, PLAYER_HOME(recipient));

	/* reset name */
	FLAGS(victim) = (FLAGS(victim) & ~TYPE_MASK) | TYPE_THING;
	OWNER(victim) = recipient;
	THING_SET_VALUE(victim, 1);

	CLEAR(oper1);
	CLEAR(oper2);
}

void
prim_objmem(PRIM_PROTOTYPE)
{
	int i;

	oper1 = POP();
	if (oper1->type != PROG_OBJECT)
		abort_interp("Argument must be a dbref.");
	ref = oper1->data.objref;
	if (ref >= db_top || ref <= NOTHING)
		abort_interp("Invalid object.");
	CLEAR(oper1);
	i = size_object(ref, 0);
	PushInt(i);
}



void
prim_instances(PRIM_PROTOTYPE)
{
   unsigned short a = 0;
   int b = 0;
   CHECKOP(1);
   oper1 = POP();

   if (!valid_object(oper1))
      abort_interp("Invalid object.");

   ref = oper1->data.objref;
   if (Typeof(ref) != TYPE_PROGRAM)
      abort_interp("Object must be a program.");

   CLEAR(oper1);
   a = PROGRAM_INSTANCES(ref);
   b = a;
   PushInt(b);
}

void
prim_compiledp(PRIM_PROTOTYPE)
{
	int i = 0;
	CHECKOP(1);
	oper1 = POP();

	if (!valid_object(oper1))
		abort_interp("Invalid object.");

	ref = oper1->data.objref;
	if (Typeof(ref) != TYPE_PROGRAM)
		abort_interp("Object must be a program.");

	CLEAR(oper1);
	i = PROGRAM_SIZ(ref);
	PushInt(i);
}


void
prim_newpassword(PRIM_PROTOTYPE)
{
    char *ptr2;
    char pad_char[] = "";

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (mlev < 4)
		abort_interp("Permission denied.  Requires Wizbit.");
    if (oper1->type != PROG_STRING)
		abort_interp("Password string expected. (2)");
    if (oper2->type != PROG_OBJECT)
		abort_interp("Player dbref expected. (1)");
    ptr2 = oper1->data.string? oper1->data.string->data : pad_char;
    ref = oper2->data.objref;
    if (ref != NOTHING && !valid_player(oper2))
		abort_interp("Player dbref expected. (1)");
    CHECKREMOTE(ref);
    free((void *) PLAYER_PASSWORD(ref));
	PLAYER_SET_PASSWORD(ref, alloc_string(ptr2));
    CLEAR(oper1);
    CLEAR(oper2);
}

void
prim_newprogram(PRIM_PROTOTYPE)
{
	dbref newprog;
	char buf[BUFFER_LEN];
	int jj;

	CHECKOP(1);
	oper1 = POP();

	if (mlev < 4)
		abort_interp("Permission denied.  Requires Wizbit.");
	if (oper1->type != PROG_STRING)
		abort_interp("Expected string argument.");
	if (!ok_name(oper1->data.string->data))
		abort_interp("Invalid name (2)");

	newprog = new_object();

	NAME(newprog) = alloc_string(oper1->data.string->data);
	sprintf(buf, "A scroll containing a spell called %s", oper1->data.string->data);
	SETDESC(newprog, buf);
	DBFETCH(newprog)->location = player;
	FLAGS(newprog) = TYPE_PROGRAM;
	jj = MLevel(player);
	if (jj < 1)
	    jj = 1;
	if (jj > 3)
	    jj = 3;
	SetMLevel(newprog, jj);

	OWNER(newprog) = OWNER(player);
	ALLOC_PROGRAM_SP(newprog);
	PROGRAM_SET_FIRST(newprog, NULL);
	PROGRAM_SET_CURR_LINE(newprog, 0);
	PROGRAM_SET_SIZ(newprog, 0);
	PROGRAM_SET_CODE(newprog, NULL);
	PROGRAM_SET_START(newprog, NULL);
	PROGRAM_SET_PUBS(newprog, NULL);
	PROGRAM_SET_MCPBINDS(newprog, NULL);
	PROGRAM_SET_PROFTIME(newprog, 0, 0);
	PROGRAM_SET_PROFSTART(newprog, 0);
	PROGRAM_SET_PROF_USES(newprog, 0);

	PLAYER_SET_CURR_PROG(player, newprog);

	PUSH(newprog, DBFETCH(player)->contents);
	DBDIRTY(newprog);
	DBDIRTY(player);

	PushObject(newprog);
}

extern struct line *read_program(dbref prog);

void
prim_compile(PRIM_PROTOTYPE)
{
	dbref ref;
	struct line *tmpline;

	CHECKOP(2);
	oper2 = POP();
	oper1 = POP();
	if (mlev < 4)
		abort_interp("Permission denied.  Requires Wizbit.");
	if (!valid_object(oper1))
		abort_interp("No program dbref given. (1)");
	ref = oper1->data.objref;
	if (Typeof(ref) != TYPE_PROGRAM)
		abort_interp("No program dbref given. (1)");
	if (oper2->type != PROG_INTEGER)
		abort_interp("No boolean integer given. (2)");
    if (PROGRAM_INSTANCES(ref) > 0)
		abort_interp("That program is currently in use.");
	tmpline = PROGRAM_FIRST(ref);
	PROGRAM_SET_FIRST(ref, read_program(ref));
	do_compile(fr->descr, player, ref, oper2->data.number);
	free_prog_text(PROGRAM_FIRST(ref));
	PROGRAM_SET_FIRST(ref, tmpline);
	PushInt(PROGRAM_SIZ(ref));
}


void
prim_uncompile(PRIM_PROTOTYPE)
{
	dbref ref;

	CHECKOP(1);
	oper1 = POP();
	if (mlev < 4)
		abort_interp("Permission denied.  Requires Wizbit.");
	if (!valid_object(oper1))
		abort_interp("No program dbref given.");
	ref = oper1->data.objref;
	if (Typeof(ref) != TYPE_PROGRAM)
		abort_interp("No program dbref given.");
    if (PROGRAM_INSTANCES(ref) > 0)
		abort_interp("That program is currently in use.");
	uncompile_program(ref);
}


void
prim_getpids(PRIM_PROTOTYPE)
{
	stk_array *nw;

	CHECKOP(1);
	oper1 = POP();
	if (mlev < 3)
		abort_interp("Permission denied.  Requires Mucker Level 3.");
	if (oper1->type != PROG_OBJECT)
		abort_interp("Non-object argument (1)");
	nw = get_pids(oper1->data.objref);
	CLEAR(oper1);
	PushArrayRaw(nw);
}


void
prim_getpidinfo(PRIM_PROTOTYPE)
{
	stk_array *nw;

	CHECKOP(1);
	oper1 = POP();
	if (mlev < 3)
		abort_interp("Permission denied.  Requires Mucker Level 3.");
	if (oper1->type != PROG_INTEGER)
		abort_interp("Non-integer argument (1)");
	nw = get_pidinfo(oper1->data.number);
	CLEAR(oper1);
	PushArrayRaw(nw);
}


void
prim_contents_array(PRIM_PROTOTYPE)
{
	struct inst temp1, temp2;
	stk_array *nw;
	int count = 0;

	CHECKOP(1);
	oper1 = POP();
	if (!valid_object(oper1))
		abort_interp("Invalid dbref (1)");
	ref = oper1->data.objref;
	if ((Typeof(ref) == TYPE_PROGRAM) || (Typeof(ref) == TYPE_EXIT))
		abort_interp("Dbref cannot be a program nor exit (1)");
	CHECKREMOTE(oper1->data.objref);
	nw = new_array_packed(0);
	ref = DBFETCH(ref)->contents;
	/* WORK: Using this on #0 is probably bad. */
	while ((ref > 0) && (ref < db_top)) {
		temp1.type = PROG_INTEGER;
		temp1.data.number = count++;
		temp2.type = PROG_OBJECT;
		temp2.data.objref = ref;
		array_setitem(&nw, &temp1, &temp2);
		CLEAR(&temp1);
		CLEAR(&temp2);
		ref = DBFETCH(ref)->next;
	}
	CLEAR(oper1);
	PushArrayRaw(nw);
}

void
prim_exits_array(PRIM_PROTOTYPE)
{
	struct inst temp1, temp2;
	stk_array *nw;
	int count = 0;

	CHECKOP(1);
	oper1 = POP();
	if (!valid_object(oper1))
		abort_interp("Invalid dbref (1)");
	ref = oper1->data.objref;
	CHECKREMOTE(oper1->data.objref);
	if ((Typeof(ref) == TYPE_PROGRAM) || (Typeof(ref) == TYPE_EXIT))
		abort_interp("Dbref cannot be a program nor exit (1)");
	nw = new_array_packed(0);
	ref = DBFETCH(ref)->exits;
	while ((ref > 0) && (ref < db_top)) {
		temp1.type = PROG_INTEGER;
		temp1.data.number = count++;
		temp2.type = PROG_OBJECT;
		temp2.data.objref = ref;
		array_setitem(&nw, &temp1, &temp2);
		CLEAR(&temp1);
		CLEAR(&temp2);
		ref = DBFETCH(ref)->next;
	}
	CLEAR(oper1);
	PushArrayRaw(nw);
}

stk_array *
array_getlinks(dbref obj)
{
	struct inst temp1, temp2;
	stk_array *nw;
	int count = 0;

	nw = new_array_packed(0);
	if ((obj >= NOTHING) && (obj < db_top)) {
		switch (Typeof(obj)) {
			case TYPE_ROOM: {
				temp1.type = PROG_INTEGER;
				temp1.data.number = count++;
				temp2.type = PROG_OBJECT;
				temp2.data.objref = DBFETCH(obj)->sp.room.dropto;
				array_setitem(&nw, &temp1, &temp2);
				CLEAR(&temp1);
				CLEAR(&temp2);
				break;
			}
			case TYPE_THING: {
				temp1.type = PROG_INTEGER;
				temp1.data.number = count++;
				temp2.type = PROG_OBJECT;
				temp2.data.objref = THING_HOME(obj);
				array_setitem(&nw, &temp1, &temp2);
				CLEAR(&temp1);
				CLEAR(&temp2);
				break;
			}
			case TYPE_PLAYER: {
				temp1.type = PROG_INTEGER;
				temp1.data.number = count++;
				temp2.type = PROG_OBJECT;
				temp2.data.objref = PLAYER_HOME(obj);
				array_setitem(&nw, &temp1, &temp2);
				CLEAR(&temp1);
				CLEAR(&temp2);
				break;
			}
			case TYPE_EXIT: {
				for (count = 0; count < (DBFETCH(obj)->sp.exit.ndest); count++) {
					temp1.type = PROG_INTEGER;
					temp1.data.number = count;
					temp2.type = PROG_OBJECT;
					temp2.data.objref = (DBFETCH(obj)->sp.exit.dest)[count];
					array_setitem(&nw, &temp1, &temp2);
				}
				CLEAR(&temp1);
				CLEAR(&temp2);
				break;
			}
		}
	}
	return nw;
}

void
prim_getlinks_array(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();

	if (!valid_object(oper1))
		abort_interp("Invalid object dbref. (1)");
	ref = oper1->data.objref;
	CHECKREMOTE(oper1->data.objref);

	CLEAR(oper1);
	PushArrayRaw(array_getlinks(ref));
}


