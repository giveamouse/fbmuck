/* $Header$ */

/*
 * $Log: wiz.c,v $
 * Revision 1.4  2000/07/14 21:53:04  revar
 * Fixed a bug with @toad sending the inv of the toader home, not the toadee.
 *
 * Revision 1.3  2000/07/07 18:41:04  revar
 * Fixed a db corruption bug with @toading players.
 *
 * Revision 1.2  2000/03/29 12:21:02  revar
 * Reformatted all code into consistent format.
 * 	Tabs are 4 spaces.
 * 	Indents are one tab.
 * 	Braces are generally K&R style.
 * Added ARRAY_DIFF, ARRAY_INTERSECT and ARRAY_UNION to man.txt.
 * Rewrote restart script as a bourne shell script.
 *
 * Revision 1.1.1.1  2000/01/14 22:56:07  revar
 * Initial Sourceforge checkin, fb6.00a29
 *
 * Revision 1.2  2000/01/14 22:53:01  foxen
 * Added Points' SECURE_THING_MOVING @tune support.
 *
 * Revision 1.1.1.1  1999/12/12 07:27:44  foxen
 * Initial FB6 CVS checkin.
 *
 * Revision 1.1  1996/06/12 03:07:36  foxen
 * Initial revision
 *
 * Revision 5.21  1994/03/21  11:00:42  foxen
 * Autoconfiguration mods.
 *
 * Revision 5.20  1994/03/14  12:20:58  foxen
 * Fb5.20 release checkpoint.
 *
 * Revision 5.19  1994/03/14  12:08:46  foxen
 * Initial portability mods and bugfixes.
 *
 * Revision 5.18  1994/02/15  00:28:32  foxen
 * Foxen do_memory to compile with NO_MEMORY_COMMAND defined.
 *
 * Revision 5.17  1994/02/14  03:01:50  foxen
 * Don't include malloc.h if doing MALLOC_PROFILING.
 *
 * Revision 5.16  1994/02/11  05:52:41  foxen
 * Memory cleanup and monitoring code mods.
 *
 * Revision 5.15  1994/02/09  11:11:28  foxen
 * Made fixes to allow compiling with diskbasing turned off.
 *
 * Revision 5.14  1994/02/09  01:44:26  foxen
 * Added in the code for MALLOC_PROFILING a la Cynbe's code.
 *
 * Revision 5.13  1994/01/18  20:52:20  foxen
 * Version 5.15 release.
 *
 * Revision 5.12  1994/01/06  03:15:30  foxen
 * version 5.12
 *
 * Revision 5.11  1993/12/20  06:22:51  foxen
 * *** empty log message ***
 *
 * Revision 5.1  1993/12/17  00:07:33  foxen
 * initial revision.
 *
 * Revision 1.1  91/01/24  00:44:35  cks
 * @pcreate is now always permitted for all wizards, regardless of GOD_PRIV
 * versus no GOD_PRIV.
 *
 * Revision 1.0  91/01/22  22:11:02  cks
 * Initial revision
 *
 * Revision 1.16  90/09/28  12:25:37  rearl
 * Fixed missing newline bug in @newpassword logging.
 *
 * Revision 1.15  90/09/18  08:02:56  rearl
 * Fixed @tel for rooms -- a bug in permissions checking.
 *
 * Revision 1.14  90/09/16  04:43:20  rearl
 * Preparation code added for disk-based MUCK.
 *
 * Revision 1.13  90/09/15  22:28:34  rearl
 * Send inventory of the toad home, not the wizard's!
 *
 * Revision 1.12  90/09/13  06:30:20  rearl
 * @toad modified to chown the victim's items to a recipient player.
 *
 * Revision 1.11  90/09/10  02:19:06  rearl
 * Changed NL line termination to CR/LF pairs.
 *
 * Revision 1.10  90/09/05  02:32:31  rearl
 * Added match_here() for room parent setting.
 *
 * Revision 1.9  90/09/01  06:00:02  rearl
 * Fixed code in @teleport.
 *
 * Revision 1.8  90/08/27  03:35:41  rearl
 * Changed teleport checks...
 *
 * Revision 1.7  90/08/11  04:12:47  rearl
 * *** empty log message ***
 *
 * Revision 1.6  90/08/06  03:49:14  rearl
 * Added logging of @force, @boot, and @toad.
 *
 * Revision 1.5  90/08/05  03:20:16  rearl
 * Redid matching routines.
 *
 * Revision 1.4  90/08/02  22:07:04  rearl
 * Changed one call to a log function, that's it.
 *
 * Revision 1.3  90/07/29  17:46:28  rearl
 * Made @stat command a little cleaner, toaded victims are now
 * toaded first, then all their connections are booted from the game.
 *
 * Revision 1.2  90/07/23  14:48:37  casie
 * *** empty log message ***
 *
 * Revision 1.1  90/07/19  23:04:20  casie
 * Initial revision
 *
 *
 */

