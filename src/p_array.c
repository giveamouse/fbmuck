/* Primitives Package */

#include "copyright.h"
#include "config.h"

#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include "db.h"
#include "tune.h"
#include "inst.h"
#include "externs.h"
#include "match.h"
#include "interface.h"
#include "params.h"
#include "strings.h"
#include "interp.h"

static struct inst *oper1, *oper2, *oper3, *oper4;
static struct inst temp1, temp2;
static int result;
static char buf[BUFFER_LEN];



void
prim_array_make(PRIM_PROTOTYPE)
{
    stk_array* new;
    int i;

    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_INTEGER)
        abort_interp("Invalid item count.");
    result = oper1->data.number;
    if (result < 0)
        abort_interp("Item count must be positive.");
    CLEAR(oper1);
    nargs = 0;

    if (*top < result)
        abort_interp("Stack underflow.");

    temp1.type = PROG_INTEGER;
    temp1.line = 0;
    new = new_array_packed(result);
    for (i = result; i-->0; ) {
        temp1.data.number = i;
        oper1 = POP();
        array_setitem(&new, &temp1, oper1);
        CLEAR(oper1);
    }

    PushArrayRaw(new);
}



void
prim_array_make_dict(PRIM_PROTOTYPE)
{
    stk_array* new;
    int i;

    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_INTEGER)
        abort_interp("Invalid item count.");
    result = oper1->data.number;
    if (result < 0)
        abort_interp("Item count must be positive.");
    CLEAR(oper1);
    nargs = 0;

    if (*top < (2 * result))
        abort_interp("Stack underflow.");
    nargs = 2;

    new = new_array_dictionary();
    for (i = result; i-->0; ) {
        oper1 = POP(); /* val */
        oper2 = POP(); /* key */
        if (oper2->type != PROG_INTEGER && oper2->type != PROG_STRING) {
            array_free(new);
            abort_interp("Keys must be integers or strings.");
        }
        array_setitem(&new, oper2, oper1);
        CLEAR(oper1);
        CLEAR(oper2);
    }

    PushArrayRaw(new);
}



void
prim_array_explode(PRIM_PROTOTYPE)
{
    stk_array* arr;

    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_ARRAY)
        abort_interp("Argument not an array.");
    copyinst(oper1, &temp2);
    arr = temp2.data.array;
    result = array_count(arr);
    CHECKOFLOW(((2 * result) + 1));
    CLEAR(oper1);

    if (array_first(arr, &temp1)) {
        do {
            copyinst(&temp1, &arg[((*top)++)]);
            oper2 = array_getitem(arr, &temp1);
            copyinst(oper2, &arg[((*top)++)]);
            oper2 = NULL;
        } while (array_next(arr, &temp1));
    }

    CLEAR(&temp1);
    CLEAR(&temp2);
    PushInt(result);
}



void
prim_array_vals(PRIM_PROTOTYPE)
{
    stk_array* arr;

    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_ARRAY)
        abort_interp("Argument not an array.");
    copyinst(oper1, &temp2);
    arr = temp2.data.array;
    result = array_count(arr);
    CHECKOFLOW((result + 1));
    CLEAR(oper1);

    if (array_first(arr, &temp1)) {
        do {
            oper2 = array_getitem(arr, &temp1);
            copyinst(oper2, &arg[((*top)++)]);
            oper2 = NULL;
        } while (array_next(arr, &temp1));
    }

    CLEAR(&temp1);
    CLEAR(&temp2);
    PushInt(result);
}



void
prim_array_keys(PRIM_PROTOTYPE)
{
    stk_array* arr;

    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_ARRAY)
        abort_interp("Argument not an array.");
    copyinst(oper1, &temp2);
    arr = temp2.data.array;
    result = array_count(arr);
    CHECKOFLOW((result + 1));
    CLEAR(oper1);

    if (array_first(arr, &temp1)) {
        do {
            copyinst(&temp1, &arg[((*top)++)]);
        } while (array_next(arr, &temp1));
    }

    CLEAR(&temp1);
    CLEAR(&temp2);
    PushInt(result);
}



void
prim_array_count(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_ARRAY)
        abort_interp("Argument not an array.");
    result = array_count(oper1->data.array);

    CLEAR(oper1);
    PushInt(result);
}



void
prim_array_first(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();  /* arr  Array */
    if (oper1->type != PROG_ARRAY)
        abort_interp("Argument not an array.");

    temp1.type = PROG_INTEGER;
    temp1.data.number = 0;
    result = array_first(oper1->data.array, &temp1);

    CLEAR(oper1);
    if (result) {
        copyinst(&temp1, &arg[((*top)++)]);
    } else {
        result = 0;
        PushInt(result);
    }
    PushInt(result);
}



