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
 *===========================================================================*/

#include "AlpsKnowledgeBroker.h"
#include "Alps.h"

//#############################################################################
// Initialize static member. 
std::map<const char*, const AlpsKnowledge*, AlpsStrLess>*
AlpsKnowledgeBroker::decodeMap_ = new std::map<const char*, 
  const AlpsKnowledge*, AlpsStrLess>;

//#############################################################################

/* Default constructor. */
AlpsKnowledgeBroker::AlpsKnowledgeBroker()
    : 
    model_(NULL),
    phase_(ALPS_PHASE_SEARCH),
    subTreePool_ (new AlpsSubTreePool),
    solPool_ (new AlpsSolutionPool),
    pools_(0),
    workingSubTree_(0),
    needWorkingSubTree_(true),// Initially workingSubTree_ points to NULL
    nextIndex_(0),
    maxIndex_(INT_MAX),
    nodeProcessedNum_(0),
    nodeLeftNum_(0),
    treeDepth_(0),
    solStatus_(ALPS_UNKNOWN),
    treeSelection_(0),
    nodeSelection_(0),
    msgLevel_(2),
    hubMsgLevel_(0),
    workerMsgLevel_(0),
    logFileLevel_(0),
    nodeMemSize_(0),
    nodeProcessingTime_(ALPS_NODE_PROCESS_TIME), // Positive
    largeSize_(100000)
{
    registerClass("ALPS_SUBTREE", new AlpsSubTree(this));
    handler_ = new CoinMessageHandler();
    handler_->setLogLevel(2);
    messages_ = AlpsMessage();
}

//#############################################################################

AlpsKnowledgeBroker:: ~AlpsKnowledgeBroker() 
{
    if (subTreePool_) {
	//std::cout << "* delete subtree pool" << std::endl;
	delete subTreePool_;
	subTreePool_ = 0;
    }
    if (solPool_) {
	delete solPool_; 
	solPool_ = 0;
    }
    if (pools_) {
	delete pools_; 
	pools_ = 0;
    }
    if (workingSubTree_) {
	//std::cout << "* delete working subtree" << std::endl;
	delete workingSubTree_; 
	workingSubTree_ = 0;
    }
    if (nodeSelection_){
	delete nodeSelection_;
	nodeSelection_ = 0;
    }
    if (rampUpNodeSelection_){
	delete rampUpNodeSelection_;
	rampUpNodeSelection_ = 0;
    }
    if (treeSelection_){
	delete treeSelection_;
	treeSelection_ = 0;
    }
    if (handler_) {
	delete  handler_;
	handler_ = 0;
    }
}

//#############################################################################
 
int 
AlpsKnowledgeBroker::updateNumNodesLeft()
{
    nodeLeftNum_ = 0;
    
    if (workingSubTree_ != 0) {
        nodeLeftNum_ += workingSubTree_->getNumNodes();
    }
    
    std::vector<AlpsSubTree*> subTreeVec = 
        subTreePool_->getSubTreeList().getContainer();

    std::vector<AlpsSubTree*>::iterator pos1 = subTreeVec.begin();
    std::vector<AlpsSubTree*>::iterator pos2 = subTreeVec.end();

    for ( ; pos1 != pos2; ++pos1) {
        nodeLeftNum_ += (*pos1)->getNumNodes();
    }
    
    return nodeLeftNum_;
}

//#############################################################################
AlpsTreeNode* 
AlpsKnowledgeBroker::getBestNode() const 
{
    AlpsTreeNode *bestNode = NULL;
    AlpsTreeNode *node = NULL;
    
    if (workingSubTree_ ) {
        bestNode = workingSubTree_->getBestNode();
    }
    
    std::vector<AlpsSubTree*> subTreeVec = 
        subTreePool_->getSubTreeList().getContainer();
    
    std::vector<AlpsSubTree*>::iterator pos1 = subTreeVec.begin();
    std::vector<AlpsSubTree*>::iterator pos2 = subTreeVec.end();
    
    for ( ; pos1 != pos2; ++pos1) {
        node = (*pos1)->getBestNode();
        if (node) {
            if (bestNode) {
                if (node->getQuality() < bestNode->getQuality()) {
                    bestNode = node;
                }
            }
            else {
                bestNode = node;
            }
        }
    }
    
    return bestNode;
}

//#############################################################################

