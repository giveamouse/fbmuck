/* $Header$ */

#include "copyright.h"
#include "config.h"

#include <ctype.h>

#include "db.h"
#include "props.h"
#include "params.h"
#include "tune.h"
#include "interface.h"

#include "externs.h"

struct object *db = 0;
dbref db_top = 0;
dbref recyclable = NOTHING;
int db_load_format = 0;

#ifndef DB_INITIAL_SIZE
#define DB_INITIAL_SIZE 10000
#endif							/* DB_INITIAL_SIZE */

#ifdef DB_DOUBLING

dbref db_size = DB_INITIAL_SIZE;

#endif							/* DB_DOUBLING */

struct macrotable *macrotop;

#ifndef MALLOC_PROFILING
extern char *alloc_string(const char *);
#endif

extern short db_conversion_flag;
extern short db_decompression_flag;

int number(const char *s);
int ifloat(const char *s);
void putproperties(FILE * f, int obj);
void getproperties(FILE * f, int obj, const char *pdir);


dbref
getparent(dbref obj)
{
	int limit = 88;

	if (tp_thing_movement) {
		obj = getloc(obj);
	} else {
		do {
			if (Typeof(obj) == TYPE_THING && (FLAGS(obj) & VEHICLE) && limit-- > 0) {
				obj = THING_HOME(obj);
				if (obj != NOTHING && Typeof(obj) == TYPE_PLAYER) {
					obj = PLAYER_HOME(obj);
				}
			} else {
				obj = getloc(obj);
			}
		} while (obj != NOTHING && Typeof(obj) == TYPE_THING);
	}
	return obj;
}


void
free_line(struct line *l)
{
	if (l->this_line)
		free((void *) l->this_line);
	free((void *) l);
}

void
free_prog_text(struct line *l)
{
	struct line *next;

	while (l) {
		next = l->next;
		free_line(l);
		l = next;
	}
}

#ifdef DB_DOUBLING

static void
db_grow(dbref newtop)
{
	struct object *newdb;

	if (newtop > db_top) {
		db_top = newtop;
		if (!db) {
			/* make the initial one */
			db_size = DB_INITIAL_SIZE;
			while (db_top > db_size)
				db_size += 1000;
			if ((db = (struct object *) malloc(db_size * sizeof(struct object))) == 0) {
				abort();
			}
		}
		/* maybe grow it */
		if (db_top > db_size) {
			/* make sure it's big enough */
			while (db_top > db_size)
				db_size += 1000;
			if ((newdb = (struct object *) realloc((void *) db,
												   db_size * sizeof(struct object))) == 0) {
				abort();
			}
			db = newdb;
		}
	}
}

#else							/* DB_DOUBLING */

static void
db_grow(dbref newtop)
{
	struct object *newdb;

	if (newtop > db_top) {
		db_top = newtop;
		if (db) {
			if ((newdb = (struct object *)
				 realloc((void *) db, db_top * sizeof(struct object))) == 0) {
				abort();
			}
			db = newdb;
		} else {
			/* make the initial one */
			int startsize = (newtop >= DB_INITIAL_SIZE) ? newtop : DB_INITIAL_SIZE;

			if ((db = (struct object *)
				 malloc(startsize * sizeof(struct object))) == 0) {
				abort();
			}
		}
	}
}

#endif							/* DB_DOUBLING */

void
db_clear_object(dbref i)
{
	struct object *o = DBFETCH(i);

	bzero(o, sizeof(struct object));

	NAME(i) = 0;
	ts_newobject(o);
	o->location = NOTHING;
	o->contents = NOTHING;
	o->exits = NOTHING;
	o->next = NOTHING;
	o->properties = 0;

#ifdef DISKBASE
	o->propsfpos = 0;
	o->propstime = 0;
	o->propsmode = PROPS_UNLOADED;
	o->nextold = NOTHING;
	o->prevold = NOTHING;
#endif

	/* DBDIRTY(i); */
	/* flags you must initialize yourself */
	/* type-specific fields you must also initialize */
}

dbref
new_object(void)
{
	dbref newobj;

	if (recyclable != NOTHING) {
		newobj = recyclable;
		recyclable = DBFETCH(newobj)->next;
		db_free_object(newobj);
	} else {
		newobj = db_top;
		db_grow(db_top + 1);
	}

	/* clear it out */
	db_clear_object(newobj);
	DBDIRTY(newobj);
	return newobj;
}

void
putref(FILE * f, dbref ref)
{
	if (fprintf(f, "%d\n", ref) < 0) {
		abort();
	}
}

static void
putstring(FILE * f, const char *s)
{
	if (s) {
		if (fputs(s, f) == EOF) {
			abort();
		}
	}
	if (putc('\n', f) == EOF) {
		abort();
	}
}

void
putproperties_rec(FILE * f, const char *dir, dbref obj)
{
	PropPtr pref;
	PropPtr p, pptr;
	char buf[BUFFER_LEN];
	char name[BUFFER_LEN];

	pref = first_prop_nofetch(obj, dir, &pptr, name);
	while (pref) {
		p = pref;
		db_putprop(f, dir, p);
		strcat(strcpy(buf, dir), name);
		if (PropDir(p))
			putproperties_rec(f, strcat(buf, "/"), obj);
		pref = next_prop(pptr, pref, name);
	}
}

/*** CHANGED:
was: void putproperties(FILE *f, PropPtr p)
 is: void putproperties(FILE *f, dbref obj)
***/
void
putproperties(FILE * f, dbref obj)
{
	putstring(f, "*Props*");
	db_dump_props(f, obj);
	/* putproperties_rec(f, "/", obj); */
	putstring(f, "*End*");
}


