/* $Header$ */

/*
 * $Log: rob.c,v $
 * Revision 1.4  2000/07/19 01:33:18  revar
 * Compiling cleanup for -Wall -Wstrict-prototypes -Wno-format.
 * Changed the mcpgui package to use 'const char*'s instead of 'char *'s
 *
 * Revision 1.3  2000/07/07 15:37:30  revar
 * Made rob give pennies to owner of robber puppet.
 * This is more consistent with the way give works for puppets.
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
 * Revision 1.1  1996/06/12 02:58:27  foxen
 * Initial revision
 *
 * Revision 5.14  1994/03/21  11:00:42  foxen
 * Autoconfiguration mods.
 *
 * Revision 5.13  1994/03/14  12:20:58  foxen
 * Fb5.20 release checkpoint.
 *
 * Revision 5.12  1994/01/18  20:52:20  foxen
 * Version 5.15 release.
 *
 * Revision 5.11  1993/12/20  06:22:51  foxen
 * *** empty log message ***
 *
 * Revision 5.1  1993/12/17  00:07:33  foxen
 * initial revision.
 *
 * Revision 1.5  90/09/16  04:42:51  rearl
 * Preparation code added for disk-based MUCK.
 *
 * Revision 1.4  90/08/15  03:10:13  rearl
 * Fiddled with do_kill() and put in pronoun subs for it.
 *
 * Revision 1.3  90/08/05  03:19:55  rearl
 * Redid matching routines.
 *
 * Revision 1.2  90/08/02  02:16:20  rearl
 * Odrop and x killed y! messages now come before y has left.
 * Pronoun substitution added for player odrop.
 *
 * Revision 1.1  90/07/19  23:04:05  casie
 * Initial revision
 *
 *
 */

#include "copyright.h"
#include "config.h"

/* rob and kill */

#include "db.h"
#include "props.h"
#include "params.h"
#include "tune.h"
#include "interface.h"
#include "match.h"
#include "externs.h"

void
do_rob(int descr, dbref player, const char *what)
{
	dbref thing;
	char buf[BUFFER_LEN];
	struct match_data md;

	init_match(descr, player, what, TYPE_PLAYER, &md);
	match_neighbor(&md);
	match_me(&md);
	if (Wizard(OWNER(player))) {
		match_absolute(&md);
		match_player(&md);
	}
	thing = match_result(&md);

	switch (thing) {
	case NOTHING:
		notify(player, "Rob whom?");
		break;
	case AMBIGUOUS:
		notify(player, "I don't know who you mean!");
		break;
	default:
		if (Typeof(thing) != TYPE_PLAYER) {
			notify(player, "Sorry, you can only rob other players.");
		} else if (PLAYER_PENNIES(thing) < 1) {
			sprintf(buf, "%s has no %s.", NAME(thing), tp_pennies);
			notify(player, buf);
			sprintf(buf,
					"%s tried to rob you, but you have no %s to take.",
					NAME(player), tp_pennies);
			notify(thing, buf);
		} else if (can_doit(descr, player, thing, "Your conscience tells you not to.")) {
			/* steal a penny */
			PLAYER_ADD_PENNIES(OWNER(player), 1);
			DBDIRTY(player);
			PLAYER_ADD_PENNIES(thing, -1);
			DBDIRTY(thing);
			notify_fmt(player, "You stole a %s.", tp_penny);
			sprintf(buf, "%s stole one of your %s!", NAME(player), tp_pennies);
			notify(thing, buf);
		}
		break;
	}
}