void
prim_array_last(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();  /* arr  Array */
    if (oper1->type != PROG_ARRAY)
        abort_interp("Argument not an array.");

    temp1.type = PROG_INTEGER;
    temp1.data.number = 0;
    result = array_last(oper1->data.array, &temp1);

    CLEAR(oper1);
    if (result) {
        copyinst(&temp1, &arg[((*top)++)]);
    } else {
        result = 0;
        PushInt(result);
    }
    PushInt(result);
}



void
prim_array_prev(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();  /* ???  previous index */
    oper2 = POP();  /* arr  Array */
    if (oper1->type != PROG_INTEGER && oper1->type != PROG_STRING)
        abort_interp("Argument not an integer or string. (2)");
    if (oper2->type != PROG_ARRAY)
        abort_interp("Argument not an array.(1)");

    copyinst(oper1, &temp1);
    result = array_prev(oper2->data.array, &temp1);

    CLEAR(oper1);
    CLEAR(oper2);

    if (result) {
        copyinst(&temp1, &arg[((*top)++)]);
    } else {
        result = 0;
        PushInt(result);
    }
    CLEAR(&temp1);
    PushInt(result);
}



void
prim_array_next(PRIM_PROTOTYPE)
{

    CHECKOP(2);
    oper1 = POP();  /* ???  previous index */
    oper2 = POP();  /* arr  Array */
    if (oper1->type != PROG_INTEGER && oper1->type != PROG_STRING)
        abort_interp("Argument not an integer or string. (2)");
    if (oper2->type != PROG_ARRAY)
        abort_interp("Argument not an array.(1)");

    copyinst(oper1, &temp1);
    result = array_next(oper2->data.array, &temp1);

    CLEAR(oper1);
    CLEAR(oper2);

    if (result) {
        copyinst(&temp1, &arg[((*top)++)]);
    } else {
        result = 0;
        PushInt(result);
    }
    CLEAR(&temp1);
    PushInt(result);
}