extern FILE *input_file;
extern FILE *delta_infile;
extern FILE *delta_outfile;

#ifdef DISKBASE

int
fetch_propvals(dbref obj, const char *dir)
{
	PropPtr p, pptr;
	int cnt = 0;
	char buf[BUFFER_LEN];
	char name[BUFFER_LEN];

	p = first_prop_nofetch(obj, dir, &pptr, name);
	while (p) {
		cnt = (cnt || propfetch(obj, p));
		if (PropDir(p) || (PropFlags(p) & PROP_DIRUNLOADED)) {
			strcpy(buf, dir);
			strcat(buf, name);
			strcat(buf, "/");
			if (PropFlags(p) & PROP_DIRUNLOADED) {
				SetPFlags(p, (PropFlags(p) & ~PROP_DIRUNLOADED));
				if (FLAGS(obj) & SAVED_DELTA) {
					getproperties(delta_infile, obj, buf);
				} else {
					getproperties(input_file, obj, buf);
				}
			}
			fetch_propvals(obj, buf);
		}
		p = next_prop(pptr, p, name);
	}
	return cnt;
}


void
putprops_copy(FILE * f, dbref obj)
{
	char buf[BUFFER_LEN * 3];
	char *ptr;
	FILE *g;

	if (DBFETCH(obj)->propsmode != PROPS_UNLOADED) {
		if (fetch_propvals(obj, "/")) {
			fseek(f, 0L, 2);
		}
		putproperties(f, obj);
		return;
	}
	if (db_load_format < 8 || db_conversion_flag) {
		if (fetchprops_priority(obj, 1, NULL) || fetch_propvals(obj, "/")) {
			fseek(f, 0L, 2);
		}
		putproperties(f, obj);
		return;
	}
	if (FLAGS(obj) & SAVED_DELTA) {
		g = delta_infile;
	} else {
		g = input_file;
	}
	putstring(f, "*Props*");
	if (DBFETCH(obj)->propsfpos) {
		fseek(g, DBFETCH(obj)->propsfpos, 0);
		ptr = fgets(buf, sizeof(buf), g);
		if (!ptr)
			abort();
		for (;;) {
			ptr = fgets(buf, sizeof(buf), g);
			if (!ptr)
				abort();
			if (!string_compare(ptr, "*End*\n"))
				break;
			fputs(buf, f);
		}
	}
	putstring(f, "*End*");
}

#endif							/* DISKBASE */


void
macrodump(struct macrotable *node, FILE * f)
{
	if (!node)
		return;
	macrodump(node->left, f);
	putstring(f, node->name);
	putstring(f, node->definition);
	putref(f, node->implementor);
	macrodump(node->right, f);
}

char *
file_line(FILE * f)
{
	char buf[BUFFER_LEN];

	if (!fgets(buf, BUFFER_LEN, f))
		return NULL;
	buf[strlen(buf) - 1] = '\0';
	return alloc_string(buf);
}

void
foldtree(struct macrotable *center)
{
	int count = 0;
	struct macrotable *nextcent = center;

	for (; nextcent; nextcent = nextcent->left)
		count++;
	if (count > 1) {
		for (nextcent = center, count /= 2; count--; nextcent = nextcent->left) ;
		if (center->left)
			center->left->right = NULL;
		center->left = nextcent;
		foldtree(center->left);
	}
	for (count = 0, nextcent = center; nextcent; nextcent = nextcent->right)
		count++;
	if (count > 1) {
		for (nextcent = center, count /= 2; count--; nextcent = nextcent->right) ;
		if (center->right)
			center->right->left = NULL;
		foldtree(center->right);
	}
}

int
macrochain(struct macrotable *lastnode, FILE * f)
{
	char *line, *line2;
	struct macrotable *newmacro;

	if (!(line = file_line(f)))
		return 0;
	line2 = file_line(f);

	newmacro = (struct macrotable *) new_macro(line, line2, getref(f));
	free(line);
	free(line2);

	if (!macrotop)
		macrotop = (struct macrotable *) newmacro;
	else {
		newmacro->left = lastnode;
		lastnode->right = newmacro;
	}
	return (1 + macrochain(newmacro, f));
}

void
macroload(FILE * f)
{
	int count = 0;

	macrotop = NULL;
	count = macrochain(macrotop, f);
	for (count /= 2; count--; macrotop = macrotop->right) ;
	foldtree(macrotop);
	return;
}

void
log_program_text(struct line *first, dbref player, dbref i)
{
	FILE *f;
	char fname[BUFFER_LEN];
	time_t lt = time(NULL);

	strcpy(fname, PROGRAM_LOG);
	f = fopen(fname, "a");
	if (!f) {
		log_status("Couldn't open file %s!\n", fname);
		return;
	}

	fputs("#######################################", f);
	fputs("#######################################\n", f);
	fprintf(f, "PROGRAM %s, SAVED AT %s BY %s(%d)\n",
			unparse_object(player, i), ctime(&lt), NAME(player), player);
	fputs("#######################################", f);
	fputs("#######################################\n\n", f);

	while (first) {
		if (!first->this_line)
			continue;
		fputs(first->this_line, f);
		fputc('\n', f);
		first = first->next;
	}
	fputs("\n\n\n", f);
	fclose(f);
}

void
write_program(struct line *first, dbref i)
{
	FILE *f;
	char fname[BUFFER_LEN];

	sprintf(fname, "muf/%d.m", (int) i);
	f = fopen(fname, "w");
	if (!f) {
		log_status("Couldn't open file %s!\n", fname);
		return;
	}
	while (first) {
		if (!first->this_line)
			continue;
		if (fputs(first->this_line, f) == EOF) {
			abort();
		}
		if (fputc('\n', f) == EOF) {
			abort();
		}
		first = first->next;
	}
	fclose(f);
}

