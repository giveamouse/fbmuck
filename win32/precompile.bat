@echo off
cd ..

echo Renaming .C to .CPP
cd src
ren *.c *.cpp > nul:
cd ..
echo Making compile directory...
mkdir compile > nul:
echo Moving autoconf.h...
copy /Y .\win32\autoconf.h .\include\autoconf.h > nul:
echo Moving makefile...
copy /Y .\win32\makefile.win .\makefile > nul:
echo Moving win32.h...
copy /Y .\win32\win32.h .\include\win32.h > nul:
echo Moving win32.cpp...
copy /Y .\win32\win32.cpp .\src\win32.cpp > nul:
echo Remember to setup your VCVARS before typing 'nmake'
cd win32