void
prim_array_getitem(PRIM_PROTOTYPE)
{
    struct inst *in;

    CHECKOP(2);
    oper1 = POP();  /* ???  index */
    oper2 = POP();  /* arr  Array */
    if (oper1->type != PROG_INTEGER && oper1->type != PROG_STRING)
        abort_interp("Argument not an integer or string. (2)");
    if (oper2->type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");
    in = array_getitem(oper2->data.array, oper1);

    CLEAR(oper1);
    CLEAR(oper2);

	if (in) {
		copyinst(in, &arg[(*top)++]);
	} else {
		result = 0;
		PushInt(result);
	}
}



void
prim_array_setitem(PRIM_PROTOTYPE)
{
    stk_array* new;

    CHECKOP(3);
    oper1 = POP();  /* ???  index to store at */
    oper2 = POP();  /* arr  Array to store in */
    oper3 = POP();  /* ???  item to store     */

    if (oper1->type != PROG_INTEGER && oper1->type != PROG_STRING)
        abort_interp("Argument not an integer or string. (2)");
    if (oper2->type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");

    result = array_setitem(&oper2->data.array, oper1, oper3);
    if (result < 0)
        abort_interp("Index out of array bounds. (3)");

    new = oper2->data.array;
    oper2->data.array = NULL;

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);

    PushArrayRaw(new);
}

void
prim_array_insertitem(PRIM_PROTOTYPE)
{
    stk_array* new;

    CHECKOP(3);
    oper1 = POP();  /* ???  index to store at */
    oper2 = POP();  /* arr  Array to store in */
    oper3 = POP();  /* ???  item to store     */
    if (oper1->type != PROG_INTEGER && oper1->type != PROG_STRING)
        abort_interp("Argument not an integer or string. (2)");
    if (oper2->type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");

    result = array_insertitem(&oper2->data.array, oper1, oper3);
    if (result < 0)
        abort_interp("Index out of array bounds. (3)");

    new = oper2->data.array;
    oper2->data.array = NULL;

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);

    PushArrayRaw(new);
}



void
prim_array_getrange(PRIM_PROTOTYPE)
{
    stk_array* new;

    CHECKOP(3);
    oper1 = POP();  /* int  range end item */
    oper2 = POP();  /* int  range start item */
    oper3 = POP();  /* arr  Array   */
    if (oper1->type != PROG_INTEGER && oper1->type != PROG_STRING)
        abort_interp("Argument not an integer or string. (3)");
    if (oper2->type != PROG_INTEGER && oper2->type != PROG_STRING)
        abort_interp("Argument not an integer or string. (2)");
    if (oper3->type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");

    new = array_getrange(oper3->data.array, oper2, oper1);
    if (!new)
        abort_interp("Bad array range specified.");

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);

    PushArrayRaw(new);
}



void
prim_array_setrange(PRIM_PROTOTYPE)
{
    stk_array* new;

    CHECKOP(3);
    oper1 = POP();  /* arr  array to insert */
    oper2 = POP();  /* ???  starting pos for lists */
    oper3 = POP();  /* arr  Array   */
    if (oper1->type != PROG_ARRAY)
        abort_interp("Argument not an array. (3)");
    if (oper2->type != PROG_INTEGER && oper2->type != PROG_STRING)
        abort_interp("Argument not an integer or string. (2)");
    if (oper3->type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");

    result = array_setrange(&oper3->data.array, oper2, oper1->data.array);
    if (result < 0)
        abort_interp("Index out of array bounds. (2)");

    new = oper3->data.array;
    oper3->data.array = NULL;;

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);

    PushArrayRaw(new);
}


void
prim_array_insertrange(PRIM_PROTOTYPE)
{
    stk_array* new;

    CHECKOP(3);
    oper1 = POP();  /* arr  array to insert */
    oper2 = POP();  /* ???  starting pos for lists */
    oper3 = POP();  /* arr  Array   */
    if (oper1->type != PROG_ARRAY)
        abort_interp("Argument not an array. (3)");
    if (oper2->type != PROG_INTEGER && oper2->type != PROG_STRING)
        abort_interp("Argument not an integer or string. (2)");
    if (oper3->type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");

    result = array_insertrange(&oper3->data.array, oper2, oper1->data.array);
    if (result < 0)
        abort_interp("Index out of array bounds. (2)");

    new = oper3->data.array;
    oper3->data.array = NULL;

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);

    PushArrayRaw(new);
}



void
prim_array_delitem(PRIM_PROTOTYPE)
{
    stk_array* new;

    CHECKOP(2);
    oper1 = POP();  /* int  item to delete */
    oper2 = POP();  /* arr  Array   */
    if (oper1->type != PROG_INTEGER && oper1->type != PROG_STRING)
        abort_interp("Argument not an integer or string. (2)");
    if (oper2->type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");

    result = array_delitem(&oper2->data.array, oper1);
    if (result < 0)
        abort_interp("Bad array index specified.");

    new = oper2->data.array;
    oper2->data.array = NULL;

    CLEAR(oper1);
    CLEAR(oper2);

    PushArrayRaw(new);
}



void
prim_array_delrange(PRIM_PROTOTYPE)
{
    stk_array* new;

    CHECKOP(3);
    oper1 = POP();  /* int  range end item */
    oper2 = POP();  /* int  range start item */
    oper3 = POP();  /* arr  Array   */
    if (oper1->type != PROG_INTEGER && oper1->type != PROG_STRING)
        abort_interp("Argument not an integer or string. (3)");
    if (oper2->type != PROG_INTEGER && oper2->type != PROG_STRING)
        abort_interp("Argument not an integer or string. (2)");
    if (oper3->type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");

    result = array_delrange(&oper3->data.array, oper2, oper1);
    if (result < 0)
        abort_interp("Bad array range specified.");

    new = oper3->data.array;
    oper3->data.array = NULL;

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);

    PushArrayRaw(new);
}


/*\
|*| Eeep!
\*/


void
prim_array_n_union(PRIM_PROTOTYPE)
{
    stk_array* new_union;
    stk_array* new_mash;
    int num_arrays;

    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_INTEGER)
        abort_interp("Invalid item count.");
    result = oper1->data.number;
    if (result < 0)
        abort_interp("Item count must be positive.");
    CLEAR(oper1);
    nargs = 0;

    if (*top < result)
        abort_interp("Stack underflow.");

    if (result>0) {
      new_mash = new_array_dictionary(0);
      for ( num_arrays = 0; num_arrays < result; num_arrays++ ) {
        oper1 = POP();
        array_mash(oper1->data.array, &new_mash, 1 );
        CLEAR(oper1);
      }
      new_union = array_demote_only( new_mash, 1 );
      array_free ( new_mash );
    } else {
      new_union = new_array_packed(0);
    }

    PushArrayRaw(new_union);
}

void
prim_array_n_intersection(PRIM_PROTOTYPE)
{
    stk_array* new_union;
    stk_array* new_mash;
    int num_arrays;

    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_INTEGER)
        abort_interp("Invalid item count.");
    result = oper1->data.number;
    if (result < 0)
        abort_interp("Item count must be positive.");
    CLEAR(oper1);
    nargs = 0;

    if (*top < result)
        abort_interp("Stack underflow.");

    if (result>0) {
      new_mash = new_array_dictionary(0);
      for ( num_arrays = 0; num_arrays < result; num_arrays++ ) {
        oper1 = POP();
        array_mash(oper1->data.array, &new_mash, 1 );
        CLEAR(oper1);
      }
      new_union = array_demote_only( new_mash, result );
      array_free ( new_mash );
    } else {
      new_union = new_array_packed(0);
    }

    PushArrayRaw(new_union);
}

