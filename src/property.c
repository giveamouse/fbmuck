/* $Header$ */

/*
 * $Log: property.c,v $
 * Revision 1.2  2000/02/08 23:05:53  revar
 * Changes get_property_fvalue() to return 0.0 instead of SMALL_NUM when
 *   the property doesn't exist, or is not a float.
 *
 * Revision 1.1.1.1  2000/01/11 01:56:04  revar
 * Initial Sourceforge checkin, fb6.00a29
 *
 * Revision 1.2  2000/01/11 01:56:04  foxen
 * Removed dangerous buggy propdir diskbasing code.
 *
 * Revision 1.1.1.1  1999/12/12 07:27:43  foxen
 * Initial FB6 CVS checkin.
 *
 * Revision 1.1  1996/06/12 02:55:59  foxen
 * Initial revision
 *
 * Revision 5.17  1994/03/21  15:13:01  foxen
 * Fixed bug with @set obj=:clear not clearing all props on obj.
 *
 * Revision 5.16  1994/03/21  11:00:42  foxen
 * Autoconfiguration mods.
 *
 * Revision 5.15  1994/03/14  12:20:58  foxen
 * Fb5.20 release checkpoint.
 *
 * Revision 5.14  1994/02/13  13:57:51  foxen
 * Ckeaned up display of properties in examine and such.
 *
 * Revision 5.13  1994/01/31  10:00:40  foxen
 * Fixed bug with setting properties with : at the beginning of the propname.
 *
 * Revision 5.12  1994/01/18  20:52:20  foxen
 * Version 5.15 release.
 *
 * Revision 5.11  1993/12/20  06:22:51  foxen
 * *** empty log message ***
 *
 * Revision 5.1  1993/12/17  00:07:33  foxen
 * initial revision.
 *
 * Revision 1.6  91/04/06  18:22:47  jearls
 * Rewrote for property directories
 *
 * Revision 1.5  90/09/16  04:42:47  rearl
 * Preparation code added for disk-based MUCK.
 *
 * Revision 1.4  90/09/15  22:27:45  rearl
 * Fixed dangerous COMPRESS-related bug.
 *
 * Revision 1.3  90/09/10  02:19:49  rearl
 * Introduced string compression of properties, for the
 * COMPRESS compiler option.
 *
 * Revision 1.2  90/08/02  18:50:00  rearl
 * Fixed NULL pointer problem in get_property_class().
 *
 * Revision 1.1  90/07/19  23:04:03  casie
 * Initial revision
 *
 *
 */

#include "copyright.h"
#include "config.h"
#include "params.h"

#include "db.h"
#include "tune.h"
#include "mpi.h"
#include "props.h"
#include "externs.h"
#include "interface.h"
#include <strings.h>

#ifdef COMPRESS
extern const char *compress(const char *);
extern const char *old_uncompress(const char *);

#define alloc_compressed(x) alloc_string(compress(x))
#else                                /* !COMPRESS */
#define alloc_compressed(x) alloc_string(x)
#endif                                /* COMPRESS */

/* property.c
   A whole new lachesis mod.
   Adds property manipulation routines to TinyMUCK.   */

/* Completely rewritten by darkfox and Foxen, for propdirs and other things */