int
db_write_object(FILE * f, dbref i)
{
	struct object *o = DBFETCH(i);
	int j;
	long tmppos;

	putstring(f, NAME(i));
	putref(f, o->location);
	putref(f, o->contents);
	putref(f, o->next);
	putref(f, (FLAGS(i) & ~DUMP_MASK));	/* write non-internal flags */

	putref(f, o->ts.created);
	putref(f, o->ts.lastused);
	putref(f, o->ts.usecount);
	putref(f, o->ts.modified);


#ifdef DISKBASE
	tmppos = ftell(f) + 1;
	putprops_copy(f, i);
	o->propsfpos = tmppos;
	undirtyprops(i);
#else							/* !DISKBASE */
	putproperties(f, i);
#endif							/* DISKBASE */


	switch (Typeof(i)) {
	case TYPE_THING:
		putref(f, THING_HOME(i));
		putref(f, o->exits);
		putref(f, OWNER(i));
		putref(f, THING_VALUE(i));
		break;

	case TYPE_ROOM:
		putref(f, o->sp.room.dropto);
		putref(f, o->exits);
		putref(f, OWNER(i));
		break;

	case TYPE_EXIT:
		putref(f, o->sp.exit.ndest);
		for (j = 0; j < o->sp.exit.ndest; j++) {
			putref(f, (o->sp.exit.dest)[j]);
		}
		putref(f, OWNER(i));
		break;

	case TYPE_PLAYER:
		putref(f, PLAYER_HOME(i));
		putref(f, o->exits);
		putref(f, PLAYER_PENNIES(i));
		putstring(f, PLAYER_PASSWORD(i));
		break;

	case TYPE_PROGRAM:
		putref(f, OWNER(i));
		break;
	}

	return 0;
}

int deltas_count = 0;

#ifndef CLUMP_LOAD_SIZE
#define CLUMP_LOAD_SIZE 20
#endif


/* mode == 1 for dumping all objects.  mode == 0 for deltas only.  */

void
db_write_list(FILE * f, int mode)
{
	dbref i;

	for (i = db_top; i-- > 0;) {
		if (mode == 1 || (FLAGS(i) & OBJECT_CHANGED)) {
			if (fprintf(f, "#%d\n", i) < 0)
				abort();
			db_write_object(f, i);
#ifdef DISKBASE
			if (mode == 1) {
				FLAGS(i) &= ~SAVED_DELTA;	/* clear delta flag */
			} else {
				FLAGS(i) |= SAVED_DELTA;	/* set delta flag */
				deltas_count++;
			}
#endif
			FLAGS(i) &= ~OBJECT_CHANGED;	/* clear changed flag */
		}
	}
}


dbref
db_write(FILE * f)
{
	putstring(f, "***Foxen6 TinyMUCK DUMP Format***");

	putref(f, db_top);
	putref(f, DB_PARMSINFO
#ifdef COMPRESS
		   + (db_decompression_flag ? 0 : DB_COMPRESSED)
#endif
			);
	putref(f, tune_count_parms());
	tune_save_parms_to_file(f);

#ifdef COMPRESS
	if (!db_decompression_flag) {
		save_compress_words_to_file(f);
	}
#endif

	db_write_list(f, 1);

	fseek(f, 0L, 2);
	putstring(f, "***END OF DUMP***");

	fflush(f);
	deltas_count = 0;
	return (db_top);
}



dbref
db_write_deltas(FILE * f)
{
	fseek(f, 0L, 2);			/* seek end of file */
	putstring(f, "***Foxen6 Deltas Dump Extention***");
	db_write_list(f, 0);

	fseek(f, 0L, 2);
	putstring(f, "***END OF DUMP***");
	fflush(f);
	return (db_top);
}



dbref
parse_dbref(const char *s)
{
	const char *p;
	long x;

	x = atol(s);
	if (x > 0) {
		return x;
	} else if (x == 0) {
		/* check for 0 */
		for (p = s; *p; p++) {
			if (*p == '0')
				return 0;
			if (!isspace(*p))
				break;
		}
	}
	/* else x < 0 or s != 0 */
	return NOTHING;
}

static int
do_peek(FILE * f)
{
	int peekch;

	ungetc((peekch = getc(f)), f);

	return (peekch);
}

dbref
getref(FILE * f)
{
	static char buf[BUFFER_LEN];
	int peekch;

	/*
	 * Compiled in with or without timestamps, Sep 1, 1990 by Fuzzy, added to
	 * Muck by Kinomon.  Thanks Kino!
	 */
	if ((peekch = do_peek(f)) == '#' || peekch == '*') {
		return (0);
	}
	fgets(buf, sizeof(buf), f);
	return (atol(buf));
}


static char xyzzybuf[BUFFER_LEN];
static const char *
getstring_noalloc(FILE * f)
{
	char *p;
	char c;

	if (fgets(xyzzybuf, sizeof(xyzzybuf), f) == NULL) {
		xyzzybuf[0] = '\0';
		return xyzzybuf;
	}

	if (strlen(xyzzybuf) == BUFFER_LEN - 1) {
		/* ignore whatever comes after */
		if (xyzzybuf[BUFFER_LEN - 2] != '\n')
			while ((c = fgetc(f)) != '\n') ;
	}
	for (p = xyzzybuf; *p; p++) {
		if (*p == '\n') {
			*p = '\0';
			break;
		}
	}

	return xyzzybuf;
}

#define getstring(x) alloc_string(getstring_noalloc(x))

