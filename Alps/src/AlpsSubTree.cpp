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

#include <cmath>
#include <iostream>
#include <queue>
#include <stack>

#include "CoinError.hpp"
#include "CoinTime.hpp"

#include "Alps.h"
#include "AlpsTime.h"
#include "AlpsHelperFunctions.h"
#include "AlpsKnowledgeBroker.h"
#include "AlpsSubTree.h"
#include "AlpsNodePool.h"
#include "AlpsMessage.h"
#include "AlpsMessageTag.h"

#ifdef ALPS_MEMORY_USAGE
#include <malloc.h>
#endif

//#############################################################################

static int computeRampUpNumNodes(int minNumNodes,
                                 int requiredNumNodes,
                                 double nodeProcessingTime) 
{
    // TODO: Should related to unit work.
    int newNumNodes;
    if (nodeProcessingTime < 1.0e-14) {
        nodeProcessingTime = 1.0e-5;
    }
    
    if (nodeProcessingTime > 5.0) {
        newNumNodes = minNumNodes;
    }
    else if (nodeProcessingTime > 1.0) {
        newNumNodes = minNumNodes;
    }
    else if (nodeProcessingTime > 0.5) {
        newNumNodes = minNumNodes;
    }
    else if (nodeProcessingTime > 0.1) {
        newNumNodes = minNumNodes;
    }
    else if (nodeProcessingTime > 0.05) {
        newNumNodes = minNumNodes;
    }
    else if (nodeProcessingTime > 0.01) {
        newNumNodes = minNumNodes;
    }
    else if (nodeProcessingTime > 0.005) {
        newNumNodes = minNumNodes * 2;
    }
    else if (nodeProcessingTime > 0.001) {
        newNumNodes = minNumNodes *2;
    }
    else if (nodeProcessingTime > 0.0005){
        newNumNodes = minNumNodes * 5;
    }
    else if (nodeProcessingTime > 0.0001){
        newNumNodes = minNumNodes * 30;
    }
    else if (nodeProcessingTime > 0.00005){
        newNumNodes = minNumNodes * 60;
    }
    else {
        newNumNodes = minNumNodes * 80;
    }

    if (requiredNumNodes > 0) {
        newNumNodes = (int) (0.5 *(requiredNumNodes + newNumNodes));
    }

    newNumNodes = CoinMax(newNumNodes, minNumNodes);  

#if 0    
    std::cout << "+++++ newNumNodes = " << newNumNodes 
              << ", nodeProcessingTime = " << nodeProcessingTime << std::endl;
#endif
    return newNumNodes;
}

//#############################################################################

/** Default constructor. */
AlpsSubTree::AlpsSubTree() 
    : 
    root_(0),
    //nextIndex_(0), 
    nodePool_(new AlpsNodePool), 
    diveNodePool_(new AlpsNodePool), 
    diveNodeRule_(new AlpsNodeSelectionEstimate),
    activeNode_(0),
    quality_(ALPS_OBJ_MAX),
    broker_(0)
    //eliteSize_(-1)
{ 
    diveNodePool_->setNodeSelection(*diveNodeRule_);
}

//#############################################################################

/** Useful constructor. */
AlpsSubTree::AlpsSubTree(AlpsKnowledgeBroker* kb) 
    : 
    root_(0),
    // nextIndex_(0), 
    nodePool_(new AlpsNodePool),
    diveNodePool_(new AlpsNodePool),
    diveNodeRule_(new AlpsNodeSelectionEstimate),
    activeNode_(0),
    quality_(ALPS_OBJ_MAX)
{ 
    assert(kb);
    broker_ = kb;
    
    //eliteSize_ = kb->getDataPool()->
    //getOwnParams()->entry(AlpsParams::eliteSize);
    
    diveNodePool_->setNodeSelection(*diveNodeRule_);
}

//#############################################################################

/** Destructor. */
AlpsSubTree::~AlpsSubTree() 
{
    //std::cout << "- delete subtree" << std::endl;
    if (nodePool_ != NULL) {
        nodePool_->clear(); // Nodes will be freed by deleting root
        delete nodePool_;
        nodePool_ = NULL; 
    }
    
    if (diveNodePool_ != NULL) { 
        diveNodePool_->clear(); // Nodes will be freed by deleting root
        delete diveNodePool_; 
        diveNodePool_ = NULL; 
    }

    if (root_ != NULL) {
	//std::cout << "- delete root" << std::endl;
	
        root_->removeDescendants();
        delete root_;
	root_ = NULL;
    }
    
    delete diveNodeRule_;
}

//#############################################################################

void
AlpsSubTree::removeDeadNodes(AlpsTreeNode*& node) throw(CoinError)
{
    if (!node->isFathomed()) {
	throw CoinError("node->isFathomed()","removeDeadNodes","AlpsSubTree");   
    }

    AlpsTreeNode* parent = node->getParent();
    if (parent) {
        /* Free memory of node. */
	parent->removeChild(node);
        
	if (parent->getNumChildren() == 0) {
            /* If parent has no child, fathom it. This repeats recursively. */
	    parent->setStatus(AlpsNodeStatusFathomed);
	    removeDeadNodes(parent);
	}
    } 
    else {
	// We are in the root
	node->setStatus(AlpsNodeStatusFathomed);
	//delete node;
	//node = NULL;
    }
}

//#############################################################################

void
AlpsSubTree::replaceNode(AlpsTreeNode* oldNode, AlpsTreeNode* newNode)
{
    AlpsTreeNode* parent = oldNode->getParent();
    
    oldNode->removeDescendants();

    if (parent) {
	parent->removeChild(oldNode);
	parent->addChild(newNode);
	newNode->setParent(parent);
	newNode->setParentIndex(parent->getIndex());
    } 
    else {
	delete root_;
	root_ = newNode;
    }
}

//#############################################################################

