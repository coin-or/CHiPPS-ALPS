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
 * Copyright (C) 2001-2019, Lehigh University, Yan Xu, Aykut Bulut, and      *
 *                          Ted Ralphs.                                      *
 * All Rights Reserved.                                                      *
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

  if (newNumNodes > 20000) {
    // at most 50 nodes.
    newNumNodes = CoinMin(20000, minNumNodes * 50);
    // at least 10 nodes.
    newNumNodes = CoinMax(newNumNodes, minNumNodes * 10);
  }

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
  AlpsKnowledge(AlpsKnowledgeTypeSubTree),
  root_(0),
  //nextIndex_(0),
  //nodePool_(new AlpsNodePool),
  //diveNodePool_(new AlpsNodePool),
  diveNodeRule_(new AlpsNodeSelectionBest),
  activeNode_(0),
  quality_(ALPS_OBJ_MAX)
{
  nodePool_ = new AlpsNodePool((AlpsSearchType)broker_->getModel()->AlpsPar()->
                               entry(AlpsParams::searchStrategy));
  diveNodePool_ = new AlpsNodePool((AlpsSearchType)broker_->getModel()->
                                   AlpsPar()->
                                   entry(AlpsParams::searchStrategy));

  diveNodePool_->setNodeSelection(*diveNodeRule_);
}

//#############################################################################

