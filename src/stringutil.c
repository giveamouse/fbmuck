/* $Header$ */


#include "copyright.h"
#include "config.h"
#include "db.h"
#include "tune.h"
#include "props.h"
#include "params.h"
#include "interface.h"
#include "mpi.h"

/* String utilities */

#include <stdio.h>
#include <ctype.h>
#include "externs.h"

#define DOWNCASE(x) (tolower(x))

#ifdef COMPRESS
extern const char *uncompress(const char *);

#endif							/* COMPRESS */

/*
 * routine to be used instead of strcasecmp() in a sorting routine
 * Sorts alphabetically or numerically as appropriate.
 * This would compare "network100" as greater than "network23"
 * Will compare "network007" as less than "network07"
 * Will compare "network23a" as less than "network23b"
 * Takes same params and returns same comparitive values as strcasecmp.
 * This ignores minus signs and is case insensitive.
 */
int
alphanum_compare(const char *t1, const char *s2)
{
	int n1, n2, cnt1, cnt2;
	const char *u1, *u2;
	register const char *s1 = t1;

	while (*s1 && DOWNCASE(*s1) == DOWNCASE(*s2))
		s1++, s2++;

	/* if at a digit, compare number values instead of letters. */
	if (isdigit(*s1) && isdigit(*s2)) {
		u1 = s1;
		u2 = s2;
		n1 = n2 = 0;			/* clear number values */
		cnt1 = cnt2 = 0;

		/* back up before zeros */
		if (s1 > t1 && *s2 == '0')
			s1--, s2--;			/* curr chars are diff */
		while (s1 > t1 && *s1 == '0')
			s1--, s2--;			/* prev chars are same */
		if (!isdigit(*s1))
			s1++, s2++;

		/* calculate number values */
		while (isdigit(*s1))
			cnt1++, n1 = (n1 * 10) + (*s1++ - '0');
		while (isdigit(*s2))
			cnt2++, n2 = (n2 * 10) + (*s2++ - '0');

		/* if more digits than int can handle... */
		if (cnt1 > 8 || cnt2 > 8) {
			if (cnt1 == cnt2)
				return (*u1 - *u2);	/* cmp chars if mag same */
			return (cnt1 - cnt2);	/* compare magnitudes */
		}

		/* if number not same, return count difference */
		if (n1 && n2 && n1 != n2)
			return (n1 - n2);

		/* else, return difference of characters */
		return (*u1 - *u2);
	}
	/* if characters not digits, and not the same, return difference */
	return (DOWNCASE(*s1) - DOWNCASE(*s2));
}

int
string_compare(register const char *s1, register const char *s2)
{
	while (*s1 && DOWNCASE(*s1) == DOWNCASE(*s2))
		s1++, s2++;

	return (DOWNCASE(*s1) - DOWNCASE(*s2));
}

const char *
exit_prefix(register const char *string, register const char *prefix)
{
	const char *p;
	const char *s = string;

	while (*s) {
		p = prefix;
		string = s;
		while (*s && *p && DOWNCASE(*s) == DOWNCASE(*p)) {
			s++;
			p++;
		}
		while (*s && isspace(*s))
			s++;
		if (!*p && (!*s || *s == EXIT_DELIMITER)) {
			return string;
		}
		while (*s && (*s != EXIT_DELIMITER))
			s++;
		if (*s)
			s++;
		while (*s && isspace(*s))
			s++;
	}
	return 0;
}

int
string_prefix(register const char *string, register const char *prefix)
{
	while (*string && *prefix && DOWNCASE(*string) == DOWNCASE(*prefix))
		string++, prefix++;
	return *prefix == '\0';
}

/* accepts only nonempty matches starting at the beginning of a word */
const char *
string_match(register const char *src, register const char *sub)
{
	if (*sub != '\0') {
		while (*src) {
			if (string_prefix(src, sub))
				return src;
			/* else scan to beginning of next word */
			while (*src && isalnum(*src))
				src++;
			while (*src && !isalnum(*src))
				src++;
		}
	}
	return 0;
}

/*
 * pronoun_substitute()
 *
 * %-type substitutions for pronouns
 *
 * %a/%A for absolute possessive (his/hers/hirs/its, His/Hers/Hirs/Its)
 * %s/%S for subjective pronouns (he/she/sie/it, He/She/Sie/It)
 * %o/%O for objective pronouns (him/her/hir/it, Him/Her/Hir/It)
 * %p/%P for possessive pronouns (his/her/hir/its, His/Her/Hir/Its)
 * %r/%R for reflexive pronouns (himself/herself/hirself/itself,
 *                                Himself/Herself/Hirself/Itself)
 * %n    for the player's name.
 */