void 
AlpsSubTree::createChildren(
    AlpsTreeNode* parent,
    std::vector< CoinTriple<AlpsNodeDesc*, AlpsNodeStatus, double> >& children,
    AlpsNodePool *diveNodePool
    )
{
    int i;
    const bool deleteNode = 
	broker_->getModel()->AlpsPar()->entry(AlpsParams::deleteDeadNode);
    const int numChildren = children.size();
 
    parent->setNumChildren(numChildren);
    parent->setStatus(AlpsNodeStatusBranched);
    
    for (i = 0; i < numChildren; ++i) {
	AlpsTreeNode* child = parent->createNewTreeNode(children[i].first);
	parent->setChild(i, child);
	child->setStatus(children[i].second);
	child->setQuality(children[i].third);
	child->setParent(parent);
	child->setParentIndex(parent->getIndex());
	child->setKnowledgeBroker(parent->getKnowledgeBroker());
	child->setActive(false);
	child->setDepth(parent->getDepth() + 1);
	child->setIndex(nextIndex());
    }

    for (i = 0; i < numChildren; ++i) {
	AlpsTreeNode* child = parent->getChild(i);
	switch (child->getStatus()) {
	case AlpsNodeStatusCandidate:
	case AlpsNodeStatusEvaluated:
	case AlpsNodeStatusPregnant:
	    if (diveNodePool) {
		diveNodePool->addKnowledge(child, child->getSolEstimate());
	    }
	    else {
		nodePool_->addKnowledge(child, child->getQuality());
	    }
	    break;
	case AlpsNodeStatusFathomed:
	    // Based on a parameter deleteNode, we decide whether to
	    // clean up the tree or preserve it for posterity
	    if (deleteNode) {
		removeDeadNodes(child);
            }
	    break;
	default: // AlpsNodeStatus::branched ==> this is impossible
	    throw CoinError("impossible status: branched",
			    "createChildren", "AlpsSubTree");
	}
    }
}

//#############################################################################

int 
AlpsSubTree::nextIndex()
{
    return getKnowledgeBroker()->nextNodeIndex();
}

//#############################################################################

int 
AlpsSubTree::getNextIndex() const
{ 
    return getKnowledgeBroker()->getNextNodeIndex(); 
}

//#############################################################################

void 
AlpsSubTree::setNextIndex(int next) 
{ 
    getKnowledgeBroker()->setNextNodeIndex(next); 
}

//#############################################################################

double
AlpsSubTree::calculateQuality()
{
    quality_ = ALPS_OBJ_MAX;

    const int eliteSize = 
	broker_->getModel()->AlpsPar()->entry(AlpsParams::eliteSize);
    assert(eliteSize > 0);

    int nodeSelectionType = broker_->getNodeSelection()->getType();
    
    if ( ((nodeSelectionType == ALPS_SEARCH_BEST) ||
	  (nodeSelectionType == ALPS_SEARCH_HYBRID)) &&
	 (eliteSize == 1) ) {
	quality_ = nodePool_->getKnowledge().second;
	//std::cout << "quality_ = " << quality_ << std::endl;
	return quality_;
    }
    
    const int nodeNum = nodePool_->getNumKnowledges();
    if (nodeNum <= 0) {
	std::cout << "PROC[" << getKnowledgeBroker()->getProcRank()
		  << "] has a subtree with no node" << std::endl;
	assert(nodeNum > 0);
    }
   
    std::vector<AlpsTreeNode* > allNodes = 
	nodePool_->getCandidateList().getContainer();
    const int numNodes = getNumNodes();
    std::vector<AlpsTreeNode* >::iterator pos = allNodes.begin();
     
    std::multimap<double, AlpsTreeNode*> eliteList;
    std::multimap<double, AlpsTreeNode*>::iterator posEnd;    

    for (int i = 0; i < numNodes; ++i) {
	AlpsTreeNode* node = (*pos);
	double quality = (*pos)->getQuality();

	++pos;
        
        if (eliteSize == 1) {
            if (quality_ > quality) {
                quality_ = quality;
            }
        }
        else {
            if (static_cast<int>(eliteList.size()) < eliteSize) {
                eliteList.insert(std::pair<double,AlpsTreeNode*>(quality,node));
		
            }
            else {  // ==   
                posEnd = eliteList.end();
                --posEnd;
                if (quality < posEnd->first) {
                    eliteList.insert(std::pair<double, AlpsTreeNode*>
				 (quality, node));
                    posEnd = eliteList.end();
                    --posEnd;
                    eliteList.erase(posEnd);
                }
            }
        }
    }
    
    if (eliteSize > 1) {
        quality_ = 0.0;
        for (posEnd = eliteList.begin(); posEnd != eliteList.end(); ++posEnd){
            quality_ += posEnd->first;
        }
        quality_ /= eliteList.size();
    }
    
    return quality_;
}

//#############################################################################

AlpsReturnCode
AlpsSubTree::exploreSubTree(AlpsTreeNode* root,
			    int nodeLimit,
			    double timeLimit,
			    int & numNodesProcessed, /* Output */
			    int & depth)             /* Output */
{
    AlpsReturnCode status = ALPS_OK;
    AlpsSolStatus exploreStatus = ALPS_INFEASIBLE;
    
    bool betterSolution = false;

    //------------------------------------------------------
    // Set the root node and put it into the queue.
    //------------------------------------------------------

    root_ = root;
    nodePool_->addKnowledge(root_, root->getQuality());

    //------------------------------------------------------
    // Explore the tree.
    //------------------------------------------------------

    status = exploreUnitWork(false, 
			     nodeLimit,
                             timeLimit,
                             exploreStatus,
                             numNodesProcessed, /* Output */
                             depth,             /* Output */
                             betterSolution);   /* Output */

    if (exploreStatus == ALPS_NODE_LIMIT) {
        broker_->setSolStatus(ALPS_NODE_LIMIT);
    }
    else if (exploreStatus == ALPS_TIME_LIMIT) {
        broker_->setSolStatus(ALPS_TIME_LIMIT);
    }
    else if (exploreStatus == ALPS_UNBOUNDED) {
        broker_->setSolStatus(ALPS_UNBOUNDED);
    }
    else {
        // Search to end.
        if (broker_->hasKnowledge(ALPS_SOLUTION)) {
            broker_->setSolStatus(ALPS_OPTIMAL);
        }
        else {
            broker_->setSolStatus(ALPS_INFEASIBLE);
        }
    }

    return status;
}   