void
set_property_nofetch(dbref player, const char* pname, PData* dat)
{
    PropPtr p, l;
    char buf[BUFFER_LEN];
    char   *n, *w;

    /* Make sure that we are passed a valid property name */
    if (!pname) return;

    while (*pname == PROPDIR_DELIMITER) pname++;
    if ((!(FLAGS(player) & LISTENER)) &&
            (string_prefix(pname, "_listen") ||
             string_prefix(pname, "~listen") ||
             string_prefix(pname, "~olisten"))) {
        FLAGS(player) |= LISTENER;
    }

    w = strcpy(buf, pname);

    /* truncate propnames with a ':' in them at the ':' */
    n = index(buf, PROP_DELIMITER);
    if (n) *n = '\0';
    if (!*buf) return;

    p = propdir_new_elem(&(DBFETCH(player)->properties), w);

    /* free up any old values */
    clear_propnode(p);

    SetPFlagsRaw(p, dat->flags);
    if (PropFlags(p) & PROP_ISUNLOADED) {
        SetPDataUnion(p, dat->data);
        return;
    }
    switch (PropType(p)) {
        case PROP_STRTYP:
            if (!dat->data.str || !*(dat->data.str)) {
                SetPType(p, PROP_DIRTYP);
                SetPDataStr(p, NULL);
                if (!PropDir(p)) {
                    remove_property_nofetch(player, pname);
                }
            } else {
                SetPDataStr(p, alloc_compressed(dat->data.str));
#ifdef COMPRESS
                SetPFlagsRaw(p, (dat->flags | PROP_COMPRESSED));
#endif
            }
            break;
        case PROP_INTTYP:
            SetPDataVal(p, dat->data.val);
            if (!dat->data.val) {
                SetPType(p, PROP_DIRTYP);
                if (!PropDir(p)) {
                    remove_property_nofetch(player, pname);
                }
            }
            break;
        case PROP_FLTTYP:
            SetPDataFVal(p, dat->data.fval);
            if ((dat->data.fval<SMALL_NUM)&&(dat->data.fval>NSMALL_NUM)) {
                SetPType(p, PROP_DIRTYP);
                if (!PropDir(p)) {
                    remove_property_nofetch(player, pname);
                }
            }
            break;
        case PROP_REFTYP:
            SetPDataRef(p, dat->data.ref);
            if (dat->data.ref == NOTHING) {
                SetPType(p, PROP_DIRTYP);
                SetPDataRef(p, 0);
                if (!PropDir(p)) {
                    remove_property_nofetch(player, pname);
                }
            }
            break;
        case PROP_LOKTYP:
            SetPDataLok(p, dat->data.lok);
            break;
        case PROP_DIRTYP:
            SetPDataVal(p, 0);
            if (!PropDir(p)) {
                remove_property_nofetch(player, pname);
            }
            break;
    }
}


void
set_property(dbref player, const char* name, PData* dat)
{
#ifdef DISKBASE
    fetchprops(player, propdir_name(name));
    set_property_nofetch(player, name, dat);
    dirtyprops(player);
#else
    set_property_nofetch(player, name, dat);
#endif
    DBDIRTY(player);
}

void
set_lock_property(dbref player, const char *pname, const char *lok)
{
    PData mydat;
    mydat.flags = PROP_LOKTYP;
    if (!lok || !*lok) {
        mydat.data.lok = TRUE_BOOLEXP;
    } else {
        mydat.data.lok = parse_boolexp(-1, (dbref)1, lok, 1);
    }
    set_property(player, pname, &mydat);
}



/* adds a new property to an object */
void
add_prop_nofetch(dbref player, const char *pname, const char *class, int value)
{
    PData mydat;
    if (class && *class) {
        mydat.flags = PROP_STRTYP;
        mydat.data.str = (char *)class;
    } else if (value) {
        mydat.flags = PROP_INTTYP;
        mydat.data.val = value;
    } else {
        mydat.flags = PROP_DIRTYP;
        mydat.data.str = NULL;
    }
    set_property_nofetch(player, pname, &mydat);
}


/* adds a new property to an object */
void
add_property(dbref player, const char *pname, const char *class, int value)
{

#ifdef DISKBASE
    fetchprops(player, propdir_name(pname));
    add_prop_nofetch(player, pname, class, value);
    dirtyprops(player);
#else
    add_prop_nofetch(player, pname, class, value);
#endif
    DBDIRTY(player);
}


void
remove_proplist_item(dbref player, PropPtr p, int allp)
{
    const char *ptr;

    if (!p) return;
    ptr = PropName(p);
    if (!allp) {
        if (*ptr == PROP_HIDDEN) return;
        if (*ptr == PROP_SEEONLY) return;
        if (ptr[0] == '_'  &&  ptr[1] == '\0') return;
        if (PropFlags(p) & PROP_SYSPERMS) return;
    }
    remove_property(player, ptr);
}


/* removes property list --- if it's not there then ignore */
void 
remove_property_list(dbref player, int all)
{
    PropPtr l;
    PropPtr p;
    PropPtr n;

#ifdef DISKBASE
    fetchprops(player, NULL);
#endif

    if (l = DBFETCH(player)->properties) {
        p = first_node(l);
        while (p) {
            n = next_node(l, PropName(p));
            remove_proplist_item(player, p, all);
            l = DBFETCH(player)->properties;
            p = n;
        }
    }

#ifdef DISKBASE
    dirtyprops(player);
#endif

    DBDIRTY(player);
}



