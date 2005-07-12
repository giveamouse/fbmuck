/* $Header$ */

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

int
notify(int player, const char *msg)
{
	return printf("%s\n", msg);
}


char *
string_dup(const char *s)
{
	char *p;

	p = (char *) malloc(strlen(s) + 1);
	if (!p)
		return p;
	strcpy(p, s);  /* Guaranteed enough space. */
	return p;
}

int
main(int argc, char **argv)
{
	char buf[16384];

	if (argc > 2) {
		fprintf(stderr, "Usage: %s [infile]\n", argv[0]);
		return 0;
	}

	if (argc < 2) {
		infile = stdin;
	} else {
		in_filename = (char *) string_dup(argv[1]);
		if ((infile = fopen(in_filename, "rb")) == NULL) {
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
static const char *olddecomp_c_version = "$RCSfile$ $Revision: 1.8 $";
const char *get_olddecomp_c_version(void) { return olddecomp_c_version; }