//#############################################################################

// The implementation is almost same as exploreSubtree, except it will return
// when the required number of nodes is generated or the nodepool is empty
// which means the master already solve the problem). During this step,
// master may find solutions, and later it will let hubs know the solutions.

int
AlpsSubTree::rampUp(int minNumNodes,
                    int requiredNumNodes, 
                    int& depth, 
                    AlpsTreeNode* root) 
{
    int numNodesProcessed = 0;
    int npCount = 0;
    double npTime = 0.0;

    bool firstCall = true;
    bool comRampUpNodes = true;
    
    const bool deleteNode = 
	broker_->getModel()->AlpsPar()->entry(AlpsParams::deleteDeadNode);

    AlpsTreeNode* node = NULL;

    if (requiredNumNodes > 0) {
        // User set
        comRampUpNodes = false;
    }
    
    if (root != NULL) {
	/* Master: set the root node and put it into the queue*/
	root_ = root;
	nodePool_->addKnowledge(root_, ALPS_OBJ_MAX);
    }
    else {
	/* Hub. Do nothing. */
    }

    while( nodePool_->hasKnowledge() &&
	   ((nodePool_->getNumKnowledges() < requiredNumNodes) || firstCall) ) { 
               
	node = dynamic_cast<AlpsTreeNode*>
	    (const_cast<AlpsKnowledge*>(nodePool_->getKnowledge().first) ); 
        
	nodePool_->popKnowledge();

	switch (node->getStatus()) {
	case AlpsNodeStatusPregnant : {
	    std::vector< CoinTriple<AlpsNodeDesc*, AlpsNodeStatus, double> > 
		children = node->branch();
	    createChildren(node, children);
	    if (depth < node->getDepth() + 1) {    // Record the depth of tree
		depth = node->getDepth() + 1;
	    }
	    break;
	}
	case AlpsNodeStatusCandidate :
	case AlpsNodeStatusEvaluated :
	    ++numNodesProcessed; 
            //activeNode_ = node; // Don't set, getNumNodes wrong.
	    broker_->subTreeTimer().start();
	    node->setActive(true);
	    if (node == root_) {
                node->process(true, true);
            }
	    else {
                node->process(false, true);
            }

	    node->setActive(false);
            npTime = broker_->subTreeTimer().getWallClock();
            if (comRampUpNodes && (npCount < 50)) {
                requiredNumNodes = computeRampUpNumNodes(minNumNodes,
                                                         requiredNumNodes,
                                                         npTime);
                ++npCount;
                firstCall = false; // set to false 
            }
	    switch (node->getStatus()) {
	    case AlpsNodeStatusCandidate :
	    case AlpsNodeStatusEvaluated :
	    case AlpsNodeStatusPregnant :
		nodePool_->addKnowledge(node, node->getQuality());
		break;
	    case AlpsNodeStatusFathomed :
		// Based on the parameter deleteNode, we decide whether to
		// clean up the tree or preserve it for posterity
		if (deleteNode) {
		    removeDeadNodes(node);
		}
		break;
	    default : 
		throw CoinError("Impossible Status: branched",
				"rampUp",
				"AlpsSubTreeMaster");
	    }
	    break;
	default : 
	    throw CoinError("Impossible Status: branched or fathomed",
			    "rampUp",
			    "AlpsSubTreeMaster");
	}
    }

    // Print msg
    if (comRampUpNodes) {
        if ( (broker_->getMsgLevel() > 1) &&
             (broker_->getProcType() == AlpsProcessTypeMaster) ) {
            broker_->messageHandler()->message(ALPS_RAMPUP_MASTER_NODES_AUTO, broker_->messages())
                << broker_->getProcRank()
                << requiredNumNodes
                << npTime
                << CoinMessageEol;
            
        }
        else if ( (broker_->getHubMsgLevel() > 1) &&
                  (broker_->getProcType() == AlpsProcessTypeHub) ) {
            broker_->messageHandler()->message(ALPS_RAMPUP_HUB_NODES_AUTO, broker_->messages())
                << broker_->getProcRank()
                << requiredNumNodes
                << npTime
                << CoinMessageEol;
            
        }
    }

    return numNodesProcessed;
}

//#############################################################################

