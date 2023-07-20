#=============================================================================#
# This file is part of the Abstract Library for Parallel Search (ALPS).       #
#                                                                             #
# ALPS is distributed under the Eclipse Public License as part of the         #
# COIN-OR repository (http://www.coin-or.org).                                #
#                                                                             #
# Authors:                                                                    #
#                                                                             #
#          Yan Xu, Lehigh University                                          #
#          Aykut Bulut, Lehigh University                                     #
#          Ted Ralphs, Lehigh University                                      #
#                                                                             #
# Conceptual Design:                                                          #
#                                                                             #
#          Yan Xu, Lehigh University                                          #
#          Ted Ralphs, Lehigh University                                      #
#          Laszlo Ladanyi, IBM T.J. Watson Research Center                    #
#          Matthew Saltzman, Clemson University                               #
#                                                                             #
#                                                                             #
# Copyright (C) 2001-2023, Lehigh University, Yan Xu, Aykut Bulut, and        #
#                          Ted Ralphs.                                        #
# All Rights Reserved.                                                        #
#=============================================================================#


## $Id: Makefile.am 507 2006-06-03 03:20:16Z andreasw $

# Author:  Yan Xu Lehigh

########################################################################
#                               libAlps                                #
########################################################################

# Name of the library compiled in this directory.
lib_LTLIBRARIES = libAlps.la

# List all source files for this library, including headers
libAlps_la_SOURCES = \
	AlpsConfig.h \
	AlpsAix43.h \
	AlpsCygwin.h \
	AlpsLicense.h \
	AlpsLinux.h \
	AlpsMACH.h \
	AlpsOs.h \
	AlpsSunos.h \
	AlpsTime.h \
	Alps.h \
	AlpsEncoded.h \
	AlpsEnumProcessT.h \
	AlpsHelperFunctions.h \
	AlpsParameterBase.h \
	AlpsParameterBase.cpp \
	AlpsParams.h \
	AlpsParams.cpp \
	AlpsMessageTag.h \
	AlpsMessage.h \
	AlpsMessage.cpp \
	AlpsKnowledge.h \
	AlpsKnowledge.cpp \
	AlpsPriorityQueue.h \
	AlpsKnowledgePool.h \
	AlpsNodeDesc.h \
	AlpsTreeNode.h \
	AlpsTreeNode.cpp \
	AlpsNodePool.h \
	AlpsNodePool.cpp \
	AlpsSolution.h \
	AlpsSolutionPool.h \
	AlpsSolutionPool.cpp \
	AlpsSubTree.h \
	AlpsSubTree.cpp \
	AlpsSubTreePool.h \
	AlpsSubTreePool.cpp \
	AlpsKnowledgeBroker.h \
	AlpsKnowledgeBroker.cpp \
	AlpsSearchStrategyBase.h \
	AlpsSearchStrategy.h \
	AlpsSearchStrategy.cpp \
	AlpsModel.h \
	AlpsModel.cpp

if COIN_HAS_MPI
libAlps_la_SOURCES += AlpsKnowledgeBrokerMPI.cpp AlpsKnowledgeBrokerMPI.h
else
libAlps_la_SOURCES += AlpsKnowledgeBrokerSerial.cpp AlpsKnowledgeBrokerSerial.h
endif

libAlps_la_LIBADD = $(ALPSLIB_LFLAGS)

libAlps_la_CPPFLAGS = $(ALPSLIB_CFLAGS)

########################################################################
#                            Additional flags                          #
########################################################################

# Use additional libtool flags
AM_LDFLAGS = $(LT_LDFLAGS)

########################################################################
#                Headers that need to be installed                     #
########################################################################

# Here list all the header files that are required by a user of the library,
# and that therefore should be installed in 'include/coin'
includecoindir = $(includedir)/coin-or
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
