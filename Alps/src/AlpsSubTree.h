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

#ifndef AlpsSubTree_h_
#define AlpsSubTree_h_

#include <cassert>
#include <list>

#include "CoinError.hpp"
#include "CoinSort.hpp"

#include "AlpsCompareActual.h"
#include "AlpsKnowledge.h"
#include "AlpsNodePool.h"
#include "AlpsPriorityQueue.h"
#include "AlpsTreeNode.h"

class AlpsKnowledgeBroker;

//#############################################################################

/** This class contains the data pertaining to a particular subtree in the
    search tree. In order to improve scalability, we will try to deal with
    entire subtrees as much as possible. They will be the basic unit of work
    that will be passed between processes.
*/
class AlpsSubTree : public AlpsKnowledge {

 protected:

    /** The root of the sub tree. */
    AlpsTreeNode* root_;
   
    /** A node pool containing the leaf nodes awaiting processing. */
    AlpsNodePool* nodePool_;

    /** A node pool used when diving. */
    AlpsNodePool* diveNodePool_;
    
    /** Diving node comparing rule. */
    AlpsCompareBase<AlpsTreeNode*> * diveNodeRule_;

    //   /** The next index to be assigned to a new search tree node */
    //   AlpsNodeIndex_t nextIndex_;

    /** This is the node that is currently being processed. Note that since
	this is the worker, there is only one. */
    AlpsTreeNode* activeNode_;

    /** A quantity indicating how good this subtree is. */
    double quality_;

    /** A pointer to the knowledge broker of the process where this subtree is
	processed. */
    // Need broker to query model && parameters.
    AlpsKnowledgeBroker*  broker_;

    /** Elite list of nodes. */
    //    std::multimap<double, AlpsTreeNode*> eliteNodes_;
    
    /** The number of elite nodes stored. */
    //  int eliteSize_;
    
 protected:

    /** The purpose of this method is to remove nodes that are not needed in
	the description of the subtree. The argument node must have status
	<code>fathomed</code>. First, the argument node is removed, and then
	the parent is examined to determine whether it has any children
	left. If it has none, then this function is called recursively on the
	parent. This removes all nodes that are no longer needed. */
    void removeDeadNodes(AlpsTreeNode*& node) throw(CoinError);

    /** This function replaces \c oldNode with \c newNode in the tree. */
    void replaceNode(AlpsTreeNode* oldNode, AlpsTreeNode* newNode);

    /** Get pointer to active node */
    inline AlpsTreeNode* getActiveNode() { return activeNode_; }

    /** Set pointer to active node */
    inline void setActiveNode(AlpsTreeNode *activeNode)
	{ activeNode_ = activeNode; }

    /** Create children nodes from the given parent node. */
    void createChildren(AlpsTreeNode* parent,
			std::vector< CoinTriple<AlpsNodeDesc*, AlpsNodeStatus, 
			double> >& children,
                        AlpsNodePool *kidNodePool = NULL);

 public:
    
    /** Default constructor. */
    AlpsSubTree();
    
    /** Useful constructor. */
    AlpsSubTree(AlpsKnowledgeBroker* kb);
        
    /** Destructor. */
    virtual ~AlpsSubTree();
    
 public:

    /** @name query and set member functions
     */
    //@{
    /** Get the root node of this subtree. */
    inline AlpsTreeNode* getRoot() const { return root_; }

    /** Set the root node of this subtree. */
    inline void setRoot(AlpsTreeNode* r) { root_ = r; }

    /** Get the node pool. */
    inline AlpsNodePool* getNodePool() const { return nodePool_; }

    /** Set node pool. Delete previous node pool and elements in pool if exit.*/
    inline void setNodePool(AlpsNodePool* np) { 
	if (nodePool_ != NULL) {
	    delete nodePool_; 
	    nodePool_ = NULL;
	}
	nodePool_ = np;
    }

    /** Set node pool. Delete previous node pool, but not the elements in pool.*/
    inline void changeNodePool(AlpsNodePool* np) { 
	if (nodePool_ != NULL) {
	    // Remove all elements first.
	    nodePool_->clear();
	    // Delete an empty pool.
	    assert(nodePool_->hasKnowledge() == false);
	    delete nodePool_;
	    nodePool_ = NULL;
	}
	nodePool_ = np;
    }

    /** Get the "best value" of the nodes in node pool. */
    inline double getBestKnowledgeValue() const { 
      return nodePool_->getBestKnowledgeValue();
    }

    /** Get the knowledge broker. */
    inline AlpsKnowledgeBroker*  getKnowledgeBroker() const { return broker_; }

