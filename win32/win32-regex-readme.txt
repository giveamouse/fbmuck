For regex support under windows you'll need to download the gnu-regex lib.
Grab it from the url listed below, throw both regex.c and regex.h into the
src directory, rename regex.c to regex.cpp, add regex.cpp to the makefile
and change the #include <regex.h> in p_regex.c to #include "regex.h".

	http://www.gnu.org/directory/regex.html

