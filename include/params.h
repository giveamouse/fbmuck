/* $Header$
 * $Log: params.h,v $
 * Revision 1.1.1.1  1999/12/12 07:28:13  revar
 * Initial Sourceforge checkin, fb6.00a29
 *
 * Revision 1.1.1.1  1999/12/12 07:28:13  foxen
 * Initial FB6 CVS checkin.
 *
 * Revision 1.1  1996/06/17 17:29:45  foxen
 * Initial revision
 *
 * Revision 5.122  1994/01/15  00:08:39  foxen
 * @doing mods.
 *
 * Revision 5.121  1994/01/06  03:51:18  foxen
 * updated version string.
 *
 * Revision 5.12  1994/01/06  03:16:30  foxen
 * Version 5.12
 *
 * Revision 5.1  1993/12/17  00:35:54  foxen
 * initial revision.
 *
 * Revision 1.6  90/09/18  08:06:32  rearl
 * Moved some stuff in from config.h.
 * 
 * Revision 1.5  90/09/13  06:34:46  rearl
 * PROP_PRIVATE added for unreadable properties.
 * 
 * Revision 1.4  90/08/27  14:09:25  rearl
 * Provision for read-only properties: '_' signals these.
 * 
 * Revision 1.3  90/08/09  21:01:21  rearl
 * Took out some useless stuff, fixed tabs.
 * 
 * Revision 1.2  90/07/19  23:14:44  casie
 * Removed log comments from top.
 * 
 * 
 */

#include "copyright.h"
#include "version.h"

/* penny related stuff */
/* amount of object endowment, based on cost */
#define OBJECT_ENDOWMENT(cost) (((cost)-5)/5)
#define OBJECT_DEPOSIT(endow) ((endow)*5+4)


/* timing stuff */
#define TIME_MINUTE(x)  (60 * (x))                /* 60 seconds */
#define TIME_HOUR(x)    ((x) * (TIME_MINUTE(60))) /* 60 minutes */
#define TIME_DAY(x)     ((x) * (TIME_HOUR(24)))   /* 24 hours   */


#define MAX_OUTPUT 65535        /* maximum amount of queued output */

#define DB_INITIAL_SIZE 100  /* initial malloc() size for the db */


/* User interface low level commands */
#define QUIT_COMMAND "QUIT"
#define WHO_COMMAND "WHO"
#define PREFIX_COMMAND "OUTPUTPREFIX"
#define SUFFIX_COMMAND "OUTPUTSUFFIX"

/* Turn this back on when you want MUD to set from root to some user_id */
/* #define MUD_ID "MUCK" */

/* Used for breaking out of muf READs or for stopping foreground programs. */
#define BREAK_COMMAND "@Q"

#define EXIT_DELIMITER ';'      /* delimiter for lists of exit aliases  */
#define MAX_LINKS 50    /* maximum number of destinations for an exit */



#define PCREATE_FLAGS (BUILDER)   /* default flag bits for created players */

#define GLOBAL_ENVIRONMENT ((dbref) 0)  /* parent of all rooms.  Always #0 */

/* magic cookies (not chocolate chip) :) */

#define ESCAPE_CHAR 27
#define NOT_TOKEN '!'
#define AND_TOKEN '&'
#define OR_TOKEN '|'
#define LOOKUP_TOKEN '*'
#define REGISTERED_TOKEN '$'
#define NUMBER_TOKEN '#'
#define ARG_DELIMITER '='
#define PROP_DELIMITER ':'
#define PROPDIR_DELIMITER '/'
#define PROP_RDONLY '_'
#define PROP_RDONLY2 '%'
#define PROP_PRIVATE '.'
#define PROP_HIDDEN '@'
#define PROP_SEEONLY '~'

/* magic command cookies (oh me, oh my!) */

#define SAY_TOKEN '"'
#define POSE_TOKEN ':'
#define OVERIDE_TOKEN '!'


/* @edit'or stuff */

#define EXIT_INSERT "."         /* character to exit from insert mode    */
#define INSERT_COMMAND 'i'
#define DELETE_COMMAND 'd'
#define QUIT_EDIT_COMMAND   'q'
#define COMPILE_COMMAND 'c'
#define LIST_COMMAND   'l'
#define EDITOR_HELP_COMMAND 'h'
#define KILL_COMMAND 'k'
#define SHOW_COMMAND 's'
#define SHORTSHOW_COMMAND 'a'
#define VIEW_COMMAND 'v'
#define UNASSEMBLE_COMMAND 'u'
#define NUMBER_COMMAND 'n'
#define PUBLICS_COMMAND 'p'

/* maximum number of arguments */
#define MAX_ARG  2

/* Usage comments:
   Line numbers start from 1, so when an argument variable is equal to 0, it
   means that it is non existent.

   I've chosen to put the parameters before the command, because this should
   more or less make the players get used to the idea of forth coding..     */



/* ANSI attributes and color codes */

#define ANSI_RESET	"\033[0m"

#define ANSI_BOLD	"\033[1m"
#define ANSI_DIM      	"\033[2m"
#define ANSI_UNDERLINE	"\033[4m"
#define ANSI_FLASH	"\033[5m"
#define ANSI_REVERSE	"\033[7m"

#define ANSI_FG_BLACK	"\033[30m"
#define ANSI_FG_RED	"\033[31m"
#define ANSI_FG_YELLOW	"\033[33m"
#define ANSI_FG_GREEN	"\033[32m"
#define ANSI_FG_CYAN	"\033[36m"
#define ANSI_FG_BLUE	"\033[34m"
#define ANSI_FG_MAGENTA	"\033[35m"
#define ANSI_FG_WHITE	"\033[37m"

#define ANSI_BG_BLACK	"\033[40m"
#define ANSI_BG_RED	"\033[41m"
#define ANSI_BG_YELLOW	"\033[43m"
#define ANSI_BG_GREEN	"\033[42m"
#define ANSI_BG_CYAN	"\033[46m"
#define ANSI_BG_BLUE	"\033[44m"
#define ANSI_BG_MAGENTA	"\033[45m"
#define ANSI_BG_WHITE	"\033[47m"

