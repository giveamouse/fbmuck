/*
 * signal.c -- Curently included into interface.c
 *
 * Seperates the signal handlers and such into a seperate file
 * for maintainability.
 *
 * Broken off from interface.c, and restructured for POSIX
 * compatible systems by Peter A. Torkelson, aka WhiteFire.
 */

#ifdef SOLARIS
#  ifndef _POSIX_SOURCE
#    define _POSIX_SOURCE		/* Solaris needs this */
#  endif
#endif

#include "config.h"
#include "interface.h"
#include "externs.h"
#include "version.h"

#ifndef WIN32
#include <signal.h>
#include <sys/wait.h>

/*
 * SunOS can't include signal.h and sys/signal.h, stupid broken OS.
 */
#if defined(HAVE_SYS_SIGNAL_H) && !defined(SUN_OS)
# include <sys/signal.h>
#endif

#if defined(ULTRIX) || defined(_POSIX_VERSION)
#undef RETSIGTYPE
#define RETSIGTYPE void
#endif

/*
 * Function prototypes
 */
void set_signals(void);
RETSIGTYPE bailout(int);
RETSIGTYPE sig_dump_status(int i);
RETSIGTYPE sig_shutdown(int i);
RETSIGTYPE sig_reap(int i);

#ifdef _POSIX_VERSION
void our_signal(int signo, void (*sighandler) (int));
#else
# define our_signal(s,f) signal((s),(f))
#endif

/*
 * our_signal(signo, sighandler)
 *
 * signo      - Signal #, see defines in signal.h
 * sighandler - The handler function desired.
 *
 * Calls sigaction() to set a signal, if we are posix.
 */
#ifdef _POSIX_VERSION
void
our_signal(int signo, void (*sighandler) (int))
{
	struct sigaction act, oact;

	act.sa_handler = sighandler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	/* Restart long system calls if a signal is caught. */
#ifdef SA_RESTART
	act.sa_flags |= SA_RESTART;
#endif

	/* Make it so */
	sigaction(signo, &act, &oact);
}

#endif							/* _POSIX_VERSION */

/*
 * set_signals()
 * set_sigs_intern(bail)
 *
 * Traps a bunch of signals and reroutes them to various
 * handlers. Mostly bailout.
 *
 * If called from bailout, then reset all to default.
 *
 * Called from main() and bailout()
 */
#define SET_BAIL (bail ? SIG_DFL : bailout)
#define SET_IGN  (bail ? SIG_DFL : SIG_IGN)

static void
set_sigs_intern(int bail)
{
	/* we don't care about SIGPIPE, we notice it in select() and write() */
	our_signal(SIGPIPE, SET_IGN);

	/* didn't manage to lose that control tty, did we? Ignore it anyway. */
	our_signal(SIGHUP, SET_IGN);

	/* resolver's exited. Better clean up the mess our child leaves */
	our_signal(SIGCHLD, bail ? SIG_DFL : sig_reap);

	/* standard termination signals */
	our_signal(SIGINT, SET_BAIL);
	our_signal(SIGTERM, SET_BAIL);

	/* catch these because we might as well */
/*  our_signal(SIGQUIT, SET_BAIL);  */
#ifdef SIGTRAP
	our_signal(SIGTRAP, SET_IGN);
#endif
#ifdef SIGIOT
	our_signal(SIGIOT, SET_BAIL);
#endif
#ifdef SIGEMT
	our_signal(SIGEMT, SET_BAIL);
#endif
#ifdef SIGBUS
	our_signal(SIGBUS, SET_BAIL);
#endif
#ifdef SIGSYS
	our_signal(SIGSYS, SET_BAIL);
#endif
	our_signal(SIGFPE, SET_BAIL);
	our_signal(SIGSEGV, SET_BAIL);
	our_signal(SIGTERM, bail ? SET_BAIL : sig_shutdown);
#ifdef SIGXCPU
	our_signal(SIGXCPU, SET_BAIL);
#endif
#ifdef SIGXFSZ
	our_signal(SIGXFSZ, SET_BAIL);
#endif
#ifdef SIGVTALRM
	our_signal(SIGVTALRM, SET_BAIL);
#endif
	our_signal(SIGUSR2, SET_BAIL);

	/* status dumper (predates "WHO" command) */
	our_signal(SIGUSR1, bail ? SIG_DFL : sig_dump_status);
}

void
set_signals(void)
{
	set_sigs_intern(FALSE);
}

/*
 * Signal handlers
 */

/*
 * BAIL!
 */
RETSIGTYPE bailout(int sig)
{
	char message[1024];

	/* turn off signals */
	set_sigs_intern(TRUE);

	snprintf(message, sizeof(message), "BAILOUT: caught signal %d", sig);

	panic(message);
	_exit(7);

#if !defined(SYSV) && !defined(_POSIX_VERSION) && !defined(ULTRIX)
	return 0;
#endif
}