/* removes property --- if it's not there then ignore */
void 
remove_property_nofetch(dbref player, const char *pname)
{
    PropPtr l;
    char buf[BUFFER_LEN];
    char   *n, *w;

    w = strcpy(buf, pname);

    l = DBFETCH(player)->properties;
    l = propdir_delete_elem(l, w);
    DBFETCH(player)->properties = l;
    DBDIRTY(player);
}


void 
remove_property(dbref player, const char *pname)
{
#ifdef DISKBASE
    fetchprops(player, propdir_name(pname));
#endif

    remove_property_nofetch(player, pname);

#ifdef DISKBASE
    dirtyprops(player);
#endif
}


PropPtr 
get_property(dbref player, const char *pname)
{
    PropPtr p, l;
    char buf[BUFFER_LEN];
    char   *n, *w;

#ifdef DISKBASE
    fetchprops(player, propdir_name(pname));
#endif

    w = strcpy(buf, pname);

    p = propdir_get_elem(DBFETCH(player)->properties, w);
    return (p);
}


/* checks if object has property, returning 1 if it or any of it's contents has
   the property stated                                                      */
int 
has_property(int descr, dbref player, dbref what, const char *pname, const char *class, int value)
{
    dbref   things;

    if (has_property_strict(descr, player, what, pname, class, value))
        return 1;
    for (things = DBFETCH(what)->contents; things != NOTHING;
            things = DBFETCH(things)->next) {
        if (has_property(descr, player, things, pname, class, value))
            return 1;
    }
    if (tp_lock_envcheck) {
        things = getparent(what);
        while (things != NOTHING) {
            if (has_property_strict(descr, player, things, pname, class, value))
                return 1;
            things = getparent(things);
        }
    }
    return 0;
}


/* checks if object has property, returns 1 if it has the property */
int 
has_property_strict(int descr, dbref player, dbref what, const char *pname, const char *class, int value)
{
    PropPtr p;
    const char *str;
    char   *ptr;
    char buf[BUFFER_LEN];

    p = get_property(what, pname);

    if (p) {
#ifdef DISKBASE
        propfetch(what, p);
#endif
        if (PropType(p) == PROP_STRTYP) {
#ifdef COMPRESS
            str = uncompress(DoNull(PropDataStr(p)));
#else                                /* !COMPRESS */
            str = DoNull(PropDataStr(p));
#endif                                /* COMPRESS */

            ptr = do_parse_mesg(descr, player, what, str, "(Lock)",
                                buf, (MPI_ISPRIVATE | MPI_ISLOCK));
            return (equalstr((char *)class, ptr));
        } else if (PropType(p) == PROP_LOKTYP) {
            return 0;
        } else if (PropType(p) == PROP_INTTYP) {
            return (value == PropDataVal(p));
        } else {
            return (value == (int)PropDataFVal(p));
        }
    }
    return 0;
}

/* return class (string value) of property */
const char *
get_property_class(dbref player, const char *pname)
{
    PropPtr p;

    p = get_property(player, pname);
    if (p) {
#ifdef DISKBASE
        propfetch(player, p);
#endif
        if (PropType(p) != PROP_STRTYP)
            return (char *) NULL;
        return (PropDataStr(p));
    } else {
        return (char *) NULL;
    }
}

/* return value of property */
int 
get_property_value(dbref player, const char *pname)
{
    PropPtr p;

    p = get_property(player, pname);

    if (p) {
#ifdef DISKBASE
        propfetch(player, p);
#endif
        if (PropType(p) != PROP_INTTYP)
            return 0;
        return (PropDataVal(p));
    } else {
        return 0;
    }
}

/* return float value of a property */
float
get_property_fvalue(dbref player, const char *pname)
{
    PropPtr p;
    p = get_property(player, pname);
    if (p) {
#ifdef DISKBASE
      propfetch(player,p);
#endif
      if (PropType(p) != PROP_FLTTYP)
        return 0.0;
      return (PropDataFVal(p));
    } else {
      return 0.0;
    }
}

/* return boolexp lock of property */
dbref
get_property_dbref(dbref player, const char *class)
{
    PropPtr p;
    p = get_property(player, class);
    if (!p) return NOTHING;
#ifdef DISKBASE
    propfetch(player, p);
#endif
    if (PropType(p) != PROP_REFTYP)
        return NOTHING;
    return PropDataRef(p);
}