AlpsSubTree* 
AlpsSubTree::splitSubTree(int& returnSize, int size)
{
    const int numNode = getNumNodes();
    AlpsSubTree* st = NULL;

    if (numNode < 2) {
	// Current has less than 2 nodes, can not split.
	returnSize = 0;
	return st;
    }

    int i;
    int numChildren = 0;

    //------------------------------------------------------
    // Move nodes in diving pool to normal pool.
    //------------------------------------------------------

    AlpsTreeNode* tempNode = 0;

    while (diveNodePool_->getNumKnowledges() > 0) {
	tempNode = dynamic_cast<AlpsTreeNode *>
	    (diveNodePool_->getKnowledge().first);
	diveNodePool_->popKnowledge();
	nodePool_->addKnowledge(tempNode, tempNode->getQuality());
    }
    if (activeNode_) {
	nodePool_->addKnowledge(activeNode_, activeNode_->getQuality());
	activeNode_ = 0;
    }
    
    //------------------------------------------------------
    // Find the root of the subtree to be splitted off.
    //------------------------------------------------------

    const std::vector<AlpsTreeNode*> nodeVec = 
	nodePool()->getCandidateList().getContainer();


    AlpsTreeNode* subTreeRoot = 0;
    AlpsTreeNode* rootParent = 0;

    int LS = broker_->getLargeSize();
    int maxAllowNodes = LS / getKnowledgeBroker()->getNodeMemSize();
    
#if 0
    //------------------------------------------------------
    // This is a way to find the subtree root. Do a breath first search
    // from root, stop when find a node has more than two children. Choose
    // any one child as the subtree root.
    //------------------------------------------------------

    int j;
    bool foundRoot = false;
    
    std::queue<AlpsTreeNode*> nodeQueue;
    nodeQueue.push(root_);
    while (!nodeQueue.empty()) {
	rootParent = nodeQueue.front();
	nodeQueue.pop();
	if (rootParent->getNumChildren() >= 2) {
	    foundRoot = true;
	    break;
	}
	const int numCC = rootParent->getNumChildren();
	for (i = 0; i < numCC; ++i) {
	    nodeQueue.push(rootParent->getChild(i));
	}
    }
    while (!nodeQueue.empty()) {
	nodeQueue.pop();
    }

    if(foundRoot) {
	subTreeRoot = rootParent->getChild(0);  // rootParent is the parent

#ifdef NF_DEBUG_MORE
	numChildren = rootParent->getNumChildren();
	std::cout << "splitSubTree : foundRoot = 1: numChildren =" 
		  << numChildren << std::endl;
#endif

    } 
    else {
	returnSize = 0;
	return st;
    }
#endif  // EOF #if 0

    //------------------------------------------------------
    // This is the other way to find the subtree root. Pick the best node,
    // track back along the path to root, stop when the number of nodes in
    // current subtree is approximately half of the total nodes or the 
    // maximum nodes allowed. 
    //------------------------------------------------------

    // Initially, subtree root is the best leaf node.
    subTreeRoot = dynamic_cast<AlpsTreeNode*>(nodePool_->getKnowledge().first);

    //------------------------------------------------------
    // Find the root of subtree by doing depth first search.
    //------------------------------------------------------
    
    AlpsTreeNode* preSubTreeRoot = NULL;
    AlpsTreeNode* curNode = NULL;
    int numSendNode = 0;
    int numInPool = 0;
    
    while (subTreeRoot != root_) {

        // First time, just the best leaf node, then if no enough nodes,
        // backtrack to its parent, and repeat this process until enough.
	preSubTreeRoot = subTreeRoot;
	subTreeRoot = subTreeRoot->getParent();

	numSendNode = 0;
	numInPool = 0;
	
	std::stack<AlpsTreeNode* > nodeStack;
	curNode = 0;
	nodeStack.push(subTreeRoot); 

	while( !nodeStack.empty() ) {
	    curNode = nodeStack.top(); 
	    if (!curNode->isFathomed() && !curNode->isBranched()){
		++numInPool;
	    }
	    nodeStack.pop();
	    ++numSendNode;
	    // curNode->setSentMark(1);        // Mark as in subtree
	    const int numC = curNode->getNumChildren();
	    for (i = 0; i < numC; ++i) {
		nodeStack.push(curNode->getChild(i));
	    }
	}
	// 4.1 looks fine
	if ((4.1 * numInPool > numNode) || numSendNode >= maxAllowNodes) {
	    break;
	}
	
	//if (numSendNode > maxAllowNodes) {
	//  std::cout << "splitSubTree: Too many nodes ("<< numSendNode 
	//      << ") to be sent. Give up" << " this time."<<std::endl;
	//  returnSize = 0;
	//  return st;
	//}
    }
    
#ifdef NF_DEBUG
    std::cout << "splitSubTree 1: numSendNode = " << numSendNode
	      << ", numInPool = " << numInPool 
	      << ", numNode = " << numNode << std::endl;
#endif

    //------------------------------------------------------
    // Now, we get root of the sent subtree, do following:
    // 1. Make sure this root is explicit,
    // 2. Mark all the nodes in the subtree.
    //------------------------------------------------------

    numSendNode = 0;
    numInPool = 0;
    
    if (subTreeRoot == root_) {
	// Hit root, then back one level.
	subTreeRoot = preSubTreeRoot;
#ifdef NF_DEBUG
	std::cout << "splitSubTree(), hit root." << std::endl;
#endif
    }
    
    subTreeRoot->convertToExplicit();
    subTreeRoot->setExplicit(1);

    std::stack<AlpsTreeNode* > finalStack;
    finalStack.push(subTreeRoot);       // The first is root.
    while( !finalStack.empty() ) {
	curNode = finalStack.top();
	finalStack.pop();
	// FIXME:
	++numSendNode;
	if (!curNode->isFathomed() && !curNode->isBranched()){
	    ++numInPool;
	}
	curNode->setSentMark(1);       // Mark as in splitted subtree.
	const int numC = curNode->getNumChildren();
	for (i = 0; i < numC; ++i) {
	    finalStack.push(curNode->getChild(i));
	}
    }

#ifdef NF_DEBUG
    std::cout << "splitSubTree 2: numSendNode = " << numSendNode
	      << ", num in splitted st pool = " << numInPool << std::endl;
#endif

    //------------------------------------------------------
    // Modify rootParent, then split the current subtree into 
    // two subtrees.
    //------------------------------------------------------

    rootParent = subTreeRoot->getParent();

    numChildren = rootParent->getNumChildren();
    
    for (i = 0; i < numChildren; ++i) {
	if (rootParent->getChild(i) == subTreeRoot) break;
    }
    if (i == numChildren) {
	throw CoinError("Can find this child",
			"splitSubtree",
			"AlpsSubTree");
    }

    rootParent->setChild(i, rootParent->getChild(numChildren - 1));
    rootParent->setChild(numChildren - 1, NULL);
    rootParent->modifyNumChildren(-1);  // A child have gone

    // Splitted subtree's node pool.
    AlpsNodePool* nodePool1 = new AlpsNodePool;
    nodePool1->setNodeSelection(*(broker_->getNodeSelection()));

    // Left node pool.
    AlpsNodePool* nodePool2 = new AlpsNodePool;
    nodePool2->setNodeSelection(*(broker_->getNodeSelection()));

    while(nodePool()->hasKnowledge()) {
	curNode = dynamic_cast<AlpsTreeNode* >(
	    nodePool()->getKnowledge().first);
	nodePool()->popKnowledge();
	if (curNode->getSentMark() == 1) {
	    curNode->setSentMark(2);   // Mark as in subtree's node pool
	    nodePool1->addKnowledge(curNode, curNode->getQuality());
	}
	else {
	    nodePool2->addKnowledge(curNode, curNode->getQuality());
	}
    }
    
    st = new AlpsSubTree;
    st->changeNodePool(nodePool1);
    st->setRoot(subTreeRoot);
    returnSize = st->getNumNodes();

    changeNodePool(nodePool2);
    
#ifdef NF_DEBUG
    std::cout << "splitSubTree 3:  returnSize(splitted tree size) = " << returnSize 
	      << ", num of left nodes is " << nodePool2->getNumKnowledges() 
	      << std::endl;
#endif

    return st;
}