#ifdef COMPRESS
extern const char *compress(const char *);
extern const char *old_uncompress(const char *);

#define alloc_compressed(x) alloc_string(compress(x))
#else
#define alloc_compressed(x) alloc_string(x)
#endif							/* COMPRESS */

/* returns true for numbers of form [ + | - ] <series of digits> */
int
number(const char *s)
{
	if (!s)
		return 0;
	while (isspace(*s))
		s++;
	if (*s == '+' || *s == '-')
		s++;
	if (!*s) 
		return 0;
	for (; *s; s++)
		if (*s < '0' || *s > '9')
			return 0;
	return 1;
}

/* returns true for floats of form  [+|-]<digits>.<digits>[E[+|-]<digits>] */
int
ifloat(const char *s)
{
	const char *hold;

	if (!s)
		return 0;
	while (isspace(*s))
		s++;
	if (*s == '+' || *s == '-')
		s++;
	hold = s;
	while ((*s) && (*s >= '0' && *s <= '9'))
		s++;
	if ((!*s) || (s == hold))
		return 0;
	if (*s != '.')
		return 0;
	s++;
	hold = s;
	while ((*s) && (*s >= '0' && *s <= '9'))
		s++;
	if (hold == s)
		return 0;
	if (!*s)
		return 1;
	if ((*s != 'e') && (*s != 'E'))
		return 0;
	s++;
	if (*s == '+' || *s == '-')
		s++;
	hold = s;
	while ((*s) && (*s >= '0' && *s <= '9'))
		s++;
	if (s == hold)
		return 0;
	if (*s)
		return 0;
	return 1;
}

/*** CHANGED:
was: PropPtr getproperties(FILE *f)
now: void getproperties(FILE *f, dbref obj, const char *pdir)
***/
void
getproperties(FILE * f, dbref obj, const char *pdir)
{
	char buf[BUFFER_LEN * 3], *p;
	int datalen;

#ifdef DISKBASE
	/* if no props, then don't bother looking. */
	if (!DBFETCH(obj)->propsfpos)
		return;

	/* seek to the proper file position. */
	fseek(f, DBFETCH(obj)->propsfpos, 0);
#endif

	/* get rid of first line */
	fgets(buf, sizeof(buf), f);

	if (strcmp(buf, "Props*\n")) {
		/* initialize first line stuff */
		fgets(buf, sizeof(buf), f);
		while (1) {
			/* fgets reads in \n too! */
			if (!strcmp(buf, "***Property list end ***\n") || !strcmp(buf, "*End*\n"))
				break;
			p = index(buf, PROP_DELIMITER);
			*(p++) = '\0';		/* Purrrrrrrrrr... */
			datalen = strlen(p);
			p[datalen - 1] = '\0';

			if ((p - buf) >= BUFFER_LEN)
				buf[BUFFER_LEN - 1] = '\0';
			if (datalen >= BUFFER_LEN)
				p[BUFFER_LEN - 1] = '\0';

			if ((*p == '^') && (number(p + 1))) {
				add_prop_nofetch(obj, buf, NULL, atol(p + 1));
			} else {
				if (*buf) {
					add_prop_nofetch(obj, buf, p, 0);
				}
			}
			fgets(buf, sizeof(buf), f);
		}
	} else {
		db_getprops(f, obj, pdir);
	}
}

#ifdef DISKBASE
void
skipproperties(FILE * f, dbref obj)
{
	char buf[BUFFER_LEN * 3];
	int islisten = 0;

	/* get rid of first line */
	fgets(buf, sizeof(buf), f);

	fgets(buf, sizeof(buf), f);
	while (strcmp(buf, "***Property list end ***\n") && strcmp(buf, "*End*\n")) {
		if (!islisten) {
			if (string_prefix(buf, "_listen"))
				islisten = 1;
			if (string_prefix(buf, "~listen"))
				islisten = 1;
			if (string_prefix(buf, "~olisten"))
				islisten = 1;
		}
		fgets(buf, sizeof(buf), f);
	}
	if (islisten) {
		FLAGS(obj) |= LISTENER;
	} else {
		FLAGS(obj) &= ~LISTENER;
	}
}

#endif



void
db_free_object(dbref i)
{
	struct object *o;

	o = DBFETCH(i);
	if (NAME(i) && Typeof(i) != TYPE_GARBAGE)
		free((void *) NAME(i));

#ifdef DISKBASE
	unloadprops_with_prejudice(i);
#else
	if (o->properties) {
		delete_proplist(o->properties);
	}
#endif

	if (Typeof(i) == TYPE_EXIT && o->sp.exit.dest) {
		free((void *) o->sp.exit.dest);
    } else if (Typeof(i) == TYPE_PLAYER) {
        if (PLAYER_PASSWORD(i)) {
			free((void*)PLAYER_PASSWORD(i));
        }
        if (PLAYER_DESCRS(i)){ 
			free(PLAYER_DESCRS(i));
			PLAYER_SET_DESCRS(i, NULL);
			PLAYER_SET_DESCRCOUNT(i, 0);
        }
    }
	if (Typeof(i) == TYPE_THING) {
		FREE_THING_SP(i);
	}
	if (Typeof(i) == TYPE_PLAYER) {
		FREE_PLAYER_SP(i);
	}
	if (Typeof(i) == TYPE_PROGRAM) {
		uncompile_program(i);
		FREE_PROGRAM_SP(i);
	}
}

void
db_free(void)
{
	dbref i;

	if (db) {
		for (i = 0; i < db_top; i++)
			db_free_object(i);
		free((void *) db);
		db = 0;
		db_top = 0;
	}
	clear_players();
	clear_primitives();
	recyclable = NOTHING;
}