#include "copyright.h"
#include "config.h"

/* Wizard-only commands */

#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>

#ifndef MALLOC_PROFILING
#  ifndef HAVE_MALLOC_H
#    include <stdlib.h>
#  else
#    include <malloc.h>
#  endif
#endif

#include "db.h"
#include "props.h"
#include "params.h"
#include "tune.h"
#include "interface.h"
#include "match.h"
#include "externs.h"

void
do_teleport(int descr, dbref player, const char *arg1, const char *arg2)
{
	dbref victim;
	dbref destination;
	const char *to;
	struct match_data md;

	/* get victim, destination */
	if (*arg2 == '\0') {
		victim = player;
		to = arg1;
	} else {
		init_match(descr, player, arg1, NOTYPE, &md);
		match_neighbor(&md);
		match_possession(&md);
		match_me(&md);
		match_here(&md);
		match_absolute(&md);
		match_registered(&md);
		match_player(&md);

		if ((victim = noisy_match_result(&md)) == NOTHING) {
			return;
		}
		to = arg2;
	}

	/* get destination */
	init_match(descr, player, to, TYPE_PLAYER, &md);
	match_possession(&md);
	match_me(&md);
	match_here(&md);
	match_home(&md);
	match_absolute(&md);
	match_registered(&md);
	if (Wizard(OWNER(player))) {
		match_neighbor(&md);
		match_player(&md);
	}
	switch (destination = match_result(&md)) {
	case NOTHING:
		notify(player, "Send it where?");
		break;
	case AMBIGUOUS:
		notify(player, "I don't know which destination you mean!");
		break;
	case HOME:
		switch (Typeof(victim)) {
		case TYPE_PLAYER:
			destination = PLAYER_HOME(victim);
			if (parent_loop_check(victim, destination))
				destination = PLAYER_HOME(OWNER(victim));
			break;
		case TYPE_THING:
			destination = THING_HOME(victim);
			if (parent_loop_check(victim, destination))
				destination = PLAYER_HOME(OWNER(victim));
			break;
		case TYPE_ROOM:
			destination = GLOBAL_ENVIRONMENT;
			break;
		case TYPE_PROGRAM:
			destination = OWNER(victim);
			break;
		default:
			destination = tp_player_start;	/* caught in the next
											   * switch anyway */
			break;
		}
	default:
		switch (Typeof(victim)) {
		case TYPE_PLAYER:
			if (!controls(player, victim) ||
				!controls(player, destination) ||
				!controls(player, getloc(victim)) ||
				(Typeof(destination) == TYPE_THING && !controls(player, getloc(destination)))) {
				notify(player, "Permission denied.");
				break;
			}
			if (Typeof(destination) != TYPE_ROOM && Typeof(destination) != TYPE_THING) {
				notify(player, "Bad destination.");
				break;
			}
			if (!Wizard(victim) &&
				(Typeof(destination) == TYPE_THING && !(FLAGS(destination) & VEHICLE))) {
				notify(player, "Destination object is not a vehicle.");
				break;
			}
			if (parent_loop_check(victim, destination)) {
				notify(player, "Objects can't contain themselves.");
				break;
			}
			notify(victim, "You feel a wrenching sensation...");
			enter_room(descr, victim, destination, DBFETCH(victim)->location);
			notify(player, "Teleported.");
			break;
		case TYPE_THING:
			if (parent_loop_check(victim, destination)) {
				notify(player, "You can't make a container contain itself!");
				break;
			}
		case TYPE_PROGRAM:
			if (Typeof(destination) != TYPE_ROOM
				&& Typeof(destination) != TYPE_PLAYER && Typeof(destination) != TYPE_THING) {
				notify(player, "Bad destination.");
				break;
			}
			if (!((controls(player, destination) ||
				   can_link_to(player, NOTYPE, destination)) &&
				  (controls(player, victim) || controls(player, DBFETCH(victim)->location)))) {
				notify(player, "Permission denied.");
				break;
			}
			/* check for non-sticky dropto */
			if (Typeof(destination) == TYPE_ROOM
				&& DBFETCH(destination)->sp.room.dropto != NOTHING
				&& !(FLAGS(destination) & STICKY))
						destination = DBFETCH(destination)->sp.room.dropto;
			if (tp_thing_movement && (Typeof(victim) == TYPE_THING)) {
				enter_room(descr, victim, destination, DBFETCH(victim)->location);
			} else {
				moveto(victim, destination);
			}
			notify(player, "Teleported.");
			break;
		case TYPE_ROOM:
			if (Typeof(destination) != TYPE_ROOM) {
				notify(player, "Bad destination.");
				break;
			}
			if (!controls(player, victim)
				|| !can_link_to(player, NOTYPE, destination)
				|| victim == GLOBAL_ENVIRONMENT) {
				notify(player, "Permission denied.");
				break;
			}
			if (parent_loop_check(victim, destination)) {
				notify(player, "Parent would create a loop.");
				break;
			}
			moveto(victim, destination);
			notify(player, "Parent set.");
			break;
		case TYPE_GARBAGE:
			notify(player, "That object is in a place where magic cannot reach it.");
			break;
		default:
			notify(player, "You can't teleport that.");
			break;
		}
		break;
	}
	return;
}