//#############################################################################

AlpsEncoded*
AlpsSubTree::encode() const
{
    std::vector<AlpsTreeNode* > nodesInPool = 
	nodePool_->getCandidateList().getContainer();

    std::vector<AlpsTreeNode* >::iterator pos1, pos2;
    pos2 = nodesInPool.end();
    
    for(pos1 = nodesInPool.begin(); pos1 != pos2; ++pos1) {
	(*pos1)->setSentMark(2);
    }

    //------------------------------------------------------
    // Identify all the nodes(those in node pool and 
    // those popped out) by doing depth first search from the 
    // root of the subtree.
    //------------------------------------------------------

    std::vector<AlpsTreeNode* > allNodes;
    std::stack<AlpsTreeNode* > nodeStack;

    int i = -1, nodeNum = 0; 
    int numChildren = 0;

    AlpsTreeNode* curNode = NULL;

    nodeStack.push(root_);

    while( !nodeStack.empty() ) {
	curNode = nodeStack.top();
	nodeStack.pop();
	allNodes.push_back(curNode);         // The first is root_ 
 
	numChildren = curNode->getNumChildren();
	for (i = 0; i < numChildren; ++i) {
	    nodeStack.push(curNode->getChild(i));
	}
    }

    nodeNum = allNodes.size();

    AlpsEncoded* encoded = new AlpsEncoded("ALPS_SUBTREE");

    encoded->writeRep(nodeNum);              // First write number of nodes
    
    AlpsTreeNode* node = NULL;
  
    for (i = 0; i < nodeNum; ++i) {          // Write all nodes to rep of enc
	node = allNodes[i];
	encoded->writeRep(node->getExplicit()); 
	AlpsEncoded* enc = node->encode();
	encoded->writeRep(enc->size());
	encoded->writeRep(enc->representation(), enc->size());
	if (enc != NULL) {
	    delete enc;
	    enc = NULL;
	}
    }
  
    node = NULL;

#ifdef NF_DEBUG
    std::cout << "encode: nodeNum = " << nodeNum << std::endl;
#endif

    return encoded;
}

//#############################################################################

AlpsKnowledge*
AlpsSubTree::decode(AlpsEncoded& encoded) const
{
    int i = -1, j = -1;
    int nodeNum = 0;
    int fullOrPartial = -1;
    char* buf = 0;
    int size = 0;
    int* numAddedChildren = 0;

    AlpsSubTree* st = new AlpsSubTree;

    AlpsEncoded* encodedNode = 0;
    AlpsTreeNode* node = 0;
    AlpsNodePool* nodePool = new AlpsNodePool;

    nodePool->setNodeSelection(*(broker_->getNodeSelection()));

    std::vector<AlpsTreeNode* > nodeVector;

    encoded.readRep(nodeNum);

#ifdef NF_DEBUG
    std::cout << "AlpsSubTree : decode: nodeNum = " << nodeNum << std::endl;
#endif

    nodeVector.reserve(nodeNum);

    if (nodeNum < 0) {
	throw CoinError("Num of nodes < 0", "decode", "AlpsSubTree");
    }

    //------------------------------------------------------
    // Decode each node.
    //------------------------------------------------------

    for (i = 0; i < nodeNum; ++i) {
	encoded.readRep(fullOrPartial);
	encoded.readRep(size);
 
#ifdef NF_DEBUG
	std::cout << "decode: fullOrPartial = " << fullOrPartial
		  << "; size = " << size << std::endl;
#endif
	
	if (buf != 0) {
	    delete buf; 
	    buf = 0;
	}
    
	encoded.readRep(buf, size);
 
	encodedNode = new AlpsEncoded("ALPS_NODE", size, buf); 
	node = dynamic_cast<AlpsTreeNode* >( (broker_->decoderObject(encodedNode->type()))->decode(*encodedNode) );

        //node->setSubTree(st);
	node->setKnowledgeBroker(getKnowledgeBroker());
	AlpsModel* mod = getKnowledgeBroker()->getModel();
	node->modifyDesc()->setModel(mod);
        
	// Add the node to a temporary vector
	nodeVector.push_back(node);
	
	if (i == 0) {   
	    // First node is root
	    st->setRoot(node);
	    node->setParent(NULL);

	    // FIXME: need set to -1?
	    node->setParentIndex(-1);
	}

        delete encodedNode;  // Free memory. Found by Totalview.
    }

    //------------------------------------------------------    
    // Indentify parent-children relationship.
    //------------------------------------------------------
    
    numAddedChildren = new int [nodeNum];
    for (i = 0; i < nodeNum; ++i) {
	// Allocate memory for children
	nodeVector[i]->setNumChildren(nodeVector[i]->getNumChildren());
	numAddedChildren[i] = 0;
	for (j = 0; j < nodeNum; ++j) {
	    if (j != i) {
		if (nodeVector[j]->getParentIndex() == 
		    nodeVector[i]->getIndex()) {
		    // Set node i as the parent of node j
		    nodeVector[j]->setParent(nodeVector[i]);

		    // Set node j as a child of node i. 
		    nodeVector[i]->setChild(numAddedChildren[i]++, 
					    nodeVector[j]);
		}
	    }
	}
    }

    //------------------------------------------------------
    // Reconstruct the subtree.
    //------------------------------------------------------

    int nodeAdded = 0;
    int nodeReceived = 0;
    nodeReceived = nodeVector.size();
    
    for (i = 0; i < nodeNum; ++i) {
	node = nodeVector.back();
	if (node->getSentMark() == 2) {
	    ++nodeAdded;
	    nodePool->addKnowledge(node, node->getQuality());
	}
	nodeVector.pop_back();
	node->setSentMark(0);   // clean up Marks
	node = 0;
    }  
    
    st->setNodePool(nodePool);
    st->setKnowledgeBroker(getKnowledgeBroker());
    st->setNodeSelection(getKnowledgeBroker()->getNodeSelection());

    //------------------------------------------------------
    // Clean up.
    //------------------------------------------------------

    encodedNode = 0;
    node = 0;
    nodePool = 0;

    if (buf != 0) {
	delete buf; 
	buf = 0;
    }
    if (numAddedChildren != 0) {
	delete [] numAddedChildren;
	numAddedChildren = 0;
    }

#ifdef NF_DEBUG
    std::cout << "decode: finished decoding a subtree, #nodes in pool is " 
	      << st->nodePool()->getNumKnowledges() 
	      << "; nodeAdded = " << nodeAdded 
	      << "; node received  = " << nodeReceived << std::endl;
#endif

    return st;
}

