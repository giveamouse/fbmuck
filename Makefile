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


# #######################################################################
# #######################################################################

# DO NOT DELETE THIS LINE -- make depend depends on it.