void
do_force(int descr, dbref player, const char *what, char *command)
{
	dbref victim, loc;
	struct match_data md;

	if (force_level) {
		notify(player, "Can't @force an @force.");
		return;
	}

	if (!tp_zombies && (!Wizard(player) || Typeof(player) != TYPE_PLAYER)) {
		notify(player, "Only Wizard players may use this command.");
		return;
	}

	/* get victim */
	init_match(descr, player, what, NOTYPE, &md);
	match_neighbor(&md);
	match_possession(&md);
	match_me(&md);
	match_here(&md);
	match_absolute(&md);
	match_registered(&md);
	match_player(&md);

	if ((victim = noisy_match_result(&md)) == NOTHING) {
		return;
	}

	if (Typeof(victim) != TYPE_PLAYER && Typeof(victim) != TYPE_THING) {
		notify(player, "Permission Denied.");
		return;
	}
#ifdef GOD_PRIV
	if (God(victim)) {
		notify(player, "You cannot force god to do anything.");
		return;
	}
#endif							/* GOD_PRIV */

/*    if (!controls(player, victim)) {
 *	notify(player, "Permission denied.");
 *	return;
 *    }
 */

	if (!Wizard(player) && !(FLAGS(victim) & XFORCIBLE)) {
		notify(player, "Permission denied: forced object not @set Xforcible.");
		return;
	}
	if (!Wizard(player) && !test_lock_false_default(descr, player, victim, "@/flk")) {
		notify(player, "Permission denied: Object not force-locked to you.");
		return;
	}

	loc = getloc(victim);
	if (!Wizard(player) && Typeof(victim) == TYPE_THING && loc != NOTHING &&
		(FLAGS(loc) & ZOMBIE) && Typeof(loc) == TYPE_ROOM) {
		notify(player, "Sorry, but that's in a no-puppet zone.");
		return;
	}

	if (!Wizard(OWNER(player)) && Typeof(victim) == TYPE_THING) {
		const char *ptr = NAME(victim);
		char objname[BUFFER_LEN], *ptr2;

		if ((FLAGS(player) & ZOMBIE)) {
			notify(player, "Permission denied.");
			return;
		}
		if (FLAGS(victim) & DARK) {
			notify(player, "Mortals can't force dark puppets!");
			return;
		}
		for (ptr2 = objname; *ptr && !isspace(*ptr);)
			*(ptr2++) = *(ptr++);
		*ptr2 = '\0';
		if (lookup_player(objname) != NOTHING) {
			notify(player, "Puppet cannot share the name of a player.");
			return;
		}
	}

	log_status("FORCED: %s(%d) by %s(%d): %s\n", NAME(victim),
			   victim, NAME(player), player, command);
	/* force victim to do command */
	force_level++;
	process_command(dbref_first_descr(victim), victim, command);
	force_level--;
}

