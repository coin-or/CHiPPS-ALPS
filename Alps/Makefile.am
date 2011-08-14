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
# Copyright (C) 2001-2011, Lehigh University, Yan Xu, and Ted Ralphs.      #
# All Rights Reserved.                                                     #
#==========================================================================#

## $Id: Makefile.am 788 2006-06-01 18:57:08Z andreasw $

AUTOMAKE_OPTIONS = foreign

########################################################################
#                          Subdirectories                              #
########################################################################

SUBDIRS = src

# We don't want to compile the test subdirectory, unless the test target is
# specified.  But we need to list it as subdirectory to make sure that it is
# included in the tarball

if ALWAYS_FALSE
  SUBDIRS += test
endif

########################################################################
#             Additional files to be included in tarball               #
########################################################################

# Here we need include all files that are not mentioned in other Makefiles

EXTRA_DIST = \
	examples/Abc/AbcBranchActual.cpp \
	examples/Abc/AbcBranchActual.h \
	examples/Abc/AbcBranchBase.cpp \
	examples/Abc/AbcBranchBase.h \
	examples/Abc/AbcCutGenerator.cpp \
	examples/Abc/AbcCutGenerator.h \
	examples/Abc/AbcHeuristic.cpp \
	examples/Abc/AbcHeuristic.h \
	examples/Abc/AbcMain.cpp \
	examples/Abc/AbcMessage.cpp \
	examples/Abc/AbcMessage.h \
	examples/Abc/AbcModel.cpp \
	examples/Abc/AbcModel.h \
	examples/Abc/AbcNodeDesc.h \
	examples/Abc/AbcParams.cpp \
	examples/Abc/AbcParams.h \
	examples/Abc/abc.par.in \
	examples/Abc/abc.pbs \
	examples/Abc/AbcSolution.cpp \
	examples/Abc/AbcSolution.h \
	examples/Abc/AbcTreeNode.cpp \
	examples/Abc/AbcTreeNode.h \
	examples/Abc/AUTHORS \
	examples/Abc/INSTALL \
	examples/Abc/LICENSE \
	examples/Abc/Makefile.in \
	examples/Abc/README \
	examples/Abc/data/egout.mps \
	examples/Abc/data/flugpl.mps \
	examples/Abc/data/gen.mps \
	examples/Abc/data/knap1.mps \
	examples/Abc/data/knap2.mps \
	examples/Abc/data/knap3.mps


########################################################################
#                           Extra Targets                              #
########################################################################

test: all
	cd test; $(MAKE) test

unitTest: test

clean-local:
	if test -r test/Makefile; then cd test; $(MAKE) clean; fi

distclean-local:
	if test -r test/Makefile; then cd test; $(MAKE) distclean; fi

.PHONY: test unitTest

########################################################################
#                  Installation of the addlibs file                    #
########################################################################

pkgconfiglibdir = $(libdir)/pkgconfig
pkgconfiglib_DATA = alps.pc

addlibsdir = $(DESTDIR)$(datadir)/coin/doc/Alps

install-data-hook:
	@$(mkdir_p) "$(addlibsdir)"
if COIN_HAS_PKGCONFIG
	PKG_CONFIG_PATH=@COIN_PKG_CONFIG_PATH@ \
	$(PKG_CONFIG) --libs alps > $(addlibsdir)/alps_addlibs.txt
else
if COIN_CXX_IS_CL
	echo "-libpath:`$(CYGPATH_W) @abs_lib_dir@` libAlps.lib @ALPSLIB_LIBS_INSTALLED@" > $(addlibsdir)/alps_addlibs_cpp.txt
else
	echo -L@abs_lib_dir@ -lAlps @ALPSLIB_LIBS_INSTALLED@ > $(addlibsdir)/alps_addlibs.txt
endif
endif

uninstall-hook:
	rm -f $(addlibsdir)/alps_addlibs.txt

########################################################################
#                         Maintainer Stuff                             #
########################################################################

# Files that are generated and should be cleaned with make distclean
DISTCLEANFILES =

include BuildTools/Makemain.inc
