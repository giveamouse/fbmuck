
/* $Header$
 * $Log: interface.h,v $
 * Revision 1.8  2002/09/12 22:58:07  sombre
 * Changed function naming convention of recently added code from InterCaps to underline_separated_words at Revar's request. (mutter grumble ;>)
 *
 * Revision 1.7  2002/09/08 23:07:18  sombre
 * Fixed memory leak when toading online players.
 * Fixed remove_prop bug so it will remove props ending in /. (bug #537744)
 * Fixed potential buffer overrun with the CHECKRETURN and ABORT_MPI macros.
 * Fixed @omessage bug where player names would not be prefixed on additional
 *   newlines. (bug #562370)
 * Added IGNORING? ( d1 d2 -- i ) returns true if d1 is ignoring d2.
 * Added IGNORE_ADD ( d1 d2 -- ) adds d2 to d1's ignore list.
 * Added IGNORE_DEL ( d1 d2 -- ) removes d2 from d1's ignore list.
 * Added ARRAY_GET_IGNORELIST ( d -- a ) returns an array of d's ignores.
 * Added support for ignoring (gagging) players, ignores are mutual in that if
 *   player A ignores player B, A will not hear B, and B will not hear A.
 * Added ignore_prop @tune to specify the directory the ignore list is held under,
 *   if set blank ignore support is disabled, defaults to "@ignore/def".
 * Added max_ml4_preempt_count @tune to specify the maximum number of instructions
 *   an mlevel4 (wizbitted) program may run before it is aborted, if set to 0
 *   no limit is imposed.  Defaults to 0.
 * Added reserved_names @tune which when set to a smatch pattern will refuse any
 *   object creations or renames which match said pattern.  Defaults to "".
 * Added reserved_player_names @tune which when set to a smatch pattern will refuse
 *   any player creations or renames which match said pattern.  Defaults to "".
 *
 * Revision 1.6  2002/06/12 04:14:11  revar
 * Added internal MUF primitives for pinning/unpinning arrays.  These are for the
 *   future planned MUV to MUF-bytecode compiler, and are not available from MUF.
 * Added DESCRBUFSIZE ( int:dscr -- int:bytes ) muf prim.  Returns the number of
 *   bytes of free space remaining in the output buffer before it will get
 *   the <output flushed> message.
 * Changed all sprintf()s in the code to snprintf()s.
 *
 * Revision 1.5  2001/01/10 07:35:26  revar
 * Ported a bunch of DESCR* connection muf prims from ProtoMuck.
 * Ported PNAME-OK? and NAME-OK? muf prims from ProtoMuck.
 *
 * Revision 1.4  2000/08/12 06:14:17  revar
 * Changed {ontime} and {idle} to refer to the least idle of a users connections.
 * Changed maximum MUF stacksize to 1024 elements.
 * Optimized almost all MUF connection primitives to be O(1) instead of O(n),
 *   by using lookup tables instead of searching a linked list.
 *
 * Revision 1.3  2000/07/18 18:18:19  winged
 * Various fixes to support warning-free compiling with -Wall -Wstrict-prototypes -Wno-format -- added single-inclusion capability to all headers.
 *
 * Revision 1.2  2000/03/29 12:21:01  revar
 * Reformatted all code into consistent format.
 * 	Tabs are 4 spaces.
 * 	Indents are one tab.
 * 	Braces are generally K&R style.
 * Added ARRAY_DIFF, ARRAY_INTERSECT and ARRAY_UNION to man.txt.
 * Rewrote restart script as a bourne shell script.
 *
 * Revision 1.1.1.1  1999/12/12 07:28:12  revar
 * Initial Sourceforge checkin, fb6.00a29
 *
 * Revision 1.1.1.1  1999/12/12 07:28:12  foxen
 * Initial FB6 CVS checkin.
 *
 * Revision 1.1  1996/06/17 17:29:45  foxen
 * Initial revision
 *
 * Revision 5.12  1994/01/06  03:18:09  foxen
 * Version 5.12
 *
 * Revision 5.1  1993/12/17  00:35:54  foxen
 * initial revision.
 *
 * Revision 1.2  90/07/19  23:14:38  casie
 * Removed log comments from top.
 * 
 * 
 */

#ifndef _INTERFACE_H
#define _INTERFACE_H

#include "copyright.h"

#include "db.h"
#include "mcp.h"

/* these symbols must be defined by the interface */
extern int notify(dbref player, const char *msg);
extern int notify_nolisten(dbref player, const char *msg, int ispriv);
extern int notify_filtered(dbref from, dbref player, const char *msg, int ispriv);
extern void wall_and_flush(const char *msg);
extern void flush_user_output(dbref player);
extern void wall_wizards(const char *msg);
extern int shutdown_flag;		/* if non-zero, interface should shut down */
extern int restart_flag;		/* if non-zero, should restart after shut down */
extern void emergency_shutdown(void);
extern int boot_off(dbref player);
extern void boot_player_off(dbref player);
extern int online(dbref player);
extern int index_descr(int c);
extern int* get_player_descrs(dbref player, int*count);
extern int least_idle_player_descr(dbref who);
extern int most_idle_player_descr(dbref who);
extern int pcount(void);
extern int pdescrcount(void);
extern int pidle(int c);
extern int pdescridle(int c);
extern int pdbref(int c);
extern int pdescrdbref(int c);
extern int pontime(int c);
extern int pdescrontime(int c);
extern char *phost(int c);
extern char *pdescrhost(int c);
extern char *puser(int c);
extern char *pdescruser(int c);
extern int pfirstdescr();
extern int plastdescr();
extern void pboot(int c);
extern int pdescrboot(int c);
extern void pnotify(int c, char *outstr);
extern int pdescrnotify(int c, char *outstr);
extern int dbref_first_descr(dbref c);
extern int pdescr(int c);
extern int pdescrcon(int c);
extern McpFrame *descr_mcpframe(int c);
extern int pnextdescr(int c);
extern int pdescrflush(int c);
extern int pdescrbufsize(int c);
extern dbref partial_pmatch(const char *name);

extern int ignore_is_ignoring(dbref Player, dbref Who);
extern int ignore_prime_cache(dbref Player);
extern void ignore_flush_cache(dbref Player);
extern void ignore_flush_all_cache();
extern void ignore_add_player(dbref Player, dbref Who);
extern void ignore_remove_player(dbref Player, dbref Who);
extern void ignore_remove_from_all_players(dbref Player);

/* the following symbols are provided by game.c */

extern void process_command(int descr, dbref player, char *command);

extern dbref create_player(const char *name, const char *password);
extern dbref connect_player(const char *name, const char *password);
extern void do_look_around(int descr, dbref player);

extern int init_game(const char *infile, const char *outfile);
extern void dump_database(void);
extern void panic(const char *);

#endif /* _INTERFACE_H */