void
do_stats(dbref player, const char *name)
{
	int rooms;
	int exits;
	int things;
	int players;
	int programs;
	int garbage = 0;
	int total;
	int altered = 0;
	int oldobjs = 0;
	int loaded = 0;
	int changed = 0;
	time_t currtime = time(NULL);
	dbref i;
	dbref owner;
	char buf[BUFFER_LEN];

	if (!Wizard(OWNER(player)) && (!name || !*name)) {
		sprintf(buf, "The universe contains %d objects.", db_top);
		notify(player, buf);
	} else {
		total = rooms = exits = things = players = programs = 0;
		if (name != NULL && *name != '\0') {
			owner = lookup_player(name);
			if (owner == NOTHING) {
				notify(player, "I can't find that player.");
				return;
			}
			if (!Wizard(OWNER(player)) && (OWNER(player) != owner)) {
				notify(player, "Permission denied.");
				return;
			}
			for (i = 0; i < db_top; i++) {

#ifdef DISKBASE
				if ((OWNER(i) == owner) && DBFETCH(i)->propsmode != PROPS_UNLOADED)
					loaded++;
				if ((OWNER(i) == owner) && DBFETCH(i)->propsmode == PROPS_CHANGED)
					changed++;
#endif

				/* count objects marked as changed. */
				if ((OWNER(i) == owner) && (FLAGS(i) & OBJECT_CHANGED))
					altered++;

				/* if unused for 90 days, inc oldobj count */
				if ((OWNER(i) == owner) &&
					(currtime - DBFETCH(i)->ts.lastused) > tp_aging_time) oldobjs++;

				switch (Typeof(i)) {
				case TYPE_ROOM:
					if (OWNER(i) == owner)
						total++, rooms++;
					break;

				case TYPE_EXIT:
					if (OWNER(i) == owner)
						total++, exits++;
					break;

				case TYPE_THING:
					if (OWNER(i) == owner)
						total++, things++;
					break;

				case TYPE_PLAYER:
					if (i == owner)
						total++, players++;
					break;

				case TYPE_PROGRAM:
					if (OWNER(i) == owner)
						total++, programs++;
					break;

				}
			}
		} else {
			for (i = 0; i < db_top; i++) {

#ifdef DISKBASE
				if (DBFETCH(i)->propsmode != PROPS_UNLOADED)
					loaded++;
				if (DBFETCH(i)->propsmode == PROPS_CHANGED)
					changed++;
#endif

				/* count objects marked as changed. */
				if (FLAGS(i) & OBJECT_CHANGED)
					altered++;

				/* if unused for 90 days, inc oldobj count */
				if ((currtime - DBFETCH(i)->ts.lastused) > tp_aging_time)
					oldobjs++;

				switch (Typeof(i)) {
				case TYPE_ROOM:
					total++;
					rooms++;
					break;
				case TYPE_EXIT:
					total++;
					exits++;
					break;
				case TYPE_THING:
					total++;
					things++;
					break;
				case TYPE_PLAYER:
					total++;
					players++;
					break;
				case TYPE_PROGRAM:
					total++;
					programs++;
					break;
				case TYPE_GARBAGE:
					total++;
					garbage++;
					break;
				}
			}
		}

		notify_fmt(player, "%7d room%s        %7d exit%s        %7d thing%s",
				   rooms, (rooms == 1) ? " " : "s",
				   exits, (exits == 1) ? " " : "s", things, (things == 1) ? " " : "s");

		notify_fmt(player, "%7d program%s     %7d player%s      %7d garbage",
				   programs, (programs == 1) ? " " : "s",
				   players, (players == 1) ? " " : "s", garbage);

		notify_fmt(player,
				   "%7d total object%s                     %7d old & unused",
				   total, (total == 1) ? " " : "s", oldobjs);

#ifdef DISKBASE
		if (Wizard(OWNER(player))) {
			notify_fmt(player,
					   "%7d proploaded object%s                %7d propchanged object%s",
					   loaded, (loaded == 1) ? " " : "s", changed, (changed == 1) ? "" : "s");

		}
#endif

#ifdef DELTADUMPS
		{
			char buf[BUFFER_LEN];
			struct tm *time_tm;
			time_t lasttime = (time_t) get_property_value(0, "_sys/lastdumptime");

			time_tm = localtime(&lasttime);
			(void) format_time(buf, 40, "%a %b %e %T %Z", time_tm);
			notify_fmt(player, "%7d unsaved object%s     Last dump: %s",
					   altered, (altered == 1) ? "" : "s", buf);
		}
#endif

	}
}