#if 1
//#############################################################################

AlpsReturnCode
AlpsSubTree::exploreUnitWork(bool leaveAsIt,
			     int unitWork,
                             double unitTime,
                             AlpsSolStatus & exploreStatus, /* Output */
                             int & numNodesProcessed,       /* Output */
                             int & depth,                   /* Output */
                             bool & betterSolution)         /* Output */
{
    // Start to count time.
    broker_->subTreeTimer().start();

    AlpsReturnCode status = ALPS_OK;
    
    bool forceLog = false;
    bool exitIfBetter = false;
        
    double oldSolQuality = ALPS_OBJ_MAX;
    double newSolQuality = ALPS_OBJ_MAX;

    AlpsTreeNode * tempNode = NULL;

    //------------------------------------------------------    
    // Get setting/parameters.
    //------------------------------------------------------
    
    const bool deleteNode = 
	broker_->getModel()->AlpsPar()->entry(AlpsParams::deleteDeadNode);

    AlpsSearchStrategy<AlpsTreeNode*> *nodeSel = broker_->getNodeSelection();

    bool checkMemory = broker_->getModel()->AlpsPar()->
	entry(AlpsParams::checkMemory);
    
    //------------------------------------------------------
    // Check if required to exit when find a solution.
    //------------------------------------------------------
    
    if (betterSolution) {
        // Need exit when find better a solution.
        exitIfBetter = true;
        betterSolution = false;
    }
    
    if( broker_->hasKnowledge(ALPS_SOLUTION) ) {
        oldSolQuality = broker_->getBestKnowledge(ALPS_SOLUTION).second;
    }    
    
    //------------------------------------------------------
    // Process nodes until reach unit limits, or better solution if check.
    //------------------------------------------------------    

    if (!leaveAsIt) {
	activeNode_ = NULL;
	exploreStatus = ALPS_INFEASIBLE;    
	numNodesProcessed = 0;
    }
    
    while ( (nodePool_->hasKnowledge() || activeNode_ || 
	     diveNodePool_->hasKnowledge()) && 
	    !betterSolution ) {
	
	broker_->subTreeTimer().stop();

#if 0
        std::cout << "unitTime = " << unitTime
                  << ", solTime = " << broker_->subTreeTimer().getTime()
                  << std::endl;
#endif
   
	if (numNodesProcessed > unitWork) {
            exploreStatus = ALPS_NODE_LIMIT;
	    break;
	}
	else if (broker_->subTreeTimer().getTime() > unitTime){
            exploreStatus = ALPS_TIME_LIMIT;
	    break;
	}
        
	// Get the next node to be processed.
        activeNode_ = nodeSel->selectNextNode(this);
        
	switch (activeNode_->getStatus()) {
	case AlpsNodeStatusPregnant: 
        {            
	    if (depth < activeNode_->getDepth() + 1) {
		depth = activeNode_->getDepth() + 1;
            }
            nodeSel->createNewNodes(this, activeNode_);
	    break;
	}
	case AlpsNodeStatusCandidate:
	case AlpsNodeStatusEvaluated:
	    activeNode_->setActive(true);
	    if (activeNode_ == root_) {
                activeNode_->process(true);
	    }
	    else {
                activeNode_->process();
	    }           
	    activeNode_->setActive(false); 

	    // Record the new sol quality if have.
	    if( broker_->hasKnowledge(ALPS_SOLUTION) ) {
		newSolQuality = 
		    broker_->getBestKnowledge(ALPS_SOLUTION).second;
		if (newSolQuality < oldSolQuality) {
		    if (exitIfBetter) {
			betterSolution = true;
		    }
		    forceLog = true;
		    exploreStatus = ALPS_FEASIBLE;
		    oldSolQuality = newSolQuality;    
		    // std::cout << "betterSolution value=" << newSolQuality
		    //  << std::endl;
		}
	    }

            // Print node log.
            broker_->getModel()->nodeLog(activeNode_, forceLog);
	    forceLog = false;

            // Check memory usage
#ifdef ALPS_MEMORY_USAGE
            //std::cout << "checkMemory = " << checkMemory << std::endl;
            
            if (checkMemory) {
                struct mallinfo memInfo = mallinfo();
                double memUsage = static_cast<double>(memInfo.uordblks + memInfo.hblkhd) / 1024.0;
                memUsage /= 1024.0;
                if (memUsage > broker_->getPeakMemory()) {
                    broker_->setPeakMemory(memUsage);
                    std::cout << "memusge = " << broker_->getPeakMemory() << std::endl;
                }
            }
#endif

	    switch (activeNode_->getStatus()) {
	    case AlpsNodeStatusCandidate :
	    case AlpsNodeStatusEvaluated :
	    case AlpsNodeStatusPregnant :
		break;
	    case AlpsNodeStatusFathomed :
		if (deleteNode) {
		    removeDeadNodes(activeNode_);
                }
                activeNode_ = NULL;
		break;
	    default : 
                // AlpsNodeStatus::branched ==> this is impossible
		throw CoinError("Impossible status: branched", 
				"exploreSubTree", "AlpsSubTree"); 
	    }

            // Increment by 1.
	    ++numNodesProcessed;
	    break;
	default : // branched or fathomed
	    throw CoinError("Impossible status: branched or fathomed", 
			    "exploreSubTree", "AlpsSubTree"); 
	}
    }
    
    if (numNodesProcessed) {
        double oldNP = broker_->getNodeProcessingTime();
        double nodeProcessingTime = (broker_->subTreeTimer().getCpuTime()) / 
            numNodesProcessed;
        if (oldNP > 1.0e-14) {
            nodeProcessingTime = 0.5 * (nodeProcessingTime + oldNP);
        }
        broker_->setNodeProcessingTime(nodeProcessingTime);
    }

    //------------------------------------------------------
    // Possible reasons for breaking while:
    // 1 no nodes:        do nothing
    // 2 reach limits:    move nodes from diving pool to regular pool
    // 3 better solution: move nodes from diving pool to regular pool
    //------------------------------------------------------

    if ( (exploreStatus == ALPS_TIME_LIMIT) ||
         (exploreStatus == ALPS_NODE_LIMIT) ||
         (exploreStatus == ALPS_FEASIBLE) ) {
        // Case 2 and 3.

	if (!leaveAsIt) {
	    // Move nodes in diving pool to normal pool.
	    while (diveNodePool_->getNumKnowledges() > 0) {
		tempNode = dynamic_cast<AlpsTreeNode *>
		    (diveNodePool_->getKnowledge().first);
		diveNodePool_->popKnowledge();
		nodePool_->addKnowledge(tempNode, tempNode->getQuality());
	    }
	    if (activeNode_) {
		nodePool_->addKnowledge(activeNode_, activeNode_->getQuality());
		activeNode_ = 0;
	    }
	}
    }
    else {
        // case 1.
        assert(nodePool_->getNumKnowledges() == 0);
        assert(diveNodePool_->getNumKnowledges() == 0);
        assert(activeNode_ == NULL);
    }

    return status;
}

