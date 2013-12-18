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
# Copyright (C) 2001-2013, Lehigh University, Yan Xu, and Ted Ralphs.      #
# All Rights Reserved.                                                     #
#==========================================================================#

## $Id: configure.ac 786 2006-06-01 04:10:46Z andreasw $

#############################################################################
#                       Names and other basic things                        #
#############################################################################

AC_PREREQ(2.59)

AC_INIT([Alps],[1.4.8],[https://projects.coin-or.org/CHiPPS/newticket])

AC_COPYRIGHT([
/*===========================================================================*
 * This file is part of the Abstract Library for Parallel Search (ALPS).     *
 *                                                                           *
 * ALPS is distributed under the Eclipse Public License as part of the       *
 * COIN-OR repository (http://www.coin-or.org).                              *
 *                                                                           *
 * Authors:                                                                  *
 *                                                                           *
 *          Yan Xu, Lehigh University                                        *
 *          Ted Ralphs, Lehigh University                                    *
 *                                                                           *
 * Conceptual Design:                                                        *
 *                                                                           *
 *          Yan Xu, Lehigh University                                        *
 *          Ted Ralphs, Lehigh University                                    *
 *          Laszlo Ladanyi, IBM T.J. Watson Research Center                  *
 *          Matthew Saltzman, Clemson University                             *
 *                                                                           *
 *                                                                           *
 * Copyright (C) 2001-2011, Lehigh University, Yan Xu, and Ted Ralphs.       *
 *===========================================================================*/
])

# List one file in the package so that the configure script can test
# whether the package is actually there
AC_CONFIG_SRCDIR(src/Alps.h)

# Where should everything be installed by default?  Here, we want it
# to be installed directly in 'bin', 'lib', 'include' subdirectories
# of the directory where configure is run.  The default would be
# /usr/local.
AC_PREFIX_DEFAULT([`pwd`])

#############################################################################
#                         Standard build tool stuff                         #
#############################################################################

# Get the system type
AC_CANONICAL_BUILD

# If this project depends on external projects, the Externals file in
# the source root directory contains definition of where to find those
# externals.  The following macro ensures that those externals are
# retrieved by svn if they are not there yet.
AC_COIN_PROJECTDIR_INIT(Alps,7:8:4)

# Check if user wants to produce debugging code
AC_COIN_DEBUG_COMPILE(Alps)

# Get the name of the C++ compiler and appropriate compiler options
AC_COIN_PROG_CXX

# We filter out the -pedantic-errors flag for C and C++ compiler in
# case the user wants to use MPI, since otherwise the compilation
# fails. 
if test "${with_mpi_lib+set}" = set; then
  CFLAGS=`echo $CFLAGS | sed -e s/-pedantic-errors//g` 
  CXXFLAGS=`echo $CXXFLAGS | sed -e s/-pedantic-errors//g` 
fi

# Initialize automake and libtool
AC_COIN_INIT_AUTO_TOOLS

# Get the EXEXT variable for CYGWIN
AC_EXEEXT

#############################################################################
#                           COIN-OR components                              #
#############################################################################

AC_COIN_CHECK_PACKAGE(CoinUtils, [coinutils], [AlpsLib])
if test $coin_has_coinutils != yes ; then
  AC_MSG_ERROR([Required package CoinUtils not available.])
fi

# These are only required for the unit test and the Abc example.
# We name it Clp, because the code checks for COIN_HAS_CLP
AC_COIN_CHECK_PACKAGE(Clp, [cgl osi-clp], [Abc])

#############################################################################
#                             Other dependencies                            #
#############################################################################

AC_COIN_CHECK_USER_LIBRARY([Mpi], [MPI], [mpi.h], [MPI_Irecv], [], [AlpsLib])
# ToDo: Automatically choose MPI compiler

#############################################################################
#                  System header                                            #
#############################################################################

AC_COIN_CHECK_CXX_CHEADER(math)
AC_COIN_CHECK_CXX_CHEADER(float)
AC_COIN_CHECK_CXX_CHEADER(ieeefp)

##############################################################################
#                   VPATH links for example input files                      #
##############################################################################

# In this case, the source code is taken from the Alps examples directory

AC_CONFIG_LINKS(test/AbcBranchActual.cpp:examples/Abc/AbcBranchActual.cpp
                test/AbcBranchActual.h:examples/Abc/AbcBranchActual.h
                test/AbcBranchBase.cpp:examples/Abc/AbcBranchBase.cpp
                test/AbcBranchBase.h:examples/Abc/AbcBranchBase.h
                test/AbcCutGenerator.cpp:examples/Abc/AbcCutGenerator.cpp
                test/AbcCutGenerator.h:examples/Abc/AbcCutGenerator.h
                test/AbcHeuristic.cpp:examples/Abc/AbcHeuristic.cpp
                test/AbcHeuristic.h:examples/Abc/AbcHeuristic.h
                test/AbcMain.cpp:examples/Abc/AbcMain.cpp
                test/AbcMessage.cpp:examples/Abc/AbcMessage.cpp
                test/AbcMessage.h:examples/Abc/AbcMessage.h
                test/AbcModel.cpp:examples/Abc/AbcModel.cpp
                test/AbcModel.h:examples/Abc/AbcModel.h
                test/AbcNodeDesc.h:examples/Abc/AbcNodeDesc.h
                test/AbcParams.cpp:examples/Abc/AbcParams.cpp
                test/AbcParams.h:examples/Abc/AbcParams.h
                test/AbcSolution.cpp:examples/Abc/AbcSolution.cpp
                test/AbcSolution.h:examples/Abc/AbcSolution.h
                test/AbcTreeNode.cpp:examples/Abc/AbcTreeNode.cpp
                test/AbcTreeNode.h:examples/Abc/AbcTreeNode.h)

AC_COIN_LINK_INPUT_CMD

mkdir -p test
EXAMPLE_FILES="flugpl.mps"
for file in $EXAMPLE_FILES; do
  rm -f $file
  $coin_link_input_cmd `cd $srcdir/examples/Abc/data; pwd`/$file test/$file
done

# In case this is a VPATH configuration we need to make sure that the
# input files for the examples are available in the VPATH directory.

# ToDo
#AC_COIN_VPATH_LINK(examples/Abc/abc.par)
#AC_COIN_VPATH_LINK(examples/input.130)

##############################################################################
#                   Finishing up by writing all the output                   #
##############################################################################

# Here list all the files that configure should create (except for the
# configuration header file)
AC_CONFIG_FILES([Makefile
		 examples/Abc/Makefile
		 examples/Abc/abc.par
		 examples/Knap/Makefile
		 examples/Knap/knap.par
                 src/Makefile
                 test/Makefile
		 alps.pc
		 alps-uninstalled.pc])

# Here put the location and name of the configuration header file
AC_CONFIG_HEADER([src/config.h src/config_alps.h])

# Finally, we let configure write all the output...
AC_COIN_FINALIZE