/* return boolexp lock of property */
struct boolexp *
get_property_lock(dbref player, const char *class)
{
    PropPtr p;
    p = get_property(player, class);
    if (!p) return TRUE_BOOLEXP;
#ifdef DISKBASE
    propfetch(player, p);
    if (PropFlags(p) & PROP_ISUNLOADED)
        return TRUE_BOOLEXP;
#endif
    if (PropType(p) != PROP_LOKTYP)
        return TRUE_BOOLEXP;
    return PropDataLok(p);
}


/* return flags of property */
int 
get_property_flags(dbref player, const char *pname)
{
    PropPtr p;

    p = get_property(player, pname);

    if (p) {
        return (PropFlags(p));
    } else {
        return 0;
    }
}


/* return type of property */
int 
get_property_type(dbref player, const char *pname)
{
    PropPtr p;

    p = get_property(player, pname);

    if (p) {
        return (PropType(p));
    } else {
        return 0;
    }
}



PropPtr
copy_prop(dbref old)
{
    PropPtr p, n = NULL;

#ifdef DISKBASE
    fetchprops(old, NULL);
#endif

    p = DBFETCH(old)->properties;
    copy_proplist(old, &n, p);
    return (n);
}

/* return old gender values for pronoun substitution code */
int 
genderof(int descr, dbref player)
{
    if (has_property_strict(descr, player, player, "sex", "male", 0))
        return GENDER_MALE;
    else if (has_property_strict(descr, player, player, "sex", "female", 0))
        return GENDER_FEMALE;
    else if (has_property_strict(descr, player, player, "sex", "neuter", 0))
        return GENDER_NEUTER;
    else
        return GENDER_UNASSIGNED;
}


/* Return a pointer to the first property in a propdir and duplicates the
   property name into 'name'.  Returns NULL if the property list is empty
   or does not exist. */
PropPtr 
first_prop_nofetch(dbref player, const char *dir, PropPtr *list, char *name)
{
    char buf[BUFFER_LEN];
    PropPtr p;

    if (dir) {
        while (*dir && *dir == PROPDIR_DELIMITER) {
            dir++;
        }
    }
    if (!dir || !*dir) {
        *list = DBFETCH(player)->properties;
        p = first_node(*list);
        if (p) {
            strcpy(name, PropName(p));
        } else {
            *name = '\0';
        }
        return (p);
    }

    strcpy(buf, dir);
    *list = p = propdir_get_elem(DBFETCH(player)->properties, buf);
    if (!p) {
        *name = '\0';
        return NULL;
    }
    *list = PropDir(p);
    p = first_node(*list);
    if (p) {
        strcpy(name, PropName(p));
    } else {
        *name = '\0';
    }

    return (p);
}


/* first_prop() returns a pointer to the first property.
 * player    dbref of object that the properties are on.
 * dir       pointer to string name of the propdir
 * list      pointer to a proplist pointer.  Returns the root node.
 * name      printer to a string.  Returns the name of the first node.
 */

PropPtr 
first_prop(dbref player, const char *dir, PropPtr *list, char *name)
{

#ifdef DISKBASE
    fetchprops(player, (char *)dir);
#endif

    return (first_prop_nofetch(player, dir, list, name));
}



/* next_prop() returns a pointer to the next property node.
 * list    Pointer to the root node of the list.
 * prop    Pointer to the previous prop.
 * name    Pointer to a string.  Returns the name of the next property.
 */

PropPtr 
next_prop(PropPtr list, PropPtr prop, char *name)
{
    PropPtr p = prop;

    if (!p || !(p = next_node(list, PropName(p))))
        return ((PropPtr) 0);

    strcpy(name, PropName(p));
    return (p);
}



/* next_prop_name() returns a ptr to the string name of the next property.
 * player   object the properties are on.
 * outbuf   pointer to buffer to return the next prop's name in.
 * name     pointer to the name of the previous property.
 *
 * Returns null if propdir doesn't exist, or if no more properties in list.
 * Call with name set to "" to get the first property of the root propdir.
 */

