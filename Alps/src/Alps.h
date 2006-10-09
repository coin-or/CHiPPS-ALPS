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
 * All Rights Reserved.                                                      *
 *===========================================================================*/

#ifndef Alps_h_
#define Alps_h_

#include <cfloat>
#include <cstdio>

// Test

//#############################################################################
/** The possible stati for the search nodes. */
//#############################################################################

enum AlpsNodeStatus {
    AlpsNodeStatusCandidate,
    AlpsNodeStatusEvaluated,
    AlpsNodeStatusPregnant,
    AlpsNodeStatusBranched,
    AlpsNodeStatusFathomed
};

//#############################################################################
/** Type of knowledge like solution, node, cut...*/
//#############################################################################

enum AlpsKnowledgeType { 
  ALPS_MODEL, 
  ALPS_NODE, 
  ALPS_SOLUTION,
  ALPS_SUBTREE
};

//#############################################################################
// Search return status
//#############################################################################

enum AlpsSolStatus {
    ALPS_OPTIMAL = 0,
    ALPS_TIME_LIMIT, 
    ALPS_NODE_LIMIT,
    ALPS_FEASIBLE,
    ALPS_INFEASIBLE,
    ALPS_UNBOUNDED
};

//#############################################################################
// Return code.
//#############################################################################

enum AlpsReturnCode {
    ALPS_OK = 0,
    ALPS_ERR_NO_INT,  /* No integer variable.*/
    ALPS_ERR_NO_MEM
};

//#############################################################################
// Seach phase
//#############################################################################

enum AlpsPhase {
    ALPS_PHASE_RAMPUP = 0,
    ALPS_PHASE_SEARCH,
    ALPS_PHASE_RAMPDOWN
};

//#############################################################################
// Big number
//#############################################################################

#define ALPS_DBL_MAX          DBL_MAX
#define ALPS_INC_MAX          1.0e80
#define ALPS_OBJ_MAX          1.0e75
#define ALPS_BND_MAX          1.0e20
#define ALPS_INFINITY         1.0e20

#define ALPS_INT_MAX          INT_MAX

//#############################################################################
// Small number
//#############################################################################

#define ALPS_ZERO             1.0e-14
#define ALPS_GEN_TOL          1.0e-6
#define ALPS_QUALITY_TOL      1.0e-5

//#############################################################################

#define ALPS_PRINTF           printf

#define ALPS_DMSG             printf


//#############################################################################

#define  ALPS_MAX( x, y )          ( ( (x) > (y) ) ? (x) : (y) )
#define  ALPS_MIN( x, y )          ( ( (x) < (y) ) ? (x) : (y) )
#define  ALPS_FABS(x)              ( (x < 0.0) ? -(x) : (x) )
#define  ALPS_ABS(x)               ( (x < 0) ? -(x) : (x) )

//#############################################################################

//#############################################################################


#endif
