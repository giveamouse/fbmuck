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
#include "mufevent.h"


struct mufevent_process {
	struct mufevent_process *next;
	dbref player;
	dbref prog;
	struct frame *fr;
} *mufevent_processes;


/* void muf_event_register(dbref player, dbref prog, struct frame* fr)
 * Called when a MUF program enters EVENT_WAIT, to register that
 * the program is ready to process MUF events.
 */
void
muf_event_register(dbref player, dbref prog, struct frame *fr)
{
	struct mufevent_process *newproc;
	struct mufevent_process *ptr;

	newproc = (struct mufevent_process *) malloc(sizeof(struct mufevent_process));

	newproc->player = player;
	newproc->prog = prog;
	newproc->fr = fr;
	newproc->next = NULL;

	ptr = mufevent_processes;
	while (ptr && ptr->next) {
		ptr = ptr->next;
	}
	if (!ptr) {
		mufevent_processes = newproc;
	} else {
		ptr->next = newproc;
	}
}


/* int muf_event_dequeue_pid(int pid)
 * removes the MUF program with the given PID from the EVENT_WAIT queue.
 */
int
muf_event_dequeue_pid(int pid)
{
	struct mufevent_process **prev;
	struct mufevent_process *next;
	int count = 0;

	prev = &mufevent_processes;
	while (*prev) {
		if ((*prev)->fr->pid == pid) {
			next = (*prev)->next;
			muf_event_purge((*prev)->fr);
			prog_clean((*prev)->fr);
			count++;
			free(*prev);
			*prev = next;
		} else {
			prev = &((*prev)->next);
		}
	}
	return count;
}


/* static int event_has_refs(dbref program, struct frame* fr)
 * Checks the MUF event queue for address references on the stack or
 * dbref references on the callstack
 */
static int
event_has_refs(dbref program, struct frame *fr)
{
	int loop;

	for (loop = 1; loop < fr->caller.top; loop++) {
		if (fr->caller.st[loop] == program) {
			return 1;
		}
	}

	for (loop = 0; loop < fr->argument.top; loop++) {
		if (fr->argument.st[loop].type == PROG_ADD &&
			fr->argument.st[loop].data.addr->progref == program) {
			return 1;
		}
	}

	return 0;
}


/* int muf_event_dequeue(dbref prog)
 * Deregisters a program from any instances of it in the EVENT_WAIT queue.
 */
int
muf_event_dequeue(dbref prog)
{
	struct mufevent_process **prev;
	struct mufevent_process *tmp;
	int count = 0;

	prev = &mufevent_processes;
	while (*prev) {
		if (prog == NOTHING || (*prev)->player == prog || (*prev)->prog == prog ||
			event_has_refs(prog, (*prev)->fr)) {
			tmp = *prev;
			*prev = tmp->next;
			muf_event_purge(tmp->fr);
			prog_clean(tmp->fr);
			count++;
			free(tmp);
		} else {
			prev = &((*prev)->next);
		}
	}
	return count;
}



/* int muf_event_controls(dbref player, int pid)
 * Returns true if the given player controls the given PID.
 */
int
muf_event_controls(dbref player, int pid)
{
	struct mufevent_process *tmp;
	struct mufevent_process *ptr = mufevent_processes;

	tmp = ptr;
	while ((ptr) && (pid != ptr->fr->pid)) {
		tmp = ptr;
		ptr = ptr->next;
	}

	if (!ptr) {
		return 0;
	}

	if (!controls(player, ptr->prog) && player != ptr->player) {
		return 0;
	}
	return 1;
}


/* int muf_event_list(dbref player, char* pat)
 * List all processes in the EVENT_WAIT queue that the given player controls.
 * This is used by the @ps command.
 */
int
muf_event_list(dbref player, char *pat)
{
	char buf[BUFFER_LEN];
	int count = 0;
	time_t rtime = time((time_t *) NULL);
	struct mufevent_process *proc = mufevent_processes;

	while (proc) {
		sprintf(buf, pat,
				proc->fr->pid, "--",
				time_format_2((long) (rtime - proc->fr->started)),
				(proc->fr->instcnt / 1000), proc->prog, NAME(proc->player), "EVENT_WAIT");
		if (Wizard(OWNER(player)) || (OWNER(proc->prog) == OWNER(player))
			|| (proc->player == player))
			notify_nolisten(player, buf, 1);
		count++;
		proc = proc->next;
	}
	return count;
}


