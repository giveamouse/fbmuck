/* $Header$ */

/*
 * $Log: speech.c,v $
 * Revision 1.4  2000/11/22 10:01:58  revar
 * Changed MPI from using Wizbit objects to give special permissions, to using
 * 'Blessed' properties.  Blessed props have few permissions restrictions.
 * Added @bless and @unbless wizard commands.
 * Added BLESSPROP and UNBLESSPROP muf primitives.
 * Added {bless} {unbless} and {revoke} MPI commands.
 * Fixed {listprops} crasher bug.
 *
 * Revision 1.3  2000/07/19 01:33:18  revar
 * Compiling cleanup for -Wall -Wstrict-prototypes -Wno-format.
 * Changed the mcpgui package to use 'const char*'s instead of 'char *'s
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
 * Revision 1.1.1.1  1999/12/12 07:27:44  foxen
 * Initial FB6 CVS checkin.
 *
 * Revision 1.1  1996/06/12 03:04:09  foxen
 * Initial revision
 *
 * Revision 5.14  1994/03/14  12:20:58  foxen
 * Fb5.20 release checkpoint.
 *
 * Revision 5.13  1994/01/18  20:52:20  foxen
 * Version 5.15 release.
 *
 * Revision 5.12  1994/01/17  03:26:16  foxen
 * Changed gripe to allow wizards to list the gripefile via 'gripe' with no args.
 *
 * Revision 5.11  1993/12/20  06:22:51  foxen
 * *** empty log message ***
 *
 * Revision 5.1  1993/12/17  00:07:33  foxen
 * initial revision.
 *
 * Revision 1.12  90/09/16  04:43:00  rearl
 *
 * Preparation code added for disk-based MUCK.
 *
 * Revision 1.11  90/09/15  22:28:08  rearl
 * Fixed bug in @wall.
 *
 * Revision 1.10  90/09/10  02:21:23  rearl
 * Put quotes around the page format.
 *
 * Revision 1.9  90/08/27  03:34:21  rearl
 * Changed page format (yet again)
 *
 * Revision 1.8  90/08/11  04:10:47  rearl
 * *** empty log message ***
 *
 * Revision 1.7  90/08/09  21:07:44  rearl
 * Removed notify_except2().
 *
 * Revision 1.6  90/08/05  03:21:36  rearl
 * Redid matching routines.
 *
 * Revision 1.5  90/08/02  18:54:29  rearl
 * Fixed some calls to logging functions.
 *
 * Revision 1.4  90/07/29  17:44:45  rearl
 * Took out reconstruct_message() braindamage.
 *
 * Revision 1.3  90/07/21  16:30:04  casie
 * removed fflush(stderr); call
 *
 * Revision 1.2  90/07/21  13:04:18  casie
 * modified for logging handling
 *
 * Revision 1.1  90/07/19  23:04:11  casie
 * Initial revision
 *
 */

#include "copyright.h"
#include "config.h"

#include "db.h"
#include "mpi.h"
#include "interface.h"
#include "match.h"
#include "params.h"
#include "tune.h"
#include "props.h"
#include "externs.h"
#include <ctype.h>

/* Commands which involve speaking */

int blank(const char *s);

void
do_say(dbref player, const char *message)
{
	dbref loc;
	char buf[BUFFER_LEN];

	if ((loc = getloc(player)) == NOTHING)
		return;

	/* notify everybody */
	sprintf(buf, "You say, \"%s\"", message);
	notify(player, buf);
	sprintf(buf, "%s says, \"%s\"", PNAME(player), message);
	notify_except(DBFETCH(loc)->contents, player, buf, player);
}

void
do_whisper(int descr, dbref player, const char *arg1, const char *arg2)
{
	dbref who;
	char buf[BUFFER_LEN];
	struct match_data md;

	init_match(descr, player, arg1, TYPE_PLAYER, &md);
	match_neighbor(&md);
	match_me(&md);
	if (Wizard(player) && Typeof(player) == TYPE_PLAYER) {
		match_absolute(&md);
		match_player(&md);
	}
	switch (who = match_result(&md)) {
	case NOTHING:
		notify(player, "Whisper to whom?");
		break;
	case AMBIGUOUS:
		notify(player, "I don't know who you mean!");
		break;
	default:
		sprintf(buf, "%s whispers, \"%s\"", PNAME(player), arg2);
		if (!notify_from(player, who, buf)) {
			sprintf(buf, "%s is not connected.", PNAME(who));
			notify(player, buf);
			break;
		}
		sprintf(buf, "You whisper, \"%s\" to %s.", arg2, PNAME(who));
		notify(player, buf);
		break;
	}
}

void
do_pose(dbref player, const char *message)
{
	dbref loc;
	char buf[BUFFER_LEN];

	if ((loc = getloc(player)) == NOTHING)
		return;

	/* notify everybody */
	sprintf(buf, "%s %s", PNAME(player), message);
	notify_except(DBFETCH(loc)->contents, NOTHING, buf, player);
}

void
do_wall(dbref player, const char *message)
{
	dbref i;
	char buf[BUFFER_LEN];

	if (Wizard(player) && Typeof(player) == TYPE_PLAYER) {
		log_status("WALL from %s(%d): %s\n", NAME(player), player, message);
		sprintf(buf, "%s shouts, \"%s\"", NAME(player), message);
		for (i = 0; i < db_top; i++) {
			if (Typeof(i) == TYPE_PLAYER) {
				notify_from(player, i, buf);
			}
		}
	} else {
		notify(player, "But what do you want to do with the wall?");
	}
}

