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
 * Copyright (C) 2001-2018, Lehigh University, Yan Xu, and Ted Ralphs.       *
 * All Rights Reserved.                                                      *
 *===========================================================================*/

#ifndef Alps_h_
#define Alps_h_

#include <cfloat>
#include <cstdio>

#include "AlpsConfig.h"
#include "CoinFinite.hpp"


//! \page handle MyPage

/*! \mainpage

  Description here is a brief introduction to Abstract Library for Parallel
  tree Search (Alps). For theoretical details of parallel tree search see

  <ul>

  <li> <a href="http://coral.ie.lehigh.edu/~ted/files/papers/JSC02.pdf"> A
  Library Hierarchy for Implementing Scalable Parallel Search Algorithms</a>

  <li> <a href="http://coral.ie.lehigh.edu/~ted/files/papers/ALPS04.pdf"> ALPS:
  A Framework for Implementing Parallel Search Algorithms</a>

  <li> <a href="http://coral.ie.lehigh.edu/~ted/files/papers/CHiPPS-Rev.pdf">
  Computational Experience with a Software Framework for Parallel Integer
  Programming</a>

  <li> <a
  href="http://coral.ie.lehigh.edu/~ted/files/papers/YanXuDissertation07.pdf">Yan
  Xu's dissertation</a>.

  </ul>

  Documentation here can be considered as a condensed summary of all the work
  listed. It is also more focused in the implementation of the ideas presented
  in the listed publications.


  ## Alps Design

  Alps is designed to conduct parallel tree search. It is an abstract library
  for this purpose, i.e., it does not assume much (hence flexible) on the tree
  search problem of its user. Any tree search problem can be implemented as an
  application on top of the Alps, ie. DFS, BFS, Dijkstra's algorithm or Branch
  and Bound search for discrete optimization problems.

  ### Task granularity

  A basic unit of work in ALPS is an entire subtree. This means that each
  worker is capable of processing an entire subtree autonomously. Each broker
  is responsible for tracking a list of subtrees that it is responsible
  for. The hub broker dispenses new candidate nodes (leaves of one of the
  subtrees it is responsible for) to the workers (worker broker receives it) as
  needed and track their progress. When a worker receives a new node, it treats
  this node as the root of a subtree and begins processing that subtree,
  stopping only when the work is completed or the hub broker instructs it to
  stop. Periodically, the worker informs the hub broker of its progress.

  ### Asynchronous messaging

  ### Building Apps

  A tree search can be abstracted as defining the following,

  <ul>
    <li> representing problem in a form of data (#AlpsModel),
    <li> representing nodes in some of data (#AlpsNodeDesc),
    <li> processing a node (AlpsTreeNode::process),
    <li> which node to process next,
    <li> creating of new nodes from a given one (AlpsTreeNode::branch,
         AlpsTreeNode::createNewTreeNode),
    <li> a solution and its quality (#AlpsSolution).
  </ul>

  To define these, user need to inherit corresponding Alps classes and implement
  related virtual functions, given in the paranthesis. Once these are
  defined Alps has different strategies to cary the search related tasks, which
  node to process next, which node to branch, etc.

  Alps comes with two examples, Abc and Knap. Abc is a branch and cut solver,
  Knap is a knapsack solver. Both of them are implemented on top of Alps, where
  Alps carries a classical branch and bound/cut type of search to find optimal
  solutions.

  ## How does parallel search work?

  For parallel search to work, user should implement encode/decode virtual
  functions in the corresponding user types,

  <ul>
    <li> model inherited from #AlpsModel,
    <li> node, inherited from #AlpsTreeNode,
    <li> node description, inherited from #AlpsNodeDesc,
    <li> solution, inherited from #AlpsSolution.
  </ul>

  #AlpsNodeDesc holds subproblem data specific to the node, tree node has other
  infromation related to tree search (index, depth, branching
  info). AlpsTreeNode::desc_ is of type  #AlpsNodeDesc and keeps the problem
  data of the node. This design is intended to keep problem data separated from
  the node data related to tree search. It is for convenience.

  #AlpsModel, #AlpsTreeNode and #AlpsSolution all have #AlpsKnowledge as a base
  class. Instances of these classes are considered as knowledges generated
  during a search for the optimal solution and they are traded between
  different processors.

  All #AlpsModel, #AlpsTreeNode, #AlpsNodeDesc and #AlpsSolution have virtual
  encode() and decode() funtions. Corresponding sub-classes of user app should
  implement these virtual functions. encode() function encodes the object into
  an #AlpsEncoded object. decode() function decodes information from a given
  #AlpsEncoded object. Alps knowledge brokers sends/receives these #AlpsEncoded
  instances.

  Each processor has an AlpsKnowledgeBroker instance responsible with
  coordinating search with other processors' brokers.

  ## Ideas for future
  Each #AlpsKnowledge instance has a pointer that points to its broker.

  Knowledge broker should be able to follow created knowledges. In current
  design user apps can create knowledges in TreeNode::branch() functions. Do we
  want that?

  What about a mechanism where user app requests broker to create a new
  AlpsKnowledge object and then fills the data. This way knowledge broker can
  keep an account of the knowledges created.

  ## Fix

  AlpsKnowledgeBroker::getBestNode returns the most promising node, i.e. best
  quality node. AlpsKnowledgeBroker::getBestQuality() returns to the quality of
  the best solution found (quality of incumbent solution). This should be
  fixed. Moreover AlpsKnowledgeBroker::getBestNode does a search to locate the
  best node, this is not necessary and should be fixed (just update the best
  node whenever new nodes are created).

*/