/** Useful constructor. */
AlpsSubTree::AlpsSubTree(AlpsKnowledgeBroker* kb)
  :
  AlpsKnowledge(AlpsKnowledgeTypeSubTree, kb),
  root_(0),
  // nextIndex_(0),
  // nodePool_(new AlpsNodePool),
  // diveNodePool_(new AlpsNodePool),
  diveNodeRule_(new AlpsNodeSelectionBest),
  activeNode_(0),
  quality_(ALPS_OBJ_MAX)
{
  //eliteSize_ = kb->getDataPool()->
  //getOwnParams()->entry(AlpsParams::eliteSize);

  nodePool_ = new AlpsNodePool((AlpsSearchType)broker_->getModel()->AlpsPar()->
                               entry(AlpsParams::searchStrategy));
  diveNodePool_ = new AlpsNodePool((AlpsSearchType)broker_->getModel()->
                                   AlpsPar()->
                                   entry(AlpsParams::searchStrategy));

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

/** Fathom all nodes in this subtree.
 *  Set activeNode_ and root_ to NULL.
 */
void
AlpsSubTree::fathomAllNodes()
{
  if (nodePool_ != NULL) {
    nodePool_->clear(); // Nodes will be freed by deleting root
  }

  if (diveNodePool_ != NULL) {
    diveNodePool_->clear(); // Nodes will be freed by deleting root
  }

  if (root_ != NULL) {
    //std::cout << "- delete root" << std::endl;
    root_->removeDescendants();
    delete root_;
    root_ = NULL;
  }

  activeNode_ = NULL;
}

//#############################################################################

void
AlpsSubTree::removeDeadNodes(AlpsTreeNode*& node)
{
  if (!node->isFathomed() && !node->isDiscarded()) {
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
  if (activeNode_==node) activeNode_ = NULL;
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
  const int msgLevel =
    broker_->getModel()->AlpsPar()->entry(AlpsParams::msgLevel);
  const int numChildren = static_cast<int> (children.size());

  parent->setNumChildren(numChildren);

  if (!numChildren){
    return;
  }

  parent->setStatus(AlpsNodeStatusFathomed);

  if (msgLevel >= 100){
    std::cout << std::endl;
    std::cout << "Creating children of node " << parent->getIndex();
    std::cout << " with indices: ";
  }

  for (i = 0; i < numChildren; ++i) {
    AlpsTreeNode* child = parent->createNewTreeNode(children[i].first);
    parent->setChild(i, child);
    child->setStatus(children[i].second);
    child->setQuality(children[i].third);
    child->setParent(parent);
    child->setParentIndex(parent->getIndex());
    child->setBroker(parent->broker_);
    child->setActive(false);
    child->setDepth(parent->getDepth() + 1);
    child->setIndex(nextIndex());
    if (msgLevel >= 100){
      std::cout << child->getIndex() << " ";
    }
  }

  if (msgLevel >= 100){
    std::cout << std::endl;
  }

  for (i = 0; i < numChildren; ++i) {
    AlpsTreeNode* child = parent->getChild(i);
    switch (child->getStatus()) {
    case AlpsNodeStatusCandidate:
    case AlpsNodeStatusEvaluated:
    case AlpsNodeStatusPregnant:
      parent->setStatus(AlpsNodeStatusBranched);
      if (diveNodePool) {
        diveNodePool->addKnowledge(child, child->getSolEstimate());
      }
      else {
        nodePool_->addKnowledge(child, child->getQuality());
      }
      break;
    case AlpsNodeStatusFathomed:
    case AlpsNodeStatusDiscarded:
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
  return broker_->nextNodeIndex();
}

//#############################################################################

int
AlpsSubTree::getNextIndex() const
{
  return broker_->getNextNodeIndex();
}

//#############################################################################

void
AlpsSubTree::setNextIndex(int next)
{
  broker_->setNextNodeIndex(next);
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
  const int nodeNum = nodePool_->getNumKnowledges();
  const int diveNum = diveNodePool_->getNumKnowledges();

  // Check if no node.
  if ( (nodeNum + diveNum <= 0) && (activeNode_ == NULL) ) {
    std::cout << "PROC[" << broker_->getProcRank()
              << "] has a subtree with no node" << std::endl;
    assert(0);
  }

  if ( ((nodeSelectionType == AlpsSearchTypeBestFirst) ||
        (nodeSelectionType == AlpsSearchTypeHybrid)) &&
       (eliteSize == 1) ) {
    if (nodeNum) {
      quality_ = nodePool_->getKnowledge().second;
    }
    if (diveNum) {
      quality_ = CoinMin(quality_, diveNodePool_->getKnowledge().second);
    }
    if (activeNode_) {
      quality_ = CoinMin(quality_, activeNode_->getQuality());
    }
    //std::cout << "quality_ = " << quality_ << std::endl;
    return quality_;
  }

  // Diving pool has no nodes if not using hybrid search.
  assert(diveNum == 0);

  if (activeNode_) {
    if ( (activeNode_->getStatus() != AlpsNodeStatusFathomed) &&
         (activeNode_->getStatus() != AlpsNodeStatusDiscarded) &&
         (activeNode_->getStatus() != AlpsNodeStatusBranched) ) {
      quality_ = activeNode_->getQuality();
    }
  }

  // Get best quality values of nodes in node pool.
  std::vector<AlpsTreeNode* > nodeVector =
    nodePool_->getCandidateList().getContainer();
  std::vector<AlpsTreeNode* >::iterator pos = nodeVector.begin();

  std::multimap<double, AlpsTreeNode*> eliteList;
  std::multimap<double, AlpsTreeNode*>::iterator posEnd;

  for (int i = 0; i < nodeNum; ++i) {
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
        std::pair<const double,AlpsTreeNode*> sa(quality,node);
        eliteList.insert(sa);

      }
      else {  // ==
        posEnd = eliteList.end();
        --posEnd;
        if (quality < posEnd->first) {
          std::pair<const double, AlpsTreeNode*> sa(quality, node);
          eliteList.insert(sa);
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
    quality_ /= static_cast<int>(eliteList.size());
  }

  return quality_;
}

//#############################################################################

AlpsReturnStatus
AlpsSubTree::exploreSubTree(AlpsTreeNode* root,
                            int nodeLimit,
                            double timeLimit,
                            int & numNodesProcessed, /* Output */
                            int & numNodesBranched,  /* Output */
                            int & numNodesDiscarded, /* Output */
                            int & numNodesPartial,  /* Output */
                            int & depth)             /* Output */
{
  AlpsReturnStatus status = AlpsReturnStatusOk;
  AlpsExitStatus exploreStatus = AlpsExitStatusInfeasible;

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
                           numNodesBranched,  /* Output */
                           numNodesDiscarded, /* Output */
                           numNodesPartial,  /* Output */
                           depth,             /* Output */
                           betterSolution);   /* Output */

  if (exploreStatus == AlpsExitStatusNodeLimit) {
    broker_->setExitStatus(AlpsExitStatusNodeLimit);
  }
  else if (exploreStatus == AlpsExitStatusTimeLimit) {
    broker_->setExitStatus(AlpsExitStatusTimeLimit);
  }
  else if (exploreStatus == AlpsExitStatusUnbounded) {
    broker_->setExitStatus(AlpsExitStatusUnbounded);
  }
  else {
    // Search to end.
    if (broker_->hasKnowledge(AlpsKnowledgeTypeSolution)) {
      broker_->setExitStatus(AlpsExitStatusOptimal);
    }
    else {
      broker_->setExitStatus(AlpsExitStatusInfeasible);
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
      if (static_cast<int> (children.size()) > 0){
        createChildren(node, children);
        if (depth < node->getDepth() + 1) {    // Record the depth of tree
          depth = node->getDepth() + 1;
        }
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
      else {
        firstCall = false;
      }

      switch (node->getStatus()) {
      case AlpsNodeStatusCandidate :
      case AlpsNodeStatusEvaluated :
      case AlpsNodeStatusPregnant :
        nodePool_->addKnowledge(node, node->getQuality());
        break;
      case AlpsNodeStatusFathomed :
      case AlpsNodeStatusDiscarded :
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
        broker_->messageHandler()->message(ALPS_RAMPUP_MASTER_NODES_AUTO,
                                            broker_->messages())
          << broker_->getProcRank()
                << requiredNumNodes
                << npTime
                << CoinMessageEol;

        }
      else if ( (broker_->getHubMsgLevel() > 1) &&
                (broker_->getProcType() == AlpsProcessTypeHub) ) {
        broker_->messageHandler()->message(ALPS_RAMPUP_HUB_NODES_AUTO,
                                            broker_->messages())
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

    int nodeMemSize = broker_->getNodeMemSize();
    int LS = broker_->getLargeSize()/2;
    int maxAllowNodes = LS / nodeMemSize;

    if (maxAllowNodes == 0) {
        returnSize = 0;
        return st;
    }

    // At most send 50 nodes
    if (nodeMemSize < 10000) {
        maxAllowNodes = (maxAllowNodes > 100) ? 100 : maxAllowNodes;
    }
    else if (nodeMemSize < 50000) {
        maxAllowNodes = (maxAllowNodes > 50) ? 30 : maxAllowNodes;
    }
    else if (nodeMemSize < 100000) {
        maxAllowNodes = (maxAllowNodes > 30) ? 20 : maxAllowNodes;
    }
    else if (nodeMemSize < 500000) {
        maxAllowNodes = (maxAllowNodes > 10) ? 5 : maxAllowNodes;
    }
    else if (nodeMemSize < 1000000) {
        maxAllowNodes = (maxAllowNodes > 3) ? 3 : maxAllowNodes;
    }
    else {
        maxAllowNodes = 1;
    }

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
        if ((6 * numInPool > numNode) || numSendNode >= maxAllowNodes) {
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

    st = new AlpsSubTree(broker_);
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

// Encode this into the given AlpsEncoded object.
AlpsReturnStatus AlpsSubTree::encode(AlpsEncoded * encoded) const {
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

  std::vector<AlpsTreeNode* > nodeVector;
  std::stack<AlpsTreeNode* > nodeStack;

  int i = -1, nodeNum = 0;
  int numChildren = 0;

  AlpsTreeNode* curNode = NULL;

  nodeStack.push(root_);

  while( !nodeStack.empty() ) {
    curNode = nodeStack.top();
    nodeStack.pop();
    nodeVector.push_back(curNode);         // The first is root_

    numChildren = curNode->getNumChildren();
    for (i = 0; i < numChildren; ++i) {
      nodeStack.push(curNode->getChild(i));
    }
  }

  nodeNum = static_cast<int> (nodeVector.size());

  encoded->writeRep(nodeNum);              // First write number of nodes

  AlpsTreeNode* node = NULL;

  for (i = 0; i < nodeNum; ++i) {          // Write all nodes to rep of enc
    node = nodeVector[i];
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

  return AlpsReturnStatusOk;
}


// Decode the given AlpsEncoded object into a new object and return a
// pointer to it.
AlpsKnowledge * AlpsSubTree::decode(AlpsEncoded & encoded) const {
  int i = -1, j = -1;
  int nodeNum = 0;
  int fullOrPartial = -1;
  char* buf = 0;
  int size = 0;
  int* numAddedChildren = 0;

  AlpsSubTree* st = new AlpsSubTree(broker_);

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

    // readRep allocate memory for buf.
    encoded.readRep(buf, size);

    // take over ownership of buf
    encodedNode = new AlpsEncoded(AlpsKnowledgeTypeNode, size, buf);

    node = dynamic_cast<AlpsTreeNode* >
      ( (broker_->decoderObject(encodedNode->type()))->decode(*encodedNode) );

    //node->setSubTree(st);
    node->setBroker(broker_);
    AlpsModel* mod = broker_->getModel();
    // todo(aykut) node desc does not hold a pointer to the model
    // it holds  pointer to the broker.
    //node->modifyDesc()->setModel(mod);

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
  nodeReceived = static_cast<int> (nodeVector.size());

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
  st->setBroker(broker_);
  st->setNodeSelection(broker_->getNodeSelection());

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

/// Decode the given AlpsEncoded object into this.
AlpsReturnStatus AlpsSubTree::decodeToSelf(AlpsEncoded & encoded)  {
  std::cerr << "Not implemented!"  << std::endl;
  throw std::exception();
}

//#############################################################################

AlpsReturnStatus
AlpsSubTree::exploreUnitWork(bool leaveAsIt,
                             int unitWork,
                             double unitTime,
                             AlpsExitStatus & exploreStatus, /* Output */
                             int & numNodesProcessed,       /* Output */
                             int & numNodesBranched,        /* Output */
                             int & numNodesDiscarded,       /* Output */
                             int & numNodesPartial,        /* Output */
                             int & depth,                   /* Output */
                             bool & betterSolution)         /* Output */
{
    // Start to count time.
  broker_->subTreeTimer().start();

    AlpsReturnStatus status = AlpsReturnStatusOk;
    AlpsNodeStatus oldStatus = AlpsNodeStatusCandidate;
    int numNodesFathomed(0), numNodesCandidate(1), oldNumNodesCandidate(1);

    bool forceLog = false;
    // call logNode only when numNodesProcessed is updated.
    bool logFlag = false;
    bool exitIfBetter = false;

    double oldSolQuality = ALPS_OBJ_MAX;
    double newSolQuality = ALPS_OBJ_MAX;

    AlpsTreeNode * tempNode = NULL;

    //------------------------------------------------------
    // Get setting/parameters.
    //------------------------------------------------------

    const bool deleteNode =
      broker_->getModel()->AlpsPar()->entry(AlpsParams::deleteDeadNode);

    const bool deletePrunedNodes =
	broker_->getModel()->AlpsPar()->entry(AlpsParams::deletePrunedNodes);

    AlpsSearchStrategy<AlpsTreeNode*> *nodeSel = broker_->getNodeSelection();

#ifdef ALPS_MEMORY_USAGE
    bool checkMemory = broker_->getModel()->AlpsPar()->
        entry(AlpsParams::checkMemory);
#endif

    //------------------------------------------------------
    // Check if required to exit when a solution is found.
    //------------------------------------------------------

    if (betterSolution) {
        // Need to exit when a better solution is found.
        exitIfBetter = true;
        betterSolution = false;
    }

    if( broker_->hasKnowledge(AlpsKnowledgeTypeSolution) ) {
        oldSolQuality =
          broker_->getBestKnowledge(AlpsKnowledgeTypeSolution).second;
    }

    //------------------------------------------------------
    // Process nodes until limits are reached or a better solution found.
    //------------------------------------------------------

    if (!leaveAsIt) {
        activeNode_ = NULL;
        exploreStatus = AlpsExitStatusInfeasible;
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

	if (numNodesProcessed + numNodesPartial > unitWork) {
	    exploreStatus = AlpsExitStatusNodeLimit;
	    break;
	}
	else if (broker_->subTreeTimer().getTime() > unitTime){
	    exploreStatus = AlpsExitStatusTimeLimit;
	    break;
	}

	assert(numNodesProcessed == numNodesBranched + numNodesFathomed);
	assert(nodePool_->getNumKnowledges() +
               diveNodePool_->getNumKnowledges() ==
               numNodesCandidate + numNodesPartial);

	// Get the next node to be processed.
	activeNode_ = nodeSel->selectNextNode(this);

	switch (activeNode_->getStatus()) {
	case AlpsNodeStatusPregnant:
	    if (depth < activeNode_->getDepth() + 1) {
		depth = activeNode_->getDepth() + 1;
	    }
	    oldNumNodesCandidate = nodePool_->getNumKnowledges();
	    nodeSel->createNewNodes(this, activeNode_);
	    numNodesCandidate += nodePool_->getNumKnowledges() -
	       oldNumNodesCandidate;
#if 0
            //Eliminated by Aykut
            if (diveNodePool_){
               numNodesCandidate += diveNodePool_->getNumKnowledges();
            }
#endif
	    --numNodesPartial;
	    ++numNodesProcessed;
	    switch (activeNode_->getStatus()) {
	    case AlpsNodeStatusBranched :
		++numNodesBranched;
		break;
	    case AlpsNodeStatusFathomed :
		++numNodesFathomed;
		if (deleteNode) {
		    removeDeadNodes(activeNode_);
		}
		break;
	    case AlpsNodeStatusDiscarded :
	    case AlpsNodeStatusPregnant :
	    case AlpsNodeStatusCandidate :
	    case AlpsNodeStatusEvaluated :
	    default :
		throw CoinError("Unexpected node status",
				"exploreSubTree", "AlpsSubTree");
	    }
	    logFlag = true;
	    break;
	case AlpsNodeStatusCandidate:
	case AlpsNodeStatusEvaluated:
            activeNode_->setActive(true);
            if ((oldStatus = activeNode_->getStatus()) ==
                AlpsNodeStatusEvaluated){
               --numNodesPartial;
            }else{
               --numNodesCandidate;
            }
            if (activeNode_ == root_) {
                activeNode_->process(true);
            }
            else {
                activeNode_->process();
            }
            activeNode_->setActive(false);

            // Record the new sol quality if have.
            if( broker_->hasKnowledge(AlpsKnowledgeTypeSolution) ) {
                newSolQuality =
                  broker_->getBestKnowledge(AlpsKnowledgeTypeSolution).second;
                if (newSolQuality < oldSolQuality) {
                    if (exitIfBetter) {
                        betterSolution = true;
                    }
                    forceLog = true;
                    exploreStatus = AlpsExitStatusFeasible;
                    oldSolQuality = newSolQuality;
                    // std::cout << "betterSolution value=" << newSolQuality
                    //  << std::endl;
                }
            }


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
            case AlpsNodeStatusPregnant :
            case AlpsNodeStatusEvaluated :
                ++numNodesPartial;
                /* Has to go back in the queue for further consideration */
                nodePool_->addKnowledge(activeNode_, activeNode_->getQuality());
                break;
            case AlpsNodeStatusFathomed :
                ++numNodesProcessed;
                ++numNodesFathomed;
                if (deleteNode) {
                    removeDeadNodes(activeNode_);
                }
                logFlag = true;
                break;
            case AlpsNodeStatusDiscarded :
                // Node cannot be marked discarded if already partially processed
                if (oldStatus == AlpsNodeStatusCandidate){
                   ++numNodesDiscarded;
                   if (deleteNode) {
                      removeDeadNodes(activeNode_);
                   }
                   break;
                }
            case AlpsNodeStatusCandidate :
                // Status cannot be left as or changed back to candidate
            default :
                throw CoinError("Status is unknown or not allowed",
                                "exploreSubTree", "AlpsSubTree");
            }
            break;
        default : // branched or fathomed
            throw CoinError("Impossible status: branched or fathomed",
                            "exploreSubTree", "AlpsSubTree");
        }

        // Print node log for serial code.
        if (logFlag || forceLog) {
          broker_->getModel()->nodeLog(activeNode_, forceLog);
          logFlag = false;
          forceLog = false;
        }
        activeNode_ = NULL;

        /* Delete all nodes if required. */
        if (broker_->getModel()->fathomAllNodes()) {
	    //Suresh: added temporarily for warm starting
	    if (!deletePrunedNodes) {
		/* Delete all nodes on this subtree. */
		numNodesDiscarded += nodePool_->getNumKnowledges()
		    - numNodesPartial;
		fathomAllNodes();
	    }
        }
    }

    if (numNodesProcessed + numNodesPartial) {
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

    if ( (exploreStatus == AlpsExitStatusTimeLimit) ||
         (exploreStatus == AlpsExitStatusNodeLimit) ||
         (exploreStatus == AlpsExitStatusFeasible) ) {
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
             (activeNode_->getStatus() != AlpsNodeStatusFathomed &&
              activeNode_->getStatus() != AlpsNodeStatusDiscarded) ) {
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
        (activeNode_->getStatus() != AlpsNodeStatusFathomed &&
         activeNode_->getStatus() != AlpsNodeStatusDiscarded) ) {

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