struct line *
get_new_line(void)
{
	struct line *nu;

	nu = (struct line *) malloc(sizeof(struct line));

	if (!nu) {
		fprintf(stderr, "get_new_line(): Out of memory!\n");
		abort();
	}
	nu->this_line = NULL;
	nu->next = NULL;
	nu->prev = NULL;
	return nu;
}

struct line *
read_program(dbref i)
{
	char buf[BUFFER_LEN];
	struct line *first;
	struct line *prev = NULL;
	struct line *nu;
	FILE *f;
	int len;

	first = NULL;
	sprintf(buf, "muf/%d.m", (int) i);
	f = fopen(buf, "r");
	if (!f)
		return 0;

	while (fgets(buf, BUFFER_LEN, f)) {
		nu = get_new_line();
		len = strlen(buf);
		if (len > 0 && buf[len - 1] == '\n') {
			buf[len - 1] = '\0';
		}
		if (!*buf)
			strcpy(buf, " ");
		nu->this_line = alloc_string(buf);
		if (!first) {
			prev = nu;
			first = nu;
		} else {
			prev->next = nu;
			nu->prev = prev;
			prev = nu;
		}
	}

	fclose(f);
	return first;
}

#ifdef COMPRESS
# define getstring_oldcomp_noalloc(foo) old_uncompress(getstring_noalloc(foo))
#else
# define getstring_oldcomp_noalloc(foo) getstring_noalloc(foo)
#endif

void
db_read_object_old(FILE * f, struct object *o, dbref objno)
{
	dbref exits;
	int pennies;
	const char *password;

	db_clear_object(objno);
	FLAGS(objno) = 0;
	NAME(objno) = getstring(f);
	LOADDESC(objno, getstring_oldcomp_noalloc(f));
	o->location = getref(f);
	o->contents = getref(f);
	exits = getref(f);
	o->next = getref(f);
	LOADLOCK(objno, getboolexp(f));
	LOADFAIL(objno, getstring_oldcomp_noalloc(f));
	LOADSUCC(objno, getstring_oldcomp_noalloc(f));
	LOADOFAIL(objno, getstring_oldcomp_noalloc(f));
	LOADOSUCC(objno, getstring_oldcomp_noalloc(f));
	OWNER(objno) = getref(f);
	pennies = getref(f);

	/* timestamps mods */
	o->ts.created = time(NULL);
	o->ts.lastused = time(NULL);
	o->ts.usecount = 0;
	o->ts.modified = time(NULL);

	FLAGS(objno) |= getref(f);
	/*
	 * flags have to be checked for conflict --- if they happen to coincide
	 * with chown_ok flags and jump_ok flags, we bump them up to the
	 * corresponding HAVEN and ABODE flags
	 */
	if (FLAGS(objno) & CHOWN_OK) {
		FLAGS(objno) &= ~CHOWN_OK;
		FLAGS(objno) |= HAVEN;
	}
	if (FLAGS(objno) & JUMP_OK) {
		FLAGS(objno) &= ~JUMP_OK;
		FLAGS(objno) |= ABODE;
	}
	password = getstring(f);
	/* convert GENDER flag to property */
	switch ((FLAGS(objno) & GENDER_MASK) >> GENDER_SHIFT) {
	case GENDER_NEUTER:
		add_property(objno, "sex", "neuter", 0);
		break;
	case GENDER_FEMALE:
		add_property(objno, "sex", "female", 0);
		break;
	case GENDER_MALE:
		add_property(objno, "sex", "male", 0);
		break;
	default:
		break;
	}
	/* For downward compatibility with databases using the */
	/* obsolete ANTILOCK flag. */
	if (FLAGS(objno) & ANTILOCK) {
		LOADLOCK(objno, negate_boolexp(copy_bool(GETLOCK(objno))))
				FLAGS(objno) &= ~ANTILOCK;
	}
	switch (FLAGS(objno) & TYPE_MASK) {
	case TYPE_THING:
		ALLOC_THING_SP(objno);
		THING_SET_HOME(objno, exits);
		THING_SET_VALUE(objno, pennies);
		o->exits = NOTHING;
		break;
	case TYPE_ROOM:
		o->sp.room.dropto = o->location;
		o->location = NOTHING;
		o->exits = exits;
		break;
	case TYPE_EXIT:
		if (o->location == NOTHING) {
			o->sp.exit.ndest = 0;
			o->sp.exit.dest = NULL;
		} else {
			o->sp.exit.ndest = 1;
			o->sp.exit.dest = (dbref *) malloc(sizeof(dbref));
			(o->sp.exit.dest)[0] = o->location;
		}
		o->location = NOTHING;
		break;
	case TYPE_PLAYER:
		ALLOC_PLAYER_SP(objno);
		PLAYER_SET_HOME(objno, exits);
		o->exits = NOTHING;
		PLAYER_SET_PENNIES(objno, pennies);
		PLAYER_SET_PASSWORD(objno, password);
		PLAYER_SET_CURR_PROG(objno, NOTHING);
		PLAYER_SET_INSERT_MODE(objno, 0);
		PLAYER_SET_DESCRS(objno, NULL);
		PLAYER_SET_DESCRCOUNT(objno, 0);
		break;
	case TYPE_GARBAGE:
		OWNER(objno) = NOTHING;
		o->next = recyclable;
		recyclable = objno;

#ifdef DISKBASE
		dirtyprops(objno);
#endif

		free((void *) NAME(objno));
		NAME(objno) = "<garbage>";
		SETDESC(objno, "<recyclable>");
		break;
	}
}