void
do_gripe(dbref player, const char *message)
{
	dbref loc;
	char buf[BUFFER_LEN];

	if (!message || !*message) {
		if (Wizard(player)) {
			spit_file(player, LOG_GRIPE);
		} else {
			notify(player, "If you wish to gripe, use 'gripe <message>'.");
		}
		return;
	}

	loc = DBFETCH(player)->location;
	log_gripe("GRIPE from %s(%d) in %s(%d): %s\n",
			  NAME(player), player, NAME(loc), loc, message);

	notify(player, "Your complaint has been duly noted.");

	sprintf(buf, "## GRIPE from %s: %s", NAME(player), message);
	wall_wizards(buf);
}

/* doesn't really belong here, but I couldn't figure out where else */
void
do_page(dbref player, const char *arg1, const char *arg2)
{
	char buf[BUFFER_LEN];
	dbref target;

	if (!payfor(player, tp_lookup_cost)) {
		notify_fmt(player, "You don't have enough %s.", tp_pennies);
		return;
	}
	if ((target = lookup_player(arg1)) == NOTHING) {
		notify(player, "I don't recognize that name.");
		return;
	}
	if (FLAGS(target) & HAVEN) {
		notify(player, "That player does not wish to be disturbed.");
		return;
	}
	if (blank(arg2))
		sprintf(buf, "You sense that %s is looking for you in %s.",
				PNAME(player), NAME(DBFETCH(player)->location));
	else
		sprintf(buf, "%s pages from %s: \"%s\"", PNAME(player),
				NAME(DBFETCH(player)->location), arg2);
	if (notify_from(player, target, buf))
		notify(player, "Your message has been sent.");
	else {
		sprintf(buf, "%s is not connected.", PNAME(target));
		notify(player, buf);
	}
}

void
notify_listeners(dbref who, dbref xprog, dbref obj, dbref room, const char *msg, int isprivate)
{
	char buf[BUFFER_LEN];
	dbref ref;

	if (obj == NOTHING)
		return;

	if (tp_listeners && (tp_listeners_obj || Typeof(obj) == TYPE_ROOM)) {
		listenqueue(-1, who, room, obj, obj, xprog, "_listen", msg, tp_listen_mlev, 1, 0);
		listenqueue(-1, who, room, obj, obj, xprog, "~listen", msg, tp_listen_mlev, 1, 1);
		listenqueue(-1, who, room, obj, obj, xprog, "~olisten", msg, tp_listen_mlev, 0, 1);
	}

	if (tp_zombies && Typeof(obj) == TYPE_THING && !isprivate) {
		if (FLAGS(obj) & VEHICLE) {
			if (getloc(who) == getloc(obj)) {
				char pbuf[BUFFER_LEN];
				const char *prefix;

				prefix = do_parse_prop(-1, who, obj, MESGPROP_OECHO,
										"(@Oecho)", pbuf, MPI_ISPRIVATE);
				if (!prefix || !*prefix)
					prefix = "Outside>";
				sprintf(buf, "%s %.*s", prefix, (BUFFER_LEN - 2 - strlen(prefix)), msg);
				ref = DBFETCH(obj)->contents;
				while (ref != NOTHING) {
					notify_nolisten(ref, buf, isprivate);
					ref = DBFETCH(ref)->next;
				}
			}
		}
	}

	if (Typeof(obj) == TYPE_PLAYER || Typeof(obj) == TYPE_THING)
		notify_nolisten(obj, msg, isprivate);
}

void
notify_except(dbref first, dbref exception, const char *msg, dbref who)
{
	dbref room, srch;

	if (first != NOTHING) {

		srch = room = DBFETCH(first)->location;

		if (tp_listeners) {
			notify_from_echo(who, srch, msg, 0);

			if (tp_listeners_env) {
				srch = DBFETCH(srch)->location;
				while (srch != NOTHING) {
					notify_from_echo(who, srch, msg, 0);
					srch = getparent(srch);
				}
			}
		}

		DOLIST(first, first) {
			if ((Typeof(first) != TYPE_ROOM) && (first != exception)) {
				/* don't want excepted player or child rooms to hear */
				notify_from_echo(who, first, msg, 0);
			}
		}
	}
}


void
parse_oprop(int descr, dbref player, dbref dest, dbref exit, const char *propname,
			   const char *prefix, const char *whatcalled)
{
	const char* msg = get_property_class(exit, propname);
	int ival = 0;
	if (Prop_Blessed(exit, propname))
		ival |= MPI_ISBLESSED;

	if (msg)
		parse_omessage(descr, player, dest, exit, msg, prefix, whatcalled, ival);
}

void
parse_omessage(int descr, dbref player, dbref dest, dbref exit, const char *msg,
			   const char *prefix, const char *whatcalled, int mpiflags)
{
	char buf[BUFFER_LEN * 2];
	char *ptr;

	do_parse_mesg(descr, player, exit, msg, whatcalled, buf, MPI_ISPUBLIC | mpiflags);
	ptr = pronoun_substitute(descr, player, buf);
	if (!*ptr)
		return;
	strcpy(buf, NAME(player));
	if (*ptr != '\'' && *ptr != ' ' && *ptr != ',' && *ptr != '-') {
		strcat(buf, " ");
	}
	strcat(buf, ptr);
	buf[BUFFER_LEN - 1] = '\0';
	notify_except(DBFETCH(dest)->contents, player, buf, player);
}


int
blank(const char *s)
{
	while (*s && isspace(*s))
		s++;

	return !(*s);
}
