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

#include "db_header.h"

extern const char *old_uncompress(const char *);
extern const char *uncompress(const char *s);
extern void init_compress_from_file(FILE * dicto);
extern char *string_dup(const char *s);

char *in_filename;
FILE *infile;

int
notify(int player, const char *msg)
{
	return printf("%s\n", msg);
}

int
main(int argc, char **argv)
{
	char buf[16384];
	const char *version;
	int db_load_format, dbflags, parmcnt;
	dbref grow;

	/* See where input and output are coming from */
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

	/* read the db header */
	dbflags = db_read_header( infile, &version, &db_load_format, &grow, &parmcnt );

	/* Now recreate a new header */

	/* Put the ***Foxen_ <etc>*** back */
	if( DB_ID_VERSIONSTRING ) {
		puts( version );
	}

	/* Put the grow parameter back */
	if ( dbflags & DB_ID_GROW ) {
		printf( "%d\n", grow );
	}

	/* Put the parms back, and copy the parm lines directly */
	if( dbflags & DB_ID_PARMSINFO ) {
		int i;
		printf( "%d\n", DB_PARMSINFO );
		printf( "%d\n", parmcnt );
		for( i=0; i<parmcnt; ++i ) {
			if( fgets(buf, sizeof(buf), infile) ) {
				buf[sizeof(buf) - 1] = '\0';
				fputs(buf, stdout);
			}
		}
	}

	/* initialize the decompression dictionary */
	if( dbflags & DB_ID_CATCOMPRESS ) {
		init_compress_from_file( infile );
	}

	/* Now handle each line in the rest of the file */
	while (fgets(buf, sizeof(buf), infile)) {
		buf[sizeof(buf) - 1] = '\0';
		if( dbflags & DB_ID_CATCOMPRESS ) {
			fputs(uncompress(buf), stdout);
		} else if ( dbflags & DB_ID_OLDCOMPRESS ) {
			fputs(old_uncompress(buf), stdout);
		} else {
			fputs(buf, stdout);
		}
	}

	exit(0);
	return 0;
}
static const char *olddecomp_c_version = "$RCSfile$ $Revision: 1.9 $";
const char *get_olddecomp_c_version(void) { return olddecomp_c_version; }
