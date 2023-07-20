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


## $Id: configure.ac 786 2006-06-01 04:10:46Z andreasw $

#############################################################################
#                       Names and other basic things                        #
#############################################################################

AC_INIT([Alps],[devel],[https://github.com/coin-or/CHiPPS-ALPS/issues/new],[coin-or-alps],[https://github.com/coin-or/CHiPPS-ALPS])

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
 *          Aykut Bulut, Lehigh University                                   *
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
 * Copyright (C) 2001-2023, Lehigh University, Yan Xu, and Ted Ralphs.       *
 *===========================================================================*/
])

# List one file in the package so that the configure script can test
# whether the package is actually there
AC_CONFIG_SRCDIR(src/Alps.h)

# Do some initialization work (version numbers, change prefix default, ...)
AC_COIN_INITIALIZE

#############################################################################
#                         Standard build tool stuff                         #
#############################################################################

# Get the name of the C++ compiler and appropriate compiler options.
AC_COIN_PROG_CXX

# This is a C++ package, set the language accordingly.
AC_LANG_PUSH(C++)

# Initialize libtool
AC_COIN_PROG_LIBTOOL

# set RPATH_FLAGS to the compiler link flags required to hardcode location
# of the shared objects (expanded_libdir is set somewhere in configure before)
# (use in examples Makefile)
AC_COIN_RPATH_FLAGS([$expanded_libdir])

# We filter out the -pedantic-errors flag for C and C++ compiler in
# case the user wants to use MPI, since otherwise the compilation
# fails.
#if test "${with_mpi_lib+set}" = set; then
#  CFLAGS=`echo $CFLAGS | sed -e s/-pedantic-errors//g`
#  CXXFLAGS=`echo $CXXFLAGS | sed -e s/-pedantic-errors//g`
#fi

#############################################################################
#                   Math headers                                            #
#############################################################################

# Check for cmath/math.h, cfloat/float.h, cieeefp/ieeefp.h
AC_COIN_CHECK_MATH_HDRS

#############################################################################
#                           COIN-OR components                              #
#############################################################################

AC_COIN_CHK_PKG(CoinUtils,[AlpsLib])
if test $coin_has_coinutils != yes ; then
  AC_MSG_ERROR([Required package CoinUtils not available.])
fi

# These are only required for the unit test and the Abc example.
AC_COIN_CHK_PKG(Osi, [Abc])
AC_COIN_CHK_PKG(Clp, [Abc], [osi-clp])
AC_COIN_CHK_PKG(Cgl, [Abc])

#############################################################################
#                             Other dependencies                            #
#############################################################################

AC_COIN_CHK_LIB(MPI,[AlpsLib],[-lmpi],[],[],[MPI_Irecv],[#include "mpi.h"],
                [no])
# ToDo: Automatically choose MPI compiler

##############################################################################
#                   VPATH links for example input files                      #
##############################################################################

# In this case, the source code is taken from the Alps examples directory

AC_COIN_VPATH_LINK(test/AbcBranchActual.cpp:examples/Abc/AbcBranchActual.cpp
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
                   test/AbcTreeNode.h:examples/Abc/AbcTreeNode.h
                   test/flugpl.mps:examples/Abc/data/flugpl.mps)
   
#############################################################################
#                  Check for doxygen                                        #
#############################################################################

AC_COIN_DOXYGEN([CoinUtils])

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
                 alps.pc])

AC_CONFIG_FILES([doxydoc/doxygen.conf])

# Here put the location and name of the configuration header file
AC_CONFIG_HEADERS([src/config.h src/config_alps.h])

# Finalize libs and cflags.
AC_COIN_FINALIZE_FLAGS([AlpsLib Abc])

# Finally, we let configure write all the output...
AC_COIN_FINALIZE
