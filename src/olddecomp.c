/* $Header$ */

/*
 * $Log: olddecomp.c,v $
 * Revision 1.3  2000/07/07 01:27:33  revar
 * 64 bit cleanups.
 * Introduced microscopic memory leaks in p_mcp.c and documented them in
 *   comments marked with WORK.  The leaks are on the order of a few bytes
 *   when someone re-registers what program handles a given MCP package.
 *   Since packages generally are only registered once, this is ignorable.
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
 * Revision 1.1  1996/06/12 02:44:15  foxen
 * Initial revision
 *
 * Revision 5.6  1994/03/21  11:00:42  foxen
 * Autoconfiguration mods.
 *
 * Revision 5.5  1994/03/14  12:20:58  foxen
 * Fb5.20 release checkpoint.
 *
 * Revision 5.4  1994/03/14  12:08:46  foxen
 * Initial portability mods and bugfixes.
 *
 * Revision 5.3  1994/02/13  13:53:22  foxen
 * fixed to compile despite CrT_malloc.
 *
 * Revision 5.2  1994/01/18  20:52:20  foxen
 * Version 5.15 release.
 *
 * Revision 5.1  1993/12/17  00:07:33  foxen
 * initial revision.
 *
 * Revision 1.1  90/07/19  23:03:27  casie
 * Initial revision
 *
 *
 */

#include "copyright.h"
#include "config.h"

#include <stdio.h>

#undef malloc
#undef calloc
#undef realloc
#undef free
#undef alloc_string
#undef string_dup

extern const char *old_uncompress(const char *);

char *in_filename;
FILE *infile;

char *
string_dup(const char *s)
{
	char *p;

	p = (char *) malloc(strlen(s) + 1);
	if (!p)
		return p;
	strcpy(p, s);
	return p;
}

int
main(int argc, char **argv)
{
	char buf[16384];
	int i;

	if (argc > 2) {
		fprintf(stderr, "Usage: %s [infile]\n", argv[0]);
		return 0;
	}

	if (argc < 2) {
		infile = stdin;
	} else {
		in_filename = (char *) string_dup(argv[1]);
		if ((infile = fopen(in_filename, "r")) == NULL) {
			fprintf(stderr, "%s: unable to open input file.\n", argv[0]);
			return 0;
		}
	}

	while (fgets(buf, sizeof(buf), infile)) {
		buf[sizeof(buf) - 1] = '\0';
		fputs(old_uncompress(buf), stdout);
	}
	exit(0);
	return 0;
}
