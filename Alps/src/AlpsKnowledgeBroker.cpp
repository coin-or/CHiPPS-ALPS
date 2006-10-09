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

#include "AlpsKnowledgeBroker.h"


//#############################################################################
// Initialize static member. 
std::map<const char*, const AlpsKnowledge*, AlpsStrLess>*
AlpsKnowledgeBroker::decodeMap_ = new std::map<const char*, 
  const AlpsKnowledge*, AlpsStrLess>;

//#############################################################################

/* Default constructor. */
AlpsKnowledgeBroker::AlpsKnowledgeBroker()
    : 
    //phase_(ALPS_PHASE_RAMPUP),
    model_(NULL),
    phase_(ALPS_PHASE_SEARCH),
    subTreePool_ (new AlpsSubTreePool),
    solPool_ (new AlpsSolutionPool),
    pools_(0),
    workingSubTree_(0),
    needWorkingSubTree_(true),// Initially workingSubTree_ points to NULL
    nextIndex_(0),
    maxIndex_(INT_MAX),
    nodeMemSize_(0),
    nodeProcessedNum_(0),
    nodeLeftNum_(0),
    treeDepth_(0),
    termStatus_(ALPS_OPTIMAL),
    treeCompare_(0),
    nodeCompare_(0),
    msgLevel_(0),
    logFileLevel_(0)
{
    registerClass("ALPS_SUBTREE", new AlpsSubTree(this));
    handler_ = new CoinMessageHandler();
    handler_->setLogLevel(2);
    messages_ = AlpsMessage();
}

//#############################################################################

AlpsKnowledgeBroker:: ~AlpsKnowledgeBroker() 
{
    if (subTreePool_ != 0) {
	delete subTreePool_;
	subTreePool_ = 0;
    }
    if (solPool_ != 0) {
	delete solPool_; 
	solPool_ = 0;
    }
    if (pools_ != 0) {
	delete pools_; 
	pools_ = 0;
    }
    if (workingSubTree_ != 0) {
	delete workingSubTree_; 
	workingSubTree_ = 0;
    }
    if (nodeCompare_ != 0){
	delete  nodeCompare_;
	nodeCompare_ = 0;
    }
    if (treeCompare_ != 0){
	delete  treeCompare_;
	treeCompare_ = 0;
    }
    if (handler_ != 0) {
	delete  handler_;
	handler_ = 0;
    }
}

//#############################################################################