/*
 * Spew WHO to file
 */
RETSIGTYPE sig_dump_status(int i)
{
	dump_status();
#if !defined(SYSV) && !defined(_POSIX_VERSION) && !defined(ULTRIX)
	return 0;
#endif
}

/*
 * Gracefully shut the server down.
 */
RETSIGTYPE sig_shutdown(int i)
{
	log_status("SHUTDOWN: via SIGNAL\n");
	shutdown_flag = 1;
	restart_flag = 0;
#if !defined(SYSV) && !defined(_POSIX_VERSION) && !defined(ULTRIX)
	return 0;
#endif
}

/*
 * Clean out Zombie Resolver Process.
 */
#if !defined(SYSV) && !defined(_POSIX_VERSION) && !defined(ULTRIX)
#define RETSIGVAL 0
#else
#define RETSIGVAL 
#endif

RETSIGTYPE sig_reap(int i)
{
	/* If DISKBASE is not defined, then there are two types of
	 * children that can die.  First is the nameservice resolver.
	 * Second is the database dumper.  If resolver exits, we should
	 * note it in the log -- at least give the admin the option of
	 * knowing about it, and dealing with it as necessary. */

	/* The fix for SSL connections getting closed when databases were
	 * saved with DISKBASE disabled required closing all sockets 
	 * when the server fork()ed.  This made it impossible for that
	 * process to spit out the "save done" message.  However, because
	 * that process dies as soon as it finishes dumping the database,
	 * can detect that the child died, and broadcast the "save done"
	 * message anyway. */

	int status = 0;
	int reapedpid = 0;

	reapedpid = waitpid(-1, &status, WNOHANG);
	if(!reapedpid)
	{
#ifdef DETACH
		log2file(LOG_ERR_FILE,"SIG_CHILD signal handler called with no pid!");
#else
		fprintf(stderr, "SIG_CHILD signal handler called with no pid!\n");
#endif
	} else {
		if (reapedpid == global_resolver_pid) {
			log_status("resolver exited with status %d\n", status);
#ifndef DISKBASE
		} else if(reapedpid == global_dumper_pid) {
			log_status("forked DB dump task exited with status %d", status);
			global_dumpdone = 1;
#endif
		} else {
			fprintf(stderr, "unknown child process (pid %d) exited with status %d", reapedpid, status);
		}
	}
	return RETSIGVAL;
}



#else /* WIN32 */

#include <wincon.h>
#include <windows.h>
#define VK_C         0x43
#define CONTROL_KEY (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED) 



void sig_reap(int i) {}
void sig_shutdown(int i) {}
void sig_dumpstatus(int i) {}
void set_signals(void) {}
void set_sigs_intern(int bail) {}
void bailout(int sig) {
	char message[1024];
	sprintf(message, "BAILOUT: caught signal %d", sig);
	panic(message);
	_exit(7);
}

void set_console() {
	HANDLE InputHandle;

	InputHandle = GetStdHandle(STD_INPUT_HANDLE);
	if (InputHandle == INVALID_HANDLE_VALUE) {
		printf("GetStdHandle: failed\n");
		_exit(7);
	}
	if (!SetConsoleMode(InputHandle, !ENABLE_PROCESSED_INPUT)) {
		printf("SetConsoleMode: failed\n");
		_exit(7);
	}
	SetConsoleTitle(VERSION);

}

void check_console() {

	HANDLE InputHandle;
	INPUT_RECORD ipRecord;
	DWORD EventsWaiting;
	DWORD EventsRead;
	DWORD EventCount=0;
	BOOL result;
	CHAR c;

	InputHandle = GetStdHandle(STD_INPUT_HANDLE);
	if (InputHandle == INVALID_HANDLE_VALUE) {
		printf("GetStdHandle: Failed\n");
		bailout(2);
	}


	result = GetNumberOfConsoleInputEvents(InputHandle, &EventsWaiting);
	if (result) {
		if (EventsWaiting > 0 ) {
			EventCount = 0;
			while (EventCount < EventsWaiting) {
				result = ReadConsoleInput(InputHandle, &ipRecord, 1, &EventsRead);
				if (result) {
					switch(ipRecord.EventType) {
					case KEY_EVENT:
						c = ipRecord.Event.KeyEvent.uChar.AsciiChar;
						if (3 == c )
							bailout(2);
						break;
					default:
						break;
					}
				} else {
					printf("ReadConsoleInput: failed\n");
					bailout(2);
				}
				EventCount++;
			}
		}
	} else {
		printf("GetNumberOfConsoleInputEvents: failed\n");
		bailout(2);
	}

}


#endif // WIN32

