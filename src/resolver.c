#include "copyright.h"
#include "config.h"

#ifdef SOLARIS
#  ifndef _POSIX_SOURCE
#    define _POSIX_SOURCE  		/* Solaris needs this */
#  endif
#endif

#include <sys/socket.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

#ifdef USE_IPV6
#include <netinet6/in6.h>
#endif

/*
 * SunOS can't include signal.h and sys/signal.h, stupid broken OS.
 */
#if defined(HAVE_SYS_SIGNAL_H) && !defined(SUN_OS)
# include <sys/signal.h>
#endif



/* number of hostnames cached in an LRU queue */
#define HOST_CACHE_SIZE 256

/* Time before retrying to resolve a previously unresolved IP address. */
/* 1800 seconds == 30 minutes */
#define EXPIRE_TIME 1800

/* Time before the resolver gives up on a identd lookup.  Prevents hangs. */
#define IDENTD_TIMEOUT 60


extern int errno;

#ifdef USE_IPV6
const char *addrout( struct in6_addr *, unsigned short, unsigned short );
#else
const char *addrout(long, unsigned short, unsigned short );
#endif

#define MALLOC(result, type, number) do {   \
                                       if (!((result) = (type *) malloc ((number) * sizeof (type)))) \
                                       abort();                             \
                                     } while (0)

#define FREE(x) (free((void *) x))


struct hostcache {
#ifdef USE_IPV6
    struct in6_addr ipnum;
#else
    long ipnum;
#endif
    char name[128];
    time_t time;
    struct hostcache *next;
    struct hostcache **prev;
} *hostcache_list = 0;


#ifdef USE_IPV6
int ipcmp( struct in6_addr *a, struct in6_addr *b )
{
  int i=128;
  char *A = (char*)a; 
  char *B = (char*)b; 
  while (i) { 
    i--;
    if (*A++ != *B++) break;
  }
  return i;
}
#endif

