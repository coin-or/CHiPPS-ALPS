/*===========================================================================*
 * This file is part of the Abstract Library for Parallel Search (ALPS).     *
 *                                                                           *
 * ALPS is distributed under the Common Public License as part of the        *
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
 * Copyright (C) 2001-2006, Lehigh University, Yan Xu, and Ted Ralphs.       *
 * Corporation, Lehigh University, Yan Xu, Ted Ralphs, Matthew Salzman and   *
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


//#############################################################################

/** Default constructor. */
AlpsSubTree::AlpsSubTree() 
    : 
    root_(0),
    //nextIndex_(0), 
    nodePool_(new AlpsNodePool), 
    diveNodePool_(new AlpsNodePool), 
    diveNodeRule_(new AlpsCompareTreeNodeEstimate),
    activeNode_(0),
    quality_(ALPS_OBJ_MAX),
    broker_(0)
    //eliteSize_(-1)
{ 
    diveNodePool_->setComparison(*diveNodeRule_);
}

//#############################################################################

/** Useful constructor. */
AlpsSubTree::AlpsSubTree(AlpsKnowledgeBroker* kb) 
    : 
    root_(0),
    // nextIndex_(0), 
    nodePool_(new AlpsNodePool),
    diveNodePool_(new AlpsNodePool),
    diveNodeRule_(new AlpsCompareTreeNodeEstimate),
    activeNode_(0),
    quality_(ALPS_OBJ_MAX)
{ 
    assert(kb);
    broker_ = kb;
    
    //eliteSize_ = kb->getDataPool()->
    //getOwnParams()->entry(AlpsParams::eliteSize);
    
    diveNodePool_->setComparison(*diveNodeRule_);
}

//#############################################################################

