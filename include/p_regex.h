#ifndef _P_REGEX_H
#define _P_REGEX_H

#define MUF_RE_ICASE		1
#define MUF_RE_NEWLINE		2
#define MUF_RE_ALL			4

#define MUF_RE_ICASE_STR	"1"
#define MUF_RE_NEWLINE_STR	"2"
#define MUF_RE_ALL_STR		"4"

extern void prim_regexp(PRIM_PROTOTYPE);
extern void prim_regsub(PRIM_PROTOTYPE);

#define PRIMS_REGEX_FUNCS prim_regexp, prim_regsub

#define PRIMS_REGEX_NAMES "REGEXP", "REGSUB"

#define PRIMS_REGEX_CNT 2


#endif /* _P_REGEX_H */

