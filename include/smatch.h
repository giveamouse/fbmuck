
/*
#ifndef BSD43
# include <string.h>
#else
# ifdef __GNUC__
extern char *strchr(), *strrchr();
# else
#  include <strings.h>
#  ifndef __STDC__
#   define strchr(x,y) index((x),(y))
#   define strrchr(x,y) rindex((x),(y))
#  endif
# endif
#endif
*/

#ifdef DEFINE_HEADER_VERSIONS


const char *smatch_h_version = "$RCSfile$ $Revision: 1.4 $";

#else
extern const char *smatch_h_version;
#endif