void
do_kill(int descr, dbref player, const char *what, int cost)
{
	dbref victim;
	char buf[BUFFER_LEN];
	struct match_data md;

	init_match(descr, player, what, TYPE_PLAYER, &md);
	match_neighbor(&md);
	match_me(&md);
	if (Wizard(OWNER(player))) {
		match_player(&md);
		match_absolute(&md);
	}
	victim = match_result(&md);

	switch (victim) {
	case NOTHING:
		notify(player, "I don't see that player here.");
		break;
	case AMBIGUOUS:
		notify(player, "I don't know who you mean!");
		break;
	default:
		if (Typeof(victim) != TYPE_PLAYER) {
			notify(player, "Sorry, you can only kill other players.");
		} else {
			/* go for it */
			/* set cost */
			if (cost < tp_kill_min_cost)
				cost = tp_kill_min_cost;

			if (FLAGS(DBFETCH(player)->location) & HAVEN) {
				notify(player, "You can't kill anyone here!");
				break;
			}

			if (tp_restrict_kill) {
				if (!(FLAGS(player) & KILL_OK)) {
					notify(player, "You have to be set Kill_OK to kill someone.");
					break;
				}
				if (!(FLAGS(victim) & KILL_OK)) {
					notify(player, "They don't want to be killed.");
					break;
				}
			}

			/* see if it works */
			if (!payfor(player, cost)) {
				notify_fmt(player, "You don't have enough %s.", tp_pennies);
			} else if ((RANDOM() % tp_kill_base_cost) < cost && !Wizard(OWNER(victim))) {
				/* you killed him */
				if (GETDROP(victim))
					/* give him the drop message */
					notify(player, GETDROP(victim));
				else {
					sprintf(buf, "You killed %s!", NAME(victim));
					notify(player, buf);
				}

				/* now notify everybody else */
				if (GETODROP(victim)) {
					sprintf(buf, "%s killed %s! ", PNAME(player), PNAME(victim));
					parse_omessage(descr, player, getloc(player), victim,
								   GETODROP(victim), buf, "(@Odrop)");
				} else {
					sprintf(buf, "%s killed %s!", NAME(player), NAME(victim));
				}
				notify_except(DBFETCH(DBFETCH(player)->location)->contents, player, buf,
							  player);

				/* maybe pay off the bonus */
				if (PLAYER_PENNIES(victim) < tp_max_pennies) {
					sprintf(buf, "Your insurance policy pays %d %s.",
							tp_kill_bonus, tp_pennies);
					notify(victim, buf);
					PLAYER_ADD_PENNIES(victim, tp_kill_bonus);
					DBDIRTY(victim);
				} else {
					notify(victim, "Your insurance policy has been revoked.");
				}
				/* send him home */
				send_home(descr, victim, 1);

			} else {
				/* notify player and victim only */
				notify(player, "Your murder attempt failed.");
				sprintf(buf, "%s tried to kill you!", NAME(player));
				notify(victim, buf);
			}
			break;
		}
	}
}

void
do_give(int descr, dbref player, const char *recipient, int amount)
{
	dbref who;
	char buf[BUFFER_LEN];
	struct match_data md;

	/* do amount consistency check */
	if (amount < 0 && !Wizard(OWNER(player))) {
		notify(player, "Try using the \"rob\" command.");
		return;
	} else if (amount == 0) {
		notify_fmt(player, "You must specify a positive number of %s.", tp_pennies);
		return;
	}
	/* check recipient */
	init_match(descr, player, recipient, TYPE_PLAYER, &md);
	match_neighbor(&md);
	match_me(&md);
	if (Wizard(OWNER(player))) {
		match_player(&md);
		match_absolute(&md);
	}
	switch (who = match_result(&md)) {
	case NOTHING:
		notify(player, "Give to whom?");
		return;
	case AMBIGUOUS:
		notify(player, "I don't know who you mean!");
		return;
	default:
		if (!Wizard(OWNER(player))) {
			if (Typeof(who) != TYPE_PLAYER) {
				notify(player, "You can only give to other players.");
				return;
			} else if (PLAYER_PENNIES(who) + amount > tp_max_pennies) {
				notify_fmt(player, "That player doesn't need that many %s!", tp_pennies);
				return;
			}
		}
		break;
	}

	/* try to do the give */
	if (!payfor(player, amount)) {
		notify_fmt(player, "You don't have that many %s to give!", tp_pennies);
	} else {
		/* he can do it */
		switch (Typeof(who)) {
		case TYPE_PLAYER:
			PLAYER_ADD_PENNIES(who, amount);
			sprintf(buf, "You give %d %s to %s.",
					amount, amount == 1 ? tp_penny : tp_pennies, NAME(who));
			notify(player, buf);
			sprintf(buf, "%s gives you %d %s.",
					NAME(player), amount, amount == 1 ? tp_penny : tp_pennies);
			notify(who, buf);
			break;
		case TYPE_THING:
			THING_SET_VALUE(who, (THING_VALUE(who) + amount));
			sprintf(buf, "You change the value of %s to %d %s.",
					NAME(who),
					THING_VALUE(who), THING_VALUE(who) == 1 ? tp_penny : tp_pennies);
			notify(player, buf);
			break;
		default:
			notify_fmt(player, "You can't give %s to that!", tp_pennies);
			break;
		}
		DBDIRTY(who);
	}
}