char *
pronoun_substitute(int descr, dbref player, const char *str)
{
	char c;
	char d;
	char prn[3];
	char globprop[128];
	static char buf[BUFFER_LEN * 2];
	char orig[BUFFER_LEN];
	char sexbuf[BUFFER_LEN];
	char *result;
	const char *sexstr;
	const char *self_sub;		/* self substitution code */
	const char *temp_sub;
	dbref mywhere = player;
	int sex;

	static const char *subjective[5] = { "", "it", "she", "he", "sie" };
	static const char *possessive[5] = { "", "its", "her", "his", "hir" };
	static const char *objective[5] = { "", "it", "her", "him", "hir" };
	static const char *reflexive[5] = { "", "itself", "herself", "himself", "hirself" };
	static const char *absolute[5] = { "", "its", "hers", "his", "hirs" };

	prn[0] = '%';
	prn[2] = '\0';

#ifdef COMPRESS
	str = uncompress(str);
#endif							/* COMPRESS */

	strcpy(orig, str);
	str = orig;

	sex = genderof(descr, player);
	sexstr = get_property_class(player, "sex");
	if (sexstr) {
		sexstr = do_parse_mesg(descr, player, player, sexstr, "(Lock)", sexbuf,
						(MPI_ISPRIVATE | MPI_ISLOCK |
							(Prop_Blessed(player, "sex")? MPI_ISBLESSED : 0)));
	}
	while (sexstr && isspace(*sexstr)) sexstr++;
	if (!sexstr || !*sexstr) {
		sexstr = "_default";
	}
	result = buf;
	while (*str) {
		if (*str == '%') {
			*result = '\0';
			prn[1] = c = *(++str);
			if (c == '%') {
				*(result++) = '%';
				str++;
			} else {
				mywhere = player;
				d = (isupper(c)) ? c : toupper(c);

#ifdef COMPRESS
				snprintf(globprop, sizeof(globprop), "_pronouns/%.64s/%s", uncompress(sexstr), prn);
				if (d == 'A' || d == 'S' || d == 'O' || d == 'P' || d == 'R' || d == 'N') {
					self_sub = uncompress(get_property_class(mywhere, prn));
				} else {
					self_sub = uncompress(envpropstr(&mywhere, prn));
				}
				if (!self_sub) {
					self_sub = uncompress(get_property_class(player, globprop));
				}
				if (!self_sub) {
					self_sub = uncompress(get_property_class(0, globprop));
				}
#else
				snprintf(globprop, sizeof(globprop), "_pronouns/%.64s/%s", sexstr, prn);
				if (d == 'A' || d == 'S' || d == 'O' || d == 'P' || d == 'R' || d == 'N') {
					self_sub = get_property_class(mywhere, prn);
				} else {
					self_sub = envpropstr(&mywhere, prn);
				}
				if (!self_sub) {
					self_sub = get_property_class(player, globprop);
				}
				if (!self_sub) {
					self_sub = get_property_class(0, globprop);
				}
#endif

				if (self_sub) {
					temp_sub = NULL;
					if (self_sub[0] == '%' && toupper(self_sub[1]) == 'N') {
						temp_sub = self_sub;
						self_sub = PNAME(player);
					}
					if (((result - buf) + strlen(self_sub)) > (BUFFER_LEN - 2))
						return buf;
					strcat(result, self_sub);
					if (isupper(prn[1]) && islower(*result))
						*result = toupper(*result);
					result += strlen(result);
					str++;
					if (temp_sub) {
						if (((result - buf) + strlen(temp_sub+2)) > (BUFFER_LEN - 2))
							return buf;
						strcat(result, temp_sub+2);
						if (isupper(temp_sub[1]) && islower(*result))
							*result = toupper(*result);
						result += strlen(result);
						str++;
					}
				} else if (sex == GENDER_UNASSIGNED) {
					switch (c) {
					case 'n':
					case 'N':
					case 'o':
					case 'O':
					case 's':
					case 'S':
					case 'r':
					case 'R':
						strcat(result, PNAME(player));
						break;
					case 'a':
					case 'A':
					case 'p':
					case 'P':
						strcat(result, PNAME(player));
						strcat(result, "'s");
						break;
					default:
						result[0] = *str;
						result[1] = 0;
						break;
					}
					str++;
					result += strlen(result);
					if ((result - buf) > (BUFFER_LEN - 2)) {
						buf[BUFFER_LEN - 1] = '\0';
						return buf;
					}
				} else {
					switch (c) {
					case 'a':
					case 'A':
						strcat(result, absolute[sex]);
						break;
					case 's':
					case 'S':
						strcat(result, subjective[sex]);
						break;
					case 'p':
					case 'P':
						strcat(result, possessive[sex]);
						break;
					case 'o':
					case 'O':
						strcat(result, objective[sex]);
						break;
					case 'r':
					case 'R':
						strcat(result, reflexive[sex]);
						break;
					case 'n':
					case 'N':
						strcat(result, PNAME(player));
						break;
					default:
						*result = *str;
						result[1] = '\0';
						break;
					}
					if (isupper(c) && islower(*result)) {
						*result = toupper(*result);
					}
					result += strlen(result);
					str++;
					if ((result - buf) > (BUFFER_LEN - 2)) {
						buf[BUFFER_LEN - 1] = '\0';
						return buf;
					}
				}
			}
		} else {
			if ((result - buf) > (BUFFER_LEN - 2)) {
				buf[BUFFER_LEN - 1] = '\0';
				return buf;
			}
			*result++ = *str++;
		}
	}
	*result = '\0';
	return buf;
}