/** Destructor. */
AlpsSubTree::~AlpsSubTree() 
{ 
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
	throw CoinError("node->isFathomed()", "removeDeadNodes", 
			"AlpsSubTree");   
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
        //child->setSubTree(this);
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
AlpsSubTree::calculateQuality(double inc, double rho)
{
    quality_ = 0.0;
    
    const int nodeNum = nodePool_->getNumKnowledges();
    if (nodeNum <= 0) {
	std::cout << "PROC[" << getKnowledgeBroker()->getProcRank()
		  << "] has a subtree with no node" << std::endl;
	assert(nodeNum > 0);
    }
    const int eliteSize = 
	broker_->getModel()->AlpsPar()->entry(AlpsParams::eliteSize);
    assert(eliteSize > 0);
   
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
	if (static_cast<int>(eliteList.size()) < eliteSize) {
	    eliteList.insert(std::pair<double, AlpsTreeNode*>(quality, node));
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
    
    for (posEnd = eliteList.begin(); posEnd != eliteList.end(); ++posEnd){
	quality_ += posEnd->first;
    }
    quality_ /= eliteList.size();
    //quality_ = getKnowledgeBroker()->getIncumbentValue() - quality_;
    //assert(quality_ >= 0.0);
    
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
    AlpsSolStatus solStatus = ALPS_INFEASIBLE;
    
    bool betterSolution = false;

    //------------------------------------------------------
    // Set the root node and put it into the queue.
    //------------------------------------------------------

    root_ = root;
    nodePool_->addKnowledge(root_, root->getQuality());

    //------------------------------------------------------
    // Explore the tree.
    //------------------------------------------------------

    status = exploreUnitWork(nodeLimit,
                             timeLimit,
                             solStatus,
                             numNodesProcessed, /* Output */
                             depth,             /* Output */
                             betterSolution);   /* Output */

    if (solStatus == ALPS_NODE_LIMIT) {
        broker_->setTermStatus(ALPS_NODE_LIMIT);
    }
    else if (solStatus == ALPS_TIME_LIMIT) {
        broker_->setTermStatus(ALPS_TIME_LIMIT);
    }
    else {
        // Search to end.
        if (broker_->hasKnowledge(ALPS_SOLUTION)) {
            broker_->setTermStatus(ALPS_OPTIMAL);
        }
        else {
            broker_->setTermStatus(ALPS_INFEASIBLE);
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
AlpsSubTree::rampUp(int& depth, AlpsTreeNode* root) 
{
    int numNodesProcessed = 0;
    AlpsTreeNode* node = NULL;

    int requiredNumNodes;
    const bool deleteNode = 
	broker_->getModel()->AlpsPar()->entry(AlpsParams::deleteDeadNode);

    if (root != NULL) {
	/* Master: set the root node and put it into the queue*/
	root_ = root;
	nodePool_->addKnowledge(root_, ALPS_OBJ_MAX);
	requiredNumNodes = 
	    broker_->getModel()->AlpsPar()->entry(AlpsParams::masterInitNodeNum);
    }
    else {
	/* Hub. */
	requiredNumNodes = 
	    broker_->getModel()->AlpsPar()->entry(AlpsParams::hubInitNodeNum);
    }

    while( nodePool_->hasKnowledge() &&
	   (nodePool_->getNumKnowledges() < requiredNumNodes) ) { 
 
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
#if 1
	    if (nodePool_->getNumKnowledges() >= requiredNumNodes) {
		// Has enought nodes after branching and creating children.
		break;
	    }
#endif	    
	    break;
	}
	case AlpsNodeStatusCandidate :
	case AlpsNodeStatusEvaluated :
	    ++numNodesProcessed; 
	    node->setActive(true);
	    if (node == root_) {
                node->process(true, true);
            }
	    else {
                node->process(false, true);
            }
            
	    node->setActive(false);

	    switch (node->getStatus()) {
	    case AlpsNodeStatusCandidate :
	    case AlpsNodeStatusEvaluated :
	    case AlpsNodeStatusPregnant :
		nodePool_->addKnowledge(node, node->getQuality());
		break;
	    case AlpsNodeStatusFathomed :
		// Based on the parameter deleteNode, we decide whether to
		// clean up the tree or preserve it for posterity
		if (deleteNode)
		    removeDeadNodes(node);
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

    //numNodesInPool = nodePool_->getNumKnowledges();
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
    // Find the root of the subtree to be splitted off.
    //------------------------------------------------------

    const std::vector<AlpsTreeNode*> nodeVec = 
	getNodePool()->getCandidateList().getContainer();


    AlpsTreeNode* subTreeRoot = 0;
    AlpsTreeNode* rootParent = 0;

    const int LS = broker_->getModel()->AlpsPar()->entry(AlpsParams::largeSize);

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

    subTreeRoot = dynamic_cast<AlpsTreeNode*>(nodePool_->getKnowledge().first);

#if 0
    int depthUp = 1;  // IMPORTANT: avoid numNodes = 1 case
    int numCount = 1;

    while(subTreeRoot != root_) {
	++depthUp;
	//std::cout << "depthUp = " << depthUp << std::endl;
	
	numCount += static_cast<int>(pow(2, static_cast<double>(depthUp)));
	if ((numCount  > maxAllowNodes) || (numCount * 2 > numNode)) {
	    break;
	}
	subTreeRoot = subTreeRoot->getParent();
    }
#endif
    
    //------------------------------------------------------
    // Find the root of subtree by doing depth first search.
    //------------------------------------------------------
    
    AlpsTreeNode* preSubTreeRoot = NULL;
    AlpsTreeNode* curNode = NULL;
    int numSendNode = 0;
    int numInPool = 0;
    
    while (subTreeRoot != root_) {
	preSubTreeRoot = subTreeRoot;
	subTreeRoot = subTreeRoot->getParent();

	numSendNode = 0;
	numInPool = 0;
	
	std::stack<AlpsTreeNode* > nodeStack;
	curNode = 0;
	nodeStack.push(subTreeRoot);           // The first is root
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
	if ((4.1 * numInPool > numNode) || numSendNode > maxAllowNodes) {
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
    nodePool1->setComparison(*(broker_->getNodeCompare()));

    // Left node pool.
    AlpsNodePool* nodePool2 = new AlpsNodePool;
    nodePool2->setComparison(*(broker_->getNodeCompare()));

    while(getNodePool()->hasKnowledge()) {
	curNode = dynamic_cast<AlpsTreeNode* >(
	    getNodePool()->getKnowledge().first);
	getNodePool()->popKnowledge();
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
    nodePool->setComparison(*(broker_->getNodeCompare()));

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
    st->setNodeCompare(getKnowledgeBroker()->getNodeCompare());

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
	      << st->getNodePool()->getNumKnowledges() 
	      << "; nodeAdded = " << nodeAdded 
	      << "; node received  = " << nodeReceived << std::endl;
#endif

    return st;
}






//#############################################################################

AlpsReturnCode
AlpsSubTree::exploreUnitWork(int unitWork,
                             double unitTime,
                             AlpsSolStatus & solStatus,
                             int & numNodesProcessed, /* Output */
                             int & depth,             /* Output */
                             bool & betterSolution)   /* Output */
{
    AlpsReturnCode status = ALPS_OK;
    
    bool diving = false;

    int numChildren = 0;
        
    double relBound = ALPS_OBJ_MAX;
    double startTime = AlpsCpuTime();
    double oldSolQuality = ALPS_OBJ_MAX;
    double newSolQuality = ALPS_OBJ_MAX;

    AlpsTreeNode * node = NULL;
    AlpsTreeNode * tempNode = NULL;

    //------------------------------------------------------    
    // Get parameters.
    //------------------------------------------------------

    const int msgLevel =
	broker_->getModel()->AlpsPar()->entry(AlpsParams::msgLevel);

    const int nodeInterval = 
	broker_->getModel()->AlpsPar()->entry(AlpsParams::nodeInterval);

    const bool deleteNode = 
	broker_->getModel()->AlpsPar()->entry(AlpsParams::deleteDeadNode);

    //------------------------------------------------------
    // If need to stop whenever find a better solution.
    //------------------------------------------------------

    bool checkBetter = false;
    if (betterSolution) {
        checkBetter = true;
        betterSolution = false;
    }
    
    if( broker_->hasKnowledge(ALPS_SOLUTION) ) {
        oldSolQuality=broker_->getBestKnowledge(ALPS_SOLUTION).second;
    }    

    //------------------------------------------------------
    // Process nodes until reach unit limits, or better solution if check.
    //------------------------------------------------------    

    solStatus = ALPS_INFEASIBLE;    
    numNodesProcessed = 0;

    while ( (nodePool_->hasKnowledge() || diving) && !betterSolution ) {
	
	if (numNodesProcessed >= unitWork) {
            solStatus = ALPS_NODE_LIMIT;
	    break;
	}
	else if (AlpsCpuTime() - startTime > unitTime) {
            solStatus = ALPS_TIME_LIMIT;
	    // getKnowledgeBroker()->setTermStatus(ALPS_TIME_LIMIT);
	    break;
	}
        
        if (!node) {
            node=dynamic_cast<AlpsTreeNode*>(nodePool_->getKnowledge().first); 
            node->setDiving(false);
            
#ifdef NF_DEBUG_MORE
            std::cout << "======= NOTE[" << node->getIndex() 
                      << "]: JUMP : depth = " << node->getDepth() 
                      << ", quality = " << node->getQuality()
                      << ", estimate = " << node->getSolEstimate()
                      << std::endl;
#endif            

            nodePool_->popKnowledge();
            
	}
        else {
            node->setDiving(true);
        }
        
	switch (node->getStatus()) {
	case AlpsNodeStatusPregnant: 
        {            
	    if (depth < node->getDepth() + 1) {// Record the depth of tree
		depth = node->getDepth() + 1;
            }
            
            // (1) Move left nodes in diving pool to normal pool.
            while (diveNodePool_->getNumKnowledges() > 0) {
                tempNode = dynamic_cast<AlpsTreeNode *>
                    (diveNodePool_->getKnowledge().first);
                diveNodePool_->popKnowledge();
                nodePool_->addKnowledge(tempNode, tempNode->getQuality());
            }

            // (2) Branching to form children descriptions.
	    std::vector< CoinTriple<AlpsNodeDesc*, AlpsNodeStatus, double> > 
		children = node->branch();
            
	    // (3) Create actual children nodes and store in diving pool
            createChildren(node, children, diveNodePool_);
            numChildren = diveNodePool_->getNumKnowledges();
            
            // (4) Get a new node for diving pool.
            if (numChildren > 0) {
                node = dynamic_cast<AlpsTreeNode *>
                    (diveNodePool_->getKnowledge().first);
                diveNodePool_->popKnowledge();
                diving = true;
            }
            else {
                node = NULL;
                diving = false;
            }
	    break;
	}
	case AlpsNodeStatusCandidate:
	case AlpsNodeStatusEvaluated:
            // Process the node. 
	    node->setActive(true);
	    if (node == root_) {
                node->process(true);
	    }
	    else {
                node->process();
	    }           
	    node->setActive(false); 

	    // Record the new sol quality if have. 
	    // TODO: replace use a function.
            if (checkBetter) {
                if( broker_->hasKnowledge(ALPS_SOLUTION) ) {
                    newSolQuality = 
                        broker_->getBestKnowledge(ALPS_SOLUTION).second;
		    if (newSolQuality < oldSolQuality) {
			betterSolution = true;
			solStatus = ALPS_FEASIBLE;
			std::cout << "betterSolution value=" << newSolQuality
				  << std::endl;
		    }
		}
            }

            // Increment by 1.
	    ++numNodesProcessed;

	    if ( (msgLevel > -1) && (numNodesProcessed % nodeInterval == 0) ) {
                double feasBound = ALPS_OBJ_MAX;
                relBound = ALPS_OBJ_MAX;
                if (getKnowledgeBroker()->getNumKnowledges(ALPS_SOLUTION)) {
                    feasBound = (getKnowledgeBroker()->
                                 getBestKnowledge(ALPS_SOLUTION)).second;
                }


                relBound = getBestKnowledgeValue();
                if (node) {
                    relBound = ALPS_MIN(node->getQuality(), relBound);
                }
                if (diveNodePool_->getNumKnowledges() > 0) {
                    relBound = ALPS_MIN(diveNodePool_->getBestKnowledgeValue(),
                                        relBound);
                }
                getKnowledgeBroker()->messageHandler()->
                    message(ALPS_S_NODE_COUNT,getKnowledgeBroker()->messages())
                        << numNodesProcessed 
                        << getKnowledgeBroker()->getNumNodesLeft()
                        << relBound
                        << feasBound
                        << CoinMessageEol;
	    }
            
            // Check processing status.
	    switch (node->getStatus()) {
	    case AlpsNodeStatusCandidate :
	    case AlpsNodeStatusEvaluated :
	    case AlpsNodeStatusPregnant :
                diving = true;
		break;
	    case AlpsNodeStatusFathomed :
		if (deleteNode) {
		    removeDeadNodes(node);
                }
                if (diveNodePool_->getNumKnowledges() > 0) {
                    node = dynamic_cast<AlpsTreeNode *>
                        (diveNodePool_->getKnowledge().first);
                    diveNodePool_->popKnowledge();
                    diving = true;
                }
                else {
                    node = NULL;
                    diving = false;
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
    
    if ( (solStatus == ALPS_TIME_LIMIT) ||
         (solStatus == ALPS_NODE_LIMIT) ||
         (solStatus == ALPS_FEASIBLE) ) {
        // Case 2 and 3.

        // Move nodes in diving pool to normal pool.
        while (diveNodePool_->getNumKnowledges() > 0) {
            tempNode = dynamic_cast<AlpsTreeNode *>
                (diveNodePool_->getKnowledge().first);
            diveNodePool_->popKnowledge();
            nodePool_->addKnowledge(tempNode, tempNode->getQuality());
        }
        if (node) {
            nodePool_->addKnowledge(node, node->getQuality());
        }
    }
    else {
        // case 1.
        assert(nodePool_->getNumKnowledges() == 0);
        assert(diveNodePool_->getNumKnowledges() == 0);
        assert(node == NULL);
    }

    return status;
}   











//#############################################################################
// FOLLOWING CODE IS REMOVED
//#############################################################################
#if 0
//       approximateQuality(double inc, double rho) instead.
void
AlpsSubTree::calculateQuality(double inc, double rho)
{
    const int nodeNum = nodePool_->getNumKnowledges();
    if (nodeNum <= 0) {                  // The subtree has no node
	quality_ = 0.0;
	return;
    }

    const double tolerance = 
	broker_->getModel()->AlpsPar()->entry(AlpsParams::tolerance);
  
    if ( (rho > 1.0 - tolerance) && (rho < 1.0 + tolerance) ) {  // rho == 1
	quality_ = nodeNum;    // Only the number of nodes counts
	return;
    } 

    const int keyNodeNum = 
	broker_->getModel()->AlpsPar()->entry(AlpsParams::eliteSize);

    int popNodeNum = 0; 
    int popedNum = 0;
    int i;

    std::vector<AlpsTreeNode* >::iterator pos;
  
    std::pair<const AlpsKnowledge*, double> nodePair;
  
    quality_ = 0.0;

    if (keyNodeNum >= nodeNum)   // All the nodes are included    
	popNodeNum = nodeNum;
    else                         // Only the important nodes are included
	popNodeNum = keyNodeNum;

    AlpsTreeNode** keyNodes = new AlpsTreeNode* [popNodeNum]; 

    // Calculate quality_
    while(popedNum < popNodeNum) {
	nodePair = nodePool_->getKnowledge();
	nodePool_->popKnowledge();
	keyNodes[popedNum] = dynamic_cast<AlpsTreeNode* >(
	    const_cast<AlpsKnowledge* >(nodePair.first));
	quality_ += pow(fabs(inc - nodePair.second), rho);
	++popedNum;
    }

    // Add the poped nodes back to nodePool_
    for (i = 0; i < popedNum; ++i)
	nodePool_->addKnowledge(keyNodes[i], keyNodes[i]->getQuality());

    delete [] keyNodes;
}

//#############################################################################

void
AlpsSubTree::approximateQuality(double inc, double rho)
{
    const int nodeNum = nodePool_->getNumKnowledges();
    if (nodeNum <= 0) {                  // The subtree has no node
	quality_ = 0.0;
	return;
    }

    const double tolerance = 
	broker_->getModel()->AlpsPar()->entry(AlpsParams::tolerance);
  
    if ( (rho > 1.0 - tolerance) && (rho < 1.0 + tolerance) ) {  // rho == 1
	quality_ = nodeNum;    // Only the number of nodes counts
	return;
    } 

    const int keyNodeNum = 
	broker_->getModel()->AlpsPar()->entry(AlpsParams::eliteSize);

    int popNodeNum = 0;
  
    std::vector<AlpsTreeNode* > allNodes = 
	nodePool_->getCandidateList().getContainer();
    std::vector<AlpsTreeNode* >::iterator pos = allNodes.begin();
 
    std::pair<const AlpsKnowledge*, double> nodePair;
  
    quality_ = 0.0;

    if (keyNodeNum >= nodeNum)       // All the nodes in pool are included
	popNodeNum = nodeNum;          
    else                             // Only the important nodes are included
	popNodeNum = keyNodeNum;

    pos = allNodes.begin() + popNodeNum;
    quality_ = std::for_each(allNodes.begin(), pos, 
			      TotalWorkload(inc, rho)).result(); 

}

//#############################################################################
// Add a node to node pool and adjust elite list. 
void 
AlpsSubTree::addNode(AlpsTreeNode* node, double quality)
{
    nodePool_->addKnowledge(node, quality);
    assert(eliteSize_ > 0);
    if (eliteList_.size() < eliteSize_) {
	eliteList_.insert(std::pair<double, AlpsTreeNode*>(quality, node));
    }
    else {  // ==   
	std::multimap<double, AlpsTreeNode*>::iterator posEnd = 
	    eliteList_.end();
	--posEnd;
	if (quality < posEnd->first) {
	    eliteList_.insert(std::pair<double, AlpsTreeNode*>
			      (quality, node));
	    while(eliteList_.size() > eliteSize_) {
		posEnd = eliteList_.end();
		eliteList_.erase(--pos);
	    }
	}
    }
}

//#############################################################################

void 
AlpsSubTree::popNode()
{
    AlpsTreeNode* node = topNode();
    nodePool_->popKnowledge();
    std::multimap<double, AlpsTreeNode*>::iterator pos1 = eliteList_.begin();
    std::multimap<double, AlpsTreeNode*>::iterator pos2 = eliteList_.end();
    for (; pos1 != pos2; ++pos1) {
	if (pos1->second == node) break;
    }
    if (pos1 != pos2) {
	eliteNodes.erase(pos1);
    }
}
#endif

//#############################################################################