void
db_read_object_new(FILE * f, struct object *o, dbref objno)
{
	int j;

	db_clear_object(objno);
	FLAGS(objno) = 0;
	NAME(objno) = getstring(f);
	LOADDESC(objno, getstring_noalloc(f));
	o->location = getref(f);
	o->contents = getref(f);
	/* o->exits = getref(f); */
	o->next = getref(f);
	LOADLOCK(objno, getboolexp(f));
	LOADFAIL(objno, getstring_oldcomp_noalloc(f));
	LOADSUCC(objno, getstring_oldcomp_noalloc(f));
	LOADOFAIL(objno, getstring_oldcomp_noalloc(f));
	LOADOSUCC(objno, getstring_oldcomp_noalloc(f));

	/* timestamps mods */
	o->ts.created = time(NULL);
	o->ts.lastused = time(NULL);
	o->ts.usecount = 0;
	o->ts.modified = time(NULL);

	/* OWNER(objno) = getref(f); */
	/* o->pennies = getref(f); */
	FLAGS(objno) |= getref(f);

	/*
	 * flags have to be checked for conflict --- if they happen to coincide
	 * with chown_ok flags and jump_ok flags, we bump them up to the
	 * corresponding HAVEN and ABODE flags
	 */
	if (FLAGS(objno) & CHOWN_OK) {
		FLAGS(objno) &= ~CHOWN_OK;
		FLAGS(objno) |= HAVEN;
	}
	if (FLAGS(objno) & JUMP_OK) {
		FLAGS(objno) &= ~JUMP_OK;
		FLAGS(objno) |= ABODE;
	}
	/* convert GENDER flag to property */
	switch ((FLAGS(objno) & GENDER_MASK) >> GENDER_SHIFT) {
	case GENDER_NEUTER:
		add_property(objno, "sex", "neuter", 0);
		break;
	case GENDER_FEMALE:
		add_property(objno, "sex", "female", 0);
		break;
	case GENDER_MALE:
		add_property(objno, "sex", "male", 0);
		break;
	default:
		break;
	}

	/* o->password = getstring(f); */
	/* For downward compatibility with databases using the */
	/* obsolete ANTILOCK flag. */
	if (FLAGS(objno) & ANTILOCK) {
		LOADLOCK(objno, negate_boolexp(copy_bool(GETLOCK(objno))))
				FLAGS(objno) &= ~ANTILOCK;
	}
	switch (FLAGS(objno) & TYPE_MASK) {
	case TYPE_THING:
		ALLOC_THING_SP(objno);
		THING_SET_HOME(objno, getref(f));
		o->exits = getref(f);
		OWNER(objno) = getref(f);
		THING_SET_VALUE(objno, getref(f));
		break;
	case TYPE_ROOM:
		o->sp.room.dropto = getref(f);
		o->exits = getref(f);
		OWNER(objno) = getref(f);
		break;
	case TYPE_EXIT:
		o->sp.exit.ndest = getref(f);
		o->sp.exit.dest = (dbref *) malloc(sizeof(dbref)
										   * o->sp.exit.ndest);
		for (j = 0; j < o->sp.exit.ndest; j++) {
			(o->sp.exit.dest)[j] = getref(f);
		}
		OWNER(objno) = getref(f);
		break;
	case TYPE_PLAYER:
		ALLOC_PLAYER_SP(objno);
		PLAYER_SET_HOME(objno, getref(f));
		o->exits = getref(f);
		PLAYER_SET_PENNIES(objno, getref(f));
		PLAYER_SET_PASSWORD(objno, getstring(f));
		PLAYER_SET_CURR_PROG(objno, NOTHING);
		PLAYER_SET_INSERT_MODE(objno, 0);
		PLAYER_SET_DESCRS(objno, NULL);
		PLAYER_SET_DESCRCOUNT(objno, 0);
		break;
	}
}