#else
//#############################################################################

AlpsReturnCode
AlpsSubTree::exploreUnitWork(int unitWork,
                             double unitTime,
                             AlpsSolStatus & exploreStatus, /* Output */
                             int & numNodesProcessed,       /* Output */
                             int & depth,                   /* Output */
                             bool & betterSolution)         /* Output */
{
    // Start to count time.
    broker_->subTreeTimer().start();

    AlpsReturnCode status = ALPS_OK;
    
    int numChildren = 0;
        
    double oldSolQuality = ALPS_OBJ_MAX;
    double newSolQuality = ALPS_OBJ_MAX;

    AlpsTreeNode * tempNode = NULL;

    bool forceLog = false;
    
    //------------------------------------------------------    
    // Get parameters.
    //------------------------------------------------------

    const bool deleteNode = 
	broker_->getModel()->AlpsPar()->entry(AlpsParams::deleteDeadNode);
    
    //------------------------------------------------------
    // If need to stop whenever find a better solution.
    //------------------------------------------------------

    bool exitIfBetter = false;
    if (betterSolution) {
        exitIfBetter = true;
        betterSolution = false;
    }
    
    if( broker_->hasKnowledge(ALPS_SOLUTION) ) {
        oldSolQuality=broker_->getBestKnowledge(ALPS_SOLUTION).second;
    }    

    //------------------------------------------------------
    // Process nodes until reach unit limits, or better solution if check.
    //------------------------------------------------------    

    activeNode_ = NULL;
    exploreStatus = ALPS_INFEASIBLE;    
    numNodesProcessed = 0;

    while ( (nodePool_->hasKnowledge() || activeNode_) && 
	    !betterSolution ) {
	
	broker_->subTreeTimer().stop();
	broker_->timer().stop();
	
	if (numNodesProcessed > unitWork) {
            exploreStatus = ALPS_NODE_LIMIT;
	    break;
	}
	else if (broker_->subTreeTimer().getTime() > unitTime) {
            exploreStatus = ALPS_TIME_LIMIT;
	    // getKnowledgeBroker()->setSolStatus(ALPS_TIME_LIMIT);
	    break;
	}
        
	// Get next node to be processed.
	if (!activeNode_) {
            activeNode_ = dynamic_cast<AlpsTreeNode*>
		(nodePool_->getKnowledge().first); 
            activeNode_->setDiving(false);
            
#ifdef NF_DEBUG_MORE
            std::cout << "======= NOTE[" << activeNode_->getIndex() 
                      << "]: JUMP : depth = " << activeNode_->getDepth() 
                      << ", quality = " << activeNode_->getQuality()
                      << ", estimate = " << activeNode_->getSolEstimate()
                      << std::endl;
#endif
            nodePool_->popKnowledge();
	}
        else {
            activeNode_->setDiving(true);
        }
        
	switch (activeNode_->getStatus()) {
	case AlpsNodeStatusPregnant: 
        {            
	    if (depth < activeNode_->getDepth() + 1) {
		// Record the depth of tree
		depth = activeNode_->getDepth() + 1;
            }
            
	    // Should be part of default node selection.

            // (1) Move left nodes in diving pool to normal pool.
            while (diveNodePool_->getNumKnowledges() > 0) {
                tempNode = dynamic_cast<AlpsTreeNode *>
                    (diveNodePool_->getKnowledge().first);
                diveNodePool_->popKnowledge();
                nodePool_->addKnowledge(tempNode, tempNode->getQuality());
            }

            // (2) Branching to form children descriptions.
	    std::vector< CoinTriple<AlpsNodeDesc*, AlpsNodeStatus, double> > 
		children = activeNode_->branch();
            
	    // (3) Create actual children nodes and store in diving pool
            createChildren(activeNode_, children, diveNodePool_);
            numChildren = diveNodePool_->getNumKnowledges();
            
            // (4) Get a new node for diving pool.
            if (numChildren > 0) {
                activeNode_ = dynamic_cast<AlpsTreeNode *>
                    (diveNodePool_->getKnowledge().first);
                diveNodePool_->popKnowledge();
            }
            else {
                activeNode_ = NULL;
            }
	    break;
	}
	case AlpsNodeStatusCandidate:
	case AlpsNodeStatusEvaluated:
	    activeNode_->setActive(true);

            // Process the node.
	    if (activeNode_ == root_) {
                activeNode_->process(true);
	    }
	    else {
                activeNode_->process();
	    }           
	    activeNode_->setActive(false); 

	    // Record the new sol quality if have.
	    if( broker_->hasKnowledge(ALPS_SOLUTION) ) {
		newSolQuality = 
		    broker_->getBestKnowledge(ALPS_SOLUTION).second;
		if (newSolQuality < oldSolQuality) {
		    if (exitIfBetter) {
			betterSolution = true;
		    }
		    forceLog = true;
		    exploreStatus = ALPS_FEASIBLE;
		    oldSolQuality = newSolQuality;
		    
		    // std::cout << "betterSolution value=" << newSolQuality
		    //  << std::endl;
		}
	    }

            // Print node log.
            broker_->getModel()->nodeLog(activeNode_, forceLog);
	    forceLog = false;

            // Increment by 1.
	    ++numNodesProcessed;
            
            // Check processing status.
	    switch (activeNode_->getStatus()) {
	    case AlpsNodeStatusCandidate :
	    case AlpsNodeStatusEvaluated :
	    case AlpsNodeStatusPregnant :
		break;
	    case AlpsNodeStatusFathomed :
		if (deleteNode) {
		    removeDeadNodes(activeNode_);
                }
                if (diveNodePool_->getNumKnowledges() > 0) {
                    activeNode_ = dynamic_cast<AlpsTreeNode *>
                        (diveNodePool_->getKnowledge().first);
                    diveNodePool_->popKnowledge();
                }
                else {
                    activeNode_ = NULL;
                }
		break;
	    default : 
                // AlpsNodeStatus::branched ==> this is impossible
		throw CoinError("Impossible status: branched", 
				"exploreSubTree", "AlpsSubTree"); 
	    }
	    break;
	default : // branched or fathomed
	    throw CoinError("Impossible status: branched or fathomed", 
			    "exploreSubTree", "AlpsSubTree"); 
	}
    }

    //------------------------------------------------------
    // Clear up.
    //------------------------------------------------------

    // Possible reasons for breaking while:
    // 1 no nodes:        do nothing
    // 2 reach limits:    move nodes from diving pool to regular pool
    // 3 better solution: move nodes from diving pool to regular pool
    
    if ( (exploreStatus == ALPS_TIME_LIMIT) ||
         (exploreStatus == ALPS_NODE_LIMIT) ||
         (exploreStatus == ALPS_FEASIBLE) ) {
        // Case 2 and 3.

        // Move nodes in diving pool to normal pool.
        while (diveNodePool_->getNumKnowledges() > 0) {
            tempNode = dynamic_cast<AlpsTreeNode *>
                (diveNodePool_->getKnowledge().first);
            diveNodePool_->popKnowledge();
            nodePool_->addKnowledge(tempNode, tempNode->getQuality());
        }
        if (activeNode_) {
            nodePool_->addKnowledge(activeNode_, activeNode_->getQuality());
        }
    }
    else {
        // case 1.
        assert(nodePool_->getNumKnowledges() == 0);
        assert(diveNodePool_->getNumKnowledges() == 0);
        assert(activeNode_ == NULL);
    }

    return status;
}
#endif