//#############################################################################

#if defined(__linux__)
#define ALPS_MEMORY_USAGE 1
#endif

typedef int AlpsNodeIndex_t;

//#############################################################################
/** The possible values for clock type. */
//#############################################################################

enum AlpsClockType {
   AlpsClockTypeCpu,
   AlpsClockTypeWallClock
};

//#############################################################################
/** The possible values for static load balancing scheme. */
//#############################################################################

enum AlpsStaticBalanceScheme {
    AlpsRootInit = 0,
    AlpsSpiral
};

//#############################################################################
/** The possible stati for the search nodes. */
//#############################################################################

enum AlpsNodeStatus {
    AlpsNodeStatusCandidate,
    AlpsNodeStatusEvaluated,
    AlpsNodeStatusPregnant,
    AlpsNodeStatusBranched,
    AlpsNodeStatusFathomed,
    AlpsNodeStatusDiscarded
};

//#############################################################################
/** Search Strategies. */
//#############################################################################

enum AlpsSearchType {
    AlpsSearchTypeBestFirst = 0,
    AlpsSearchTypeBreadthFirst,
    AlpsSearchTypeDepthFirst,
    AlpsSearchTypeBestEstimate,
    AlpsSearchTypeHybrid
};

//#############################################################################
/** Type of knowledge like solution, node, cut...*/
//#############################################################################

enum AlpsKnowledgeType{
   AlpsKnowledgeTypeModel = 0,
   AlpsKnowledgeTypeModelGen,
   AlpsKnowledgeTypeNode,
   AlpsKnowledgeTypeNodeDesc,
   AlpsKnowledgeTypeSolution,
   AlpsKnowledgeTypeSubTree,
   AlpsKnowledgeTypeUndefined
};

enum AlpsKnowledgePoolType{
  AlpsKnowledgePoolTypeNode = 0,
  AlpsKnowledgePoolTypeSolution,
  AlpsKnowledgePoolTypeSubTree,
  AlpsKnowledgePoolTypeUndefined
};

//#############################################################################
// Search return status
//#############################################################################

enum AlpsExitStatus {
    AlpsExitStatusUnknown = -1,
    AlpsExitStatusOptimal,
    AlpsExitStatusTimeLimit,
    AlpsExitStatusNodeLimit,
    AlpsExitStatusSolLimit,
    AlpsExitStatusFeasible,
    AlpsExitStatusInfeasible,
    AlpsExitStatusNoMemory,
    AlpsExitStatusFailed,
    AlpsExitStatusUnbounded
};

//#############################################################################
// Return code.
//#############################################################################

enum AlpsReturnStatus {
    AlpsReturnStatusOk = 0,
    AlpsReturnStatusErr,
    AlpsReturnStatusErrNoInt,  /* No integer variable.*/
    AlpsReturnStatusErrNoMem
};

//#############################################################################
// Seach phase
//#############################################################################

enum AlpsPhase {
    AlpsPhaseRampup = 0,
    AlpsPhaseSearch,
    AlpasPhaseRampdown
};

#define ALPS_NODE_PROCESS_TIME  0.0123
#define ALPS_NONE 0
#define ALPS_NOT_SET -1

//#############################################################################
// Big number
//#############################################################################

#define ALPS_DBL_MAX          COIN_DBL_MAX
#define ALPS_INC_MAX          1.0e80
#define ALPS_OBJ_MAX          1.0e75
#define ALPS_OBJ_MAX_LESS     1.0e70
#define ALPS_BND_MAX          1.0e20
#define ALPS_INFINITY         1.0e20

#define ALPS_INT_MAX          COIN_INT_MAX

//#############################################################################
// Small number
//#############################################################################

#define ALPS_ZERO             1.0e-14
#define ALPS_GEN_TOL          1.0e-6
#define ALPS_QUALITY_TOL      1.0e-5
#define ALPS_SMALL_3          1.0e-3
#define ALPS_SMALL_4          1.0e-4
#define ALPS_SMALL_5          1.0e-5

//#############################################################################

#define ALPS_PRINTF           printf

#define ALPS_DMSG             printf


//#############################################################################

#define  ALPS_MAX( x, y )          ( ( (x) > (y) ) ? (x) : (y) )
#define  ALPS_MIN( x, y )          ( ( (x) < (y) ) ? (x) : (y) )
#define  ALPS_FABS(x)              ( (x < 0.0) ? -(x) : (x) )
#define  ALPS_ABS(x)               ( (x < 0) ? -(x) : (x) )

//#############################################################################

typedef struct ALPS_PS_STATS
{
    int qualityBalance_;
    int quantityBalance_;
    int interBalance_;
    int intraBalance_;
    int workerAsk_;
    int donateSuccess_;
    int donateFail_;
    int subtreeSplit_;
    int subtreeWhole_;
    int subtreeChange_;
} AlpsPsStats;

//#############################################################################


#endif
