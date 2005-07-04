/* $Header$ */

#ifndef _PATCHLEVEL_H
#define _PATCHLEVEL_H

# define PATCHLEVEL "1"

#endif /* _PATCHLEVEL_H */

#ifdef DEFINE_HEADER_VERSIONS

#ifndef patchlevelh_version
#define patchlevelh_version
const char *patchlevel_h_version = "$RCSfile$ $Revision: 1.6 $";
#endif
#else
extern const char *patchlevel_h_version;
#endif