void
#ifdef USE_IPV6
hostdel(struct in6_addr *ip)
#else
hostdel(long ip)
#endif
{
    struct hostcache *ptr;
    for (ptr = hostcache_list; ptr; ptr = ptr->next) {
#ifdef USE_IPV6
        if (!ipcmp ( &(ptr->ipnum), ip)) {
#else
        if (ptr->ipnum == ip) {
#endif
            if (ptr->next) {
                ptr->next->prev = ptr->prev;
	    }
	    *ptr->prev = ptr->next;
	    FREE(ptr);
	    return;
        }
    }
}

const char *
#ifdef USE_IPV6
hostfetch(struct in6_addr *ip)
#else
hostfetch(long ip)
#endif
{
    struct hostcache *ptr;
    for (ptr = hostcache_list; ptr; ptr = ptr->next) {
#ifdef USE_IPV6
        if (!ipcmp ( &(ptr->ipnum), ip)) {
#else
        if (ptr->ipnum == ip) {
#endif
            if (time(NULL) - ptr->time > EXPIRE_TIME) {
                hostdel(ip);
                return NULL;
            }
            if (ptr != hostcache_list) {
                *ptr->prev = ptr->next;
                if (ptr->next) {
                    ptr->next->prev = ptr->prev;
                }
                ptr->next = hostcache_list;
                if (ptr->next) {
                    ptr->next->prev = &ptr->next;
                }
                ptr->prev = &hostcache_list;
                hostcache_list = ptr;
            }
            return (ptr->name);
        }
    }
    return NULL;
}

void
hostprune()
{
    struct hostcache *ptr;
    struct hostcache *tmp;
    int i = HOST_CACHE_SIZE;

    ptr = hostcache_list;
    while (i-- && ptr) {
        ptr = ptr->next;
    }
    if (i < 0 && ptr) {
        *ptr->prev = NULL;
        while (ptr) {
            tmp = ptr->next;
            FREE(ptr);
            ptr = tmp;
        }
    }
}

void
#ifdef USE_IPV6
hostadd(struct in6_addr *ip, const char *name)
#else
hostadd(long ip, const char *name)
#endif
{
    struct hostcache *ptr;

    MALLOC(ptr, struct hostcache, 1);
    ptr->next = hostcache_list;
    if (ptr->next) {
        ptr->next->prev = &ptr->next;
    }
    ptr->prev = &hostcache_list;
    hostcache_list = ptr;
#ifdef USE_IPV6
    memcpy ( & (ptr->ipnum), ip, sizeof (struct in6_addr));
#else
    ptr->ipnum = ip;
#endif
    ptr->time = 0;
    strcpy(ptr->name, name);
    hostprune();
}


void
#ifdef USE_IPV6
hostadd_timestamp(struct in6_addr * ip, const char *name)
#else
hostadd_timestamp(long ip, const char *name)
#endif
{
    hostadd(ip, name);
    hostcache_list->time = time(NULL);
}








void    set_signals(void);

#ifdef _POSIX_VERSION
void our_signal(int signo, void (*sighandler)());
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
void our_signal(int signo, void (*sighandler)())
{
    struct sigaction	act, oact;
    
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
#endif /* _POSIX_VERSION */

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

void set_signals()
{
    /* we don't care about SIGPIPE, we notice it in select() and write() */
    our_signal(SIGPIPE, SIG_IGN);

    /* didn't manage to lose that control tty, did we? Ignore it anyway. */
    our_signal(SIGHUP, SIG_IGN);
}






void 
make_nonblocking(int s)
{
#if !defined(O_NONBLOCK) || defined(ULTRIX)	/* POSIX ME HARDER */
# ifdef FNDELAY 	/* SUN OS */
#  define O_NONBLOCK FNDELAY 
# else
#  ifdef O_NDELAY 	/* SyseVil */
#   define O_NONBLOCK O_NDELAY
#  endif /* O_NDELAY */
# endif /* FNDELAY */
#endif

    if (fcntl(s, F_SETFL, O_NONBLOCK) == -1) {
	perror("make_nonblocking: fcntl");
	abort();
    }
}


const char *
#ifdef USE_IPV6
get_username(struct in6_addr *a, int prt, int myprt)
#else
get_username(long a, int prt, int myprt)
#endif

{
    int fd, len, result;
    char *ptr, *ptr2;
    static char buf[1024];
    int lasterr;
    int timeout = IDENTD_TIMEOUT;

#ifdef USE_IPV6
    struct sockaddr_in6 addr;
    if ((fd = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
#else
    struct sockaddr_in addr;
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
#endif

        perror("resolver ident socket");
        return(0);
    }

    make_nonblocking(fd);

    len = sizeof(addr);
#ifdef USE_IPV6
    addr.sin6_family = AF_INET6;
    memcpy ( &addr.sin6_addr.s6_addr, a, sizeof (struct in6_addr ));
    addr.sin6_port = htons((short)113);
#else
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = a;
    addr.sin_port = htons((short)113);
#endif

    do {
	result = connect(fd, (struct sockaddr *) &addr, len);
	lasterr = errno;
        if (result < 0) {
            if (!timeout--) break;
            sleep(1);
        }
    } while (result < 0 && lasterr == EINPROGRESS);
    if (result < 0 && lasterr != EISCONN) {
        goto bad;
    }

    sprintf(buf, "%d,%d\n", prt, myprt);
    do {
	result = write(fd ,buf, strlen(buf));
	lasterr = errno;
	if (result < 0) {
	    if (!timeout--) break;
	    sleep(1);
	}
    } while (result < 0 && lasterr == EAGAIN);
    if (result < 0) goto bad;

    do {
	result = read(fd, buf, sizeof(buf));
	lasterr = errno;
	if (result < 0) {
	    if (!timeout--) break;
	    sleep(1);
	}
    } while (result < 0 && lasterr == EAGAIN);
    if (result < 0) goto bad;

    ptr = index(buf, ':');
    if (!ptr) goto bad;
    ptr++;
    if (*ptr) ptr++;
    if(strncmp(ptr, "USERID", 6)) goto bad;

    ptr = index(ptr, ':');
    if (!ptr) goto bad;
    ptr = index(ptr + 1, ':');
    if (!ptr) goto bad;
    ptr++;
    /* if (*ptr) ptr++; */

    close(fd);
    if ((ptr2 = index(ptr, '\r'))) *ptr2 = '\0';
    if (!*ptr) return(0);
    return ptr;

bad:
    close(fd);
    return(0);
}


/*  addrout -- Translate address 'a' from int to text.          */
const char *
#ifdef USE_IPV6
addrout(struct in6_addr *a, unsigned short prt, unsigned short myprt)
#else
addrout(long a, unsigned short prt, unsigned short myprt)
#endif
{
    static char buf[128];
    char tmpbuf[128];
    const char *ptr, *ptr2;
    struct hostent *he;

#ifdef USE_IPV6
    struct in6_addr addr;
    memcpy ( &addr.s6_addr, a, sizeof (struct in6_addr));
    ptr = hostfetch(a);
#else
    struct in_addr addr;
    addr.s_addr = a;
    ptr = hostfetch(ntohl(a));
#endif

    if (ptr) {
        ptr2 = get_username(a, prt, myprt);
        if (ptr2) {
	    sprintf(buf, "%s(%s)", ptr, ptr2);
	} else {
	    sprintf(buf, "%s(%d)", ptr, prt);
        }
	return buf;
    }

#ifdef USE_IPV6
    he = gethostbyaddr(((char *)&addr), sizeof(addr), AF_INET6);
#else
    he = gethostbyaddr(((char *)&addr), sizeof(addr), AF_INET);
#endif

    if (he) {
        strcpy(tmpbuf, he->h_name);
#ifdef USE_IPV6
        hostadd(a, tmpbuf);
#else
        hostadd(ntohl(a), tmpbuf);
#endif
        ptr = get_username(a, prt, myprt);
        if (ptr) {
	    sprintf(buf, "%s(%s)", tmpbuf, ptr);
	} else {
	    sprintf(buf, "%s(%d)", tmpbuf, prt);
        }
	return buf;
    }

#ifdef USE_IPV6
    inet_ntop ( AF_INET6, a, tmpbuf, 128 );
    hostadd_timestamp(a, tmpbuf);
    ptr = get_username(a, prt, myprt);
#else

    a = ntohl(a);
    sprintf(tmpbuf, "%d.%d.%d.%d",
            (a >> 24) & 0xff,
            (a >> 16) & 0xff,
            (a >> 8)  & 0xff,
             a        & 0xff
    );
    hostadd_timestamp(a, tmpbuf);
    ptr = get_username(htonl(a), prt, myprt);
#endif

    if (ptr) {
	sprintf(buf, "%s(%s)", tmpbuf, ptr);
    } else {
	sprintf(buf, "%s(%d)", tmpbuf, prt);
    }
    return buf;
}


#define erreturn { \
                     return 0; \
		 }


int 
do_resolve()
{
    int ip1, ip2, ip3, ip4;
    int prt, myprt;
    int doagain;
    char *result;
    const char *ptr;
    char buf[1024];
#ifdef USE_IPV6
    char ipv6addr[128];
    struct in6_addr fullip;
    char *bufptr;
#else
    long fullip;
#endif

    ip1 = ip2 = ip3 = ip4 = prt = myprt = -1;
    do {
        doagain = 0;
        *buf = '\0';
	result = fgets(buf, sizeof(buf), stdin);
	if (!result) {
	    if (errno == EAGAIN) {
		doagain = 1;
		sleep(1);
	    } else {
		if (feof(stdin)) erreturn;
	        perror("fgets");
	        erreturn;
	    }
        }
    } while (doagain || !strcmp(buf,"\n"));

#ifdef USE_IPV6
    bufptr = strchr ( buf, '(' );
    if (!bufptr) erreturn;
    sscanf(bufptr, "(%d)%d", &prt, &myprt);
    *bufptr='\0';
    if (myprt > 65535 || myprt < 0) erreturn;
    if (inet_pton(AF_INET6, buf, &fullip) <= 0) erreturn;
    ptr = addrout(&fullip, prt, myprt);
    printf("%s(%d)|%s\n", buf, prt, ptr);
#else
    sscanf(buf, "%d.%d.%d.%d(%d)%d", &ip1, &ip2, &ip3, &ip4, &prt, &myprt);
    if (ip1 < 0 || ip2 < 0 || ip3 < 0 || ip4 < 0 || prt < 0) erreturn;
    if (ip1>255 || ip2>255 || ip3>255 || ip4>255 || prt>65535) erreturn;
    if (myprt > 65535 || myprt < 0) erreturn;

    fullip = (ip1 << 24) | (ip2 << 16) | (ip3 << 8) | ip4;
    fullip = htonl(fullip);

    ptr = addrout(fullip, prt, myprt);
    printf("%d.%d.%d.%d(%d):%s\n",
            ip1, ip2, ip3, ip4, prt, ptr
    );
#endif
    fflush(stdout);
    return 1;
}




int 
main(int argc, char **argv)
{
    int whatport;
    FILE *ffd;

    if (argc > 1) {
	fprintf(stderr, "Usage: %s\n", *argv);
	exit(1);
	return 1;
    }

    /* remember to ignore certain signals */
    set_signals();

    /* go do it */
    while(do_resolve());
    fprintf(stderr, "Resolver exited.\n");

    exit(0);
    return 0;
}