/* Reads in Foxen, Foxen[23456], WhiteFire, Mage or Lachesis DB Formats */
void
db_read_object_foxen(FILE * f, struct object *o, dbref objno, int dtype, int read_before)
{
	int tmp, c, prop_flag = 0;
	int j = 0;

	if (read_before) {
		db_free_object(objno);
	}
	db_clear_object(objno);

	FLAGS(objno) = 0;
	NAME(objno) = getstring(f);
	if (dtype <= 3) {
		LOADDESC(objno, getstring_oldcomp_noalloc(f));
	}
	o->location = getref(f);
	o->contents = getref(f);
	o->next = getref(f);
	if (dtype < 6) {
		LOADLOCK(objno, getboolexp(f));
	}
	if (dtype == 3) {
		/* Mage timestamps */
		o->ts.created = getref(f);
		o->ts.modified = getref(f);
		o->ts.lastused = getref(f);
		o->ts.usecount = 0;
	}
	if (dtype <= 3) {
		/* Lachesis, WhiteFire, and Mage messages */
		LOADFAIL(objno, getstring_oldcomp_noalloc(f));
		LOADSUCC(objno, getstring_oldcomp_noalloc(f));
		LOADDROP(objno, getstring_oldcomp_noalloc(f));
		LOADOFAIL(objno, getstring_oldcomp_noalloc(f));
		LOADOSUCC(objno, getstring_oldcomp_noalloc(f));
		LOADODROP(objno, getstring_oldcomp_noalloc(f));
	}
	tmp = getref(f);			/* flags list */
	if (dtype >= 4)
		tmp &= ~DUMP_MASK;
	FLAGS(objno) |= tmp;

	FLAGS(objno) &= ~SAVED_DELTA;

	if (dtype != 3) {
		/* Foxen and WhiteFire timestamps */
		o->ts.created = getref(f);
		o->ts.lastused = getref(f);
		o->ts.usecount = getref(f);
		o->ts.modified = getref(f);
	}
	c = getc(f);
	if (c == '*') {

#ifdef DISKBASE
		o->propsfpos = ftell(f);
		if (o->propsmode == PROPS_CHANGED) {
			getproperties(f, objno, NULL);
		} else {
			skipproperties(f, objno);
		}
#else
		getproperties(f, objno, NULL);
#endif

		prop_flag++;
	} else {
		/* do our own getref */
		int sign = 0;
		char buf[BUFFER_LEN];
		int i = 0;

		if (c == '-')
			sign = 1;
		else if (c != '+') {
			buf[i] = c;
			i++;
		}
		while ((c = getc(f)) != '\n') {
			buf[i] = c;
			i++;
		}
		buf[i] = '\0';
		j = atol(buf);
		if (sign)
			j = -j;

		/* set gender stuff */
		/* convert GENDER flag to property */
		switch ((FLAGS(objno) & GENDER_MASK) >> GENDER_SHIFT) {
		case GENDER_NEUTER:
			add_property(objno, "sex", "neuter", 0);
			break;
		case GENDER_FEMALE:
			add_property(objno, "sex", "female", 0);
			break;
		case GENDER_MALE:
			add_property(objno, "sex", "male", 0);
			break;
		default:
			break;
		}
	}

	/* o->password = getstring(f); */
	/* For downward compatibility with databases using the */
	/* obsolete ANTILOCK flag. */
	if (FLAGS(objno) & ANTILOCK) {
		LOADLOCK(objno, negate_boolexp(copy_bool(GETLOCK(objno))))
				FLAGS(objno) &= ~ANTILOCK;
	}
	switch (FLAGS(objno) & TYPE_MASK) {
	case TYPE_THING:{
			dbref home;

			ALLOC_THING_SP(objno);
			home = prop_flag ? getref(f) : j;
			THING_SET_HOME(objno, home);
			o->exits = getref(f);
			OWNER(objno) = getref(f);
			THING_SET_VALUE(objno, getref(f));
			break;
		}
	case TYPE_ROOM:
		o->sp.room.dropto = prop_flag ? getref(f) : j;
		o->exits = getref(f);
		OWNER(objno) = getref(f);
		break;
	case TYPE_EXIT:
		o->sp.exit.ndest = prop_flag ? getref(f) : j;
		if (o->sp.exit.ndest)	/* only allocate space for linked exits */
			o->sp.exit.dest = (dbref *) malloc(sizeof(dbref) * (o->sp.exit.ndest));
		for (j = 0; j < o->sp.exit.ndest; j++) {
			(o->sp.exit.dest)[j] = getref(f);
		}
		OWNER(objno) = getref(f);
		break;
	case TYPE_PLAYER:
		ALLOC_PLAYER_SP(objno);
		PLAYER_SET_HOME(objno, (prop_flag ? getref(f) : j));
		o->exits = getref(f);
		PLAYER_SET_PENNIES(objno, getref(f));
		PLAYER_SET_PASSWORD(objno, getstring(f));
		PLAYER_SET_CURR_PROG(objno, NOTHING);
		PLAYER_SET_INSERT_MODE(objno, 0);
		PLAYER_SET_DESCRS(objno, NULL);
		PLAYER_SET_DESCRCOUNT(objno, 0);
		break;
	case TYPE_PROGRAM:
		ALLOC_PROGRAM_SP(objno);
		OWNER(objno) = getref(f);
		FLAGS(objno) &= ~INTERNAL;
		PROGRAM_SET_CURR_LINE(objno, 0);
		PROGRAM_SET_FIRST(objno, 0);
		PROGRAM_SET_CODE(objno, 0);
		PROGRAM_SET_SIZ(objno, 0);
		PROGRAM_SET_START(objno, 0);
		PROGRAM_SET_PUBS(objno, 0);
		PROGRAM_SET_MCPBINDS(objno, 0);
		PROGRAM_SET_PROFTIME(objno, 0, 0);
		PROGRAM_SET_PROFSTART(objno, 0);
		PROGRAM_SET_PROF_USES(objno, 0);

		if (dtype < 8 && (FLAGS(objno) & LINK_OK)) {
			/* set Viewable flag on Link_ok programs. */
			FLAGS(objno) |= VEHICLE;
		}
		if (dtype < 5 && MLevel(objno) == 0)
			SetMLevel(objno, 2);

		break;
	case TYPE_GARBAGE:
		break;
	}
}

void
autostart_progs(void)
{
	dbref i;
	struct object *o;
	struct line *tmp;

	if (db_conversion_flag) {
		return;
	}

	for (i = 0; i < db_top; i++) {
		if (Typeof(i) == TYPE_PROGRAM) {
			if ((FLAGS(i) & ABODE) && TrueWizard(OWNER(i))) {
				/* pre-compile AUTOSTART programs. */
				/* They queue up when they finish compiling. */
				o = DBFETCH(i);
				tmp = PROGRAM_FIRST(i);
				PROGRAM_SET_FIRST(i, (struct line *) read_program(i));
				do_compile(-1, OWNER(i), i, 0);
				free_prog_text(PROGRAM_FIRST(i));
				PROGRAM_SET_FIRST(i, tmp);
			}
		}
	}
}


