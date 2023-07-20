#==========================================================================#
# This file is part of the Abstract Library for Parallel Search (ALPS).    #
#                                                                          #
# ALPS is distributed under the Eclipse Public License as part of the      #
# COIN-OR repository (http://www.coin-or.org).                             #
#                                                                          #
# Authors:                                                                 #
#                                                                          #
#          Yan Xu, Lehigh University                                       #
#          Ted Ralphs, Lehigh University                                   #
#                                                                          #
# Conceptual Design:                                                       #
#                                                                          #
#          Yan Xu, Lehigh University                                       #
#          Ted Ralphs, Lehigh University                                   #
#          Laszlo Ladanyi, IBM T.J. Watson Research Center                 #
#          Matthew Saltzman, Clemson University                            #
#                                                                          #
#                                                                          #
# Copyright (C) 2001-2023, Lehigh University, Yan Xu, and Ted Ralphs.      #
# All Rights Reserved.                                                     #
#==========================================================================#

## $Id: Makefile.am 788 2006-06-01 18:57:08Z andreasw $

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = doxydoc/doxygen.conf

########################################################################
#                          Subdirectories                              #
########################################################################

# subdirs is set by configure as the list of all subdirectories to recurse
# into
SUBDIRS = $(subdirs)

########################################################################
#                           Extra Targets                              #
########################################################################

test: all
	cd Alps; make test

unitTest: test

tests:
	for dir in $(subdirs); do \
	  if test -r $$dir/test/Makefile; then \
	    (cd $$dir; make test) \
	  fi; \
	done

unitTests: tests

# Generate doxygen doc'n in subdirectories (except @PACKAGE_NAME@) if a doxydoc
# directory is present, then do the base, if present.

doxydoc:
	for dir in $(subdirs) ; do \
	  if test $$dir != @PACKAGE_NAME@ && test -r $$dir/doxydoc ; then \
	    (cd $$dir ; $(MAKE) doxydoc) \
	  fi ; \
	done ; \
	if test -r doxydoc/doxygen.conf ; then \
	  doxygen doxydoc/doxygen.conf ; \
	fi

clean-doxydoc:
	( cd doxydoc ; rm -rf html *.log *.tag )

# DocInstallDir is defined in Makemain.inc and is specific to the package.
# For the short term, adopt the notion that we install only the package
# doxydoc.

install-doxydoc: doxydoc
	if test -r doxydoc/doxygen.conf ; then \
	  $(mkdir_p) $(DocInstallDir) ; \
	  cp -R doxydoc $(DocInstallDir) ; \
	fi

uninstall-doxydoc:
	rm -rf $(DocInstallDir)/doxydoc

clean-local: clean-doxydoc
# install-data-local: install-doxydoc
uninstall-local: uninstall-doxydoc

.PHONY: test unitTest tests unitTests doxydoc

########################################################################
#                         Maintainer Stuff                             #
########################################################################

# Files that are generated and should be cleaned with make distclean
DISTCLEANFILES = coin_subdirs.txt

include BuildTools/Makemain.inc
