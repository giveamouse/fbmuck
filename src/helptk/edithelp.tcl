#!wish8.0

source ./tree.tcl

proc init {args} {
}

init

proc mk_main_window {base} {
    toplevel $base
    wm protocol $base WM_DELETE_WINDOW exit

    image create photo idir -data {
	R0lGODdhEAAQAPIAAAAAAHh4eLi4uPj4APj4+P///wAAAAAAACwAAAAAEAAQAAADPVi63P4w
	LkKCtTTnUsXwQqBtAfh910UU4ugGAEucpgnLNY3Gop7folwNOBOeiEYQ0acDpp6pGAFArVqt
	hQQAO///
    }
    image create photo ifile -data {
	R0lGODdhEAAQAPIAAAAAAHh4eLi4uPj4+P///wAAAAAAAAAAACwAAAAAEAAQAAADPkixzPOD
	yADrWE8qC8WN0+BZAmBq1GMOqwigXFXCrGk/cxjjr27fLtout6n9eMIYMTXsFZsogXRKJf6u
	P0kCADv/
    }

    frame $base.treefr -relief sunk -borderwidth 2
    Tree:create $base.treefr.tree -width 150 -height 200 -yscrollcommand "$base.treefr.scroll set"
    scrollbar $base.treefr.scroll -orient vertical -command "$base.treefr.tree yview"

    pack $base.treefr.tree -side left -fill both -expand 1
    pack $base.treefr.scroll -side left -fill y

    button $base.newsect -text "New Section"
    button $base.editsect -text "Edit Section"
    button $base.newtopic -text "New Topic"
    button $base.edittopic -text "Edit Topic"

    pack $base.treefr -side left -fill both -expand 1
    pack $base.newsect -side top -fill x -padx 10 -pady 5
    pack $base.editsect -side top -fill x -padx 10 -pady 5
    pack $base.newtopic -side top -fill x -padx 10 -pady 5
    pack $base.edittopic -side top -fill x -padx 10 -pady 5
}

proc main {args} {
    wm withdraw .
    mk_main_window .mw

    Tree:newitem .mw.treefr.tree {{Connection Primitives}} -image idir
    Tree:newchild .mw.treefr.tree {{Connection Primitives}} ConNotify -image ifile
    Tree:newchild .mw.treefr.tree {{Connection Primitives}} ConCount -image ifile
    Tree:newitem .mw.treefr.tree {{Math Primitives}} -image idir
    Tree:newchild .mw.treefr.tree {{Math Primitives}} Sin -image ifile
    Tree:newchild .mw.treefr.tree {{Math Primitives}} Acos -image ifile
}
main