char   *
next_prop_name(dbref player, char *outbuf, char *name)
{
    char   *ptr;
    char    buf[BUFFER_LEN];
    PropPtr p, l;

#ifdef DISKBASE
    fetchprops(player, propdir_name(name));
#endif

    strcpy(buf, name);
    if (!*name || name[strlen(name)-1] == PROPDIR_DELIMITER) {
        l = DBFETCH(player)->properties;
        p = propdir_first_elem(l, buf);
        if (!p) {
            *outbuf = '\0';
            return NULL;
        }
        strcat(strcpy(outbuf, name), PropName(p));
    } else {
        l = DBFETCH(player)->properties;
        p = propdir_next_elem(l, buf);
        if (!p) {
            *outbuf = '\0';
            return NULL;
        }
        strcpy(outbuf, name);
        ptr = rindex(outbuf, PROPDIR_DELIMITER);
        if (!ptr) ptr = outbuf;
        *(ptr++) = PROPDIR_DELIMITER;
        strcpy(ptr, PropName(p));
    }
    return outbuf;
}


long
size_properties(dbref player, int load)
{
#ifdef DISKBASE
    if (load) {
        fetchprops(player, NULL);
        fetch_propvals(player, "/");
    }
#endif
    return size_proplist(DBFETCH(player)->properties);
}


/* return true if a property contains a propdir */
int 
is_propdir_nofetch(dbref player, const char *pname)
{
    PropPtr p, l;
    char   *n;
    char w[BUFFER_LEN];

    strcpy(w, pname);
    p = propdir_get_elem(DBFETCH(player)->properties, w);
    if (!p) return 0;
    return (PropDir(p) != (PropPtr) NULL);
}


int 
is_propdir(dbref player, const char *pname)
{

#ifdef DISKBASE
    fetchprops(player, propdir_name(pname));
#endif

    return (is_propdir_nofetch(player, pname));
}


PropPtr
envprop(dbref *where, const char *propname, int typ)
{
    PropPtr temp;
    while (*where != NOTHING) {
        temp = get_property(*where, propname);
#ifdef DISKBASE
        if (temp) propfetch(*where, temp);
#endif
        if (temp && (!typ || PropType(temp) == typ))
            return temp;
        *where = getparent(*where);
    }
    return NULL;
}


const char *
envpropstr(dbref * where, const char *propname)
{
    PropPtr temp;

    temp = envprop(where, propname, PROP_STRTYP);
    if (!temp) return NULL;
    if (PropType(temp) == PROP_STRTYP)
        return (PropDataStr(temp));
    return NULL;
}


char *
displayprop(dbref player, dbref obj, const char *name, char *buf)
{
    char mybuf[BUFFER_LEN];
    int pdflag;
    PropPtr p = get_property(obj, name);

    if (!p) {
        sprintf(buf, "%s: No such property.", name);
        return buf;
    }
#ifdef DISKBASE
    propfetch(obj, p);
#endif
    pdflag = (PropDir(p) != NULL);
    sprintf(mybuf, "%.*s%c", (BUFFER_LEN/4), name,
            (pdflag)? PROPDIR_DELIMITER:'\0');
    switch (PropType(p)) {
        case PROP_STRTYP:
            sprintf(buf, "str %s:%.*s", mybuf, (BUFFER_LEN/2),
                    PropDataStr(p));
            break;
        case PROP_REFTYP:
            sprintf(buf, "ref %s:%s", mybuf,
                    unparse_object(player, PropDataRef(p)));
            break;
        case PROP_INTTYP:
            sprintf(buf, "int %s:%d", mybuf, PropDataVal(p));
            break;
        case PROP_FLTTYP:
            sprintf(buf, "flt %s:%hg", mybuf, PropDataFVal(p));
            break;
        case PROP_LOKTYP:
            if (PropFlags(p) & PROP_ISUNLOADED) {
                sprintf(buf, "lok %s:*UNLOCKED*", mybuf);
            } else {
                sprintf(buf, "lok %s:%.*s", mybuf, (BUFFER_LEN/2),
                        unparse_boolexp(player, PropDataLok(p), 1));
            }
            break;
        case PROP_DIRTYP:
            sprintf(buf, "dir %s:(no value)", mybuf);
            break;
    }
    return buf;
}



#define DOWNCASE(x) (tolower(x))

extern short db_conversion_flag;
extern short db_decompression_flag;


