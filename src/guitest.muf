@prog cmd-guitest
1 99999 d
1 i
: dlog1-okaybtn-handler
	DESCR descrcon "Okay was pressed!" connotify
;

: dispatch ( strValue dictDests -- )
	dup rot array_getitem
	dup not if
		pop " default" array_getitem
		dup not if
			pop exit
		else
			execute
		then
	else
		swap pop
		execute
	then
;

: dlog1-handler ( dictArgs -- )
	dup "id" array_getitem
	over "event" array_getitem
	"|" swap strcat strcat
	{
		"OkayBtn|buttonpress" 'dlog1-okaybtn-handler
	}dict dispatch
;

: gui-test
	DESCR GUI_AVAILABLE 0.0 > if
		VAR dlog
		descr "Gui test dialog" GUI_DLOG_SIMPLE dlog !
		dlog @ "text" "" { "value" "Hello World!" }dict GUI_CTRL_CREATE
		dlog @ "hrule" "" { }dict GUI_CTRL_CREATE
		dlog @ "button" "OkayBtn" { "value" "Okay" }dict GUI_CTRL_CREATE
		dlog @ GUI_DLOG_SHOW
		begin
			EVENT_WAIT
			{
				"GUI." dlog @ strcat 'dlog1-handler
			}dict dispatch
		repeat
	else
		( Put in old-style config system here. )
	then
;
.
c
q
