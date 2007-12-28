# Copyright (C) 2006 International Business Machines and others.
# All Rights Reserved.
# This file is distributed under the Common Public License.

## $Id: Makefile.am 790 2006-06-03 03:20:11Z andreasw $

# Author:  Andreas Waechter           IBM    2006-04-13

AUTOMAKE_OPTIONS = foreign

########################################################################
#                               libAlps                                #
########################################################################

# Name of the library compiled in this directory.  We want it to be installed
# in the 'lib' directory
lib_LTLIBRARIES = libAlps.la

# List all source files for this library, including headers
libAlps_la_SOURCES = \
	AlpsConfig.h \
	AlpsAix43.h \
	AlpsSearchStrategy.cpp AlpsSearchStrategy.h \
	AlpsSearchStrategyBase.h \
	AlpsCygwin.h \
	AlpsEncoded.h \
	AlpsEnumProcessT.h \
	Alps.h \
	AlpsHelperFunctions.h \
	AlpsKnowledgeBroker.cpp AlpsKnowledgeBroker.h \
	AlpsKnowledge.cpp AlpsKnowledge.h \
	AlpsKnowledgePool.h \
	AlpsLicense.h \
	AlpsLinux.h \
	AlpsMACH.h \
	AlpsMessage.cpp AlpsMessage.h \
	AlpsMessageTag.h \
	AlpsModel.cpp AlpsModel.h \
	AlpsNodeDesc.h \
	AlpsNodePool.h \
	AlpsOs.h \
	AlpsParameterBase.cpp AlpsParameterBase.h \
	AlpsParams.cpp AlpsParams.h \
	AlpsPriorityQueue.h \
	AlpsSolution.h \
	AlpsSolutionPool.h \
	AlpsSubTree.cpp AlpsSubTree.h \
	AlpsSubTreePool.h \
	AlpsSunos.h \
	AlpsTime.h \
	AlpsTreeNode.cpp AlpsTreeNode.h

if COIN_HAS_MPI
libAlps_la_SOURCES += AlpsKnowledgeBrokerMPI.cpp AlpsKnowledgeBrokerMPI.h
else
libAlps_la_SOURCES += AlpsKnowledgeBrokerSerial.cpp AlpsKnowledgeBrokerSerial.h
endif

# This is for libtool (on Windows)
libAlps_la_LDFLAGS = $(LT_LDFLAGS)

########################################################################
#                            Additional flags                          #
########################################################################

# Here list all include flags, relative to this "srcdir" directory.  This
# "cygpath" stuff is necessary to compile with native compilers on Windows.
AM_CPPFLAGS = \
	-I`$(CYGPATH_W) $(COINUTILSSRCDIR)/src` \
	-I`$(CYGPATH_W) $(COINUTILSOBJDIR)/inc`

# Include MPI header directory if MPI is used
if COIN_HAS_MPI
AM_CPPFLAGS +=  -I`$(CYGPATH_W) $(MPIINCDIR)`
endif

# This line is necessary to allow VPATH compilation with MS compilers
# on Cygwin
DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)` -I$(top_builddir)/inc

########################################################################
#                Headers that need to be installed                     #
########################################################################

# Here list all the header files that are required by a user of the library,
# and that therefore should be installed in 'install/coin'
includecoindir = $(includedir)/coin
includecoin_HEADERS = \
	../inc/config_alps.h \
	Alps.h \
	AlpsSearchStrategy.h \
	AlpsSearchStrategyBase.h \
	AlpsEncoded.h \
	AlpsEnumProcessT.h \
	AlpsHelperFunctions.h \
	AlpsKnowledge.h \
	AlpsKnowledgeBroker.h \
	AlpsKnowledgeBrokerMPI.h \
	AlpsKnowledgeBrokerSerial.h \
	AlpsKnowledgePool.h \
	AlpsMessage.h \
	AlpsModel.h \
	AlpsNodeDesc.h \
	AlpsNodePool.h \
	AlpsParams.h \
	AlpsParameterBase.h \
	AlpsPriorityQueue.h \
	AlpsSolution.h \
	AlpsSolutionPool.h \
	AlpsSubTree.h \
	AlpsSubTreePool.h \
	AlpsTime.h \
	AlpsTreeNode.h

#############################################################################
# Create the Config.h file that always defines HAVE_CONFIG_H and install it #
#############################################################################

# You only need to adapt the following line
ConfigHeader = AlpsConfig.h

install-exec-local:
	echo "#ifndef HAVE_CONFIG_H" >bla
	echo "#define HAVE_CONFIG_H" >>bla
	echo "#endif" >> bla
	cat $(srcdir)/$(ConfigHeader) >> bla
	$(install_sh_DATA) bla $(DESTDIR)$(includecoindir)/$(ConfigHeader)
	rm -f bla

uninstall-local:
	rm -f $(DESTDIR)$(includecoindir)/$(ConfigHeader)
