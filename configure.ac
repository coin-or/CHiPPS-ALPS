#==========================================================================#
# This file is part of the Abstract Library for Parallel Search (ALPS).    #
#                                                                          #
# ALPS is distributed under the Common Public License as part of the       #
# COIN-OR repository (http://www.coin-or.org).                             #
#                                                                          #
# Authors: Yan Xu, SAS Institute Inc.                                      #
#          Ted Ralphs, Lehigh University                                   #
#          Laszlo Ladanyi, IBM T.J. Watson Research Center                 #
#          Matthew Saltzman, Clemson University                            #
#                                                                          #
#                                                                          #
# Copyright (C) 2001-2006, Lehigh University, Yan Xu, and Ted Ralphs.      #
# All Rights Reserved.                                                     #
#==========================================================================#

## $Id: configure.ac 787 2006-06-01 04:41:29Z andreasw $

#############################################################################
#                       Names and other basic things                        #
#############################################################################

AC_PREREQ(2.59)

AC_INIT([Alps],[0.1],[https://projects.coin-or.org/Alps/newticket])

AC_COPYRIGHT([
Copyright 2006 International Business Machines and others.
All Rights Reserved.
This file is part of the open source package Coin which is distributed
under the Common Public License.])

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
AC_COIN_INIT_AUTOMAKE

#############################################################################
#                  Check which subprojects are there                        #
#############################################################################

AC_COIN_MAIN_SUBDIRS(CoinUtils)
AC_COIN_MAIN_SUBDIRS(Alps)
AC_COIN_MAIN_SUBDIRS(Bcps)
AC_COIN_MAIN_SUBDIRS(Blis)
# The following are required for Alps examples
AC_COIN_MAIN_SUBDIRS(Clp)
AC_COIN_MAIN_SUBDIRS(Osi)
AC_COIN_MAIN_SUBDIRS(Cgl)

##############################################################################
#                   Finishing up by writing all the output                   #
##############################################################################

# Here list all the files that configure should create (except for the
# configuration header file)
AC_CONFIG_FILES([Makefile])

# Finally, we let configure write all the output...
AC_COIN_FINALIZE
