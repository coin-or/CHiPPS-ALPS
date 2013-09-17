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

## $Id: configure.ac 787 2006-06-01 04:41:29Z andreasw $

#############################################################################
#                       Names and other basic things                        #
#############################################################################

AC_PREREQ(2.59)

AC_INIT([Alps],[1.4.5],[https://projects.coin-or.org/CHiPPS/newticket])

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
 * Copyright (C) 2001-2010, Lehigh University, Yan Xu, and Ted Ralphs.       *
 *===========================================================================*/
])

AC_REVISION(0.9)

# List one file in the package so that the configure script can test
# whether the package is actually there
AC_CONFIG_SRCDIR(configure.ac)

# Where should everything be installed by default?  Here, we want it
# to be installed directly in 'bin', 'lib', 'include' subdirectories
# of the directory where configure is run.  The default would be
# /usr/local.
AC_PREFIX_DEFAULT([`pwd`])

#############################################################################
#     We only need automake to generate Makefiles for the distribution      #
#############################################################################

# Initialize automake
AC_COIN_CREATE_LIBTOOL

#############################################################################
#                  Check which subprojects are there                        #
#############################################################################

AC_COIN_MAIN_PACKAGEDIR(CoinUtils)
# The following are required for Alps examples
AC_COIN_MAIN_PACKAGEDIR(Blas,ThirdParty,daxpy.f)
AC_COIN_MAIN_PACKAGEDIR(Lapack,ThirdParty,[LAPACK/SRC/dlarf.f])
AC_COIN_MAIN_PACKAGEDIR(Glpk, ThirdParty,[glpk/src/glplpx01.c])
AC_COIN_MAIN_PACKAGEDIR(Sample,Data)
AC_COIN_MAIN_PACKAGEDIR(Osi)
AC_COIN_MAIN_PACKAGEDIR(Clp)
AC_COIN_MAIN_PACKAGEDIR(Cgl)
AC_COIN_MAIN_PACKAGEDIR(Alps)

##############################################################################
#                   Finishing up by writing all the output                   #
##############################################################################

# Here list all the files that configure should create (except for the
# configuration header file)
AC_CONFIG_FILES([Makefile])

# Finally, we let configure write all the output...
AC_COIN_FINALIZE