    /** Set a pointer to the knowledge broker. */
    inline void setKnowledgeBroker(AlpsKnowledgeBroker* kb) 
	{
	    assert(kb);
	    broker_ = kb;
	    //eliteSize_ = kb->getDataPool()->
	    //getOwnParams()->entry(AlpsOwnParams::eliteSize);
	    //assert(eliteSize_ > 0);
	}

    /** Get the quality of this subtree. */
    inline double getQuality() const { return quality_; };
    
    /** Calcuate  and return the quality of this subtree, which is measured
	by the quality of the specified number of nodes.*/
    // NOTE: Param inc (incumber value) and rho are not used.
    double calculateQuality(double inc, double rho);
 
    /* Get the index of the next generated node and increment next index
       by one.*/ 
    int nextIndex();

    /** Get the index of the next generated node.*/
    int getNextIndex() const;
    
    /** Set the index of the next generated node. */
    void setNextIndex(int next);

#if 0
    /** Approximate the quality of this subtree. It is cheap than 
	<code>calculateQuality()<\code> */
    void approximateQuality(double inc, double rho);

    /** Add a node to node pool and adjust elite list. */
    void addNode(AlpsTreeNode* node, double quality = ALPS_OBJ_MAX);

    /** Remove the node with highest priority from node pool and adjust 
	elite list. */
    void popNode();

    /** Get a pointer the node with highest priority. */
    AlpsTreeNode* topNode() {   
	return dynamic_cast<AlpsTreeNode*>
	    (nodePool_->popKnowledge().first);
    }
#endif
    
    /** Return the number of nodes on this subtree. */
    int getNumNodes() const {
	assert(nodePool_);
	return nodePool_->getNumKnowledges() + 
            diveNodePool_->getNumKnowledges();
    }

    /** Set the node comparision rule. */
    void setNodeCompare(AlpsCompareBase<AlpsTreeNode*>* nc) {
	nodePool_->setComparison(*nc);
    }
    //@}

    /** The function split the subtree and return a subtree of the 
	specified size or available size. */
    AlpsSubTree* splitSubTree(int& returnSize, int size = 10);
    
    /** Explore the subtree from \c root as the root of the subtree for given
	number of nodes or time, depending on which one reach first. */
    virtual AlpsReturnCode exploreSubTree(AlpsTreeNode* root,
					  int nodeLimit,  
					  double timeLimit,
					  int & numNodesProcesse, /* Output */
					  int & depth);           /* Output */
    
    /** Explore the subtree for certain amount of work/time. */
    AlpsReturnCode exploreUnitWork(int unitWork,
                                   double unitTime,
                                   AlpsSolStatus & solStatus,
                                   int & numNodesProcessed, /* Output */
                                   int & depth,             /* Output */
                                   bool & betterSolution);  /* Output */

    /** Generate certain number (specified by a parameter) of nodes. 
	This function is used by master and hubs. */
    virtual int rampUp(int& depth, AlpsTreeNode* root = NULL);
    
    /** This method should encode the content of the subtree and return a
	pointer to the encoded form. Only parallel code need this function. */
    virtual AlpsEncoded* encode() const;
    
    /** This method should decode and return a pointer to a \em brand \em new
	\em object, i.e., the method must create a new object on the heap from
	the decoded data instead of filling up the object for which the method
	was invoked. Only parallel code need this function.*/
    virtual AlpsKnowledge* decode(AlpsEncoded& encoded) const;

    /** Create a AlpsSubtree object dynamically. Only parallel code need 
	this function.*/    
    virtual AlpsSubTree* newSubTree() const {
	return new AlpsSubTree;
    }
};
#endif


//#############################################################################
// The way to create children:
//-----------------------------------------------------------------------------
// In AlpsSubTree::exploreSubTree(root)
// If (pregnant) 
// => KnapTreeNode::branch() 
// => AlpsSubTree::createChildren(...)  {
//   AlpsTreeNode::setNumChildren(...) (allocate memory if not);
//   KnapTreeNode:: createNewTreeNode(...); 
//   AlpsSubTree::setChildren;
//   AlspSubTree::setStatus }
//#############################################################################

//#############################################################################
// The way to remove nodes:
//-----------------------------------------------------------------------------
// In AlpsSubTree::exploreSubTree(root)
// If (fathomed)
// => AlpsSubTree::removeDeadNode(node) {
//      AlpsTreeNode::removeChild(node) {
//        AlpsTreeNode::removeDescendants();
//      }
//    Check whether parent has children; 
//      if (yes), recursively removeDeadNode(parent) 
//#############################################################################
