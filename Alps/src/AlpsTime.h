/*===========================================================================*
 * This file is part of the Abstract Library for Parallel Search (ALPS).     *
 *                                                                           *
 * ALPS is distributed under the Common Public License as part of the        *
 * COIN-OR repository (http://www.coin-or.org).                              *
 *                                                                           *
 * Authors: Yan Xu, SAS Institute Inc.                                       *
 *          Ted Ralphs, Lehigh University                                    *
 *          Laszlo Ladanyi, IBM T.J. Watson Research Center                  *
 *          Matthew Saltzman, Clemson University                             *
 *                                                                           * 
 *                                                                           *
 * Copyright (C) 2001-2006, Lehigh University, Yan Xu, and Ted Ralphs.       *
 * Corporation, Lehigh University, Yan Xu, Ted Ralphs, Matthew Salzman and   *
 *===========================================================================*/

#ifndef AlpsTime_
#define AlpsTime_

//#############################################################################

#include "AlpsConfig.h"

#include "CoinTime.hpp"

#ifdef COIN_HAS_MPI
# include "mpi.h"
#endif

//#############################################################################

#define AlpsCpuTime CoinCpuTime

//#############################################################################

static inline double AlpsWallClock()
{

#ifndef COIN_HAS_MPI
    double cpu_temp;
#if defined(_MSC_VER) || defined(__MSVCRT__)
    unsigned int ticksnow;        /* clock_t is same as int */
    ticksnow = (unsigned int)clock();
    cpu_temp = (double)((double)ticksnow/CLOCKS_PER_SEC);
    double sys_temp = 0.;
#else
    double sys_temp;
    struct rusage usage;
    getrusage(RUSAGE_SELF,&usage);
    cpu_temp = usage.ru_utime.tv_sec;
    cpu_temp += 1.0e-6*((double) usage.ru_utime.tv_usec);
    sys_temp = (double) usage.ru_stime.tv_sec
	+ 1.e-6 * (double) usage.ru_stime.tv_usec;
#endif
    return cpu_temp + sys_temp;
#else
    return MPI_Wtime();
#endif
}

//#############################################################################

/* A timer used to record cpu and wallclock time. */
class AlpsTimer 
{
 public:  /* Public for parallecl gather. */
    /** Time limit. */
    double limit_;
    
    double startCpu_;
    double startWall_;
    double finishCpu_;
    double finishWall_;
    
    /** Cpu time. */
    double cpu_;

    /** Wall clock time. */
    double wall_;
    
 public:
    AlpsTimer(): limit_(ALPS_DBL_MAX) { reset(); }
    AlpsTimer(double lt) : limit_(lt) { reset(); }
    ~AlpsTimer()  {}

    /** Reset. */
    void reset() {
	startCpu_ = 0.0;
	startWall_ = 0.0;
	finishCpu_ = 0.0;
	finishWall_ = 0.0;
	cpu_ = 0.0;
	wall_ = 0.0;
    }
    
    /** Start to count times. */
    void start() {
	startCpu_ = AlpsCpuTime();
	startWall_ = AlpsWallClock();
    }
    
    /** Stop timer and computing times. */
    void stop() {
	finishCpu_ = AlpsCpuTime();
	finishWall_ = AlpsWallClock();
	cpu_ = finishCpu_ - startCpu_;
	wall_ = finishWall_ - startWall_;
    }
    
    //{@
    void setLimit(double lm) { limit_ = lm; }
    double getLimit() const { return limit_; }
    //@}

    /** Get cpu timee. */
    double getCpuTime() const { return cpu_; }

    /** Get cpu timee. */
    double getWallClock() const { return wall_; }
    
    /** Check if cpu time reach limit. */
    bool reachCpuLimit() {
	finishCpu_ = AlpsCpuTime();
	if (finishCpu_ - startCpu_ > limit_) {
	    return true;
	}
	else {
	    return false;
	}
    }
    
    /** Check if wallclock time reach limit. */
    bool reachWallLimit() {
	finishWall_ = AlpsWallClock();
	if (finishWall_ - startWall_ > limit_) {
	    return true;
	}
	else {
	    return false;
	}
    }
};

#endif