#ifndef MALLOC_PROFILING

char *
alloc_string(const char *string)
{
	char *s;

	/* NULL, "" -> NULL */
	if (!string || !*string)
		return 0;

	if ((s = (char *) malloc(strlen(string) + 1)) == 0) {
		abort();
	}
	strcpy(s, string);
	return s;
}

struct shared_string *
alloc_prog_string(const char *s)
{
	struct shared_string *ss;
	int length;

	if (s == NULL || *s == '\0')
		return (NULL);

	length = strlen(s);
	if ((ss = (struct shared_string *)
		 malloc(sizeof(struct shared_string) + length)) == NULL)
		abort();

	ss->links = 1;
	ss->length = length;
	bcopy(s, ss->data, ss->length + 1);
	return (ss);
}


char *
string_dup(const char *s)
{
	char *p;

	p = (char *) malloc(1 + strlen(s));
	if (p)
		(void) strcpy(p, s);
	return (p);
}
#endif



char *
intostr(int i)
{
	static char num[16];
	int j, k;
	char *ptr2;

	k = i;
	ptr2 = num + 14;
	num[15] = '\0';
	if (i < 0)
		i = -i;
	while (i) {
		j = i % 10;
		*ptr2-- = '0' + j;
		i /= 10;
	}
	if (!k)
		*ptr2-- = '0';
	if (k < 0)
		*ptr2-- = '-';
	return (++ptr2);
}

const char *
name_mangle(dbref obj)
{
#if defined(ANONYMITY)
	static char pad[32];
	PropPtr p;

	if (!(p = get_property(obj, "@fakename")) || PropType(p) != PROP_STRTYP)
		return db[obj].name;
	snprintf(pad, sizeof(pad), "\001%s\002", l64a(obj));
	return pad;
#else
	return db[obj].name;
#endif
}