int
db_get_single_prop(FILE *f, dbref obj, long pos, PropPtr pnode, const char *pdir)
{
    char getprop_buf[BUFFER_LEN*3];
    char *name, *flags, *value, *p;
    int flg;
    long tpos;
    struct boolexp *lok;
    short do_diskbase_propvals;
    PData mydat;

#ifdef DISKBASE
    do_diskbase_propvals = tp_diskbase_propvals;
#else
    do_diskbase_propvals = 0;
#endif

    if (pos) {
        fseek(f, pos, 0);
    } else if (do_diskbase_propvals) {
        tpos = ftell(f);
    }
    name = fgets(getprop_buf, sizeof(getprop_buf), f);
    if (!name) abort();
    if (*name == '*') {
        if (!strcmp(name,"*End*\n")) {
            return 0;
        }
        if (name[1] == '\n') {
            return 2;
        }
    }

    flags = index(name, PROP_DELIMITER);
    if (!flags) abort();
    *flags++ = '\0';

    value = index(flags, PROP_DELIMITER);
    if (!value) abort();
    *value++ = '\0';

    p = index(value, '\n');
    if (p) *p = '\0';

    if (!number(flags)) abort();
    flg = atoi(flags);

    switch (flg & PROP_TYPMASK) {
        case PROP_STRTYP:
            if (!do_diskbase_propvals || pos) {
                flg &= ~PROP_ISUNLOADED;
#ifdef COMPRESS
                if (!(flg & PROP_COMPRESSED)) {
                    value = (char *)old_uncompress(value);
                }
#endif
                if (pnode) {
                    SetPDataStr(pnode, alloc_compressed(value));
#ifdef COMPRESS
                    flg |= PROP_COMPRESSED;
#endif
                    SetPFlagsRaw(pnode, flg);
                } else {
                    mydat.flags = flg;
                    mydat.data.str = value;
                    set_property_nofetch(obj, name, &mydat);
                }
            } else {
                flg |= PROP_ISUNLOADED;
                mydat.flags = flg;
                mydat.data.val = tpos;
                set_property_nofetch(obj, name, &mydat);
            }
            break;
        case PROP_LOKTYP:
            if (!do_diskbase_propvals || pos) {
                lok = parse_boolexp(-1, (dbref)1, value, 32767);
                flg &= ~PROP_ISUNLOADED;
                if (pnode) {
                    SetPDataLok(pnode, lok);
                    SetPFlagsRaw(pnode, flg);
                } else {
                    mydat.flags = flg;
                    mydat.data.lok = lok;
                    set_property_nofetch(obj, name, &mydat);
                }
            } else {
                flg |= PROP_ISUNLOADED;
                mydat.flags = flg;
                mydat.data.val = tpos;
                set_property_nofetch(obj, name, &mydat);
            }
            break;
        case PROP_INTTYP:
            if (!number(value)) abort();
            mydat.flags = flg;
            mydat.data.val = atoi(value);
            set_property_nofetch(obj, name, &mydat);
            break;
        case PROP_FLTTYP:
            mydat.flags = flg;
            if (!number(value) && !ifloat(value)) {
                char *tpnt = value;
                int dtemp = 0;

                if ((*tpnt=='+')||(*tpnt=='-')) {
                    if (*tpnt=='+') {
                        dtemp=0; 
                    } else {
                        dtemp=1;
                    }
                    tpnt++;
                }
                tpnt[0] = toupper(tpnt[0]);
                tpnt[1] = toupper(tpnt[1]);
                tpnt[2] = toupper(tpnt[2]);
                if (!strncmp(tpnt,"INF",3)) {
                    if (!dtemp) {
                        mydat.data.fval = 9.99E999;
                    } else {
                        mydat.data.fval = -9.99E999;
                    }
                } else {
                    if (!strncmp(tpnt,"NAN",3)) {
                        mydat.data.fval = 0.0;
                    }
                    abort();
                }
            } else {
                sscanf(value,"%hg",&mydat.data.fval);
            }
            set_property_nofetch(obj, name, &mydat);
            break;
        case PROP_REFTYP:
            if (!number(value)) abort();
            mydat.flags = flg;
            mydat.data.ref = atoi(value);
            set_property_nofetch(obj, name, &mydat);
            break;
        case PROP_DIRTYP:
            break;
    }
    return 1;
}

void
db_getprops(FILE *f, dbref obj, const char *pdir)
{
    while (db_get_single_prop(f, obj, 0L, (PropPtr)NULL, pdir));
}