dbref
db_read(FILE * f)
{
	dbref i, thisref;
	struct object *o;
	const char *special;
	int doing_deltas;
	int main_db_format = 0;
	int parmcnt;
	int dbflags = 0;
	char c;

	db_load_format = 0;
	doing_deltas = 0;

	if ((c = getc(f)) == '*') {
		special = getstring(f);
		if (!strcmp(special, "**TinyMUCK DUMP Format***")) {
			db_load_format = 1;
		} else if (!strcmp(special, "**Lachesis TinyMUCK DUMP Format***") ||
				   !strcmp(special, "**WhiteFire TinyMUCK DUMP Format***")) {
			db_load_format = 2;
		} else if (!strcmp(special, "**Mage TinyMUCK DUMP Format***")) {
			db_load_format = 3;
		} else if (!strcmp(special, "**Foxen TinyMUCK DUMP Format***")) {
			db_load_format = 4;
		} else if (!strcmp(special, "**Foxen2 TinyMUCK DUMP Format***")) {
			db_load_format = 5;
		} else if (!strcmp(special, "**Foxen3 TinyMUCK DUMP Format***")) {
			db_load_format = 6;
		} else if (!strcmp(special, "**Foxen4 TinyMUCK DUMP Format***")) {
			db_load_format = 6;
			i = getref(f);
			db_grow(i);
		} else if (!strcmp(special, "**Foxen5 TinyMUCK DUMP Format***")) {
			db_load_format = 7;
			i = getref(f);
			dbflags = getref(f);
			if (dbflags & DB_PARMSINFO) {
				parmcnt = getref(f);
				tune_load_parms_from_file(f, NOTHING, parmcnt);
			}
			if (dbflags & DB_COMPRESSED) {
#ifdef COMPRESS
				init_compress_from_file(f);
#else
				fprintf(stderr, "This server is not compiled to read compressed databases.\n");
				return -1;
#endif
			}
			db_grow(i);
		} else if (!strcmp(special, "**Foxen6 TinyMUCK DUMP Format***")) {
			db_load_format = 8;
			i = getref(f);
			dbflags = getref(f);
			if (dbflags & DB_PARMSINFO) {
				parmcnt = getref(f);
				tune_load_parms_from_file(f, NOTHING, parmcnt);
			}
			if (dbflags & DB_COMPRESSED) {
#ifdef COMPRESS
				init_compress_from_file(f);
#else
				fprintf(stderr, "This server is not compiled to read compressed databases.\n");
				return -1;
#endif
			}
			db_grow(i);
		} else if (!strcmp(special, "***Foxen Deltas Dump Extention***")) {
			db_load_format = 4;
			doing_deltas = 1;
		} else if (!strcmp(special, "***Foxen2 Deltas Dump Extention***")) {
			db_load_format = 5;
			doing_deltas = 1;
		} else if (!strcmp(special, "***Foxen4 Deltas Dump Extention***")) {
			db_load_format = 6;
			doing_deltas = 1;
		} else if (!strcmp(special, "***Foxen5 Deltas Dump Extention***")) {
			db_load_format = 7;
			doing_deltas = 1;
		} else if (!strcmp(special, "***Foxen6 Deltas Dump Extention***")) {
			db_load_format = 8;
			doing_deltas = 1;
		}
		if (doing_deltas && !db) {
			fprintf(stderr, "Can't read a deltas file without a dbfile.\n");
			return -1;
		}
		free((void *) special);
		if (!doing_deltas)
			main_db_format = db_load_format;
		c = getc(f);			/* get next char */
	}
	for (i = 0;; i++) {
		switch (c) {
		case '#':
			/* another entry, yawn */
			thisref = getref(f);

			if (thisref < db_top) {
				if (doing_deltas && Typeof(thisref) == TYPE_PLAYER) {
					delete_player(thisref);
				}
			}

			/* make space */
			db_grow(thisref + 1);

			/* read it in */
			o = DBFETCH(thisref);
			switch (db_load_format) {
			case 0:
				db_read_object_old(f, o, thisref);
				break;
			case 1:
				db_read_object_new(f, o, thisref);
				break;
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
				db_read_object_foxen(f, o, thisref, db_load_format, doing_deltas);
				break;
			}
			if (Typeof(thisref) == TYPE_PLAYER) {
				OWNER(thisref) = thisref;
				add_player(thisref);
			}
			break;
		case '*':
			special = getstring(f);
			if (strcmp(special, "**END OF DUMP***")) {
				free((void *) special);
				return -1;
			} else {
				free((void *) special);
				special = getstring(f);
				if (special && !strcmp(special, "***Foxen Deltas Dump Extention***")) {
					free((void *) special);
					db_load_format = 4;
					doing_deltas = 1;
				} else if (special && !strcmp(special, "***Foxen2 Deltas Dump Extention***")) {
					free((void *) special);
					db_load_format = 5;
					doing_deltas = 1;
				} else if (special && !strcmp(special, "***Foxen4 Deltas Dump Extention***")) {
					free((void *) special);
					db_load_format = 6;
					doing_deltas = 1;
				} else if (special && !strcmp(special, "***Foxen5 Deltas Dump Extention***")) {
					free((void *) special);
					db_load_format = 7;
					doing_deltas = 1;
				} else if (special && !strcmp(special, "***Foxen6 Deltas Dump Extention***")) {
					free((void *) special);
					db_load_format = 8;
					doing_deltas = 1;
				} else {
					if (special)
						free((void *) special);
					if (main_db_format >= 7 && (dbflags & DB_PARMSINFO)) {
						rewind(f);
						free((void *) getstring(f));
						getref(f);
						getref(f);
						parmcnt = getref(f);
						tune_load_parms_from_file(f, NOTHING, parmcnt);
					}
					for (i = 0; i < db_top; i++) {
						if (Typeof(i) == TYPE_GARBAGE) {
							DBFETCH(i)->next = recyclable;
							recyclable = i;
						}
					}
					autostart_progs();
					return db_top;
				}
			}
			break;
		default:
			return -1;
			/* break; */
		}
		c = getc(f);
	}							/* for */
}								/* db_read */