const char *
unmangle(dbref player, const char *s)
{
#if defined(ANONYMITY)
	char in[16384];
	static char buf[16384];
	char pad[1024];
	char *ptr, *src, *name;
	dbref is;
	PropPtr p;

	if (!s)
		return s;

	strcpy(in, s);
	ptr = buf;
	src = in;
	*ptr = 0;

	while (name = strchr(src, 1)) {
		*(name++) = 0;
		strcpy(ptr, src);
		if (src = strchr(name, 2))
			*(src++) = 0;
		else
			src = name + strlen(name);

		is = a64l(name);
		strcpy(pad, "@knows/");
		strcat(pad, NAME(is));

		if ((p = get_property(is, "@disguise")) && PropType(p) == PROP_STRTYP) {
#ifdef DISKBASE
			propfetch(is, p);
#endif
			strcpy(pad, uncompress(PropDataStr(p)));
		} else if ((p = get_property(player, pad)) && PropType(p) == PROP_STRTYP) {
#ifdef DISKBASE
			propfetch(player, p);
#endif
			strcpy(pad, uncompress(PropDataStr(p)));
		} else if ((p = get_property(is, "@fakename")) && PropType(p) == PROP_STRTYP) {
#ifdef DISKBASE
			propfetch(is, p);
#endif
			strcpy(pad, uncompress(PropDataStr(p)));
			else
			strcpy(pad, NAME(is));

			strcat(ptr, pad);
			ptr += strlen(ptr);
		}

		strcat(ptr, src);

		return buf;
#else
	return s;
#endif
}



/*
 * Encrypt one string with another one.
 */

#define CHARCOUNT 97

static char enarr[256];
static int charset_count[] = { 96, 97, 0 };
static int initialized_crypt = 0;

void
init_crypt(void)
{
	int i;

	for (i = 0; i <= 255; i++)
		enarr[i] = (char) i;
	for (i = 'A'; i <= 'M'; i++)
		enarr[i] = (char) enarr[i] + 13;
	for (i = 'N'; i <= 'Z'; i++)
		enarr[i] = (char) enarr[i] - 13;
	enarr['\r'] = 127;
	enarr[127] = '\r';
	enarr[ESCAPE_CHAR] = 31;
	enarr[31] = ESCAPE_CHAR;
	initialized_crypt = 1;
}


const char *
strencrypt(const char *data, const char *key)
{
	static char linebuf[BUFFER_LEN];
	char buf[BUFFER_LEN + 8];
	const char *cp;
	char *ptr;
	char *ups;
	const char *upt;
	int linelen;
	int count;
	int seed, seed2, seed3;
	int limit = BUFFER_LEN;
	int result;

	if (!initialized_crypt)
		init_crypt();

	seed = 0;
	for (cp = key; *cp; cp++)
		seed = ((((*cp) ^ seed) + 170) % 192);

	seed2 = 0;
	for (cp = data; *cp; cp++)
		seed2 = ((((*cp) ^ seed2) + 21) & 0xff);
	seed3 = seed2 = ((seed2 ^ (seed ^ (RANDOM() >> 24))) & 0x3f);

	count = seed + 11;
	for (upt = data, ups = buf, cp = key; *upt; upt++) {
		count = (((*cp) ^ count) + (seed ^ seed2)) & 0xff;
		seed2 = ((seed2 + 1) & 0x3f);
		if (!*(++cp))
			cp = key;
		result = (enarr[(unsigned char)*upt] - (32 - (CHARCOUNT - 96))) + count + seed;
		*ups = enarr[(result % CHARCOUNT) + (32 - (CHARCOUNT - 96))];
		count = (((*upt) ^ count) + seed) & 0xff;
		ups++;
	}
	*ups++ = '\0';

	ptr = linebuf;

	linelen = strlen(data);
	*ptr++ = (' ' + 2);
	*ptr++ = (' ' + seed3);
	limit--;
	limit--;

	for (cp = buf; cp < &buf[linelen]; cp++) {
		limit--;
		if (limit < 0)
			break;
		*ptr++ = *cp;
	}
	*ptr++ = '\0';
	return linebuf;
}



const char *
strdecrypt(const char *data, const char *key)
{
	char linebuf[BUFFER_LEN];
	static char buf[BUFFER_LEN];
	const char *cp;
	char *ptr;
	char *ups;
	const char *upt;
	int linelen;
	int outlen;
	int count;
	int seed, seed2;
	int result;
	int chrcnt;

	if (!initialized_crypt)
		init_crypt();

	ptr = linebuf;

	if ((data[0] - ' ') < 1 || (data[0] - ' ') > 2) {
		return "";
	}

	linelen = strlen(data);
	chrcnt = charset_count[(data[0] - ' ') - 1];
	seed2 = (data[1] - ' ');

	strcpy(linebuf, data + 2);

	seed = 0;
	for (cp = key; *cp; cp++) {
		seed = (((*cp) ^ seed) + 170) % 192;
	}

	count = seed + 11;
	outlen = strlen(linebuf);
	upt = linebuf;
	ups = buf;
	cp = key;
	while ((const char *) upt < &linebuf[outlen]) {
		count = (((*cp) ^ count) + (seed ^ seed2)) & 0xff;
		if (!*(++cp))
			cp = key;
		seed2 = ((seed2 + 1) & 0x3f);

		result = (enarr[(unsigned char)*upt] - (32 - (chrcnt - 96))) - (count + seed);
		while (result < 0)
			result += chrcnt;
		*ups = enarr[result + (32 - (chrcnt - 96))];

		count = (((*ups) ^ count) + seed) & 0xff;
		ups++;
		upt++;
	}
	*ups++ = '\0';

	return buf;
}


char *
strip_ansi(char *buf, const char *input)
{
	const char *is;
	char *os;

	buf[0] = '\0';
	os = buf;

	is = input;

	while (*is) {
		if (*is == ESCAPE_CHAR) {
			is++;
			if (*is == '[') {
				is++;
				while (isdigit(*is) || *is == ';')
					is++;
				if (*is == 'm')
					is++;
			} else {
				is++;
			}
		} else {
			*os++ = *is++;
		}
	}
	*os = '\0';

	return buf;
}


char *
strip_bad_ansi(char *buf, const char *input)
{
	const char *is;
	char *os;
	int aflag = 0;
	int limit = BUFFER_LEN - 5;

	buf[0] = '\0';
	os = buf;

	is = input;

	while (*is && limit-->0) {
		if (*is == ESCAPE_CHAR) {
			if (is[1] == '\0') {
				is++;
			} else if (is[1] != '[') {
				is++;
				is++;
			} else {
				aflag = 1;
				*os++ = *is++;	/* esc */
				*os++ = *is++;	/*  [  */
				while (isdigit(*is) || *is == ';') {
					*os++ = *is++;
				}
				if (*is != 'm') {
					*os++ = 'm';
				}
				*os++ = *is++;
			}
		} else {
			*os++ = *is++;
		}
	}
	if (aflag) {
		int termrn = 0;
		if (*(os - 2) == '\r' && *(os - 1) == '\n') {
			termrn = 1;
			os -= 2;
		}
		*os++ = '\033';
		*os++ = '[';
		*os++ = '0';
		*os++ = 'm';
		if (termrn) {
			*os++ = '\r';
			*os++ = '\n';
		}
	}
	*os = '\0';

	return buf;
}

/* Prepends what before before, granted it doesn't come
 * before start in which case it returns 0.
 * Otherwise it modifies *before to point to that new location,
 * and it returns the number of chars prepended.
 */
int
prepend_string(char** before, char* start, const char* what)
{
   char* ptr;
   size_t len;
   len = strlen(what);
   ptr = *before - len;
   if (ptr < start)
       return 0;
   memcpy((void*) ptr, (const void*) what, len);
   *before = ptr;
   return len;
}

int IsValidPoseSeparator(char ch)
{
	return (ch == '\'') || (ch == ' ') || (ch == ',') || (ch == '-');
}

void PrefixMessage(char* Dest, const char* Src, const char* Prefix, int BufferLength, int SuppressIfPresent)
{
	int PrefixLength			= strlen(Prefix);
	int CheckForHangingEnter	= 0;

	while((BufferLength > PrefixLength) && (*Src != '\0'))
	{
		if (*Src == '\r')
		{
			Src++;
			continue;
		}

		if (!SuppressIfPresent || strncmp(Src, Prefix, PrefixLength) || (
				!IsValidPoseSeparator(Src[PrefixLength]) &&	
				(Src[PrefixLength] != '\r') &&
				(Src[PrefixLength] != '\0')
			))
		{
			strcpy(Dest, Prefix);

			Dest			+= PrefixLength;
			BufferLength	-= PrefixLength;

			if (BufferLength > 1)
			{
				if (!IsValidPoseSeparator(*Src))
				{
					*Dest++ = ' ';
					BufferLength--;
				}
			}
		}

		while((BufferLength > 1) && (*Src != '\0'))
		{
				*Dest++ = *Src;
				BufferLength--;

				if (*Src++ == '\r')
				{
					CheckForHangingEnter = 1;
					break;
				}
		}
	}

	if (CheckForHangingEnter && (Dest[-1] == '\r'))
		Dest--;

	*Dest = '\0';
}

int IsPropPrefix(const char* Property, const char* Prefix)
{
	while(*Property == PROPDIR_DELIMITER)
		Property++;

	while(*Prefix == PROPDIR_DELIMITER)
		Prefix++;

	while(*Prefix)
	{
		if (*Property == '\0')
			return 0;

		if (*Property++ != *Prefix++)
			return 0;
	}

	return (*Property == '\0') || (*Property == PROPDIR_DELIMITER);
}