void
db_putprop(FILE *f, const char *dir, PropPtr p)
{
    char buf[BUFFER_LEN*2];
    char *ptr;
    const char *ptr2;
    char tbuf[50];

    if (PropType(p) == PROP_DIRTYP)
        return;

    for (ptr = buf, ptr2 = dir+1; *ptr2;) *ptr++ = *ptr2++;
    for (ptr2 = PropName(p); *ptr2;) *ptr++ = *ptr2++;
    *ptr++ = PROP_DELIMITER;
    ptr2 = intostr(PropFlagsRaw(p) & ~(PROP_TOUCHED | PROP_ISUNLOADED | PROP_DIRUNLOADED));
    while (*ptr2) *ptr++ = *ptr2++;
    *ptr++ = PROP_DELIMITER;

    ptr2 = "";
    switch (PropType(p)) {
        case PROP_INTTYP:
            if (!PropDataVal(p)) return;
            ptr2 = intostr(PropDataVal(p));
            break;
        case PROP_FLTTYP:
            if (!PropDataFVal(p)) return;
            sprintf(tbuf,"%hg",PropDataFVal(p));
            ptr2 = tbuf;
            break;
        case PROP_REFTYP:
            if (PropDataRef(p) == NOTHING) return;
            ptr2 = intostr(PropDataRef(p));
            break;
        case PROP_STRTYP:
            if (!*PropDataStr(p)) return;
            if (db_decompression_flag) {
#ifdef COMPRESS
                ptr2 = uncompress(PropDataStr(p));
#else
                ptr2 = PropDataStr(p);
#endif
            } else {
                ptr2 = PropDataStr(p);
            }
            break;
        case PROP_LOKTYP:
            if (PropFlags(p) & PROP_ISUNLOADED) return;
            if (PropDataLok(p) == TRUE_BOOLEXP) return;
            ptr2 = unparse_boolexp((dbref)1, PropDataLok(p), 0);
            break;
    }
    while (*ptr2) *ptr++ = *ptr2++;
    *ptr++ = '\n';
    *ptr++ = '\0';
    if (fputs(buf, f) == EOF) {
        abort();
    }
}


int
db_dump_props_rec(dbref obj, FILE *f, const char *dir, PropPtr p)
{
    char buf[BUFFER_LEN];
    char *ptr;
    const char *ptr2;
    long tpos;
    int flg;
    short wastouched = 0;
    int count = 0;
    int pdcount;

    if (!p) return 0;

    count += db_dump_props_rec(obj, f, dir, AVL_LF(p));

#ifdef DISKBASE
    wastouched = (PropFlags(p) & PROP_TOUCHED);
    if (tp_diskbase_propvals) {
        tpos = ftell(f);
    }
    if (wastouched) {
        count++;
    }
    if (propfetch(obj, p)) {
        fseek(f, 0L, 2);
    }
#endif

    db_putprop(f, dir, p);

#ifdef DISKBASE
    if (tp_diskbase_propvals && !wastouched) {
        if (PropType(p) == PROP_STRTYP || PropType(p) == PROP_LOKTYP) {
            flg = PropFlagsRaw(p) | PROP_ISUNLOADED;
            clear_propnode(p);
            SetPFlagsRaw(p, flg);
            SetPDataVal(p, tpos);
        }
    }
#endif

    if (PropDir(p)) {
        const char *iptr;
        char *optr;

        for (iptr = dir, optr = buf; *iptr;) *optr++ = *iptr++;
        for (iptr = PropName(p); *iptr;) *optr++ = *iptr++;
        *optr++ = PROPDIR_DELIMITER;
        *optr++ = '\0';

        pdcount = db_dump_props_rec(obj, f, buf, PropDir(p));
        count += pdcount;
    }

    count += db_dump_props_rec(obj, f, dir, AVL_RT(p));

    return count;
}


void
db_dump_props(FILE *f, dbref obj)
{
    db_dump_props_rec(obj, f, "/", DBFETCH(obj)->properties);
}


void
untouchprop_rec(PropPtr p)
{
    if (!p) return;
    SetPFlags(p, (PropFlags(p) & ~PROP_TOUCHED));
    untouchprop_rec(AVL_LF(p));
    untouchprop_rec(AVL_RT(p));
    untouchprop_rec(PropDir(p));
}

static dbref untouch_lastdone = 0;
void
untouchprops_incremental(int limit)
{
    PropPtr p;

    while (untouch_lastdone < db_top) {
        /* clear the touch flags */
        p = DBFETCH(untouch_lastdone)->properties;
        if (p) {
            if (!limit--) return;
            untouchprop_rec(p);
        }
        untouch_lastdone++;
    }
    untouch_lastdone = 0;
}