/* int muf_event_count(struct frame* fr)
 * Returns how many events are waiting to be processed.
 */
int
muf_event_count(struct frame* fr)
{
	struct mufevent *ptr;
	int count = 0;

	for (ptr = fr->events; ptr; ptr = ptr->next)
		count++;

	return count;
}


/* void muf_event_add(struct frame* fr, char* event, struct inst* val)
 * Adds a MUF event to the event queue for the given program instance.
 */
void
muf_event_add(struct frame *fr, char *event, struct inst *val)
{
	struct mufevent *newevent;
	struct mufevent *ptr;

	newevent = (struct mufevent *) malloc(sizeof(struct mufevent));

	newevent->event = string_dup(event);
	copyinst(val, &newevent->data);
	newevent->next = NULL;

	ptr = fr->events;
	while (ptr && ptr->next) {
		ptr = ptr->next;
	}
	if (!ptr) {
		fr->events = newevent;
	} else {
		ptr->next = newevent;
	}
}



/* static void muf_event_free(struct mufevent* ptr)
 * Frees up a MUF event once you are done with it.  This shouldn't be used
 * outside this module.
 */
static void
muf_event_free(struct mufevent *ptr)
{
	CLEAR(&ptr->data);
	free(ptr->event);
	ptr->event = NULL;
	ptr->next = NULL;
	free(ptr);
}


/* static struct mufevent* muf_event_pop(struct frame* fr)
 * This pops the top muf event off of the given program instance's
 * event queue, and returns it to the caller.
 */
static struct mufevent *
muf_event_pop(struct frame *fr)
{
	struct mufevent *ptr = NULL;

	if (fr->events) {
		ptr = fr->events;
		fr->events = fr->events->next;
	}
	return ptr;
}



/* void muf_event_purge(struct frame* fr)
 * purges all muf events from the given program instance's event queue.
 */
void
muf_event_purge(struct frame *fr)
{
	while (fr->events) {
		muf_event_free(muf_event_pop(fr));
	}
}



/* void muf_event_process()
 * For all program instances who are in the EVENT_WAIT queue,
 * check to see if they have any items in their event queue.
 * If so, then process one each.  Up to ten programs can have
 * events processed at a time.
 */
void
muf_event_process()
{
	int limit = 10;
	struct mufevent_process *proc;
	struct mufevent_process *next;
	struct mufevent_process **prev;
	struct mufevent_process **nextprev;
	struct mufevent *ev;
	dbref tmpcp;
	int tmpbl;
	int tmpfg;

	proc = mufevent_processes;
	prev = &mufevent_processes;
	while (proc && limit > 0) {
		nextprev = &((*prev)->next);
		next = proc->next;
		if (proc->fr) {
			ev = muf_event_pop(proc->fr);
			if (ev) {
				limit--;

				nextprev = prev;
				*prev = proc->next;

				if (proc->fr->argument.top + 1 >= STACK_SIZE) {
					/*
					 * Uh oh! That MUF program's stack is full!
					 * Print an error, free the frame, and exit.
					 */
					notify_nolisten(proc->player, "Program stack overflow.", 1);
					prog_clean(proc->fr);
				} else {
					tmpcp = PLAYER_CURR_PROG(proc->player);
					tmpbl = PLAYER_BLOCK(proc->player);
					tmpfg = (proc->fr->multitask != BACKGROUND);

					copyinst(&ev->data, &(proc->fr->argument.st[proc->fr->argument.top]));
					proc->fr->argument.top++;
					push(proc->fr->argument.st, &(proc->fr->argument.top),
						 PROG_STRING, MIPSCAST alloc_prog_string(ev->event));

					interp_loop(proc->player, proc->prog, proc->fr, 0);

					if (!tmpfg) {
						PLAYER_SET_BLOCK(proc->player, tmpbl);
						PLAYER_SET_CURR_PROG(proc->player, tmpcp);
					}
				}
				muf_event_free(ev);

				proc->fr = NULL;
				proc->next = NULL;
				free(proc);
			}
		}
		prev = nextprev;
		proc = next;
	}
}
