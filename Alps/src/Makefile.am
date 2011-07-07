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

## $Id: Makefile.am 507 2006-06-03 03:20:16Z andreasw $

# Author:  Yan Xu Lehigh

AUTOMAKE_OPTIONS = foreign

########################################################################
#                               libAlps                                #
########################################################################

# Name of the library compiled in this directory.
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

# This is for libtool
libAlps_la_LDFLAGS = $(LT_LDFLAGS)

########################################################################
#                            Additional flags                          #
########################################################################

# Here list all include flags, relative to this "srcdir" directory.  This
# "cygpath" stuff is necessary to compile with native compilers on Windows.
AM_CPPFLAGS = $(ALPSLIB_CFLAGS) 

# Include MPI header directory if MPI is used
if COIN_HAS_MPI
AM_CPPFLAGS +=  -I`$(CYGPATH_W) $(MPIINCDIR)`
endif

# This line is necessary to allow VPATH compilation with MS compilers
# on Cygwin
DEFAULT_INCLUDES = -I. -I`$(CYGPATH_W) $(srcdir)` 

########################################################################
#                Headers that need to be installed                     #
########################################################################

# Here list all the header files that are required by a user of the library,
# and that therefore should be installed in 'include/coin'
includecoindir = $(includedir)/coin
includecoin_HEADERS = \
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

install-exec-local:
	$(install_sh_DATA) config_alps.h $(DESTDIR)$(includecoindir)/AlpsConfig.h

uninstall-local:
	rm -f $(DESTDIR)$(includecoindir)/AlpsConfig.h
