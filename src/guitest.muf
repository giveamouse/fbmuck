$include $lib/gui
$def tell descrcon swap connotify
  
: generic-handler (intDescr strDlogID strCtrlID strEvent -- intExit)
    var guievent guievent !
    var ctrlid ctrlid !
    var dlogid dlogid !
    var dscr dscr !
    var vals dlogid @ GUI_VALUES_GET vals !
    
    guievent @ ctrlid @ "%s sent %s event!" fmtstring dscr @ tell 
  
    vals @ foreach
        swap "=" strcat dscr @ tell
        foreach
            "    " swap strcat dscr @ tell
            pop
        repeat
    repeat
    0
;
  
: cancelbtn-handler (intDescr strDlogID strCtrlId strEvent -- intExit)
    pop pop pop "Dialog cancelled!" swap tell
    0
;
  
: gen_writer_dlog ( -- dictHandlers strDlogId )
    {SIMPLE_DLOG "Post Message"
        {LABEL ""
            "value" "Subject"
            "newline" 0
            }CTRL
        {EDIT "subj"
            "value" "This is a subject"
            "sticky" "ew"
            }CTRL
        {LABEL ""
            "value" "Keywords"
            "newline" 0
            }CTRL
        {EDIT "keywd"
            "value" "Default keywords"
            "sticky" "ew"
            "hweight" 1
            }CTRL
        {MULTIEDIT "body"
            "value" ""
            "width" 80
            "height" 20
            "colspan" 2
            }CTRL
        {FRAME "bfr"
            "sticky" "ew"
            "colspan" 2
            {BUTTON "PostBtn"
                "text" "Post"
                "width" 8
                "sticky" "e"
                "hweight" 1
                "newline" 0
                "|buttonpress" 'generic-handler
                }CTRL
            {BUTTON "CancelBtn"
                "text" "Cancel"
                "width" 8
                "sticky" "e"
                "|buttonpress" 'cancelbtn-handler
                }CTRL
        }CTRL
        "|_closed|buttonpress" 'cancelbtn-handler
    }DLOG
    DESCR swap GUI_GENERATE
    dup GUI_DLOG_SHOW
;
 
: gen_reader_dlog ( -- dictHandlers strDlogId )
    {SIMPLE_DLOG "Read Messages"
        {FRAME "foo"
        {LISTBOX "msgs"
            "value" "0"
            "sticky" "nswe"
            "options" { "first" "second" "third" }list
            "report" 1
            "height" 5
            "newline" 0
            }CTRL
        {FRAME "bfr"
            "sticky" "nsew"
            {BUTTON "WriteBtn"
                "text" "Write New"
                "width" 8
                "sticky" "n"
                "dismiss" 0
                "|buttonpress" 'generic-handler
                }CTRL
            {BUTTON "DelBtn"
                "text" "Delete"
                "width" 8
                "sticky" "n"
                "dismiss" 0
                "|buttonpress" 'generic-handler
                }CTRL
            {BUTTON "ProtectBtn"
                "text" "Protect"
                "width" 8
                "sticky" "n"
                "vweight" 1
                "dismiss" 0
                "|buttonpress" 'generic-handler
                }CTRL
            }CTRL
        {FRAME "header"
            "sticky" "ew"
            "colspan" 2
            {LABEL "from"
                "value" "Revar"
                "sticky" "w"
                "width" 16
                "newline" 0
                }CTRL
            {LABEL "subj"
                "value" "This is a subject."
                "sticky" "w"
                "newline" 0
                "hweight" 1
                }CTRL
            {LABEL "date"
                "value" "Fri Dec 24 15:52:30 PST 1999"
                "sticky" "e"
                "hweight" 1
                }CTRL
            }CTRL
        {MULTIEDIT "body"
            "value" ""
            "width" 80
            "height" 20
            "readonly" 1
            "hweight" 1
            "toppad" 0
            "colspan" 2
            }CTRL
        "|_closed|buttonpress" 'cancelbtn-handler
        }CTRL
    }DLOG
    DESCR swap GUI_GENERATE
    dup GUI_DLOG_SHOW
;
 
: testdlog
    {SIMPLE_DLOG "Button dlog"
        {BUTTON "TestBtn"
            "text" "Test"
            "width" 8
            "|buttonpress" 'generic-handler
            }CTRL
    }DLOG
    DESCR swap GUI_GENERATE
    dup GUI_DLOG_SHOW
;
 
: gui_test
    pop
    DESCR GUI_AVAILABLE 0.0 > if
        background
        {
            gen_reader_dlog swap
            gen_writer_dlog swap
            (testdlog swap)
        }dict
        {
            (no other events to watch for)
        }dict
        gui_event_process
    else
        ( Put in old-style config system here. )
        DESCR descrcon "Gui not supported!" connotify
    then
;
 
