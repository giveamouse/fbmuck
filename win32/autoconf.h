/* include/autoconf.h.  Generated automatically by configure.  */
/* include/autoconf.h.in.  Generated automatically from configure.in by autoheader.  */

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define if you don't have vprintf but do have _doprnt.  */
/* #undef HAVE_DOPRNT */

/* Define if the `long double' type works.  */
#define HAVE_LONG_DOUBLE 1

/* Define if your struct tm has tm_zone.  */
/* #undef HAVE_TM_ZONE */

/* Define if you don't have tm_zone but do have the external array
   tzname.  */
#define HAVE_TZNAME 1

/* Define if you have the vprintf function.  */
#define HAVE_VPRINTF 1

/* Define as __inline if that's what the C compiler calls it.  */
/* #undef inline */

/* Define if on MINIX.  */
/* #undef _MINIX */

/* Define to `int' if <sys/types.h> doesn't define.  */
#define pid_t int

/* Define if the system does not provide POSIX.1 features except
   with this defined.  */
/* #undef _POSIX_1_SOURCE */

/* Define if you need to in order for stat and other things to work.  */
/* #undef _POSIX_SOURCE */

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void 
/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/* #undef size_t */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
/* #define TIME_WITH_SYS_TIME 1   WIN32? */

/* Define if your <sys/time.h> declares struct tm.  */
/* #undef TM_IN_SYS_TIME */

/* Define if you have the getrlimit function.  */
#define HAVE_GETRLIMIT 1

/* Define if you have the getrusage function.  */
/* #undef HAVE_GETRUSAGE */

/* Define if you have the mallinfo function.  */
/* #undef HAVE_MALLINFO */

/* Define if you have the random function.  */
/* #define HAVE_RANDOM 1 */

/* Define if you have the snprintf function.  */
#define HAVE_SNPRINTF 1

/* Define if you have the vsnprintf function.  */
#define HAVE_VSNPRINTF 1

/* Define if you have the <dirent.h> header file.  */
/* #define HAVE_DIRENT_H 1 */

/* Define if you have the <errno.h> header file.  */
#define HAVE_ERRNO_H 1

/* Define if you have the <malloc.h> header file.  */
#define HAVE_MALLOC_H 1

/* Define if you have the <memory.h> header file.  */
#define HAVE_MEMORY_H 1

/* Define if you have the <ndir.h> header file.  */
/* #undef HAVE_NDIR_H */

/* Define if you have the <stdarg.h> header file.  */
#define HAVE_STDARG_H 1

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define if you have the <sys/dir.h> header file.  */
/* #undef HAVE_SYS_DIR_H */

/* Define if you have the <sys/errno.h> header file.  */
#define HAVE_SYS_ERRNO_H 1

/* Define if you have the <sys/ndir.h> header file.  */
/* #undef HAVE_SYS_NDIR_H */

/* Define if you have the <sys/resource.h> header file.  */
#define HAVE_SYS_RESOURCE_H 1

/* Define if you have the <sys/signal.h> header file.  */
#define HAVE_SYS_SIGNAL_H 1

/* Define if you have the <sys/time.h> header file.  */
/* #define HAVE_SYS_TIME_H 1  WIN32? */

/* Define if you have the <timebits.h> header file.  */
/* #undef HAVE_TIMEBITS_H */

/* Define if you have the <unistd.h> header file.  */
/* #define HAVE_UNISTD_H 1  WIN32? */

/* Define if you have the <varargs.h> header file.  */
#define HAVE_VARARGS_H 1

/* Define if you have the <openssl/ssl.h> header file.  */
/* #undef HAVE_OPENSSL_SSL_H */

/* Define if you have the <ssl/ssl.h> header file.  */
/* #undef HAVE_SSL_SSL_H */

/* Define if you have the <ssl.h> header file.  */
/* #undef HAVE_SSL_H */

/* Define if you have the m library (-lm).  */
#define HAVE_LIBM 1

/* Define if you have the nsl library (-lnsl).  */
/* #undef HAVE_LIBNSL */

/* Define if you have the socket library (-lsocket).  */
/* #undef HAVE_LIBSOCKET */

/* if tm_gmtoff is defined in sys/time.h define this */
/* #undef HAVE_SYS_TM_GMTOFF */

/* if tm_gmtoff is defiend in time.h define this */
/* #undef HAVE_TM_GMTOFF */

/* if _timezone is defined in time.h, define this */
#define HAVE__TIMEZONE 1

/* uname -a output for certain local programs. */
#define UNAME_VALUE "CYGWIN_ME-4.90 Revar Desmera 1.3.22(0.78/3/2) 2003-03-18 09:20 i686 unknown unknown Cygwin"

/* if your system has an hblks field in it's mallinfo struct in malloc.h */
#define HAVE_MALLINFO_HBLKS 1

/* if your system has a keepcost field in it's mallinfo struct in malloc.h */
#define HAVE_MALLINFO_KEEPCOST 1

/* if your system has a treeoverhead field in it's mallinfo struct in malloc.h */
/* #undef HAVE_MALLINFO_TREEOVERHEAD */

/* if your system has a grain field in it's mallinfo struct in malloc.h */
/* #undef HAVE_MALLINFO_GRAIN */

/* if your system has an 'allocated' field in it's mallinfo struct in malloc.h */
/* #undef HAVE_MALLINFO_ALLOCATED */

/* if you want to use SSL */
/* #undef USE_SSL */

/* if you want to do some memory usage profiling and leak detection. */
/* #undef MALLOC_PROFILING */

/* With MALLOC_PROFILING, can detect double-frees, buffer overruns, etc. */
/* #undef CRT_DEBUG_ALSO */

/* Use IPv6 instead of IPv4 for network connections. */
/* #undef USE_IPV6 */

/* if your system has the netinet6/in6.h header file */
/* #undef HAVE_NETINET6_IN6_H */

/* if your linux system has the linux/in6.h header file */
/* #undef HAVE_LINUX_IN6_H */

/* if your system has the in6.h header file */
/* #undef HAVE_IN6_H */


