To Do list
----------

Current Planned Tasks for fb6.00 (9/30/96):

    Add error trapping to MUF.  ie: ERRCATCH-ERRHANDLE-ERREND blocks.
    Save out changed objects to temp file on prop-loaded queue ejection.
    Eliminate DBSAVE lockups by caching changes in memory, and saving in BG.
    Add threadsafe DB_LOCKING

    Make free frames pool dynamic in size, based on use timestamp.

    Add MUF prim to return how a MUF program instace was instantiated.
    Work with Points and Fre'ta to improve inserver Vehicle handling.
    Perhaps change MOVETO to do conlock checking?

    Investigate inserver player command macros.
    Error msgs in MUF interpreter need cleanup.
    Make Netmuck simpler to compile/install.  Especially for doing upgrades.
    Investigate forbidding X flags on Wizbitted objects.
    Allow exiting from the MUF editor without saving.
    Allow some way to execute commands from inside the muf editor. ie: allow
      poses, says, pages, etc from inside the editor.

    Add DESCR* prims to replace/supplement CON* prims.
    Add keyhole optimizer to MUF.
    Add standardized ~sys/cmd/@teleport/osucc type mesgs for inserver cmds.
    Add lock checking for inserver commands.

    Add NEWPLAYER and TOADPLAYER primitives to MUF.
    Add {moveto} to MPI.
    Add {cutlist} primitive for MPI.  Returns all items BUT the given sublist.

    Document what command @ and stack are in 'man [dis]connect', etc.
    Document running order for @succ, etc & _arrive, _listen, _depart etc.

    Make MPI use allocated strings instead of BUFFER_LEN buffers.
    Improve prop deletion schemes for protected props.
    Add process listing/management primitives.
    Clean up the struct specific fields of objects.

    WAITPROP (d s -- s)  Waits for deletion or change of given property.
    TRIGDESCR ( -- i) relates the descriptor that started this program.

    Implement following MUF prims: Make @tunable format str for list formats.
	GETLISTRANGE (i i d s -- {s})
	SETLISTRANGE ({s} i d s -- )
	CLEARLISTRANGE (i i d s -- )
	MOVELISTRANGE (i i i d s -- )

    Add a way to let programs edit and compile programs.
    Make Properties store privs and ownership.
    Make MUF ARRAYs that are dynamically allocated.




{tell}s > prepending may be broken.  Look into it.  Tell Timur when fixed.

Risseth (Yesterday at 8:45:43PM) -- I found something that I want fixed..  
 Parseprop strips spaces from the beginning and end of a string AFTER parsing 
 it..  It shouldn't, only before.

Kimi okays. A way for _listen to know if an @message/@omessage triggered them,
 first thing. This can be done in the command variable, she thinks. Also, a way
 for _listen to know if it was another program's notify or notify_exclude that
 triggered the _listen, hopefully being able to know WHICH program, as well, as
 say/pose muf's would make the former useless.

Current:
    document the _sys/ properties somewhere.
?   Add .rsplit to $lib/strings
    Make Huh? sysparm message exec for MPI, with dbref 0 perms.

    Change compiler to bytecode style.  Use fn ptrs.

?   Make server force ZOMBIES to look whenever it moves them.  All cases.
?   Make dropto treat zombies like players for triggering.
?   Make dropto not drop zombies.

?   Document the CONTROLS and SYSPARM muf primitives.
    Write mpi-intro docs. (mostly done)

?   Make pose strip starting space for ',:;!?. or space.

    Make building quotas.
    Make _verb/ and _overb/ propqueues.
    Implement control Zones.
    Implement inserver 'look player=object' command, with apropriate lock.
    Implement {case:} to act as case statement.

    Add PROPMATCH (dObj sPropdir sString -- sPropname) exitname style match
    Add NEXT_ENTRANCE (dLinkedto dLastobj -- dNextobj) to scan inlinks.
    Add PCREATE (s s -- d), TOAD (d d -- ),  and NEWPASSWD (d s s -- i) prims.
    LOCKEDPROP?
    PARSEMPISTR
    Add SEED prim to set te random number seed.


  
KNOWN BUGS:
?   SETLINK should allow linking exits and rooms to #-3
?   Make recursion limit in boolexp parsing.  ie no (((...((((( to crash with.
1/2 Make internal MUF addresses use dbref+index instead of memory pointer.


PROBLEMS:
?   Need to make FORCE prim log forces to status file.
    Make it impossible to trigger an _listen in a prog that was itself
      triggered by an _listen propqueue.
?   M1, M2, M3, &c should be be flag aliases for 1, 2, 3, &c.
    Have Timequeues store command @ variable value.
    Need a CON_PRIMS_MLEVEL @tune that sets the MLevel CON* and ONLINE run at.
    Finish the documentation, especially MUF.manual and MUF-tutorial.


PRIMITIVES:
    MATCH_EXIT (s -- d)
    RMATCH_EXIT (d s -- d)
    OMATCH (d s -- d) match from remote object's POV.  Use player for me&here.


ENHANCEMENTS:
    Let smatch() return the unknown parameters.
    Add in Anon funcs. : Fn "cat" "dog" "bat" 3 ': strcmp > ; .sort ;
    Make ticked macros and prims compile to anony funcs.


COMMANDS:
    @unlink <obj>[=<newdest>]  to unlink and relink in one.


CODE CLEANUP:
    Make system only props that are protected.  Move passwords into sysprops.
    Make properties use skiplists. (?)
      ^- Make PUBLIC declarations use faster structure.