void
prim_array_n_difference(PRIM_PROTOTYPE)
{
    stk_array* new_union;
    stk_array* new_mash;
    int num_arrays;

    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_INTEGER)
        abort_interp("Invalid item count.");
    result = oper1->data.number;
    if (result < 0)
        abort_interp("Item count must be positive.");
    CLEAR(oper1);
    nargs = 0;

    if (*top < result)
        abort_interp("Stack underflow.");

    if (result>0) {
      new_mash = new_array_dictionary(0);

      oper1 = POP();
      array_mash(oper1->data.array, &new_mash, 1 );
      CLEAR(oper1);

      for ( num_arrays = 1; num_arrays < result; num_arrays++ ) {
        oper1 = POP();
        array_mash(oper1->data.array, &new_mash, -1 );
        CLEAR(oper1);
      }
      new_union = array_demote_only( new_mash, 1 );
      array_free ( new_mash );
    } else {
      new_union = new_array_packed(0);
    }

    PushArrayRaw(new_union);
}



void
prim_array_notify(PRIM_PROTOTYPE)
{
    stk_array* strarr;
    stk_array* refarr;
    struct inst *oper1, *oper2, *oper3, *oper4;
    struct inst temp1, temp2;
    char buf2[BUFFER_LEN*2];

    CHECKOP(2);
    oper2 = POP();
    oper1 = POP();
    if (oper1->type != PROG_ARRAY)
        abort_interp("Argument not an array of strings. (1)");
    if (!array_is_homogenous(oper1->data.array, PROG_STRING))
        abort_interp("Argument not an array of strings. (1)");
    if (oper2->type != PROG_ARRAY)
        abort_interp("Argument not an array of dbrefs. (2)");
    if (!array_is_homogenous(oper2->data.array, PROG_OBJECT))
        abort_interp("Argument not an array of dbrefs. (2)");
    strarr = oper1->data.array;
    refarr = oper2->data.array;

    if (array_first(strarr, &temp2)) {
        do {
            oper4 = array_getitem(strarr, &temp2);
            strcpy(buf, DoNullInd(oper4->data.string));
            if (tp_force_mlev1_name_notify && mlev < 2) {
                strcpy(buf2, PNAME(player));
                strcat(buf2, " ");
                if (!string_prefix(buf, buf2)) {
                    strcat(buf2, buf);
                    buf2[BUFFER_LEN-1] = '\0';
                    strcpy(buf, buf2);
                }
            }
            if (array_first(refarr, &temp1)) {
                do {
                    oper3 = array_getitem(refarr, &temp1);

                    notify_listeners(player, program, oper3->data.objref,
                                     getloc(oper3->data.objref), buf, 1);

                    oper3 = NULL;
                } while (array_next(refarr, &temp1));
            }
            oper4 = NULL;
        } while (array_next(strarr, &temp2));
    }

    CLEAR(oper1);
    CLEAR(oper2);
}



void
prim_array_reverse(PRIM_PROTOTYPE)
{
    stk_array* arr;
    stk_array* new;
    int i;

    CHECKOP(1);
    oper1 = POP();  /* arr  Array   */
    if (oper1->type != PROG_ARRAY)
        abort_interp("Argument not an array.");
    arr = oper1->data.array;
    if (arr->type != ARRAY_PACKED)
        abort_interp("Argument must be a list type array.");

    result = array_count(arr);
    new = new_array_packed(result);

    temp1.type = PROG_INTEGER;
    temp2.type = PROG_INTEGER;
    for (i = 0; i < result; i++) {
        temp1.data.number = i;
        temp2.data.number = (result - i) - 1;
        array_setitem(&new, &temp1, array_getitem(arr, &temp2));
    }

    CLEAR(oper1);
    PushArrayRaw(new);
}