int 
AlpsKnowledgeBroker::getNumKnowledges(AlpsKnowledgeType kt) const 
{
    if ( (kt == ALPS_SOLUTION) || (kt == ALPS_SUBTREE) ){
        return getKnowledgePool(kt)->getNumKnowledges();
    }
    else if (kt == ALPS_NODE) {
        return nodeLeftNum_;
    }
    else {
        throw CoinError("Broker doesn't manage this type of knowledge", 
                        "getNumKnowledgePool()", "AlpsKnowledgeBroker"); 
    }
}

//#############################################################################

std::pair<AlpsKnowledge*, double> 
AlpsKnowledgeBroker::getBestKnowledge(AlpsKnowledgeType kt) const 
{ 
    if(kt == ALPS_SOLUTION || kt == ALPS_SUBTREE) {
        return getKnowledgePool(kt)->getBestKnowledge();
    }
    else if (kt == ALPS_NODE) {
        AlpsTreeNode *bn = getBestNode();
        if (bn) {
            return std::pair<AlpsKnowledge *, double>(bn, bn->getQuality());
        }
        else {
            return std::pair<AlpsKnowledge *, double>(bn, ALPS_OBJ_MAX);
        }
    }	
    else {
        throw CoinError("Broker doesn't manage this type of knowledge", 
                        "getBestKnowledge()", "AlpsKnowledgeBroker"); 
    }
}

//#############################################################################

void 
AlpsKnowledgeBroker::setupKnowledgePools() 
{

    //--------------------------------------------------
    // Setup search strategy.
    //--------------------------------------------------
    int strategy = model_->AlpsPar()->entry(AlpsParams::searchStrategy);
    
    if (strategy == AlpsBestFirst) {
        treeSelection_ = new AlpsTreeSelectionBest;
        nodeSelection_ = new AlpsNodeSelectionBest;
    }
    else if (strategy == AlpsBreadthFirst) {
        treeSelection_ = new AlpsTreeSelectionBreadth;
        nodeSelection_ = new AlpsNodeSelectionBreadth;
    }
    else if (strategy == AlpsDepthFirst) {
        treeSelection_ = new AlpsTreeSelectionDepth;
        nodeSelection_ = new AlpsNodeSelectionDepth;
    }
    else if (strategy == AlpsEstimate) {
        treeSelection_ = new AlpsTreeSelectionEstimate;
        nodeSelection_ = new AlpsNodeSelectionEstimate;
    }
    else if (strategy == AlpsHybrid) {
        treeSelection_ = new AlpsTreeSelectionBest;
        nodeSelection_ = new AlpsNodeSelectionHybrid;
    }
    else {
        assert(0);
        throw CoinError("Unknown search strategy", 
			"setupKnowledgePools()", "AlpsKnowledgeBroker"); 
    }

    strategy = model_->AlpsPar()->entry(AlpsParams::rampUpSearchStrategy);
    
    if (strategy == AlpsBestFirst) {
        rampUpNodeSelection_ = new AlpsNodeSelectionBest;
    }
    else if (strategy == AlpsBreadthFirst) {
        rampUpNodeSelection_ = new AlpsNodeSelectionBreadth;
    }
    else if (strategy == AlpsDepthFirst) {
        rampUpNodeSelection_ = new AlpsNodeSelectionDepth;
    }
    else if (strategy == AlpsEstimate) {
        rampUpNodeSelection_ = new AlpsNodeSelectionEstimate;
    }
    else if (strategy == AlpsHybrid) {
        rampUpNodeSelection_ = new AlpsNodeSelectionHybrid;
    }
    else {
        assert(0);
        throw CoinError("Unknown ramp up search strategy", 
			"setupKnowledgePools()", "AlpsKnowledgeBroker"); 
    }

    //--------------------------------------------------
    // Create solution and subtree pools.
    //--------------------------------------------------

    pools_ = new std::map<AlpsKnowledgeType, AlpsKnowledgePool*>;

    pools_->insert( std::pair<AlpsKnowledgeType, AlpsKnowledgePool*>
                    ( ALPS_SOLUTION, solPool_ ) );

    pools_->insert( std::pair<AlpsKnowledgeType, AlpsKnowledgePool*>
                    ( ALPS_SUBTREE, subTreePool_ ) );
    
    subTreePool_->setComparison(*treeSelection_);    
}

//#############################################################################