//#############################################################################

double 
AlpsSubTree::getBestKnowledgeValue() const
{
    double bv1 = ALPS_OBJ_MAX;
    double bv2 = ALPS_OBJ_MAX;
    bv1 = nodePool_->getBestKnowledgeValue();
    bv2 = diveNodePool_->getBestKnowledgeValue();
    if (bv1 < bv2) {
	if (activeNode_) {
	    if (activeNode_->getQuality() < bv1){
		return activeNode_->getQuality();
	    }
	    else {
		return bv1;
	    }
	}
	else {
	    return bv1;
	}
    }
    else {
	if ( (activeNode_) &&
	     (activeNode_->getStatus() != AlpsNodeStatusFathomed) ) {
	    if (activeNode_->getQuality() < bv2){
		return activeNode_->getQuality();
	    }
	    else {
		return bv2;
	    }
	}
	else {
	    return bv2;
	}
    }
}

//#############################################################################

AlpsTreeNode * 
AlpsSubTree::getBestNode() const 
{
    AlpsTreeNode *bn2 = NULL;
    AlpsTreeNode *bestNode = NULL;

    bestNode = nodePool_->getBestNode();
    bn2 = diveNodePool_->getBestNode();
    
    if (bn2) {
	if (bestNode) {
	    if (bestNode->getQuality() > bn2->getQuality()) {
		bestNode = bn2;
	    }
	}
	else {
	    bestNode = bn2;
	}
    }
    
    if (activeNode_ && 
	(activeNode_->getStatus() != AlpsNodeStatusFathomed) ) {
	
	if (bestNode) {
	    if (bestNode->getQuality() > activeNode_->getQuality()) {
		bestNode = activeNode_;
	    }
	}
	else {
	    bestNode = activeNode_;
	}
    }

    return bestNode;
}

//#############################################################################
