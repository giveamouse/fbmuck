# Generated automatically from Makefile.in by configure.
#!/bin/make -f
# $Header$

subdirs='src'

all:	
	for d in ${subdirs}; do \
		cd $${d} && make all; \
	done

install:
	for d in ${subdirs}; do \
		cd $${d} && make install; \
	done

clean:
	for d in ${subdirs}; do \
		cd $${d} && make clean; \
	done

cleaner:
	for d in ${subdirs}; do \
		cd $${d} && make cleaner; \
	done

#############################################################
# Packaging stuff.
#

TMPPKGDIR=/tmp/tmp_fbmuck_pkg
TMPREV=/tmp/tmp_fbmuck_rev

package:
	rm -rf ${TMPPKGDIR}
	mkdir ${TMPPKGDIR}
	cd src && ./configure
	cd src && make prochelp
	mv -f src/prochelp ${TMPPKGDIR}
	cd src && make cleaner
	cp -fp INSTALLATION README ${TMPPKGDIR}
	ln -s src/CHANGESfb  ${TMPPKGDIR}/CHANGESfb
	ln -s src/CHANGESfb2 ${TMPPKGDIR}/CHANGESfb2
	ln -s src/CHANGESfb3 ${TMPPKGDIR}/CHANGESfb3
	ln -s src/CHANGESfb4 ${TMPPKGDIR}/CHANGESfb4
	ln -s src/CHANGESfb5 ${TMPPKGDIR}/CHANGESfb5
	ln -s src/CHANGESfb6 ${TMPPKGDIR}/CHANGESfb6
	ln -s 'src/==WARNING==' ${TMPPKGDIR}/'==WARNING=='
	cp -fpr docs ${TMPPKGDIR}
	cp -fpr src_docs ${TMPPKGDIR}
	mkdir ${TMPPKGDIR}/include
	cp -fp include/*.h include/*.in ${TMPPKGDIR}/include
	chmod +rw ${TMPPKGDIR}/include/*.h ${TMPPKGDIR}/include/*.in
	mkdir ${TMPPKGDIR}/src
	cp -fp src/configure src/*.in src/*.sh src/*.c ${TMPPKGDIR}/src
	cp -fp src/CHANGES* src/*WARN* src/*WOSSM* ${TMPPKGDIR}/src
	cp -fp src/COMPILING src/BUG_FORM src/Makefile.cfg ${TMPPKGDIR}/src
	cp -fp src/Makefile.cfg ${TMPPKGDIR}/src/Makefile
	chmod +rw ${TMPPKGDIR}/src/*
	chmod +x ${TMPPKGDIR}/src/configure
	mkdir ${TMPPKGDIR}/game
	cp -fp game/restart game/optimdb ${TMPPKGDIR}/game
	mkdir ${TMPPKGDIR}/game/logs
	cp -fp game/logs/README ${TMPPKGDIR}/game/logs
	mkdir ${TMPPKGDIR}/game/muf
	cp -fp game/muf/README ${TMPPKGDIR}/game/muf
	mkdir ${TMPPKGDIR}/game/data
	cp -fp  game/data/README            ${TMPPKGDIR}/game/data
	cp -fp  game/data/*.txt             ${TMPPKGDIR}/game/data
	cp -fp  game/data/*.raw             ${TMPPKGDIR}/game/data
	cp -fp  game/data/minimal.db        ${TMPPKGDIR}/game/data
	cp -fp  game/data/minimal.db.README ${TMPPKGDIR}/game/data
	cp -fpr game/data/man*              ${TMPPKGDIR}/game/data
	cp -fpr game/data/help*             ${TMPPKGDIR}/game/data
	cp -fpr game/data/news*             ${TMPPKGDIR}/game/data
	cp -fpr game/data/info              ${TMPPKGDIR}/game/data
	cp -fpr game/data/mpihelp*          ${TMPPKGDIR}/game/data
	cd ${TMPPKGDIR}/game/data && ${TMPPKGDIR}/prochelp mpihelp.raw mpihelp.txt ../../docs/mpihelp.html
	rm -f ${TMPPKGDIR}/game/data/info/CHANGESfb*
	rm -f ${TMPPKGDIR}/game/data/info/changesfb*
	cp -f src/CHANGESfb  ${TMPPKGDIR}/game/data/info/changesfb
	cp -f src/CHANGESfb2 ${TMPPKGDIR}/game/data/info/changesfb2
	cp -f src/CHANGESfb3 ${TMPPKGDIR}/game/data/info/changesfb3
	cp -f src/CHANGESfb4 ${TMPPKGDIR}/game/data/info/changesfb4
	cp -f src/CHANGESfb5 ${TMPPKGDIR}/game/data/info/changesfb5
	cp -f src/CHANGESfb6 ${TMPPKGDIR}/game/data/info/changesfb6
	head -1 `/bin/ls -1d src/CHANGESfb* | sort -nr | head -1` | cut -c4- > ${TMPREV}
	echo '#define VERSION "Muck2.2'`cat ${TMPREV}`'"' > ${TMPPKGDIR}/include/version.h
	rm -f ${TMPPKGDIR}/prochelp
	find ${TMPPKGDIR} -type d -name CVS -print | xargs -r -t rm -rf
	mv -f ${TMPPKGDIR} `cat ${TMPREV}`
	rm -f `cat ${TMPREV}`.tar.gz
	tar -cf - `cat ${TMPREV}` | gzip -9 > `cat ${TMPREV}`.tar.gz
	rm -rf `cat ${TMPREV}` ${TMPREV}


# #######################################################################
# #######################################################################

# DO NOT DELETE THIS LINE -- make depend depends on it.
