#include <winsock2.h>
#include <process.h>
#include <direct.h>
#pragma warning( disable : 4244 4101 4018 )

extern int gettimeofday(struct timeval *tv, struct timezone *tz);
extern void sync(void);
extern void set_console();
extern void check_console();
#define close(x) closesocket(x)
#define chdir _chdir
#define getpid _getpid
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#define vsnprintf _vsnprintf
#define snprintf _snprintf