void
do_boot(dbref player, const char *name)
{
	dbref victim;
	char buf[BUFFER_LEN];

	if (!Wizard(player) || Typeof(player) != TYPE_PLAYER) {
		notify(player, "Only a Wizard player can boot someone off.");
		return;
	}
	if ((victim = lookup_player(name)) == NOTHING) {
		notify(player, "That player does not exist.");
		return;
	}
	if (Typeof(victim) != TYPE_PLAYER) {
		notify(player, "You can only boot players!");
	}
#ifdef GOD_PRIV
	else if (God(victim)) {
		notify(player, "You can't boot God!");
	}
#endif							/* GOD_PRIV */

	else {
		notify(victim, "You have been booted off the game.");
		if (boot_off(victim)) {
			log_status("BOOTED: %s(%d) by %s(%d)\n", NAME(victim),
					   victim, NAME(player), player);
			if (player != victim) {
				sprintf(buf, "You booted %s off!", PNAME(victim));
				notify(player, buf);
			}
		} else {
			sprintf(buf, "%s is not connected.", PNAME(victim));
			notify(player, buf);
		}
	}
}

void
do_toad(int descr, dbref player, const char *name, const char *recip)
{
	dbref victim;
	dbref recipient;
	dbref stuff;
	char buf[BUFFER_LEN];

	if (!Wizard(player) || Typeof(player) != TYPE_PLAYER) {
		notify(player, "Only a Wizard player can turn a person into a toad.");
		return;
	}
	if ((victim = lookup_player(name)) == NOTHING) {
		notify(player, "That player does not exist.");
		return;
	}
	if (!*recip) {
		recipient = GOD;
	} else {
		if ((recipient = lookup_player(recip)) == NOTHING || recipient == victim) {
			notify(player, "That recipient does not exist.");
			return;
		}
	}

	if (Typeof(victim) != TYPE_PLAYER) {
		notify(player, "You can only turn players into toads!");
	} else if (TrueWizard(victim)) {
		notify(player, "You can't turn a Wizard into a toad.");
	} else {
		/* we're ok */
		/* do it */
		send_contents(descr, victim, HOME);
		for (stuff = 0; stuff < db_top; stuff++) {
			if (OWNER(stuff) == victim) {
				switch (Typeof(stuff)) {
				case TYPE_PROGRAM:
					dequeue_prog(stuff, 0);	/* dequeue player's progs */
					if (TrueWizard(recipient)) {
						FLAGS(stuff) &= ~(ABODE | WIZARD);
						SetMLevel(stuff, 1);
					}
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
		dequeue_prog(victim, 0);	/* dequeue progs that player's running */

		FLAGS(victim) = (FLAGS(victim) & ~TYPE_MASK) | TYPE_THING;
		OWNER(victim) = player;	/* you get it */
		THING_SET_VALUE(victim, 1);	/* don't let him keep his
									   * immense wealth */

		/* notify people */
		notify(victim, "You have been turned into a toad.");
		sprintf(buf, "You turned %s into a toad!", PNAME(victim));
		notify(player, buf);
		log_status("TOADED: %s(%d) by %s(%d)\n", NAME(victim), victim, NAME(player), player);

		delete_player(victim);
		FREE_PLAYER_SP(victim);
		ALLOC_THING_SP(victim);
		THING_SET_HOME(victim, PLAYER_HOME(player));

		/* reset name */
		sprintf(buf, "A slimy toad named %s", unmangle(victim, PNAME(victim)));
		free((void *) NAME(victim));
		NAME(victim) = alloc_string(buf);
		DBDIRTY(victim);
		boot_player_off(victim);
	}
}

void
do_newpassword(dbref player, const char *name, const char *password)
{
	dbref victim;
	char buf[BUFFER_LEN];

	if (!Wizard(player) || Typeof(player) != TYPE_PLAYER) {
		notify(player, "Only a Wizard player can newpassword someone.");
		return;
	} else if ((victim = lookup_player(name)) == NOTHING) {
		notify(player, "No such player.");
	} else if (*password != '\0' && !ok_password(password)) {
		/* Wiz can set null passwords, but not bad passwords */
		notify(player, "Bad password");

#ifdef GOD_PRIV
	} else if (God(victim)) {
		notify(player, "You can't change God's password!");
		return;
	} else {
		if (TrueWizard(victim) && !God(player)) {
			notify(player, "Only God can change a wizard's password.");
			return;
		}
#else							/* GOD_PRIV */
	} else {
#endif							/* GOD_PRIV */

		/* it's ok, do it */
		if (PLAYER_PASSWORD(victim))
			free((void *) PLAYER_PASSWORD(victim));
		PLAYER_SET_PASSWORD(victim, alloc_string(password));
		DBDIRTY(victim);
		notify(player, "Password changed.");
		sprintf(buf, "Your password has been changed by %s.", NAME(player));
		notify(victim, buf);
		log_status("NEWPASS'ED: %s(%d) by %s(%d)\n", NAME(victim), victim,
				   NAME(player), player);
	}
}

void
do_pcreate(dbref player, const char *user, const char *password)
{
	dbref newguy;
	char buf[BUFFER_LEN];

	if (!Wizard(player) || Typeof(player) != TYPE_PLAYER) {
		notify(player, "Only a Wizard player can create a player.");
		return;
	}
	newguy = create_player(user, password);
	if (newguy == NOTHING) {
		notify(player, "Create failed.");
	} else {
		log_status("PCREATED %s(%d) by %s(%d)\n", NAME(newguy), newguy, NAME(player), player);
		sprintf(buf, "Player %s created as object #%d.", user, newguy);
		notify(player, buf);
	}
}



#ifdef DISKBASE
extern long propcache_hits;
extern long propcache_misses;
#endif

void
do_serverdebug(int descr, dbref player, const char *arg1, const char *arg2)
{
	if (!Wizard(OWNER(player))) {
		notify(player, "Permission denied.");
		return;
	}
#ifdef DISKBASE
	if (!*arg1 || string_prefix(arg1, "cache")) {
		notify(player, "Cache info:");
		diskbase_debug(player);
	}
#endif
	if (string_prefix(arg1, "guitest")) {
		do_post_dlog(descr, arg2);
	}

	notify(player, "Done.");
}


#ifndef NO_USAGE_COMMAND
long max_open_files(void);		/* from interface.c */

void
do_usage(dbref player)
{
	int pid, psize;

#ifdef HAVE_GETRUSAGE
	struct rusage usage;
#endif

	if (!Wizard(OWNER(player))) {
		notify(player, "Permission denied.");
		return;
	}
	pid = getpid();
#ifdef HAVE_GETRUSAGE
	psize = getpagesize();
	getrusage(RUSAGE_SELF, &usage);
#endif

	notify_fmt(player, "Compiled on: %s", UNAME_VALUE);
	notify_fmt(player, "Process ID: %d", pid);
	notify_fmt(player, "Max descriptors/process: %ld", max_open_files());
#ifdef HAVE_GETRUSAGE
	notify_fmt(player, "Performed %d input servicings.", usage.ru_inblock);
	notify_fmt(player, "Performed %d output servicings.", usage.ru_oublock);
	notify_fmt(player, "Sent %d messages over a socket.", usage.ru_msgsnd);
	notify_fmt(player, "Received %d messages over a socket.", usage.ru_msgrcv);
	notify_fmt(player, "Received %d signals.", usage.ru_nsignals);
	notify_fmt(player, "Page faults NOT requiring physical I/O: %d", usage.ru_minflt);
	notify_fmt(player, "Page faults REQUIRING physical I/O: %d", usage.ru_majflt);
	notify_fmt(player, "Swapped out of main memory %d times.", usage.ru_nswap);
	notify_fmt(player, "Voluntarily context switched %d times.", usage.ru_nvcsw);
	notify_fmt(player, "Involuntarily context switched %d times.", usage.ru_nivcsw);
	notify_fmt(player, "User time used: %d sec.", usage.ru_utime.tv_sec);
	notify_fmt(player, "System time used: %d sec.", usage.ru_stime.tv_sec);
	notify_fmt(player, "Pagesize for this machine: %d", psize);
	notify_fmt(player, "Maximum resident memory: %ldk",
			   (long) (usage.ru_maxrss * (psize / 1024)));
	notify_fmt(player, "Integral resident memory: %ldk",
			   (long) (usage.ru_idrss * (psize / 1024)));
#endif							/* HAVE_GETRUSAGE */
}

#endif							/* NO_USAGE_COMMAND */



void
do_memory(dbref who)
{
	if (!Wizard(OWNER(who))) {
		notify(who, "Permission denied.");
		return;
	}
#ifndef NO_MEMORY_COMMAND
# ifdef HAVE_MALLINFO
	{
		struct mallinfo mi;

		mi = mallinfo();
		notify_fmt(who, "Total memory arena size: %6dk", (mi.arena / 1024));
		notify_fmt(who, "Small block mem alloced: %6dk", (mi.usmblks / 1024));
		notify_fmt(who, "Small block memory free: %6dk", (mi.fsmblks / 1024));
		notify_fmt(who, "Small block mem overhead:%6dk", (mi.hblkhd / 1024));

		notify_fmt(who, "Memory allocated:        %6dk", (mi.uordblks / 1024));
		notify_fmt(who, "Mem allocated overhead:  %6dk",
				   ((mi.uordbytes - mi.uordblks) / 1024));
		notify_fmt(who, "Memory free:             %6dk", (mi.fordblks / 1024));
		notify_fmt(who, "Memory free overhead:    %6dk", (mi.treeoverhead / 1024));

		notify_fmt(who, "Small block grain: %6d", mi.grain);
		notify_fmt(who, "Small block count: %6d", mi.smblks);
		notify_fmt(who, "Memory chunks:     %6d", mi.ordblks);
		notify_fmt(who, "Mem chunks alloced:%6d", mi.allocated);
	}
# endif							/* HAVE_MALLINFO */
#endif							/* NO_MEMORY_COMMAND */

#ifdef MALLOC_PROFILING
	notify(who, "  ");
	CrT_summarize(who);
	CrT_summarize_to_file("malloc_log", "Manual Checkpoint");
#endif

	notify(who, "Done.");
}
