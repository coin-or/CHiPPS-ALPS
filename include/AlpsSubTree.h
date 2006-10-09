#include "AlpsLicense.h"

#ifndef AlpsSubTree_h_
#define AlpsSubTree_h_

#include <queue>

#include "CoinError.hpp"
#include "CoinUtility.h"
#include "CoinBoostWrapper.h"
#include "AlpsSolution.h"
#include "AlpsTreeNode.h"


//#############################################################################
/** This class contains the data pertaining to a particular subtree in the
    search tree. In order to improve scalability, we will try to deal with
    entire subtrees as much as possible. They will be the basic unit of work
    that will be passed between processes. */
//#############################################################################

class AlpsSubTree {

 protected:
   /** The root of the sub tree. */
   AlpsTreeNode* root_;
   
   /** A knowledge pool containing the leaf nodes awaiting processing. */
   AlpsNodePool* nodepool_;
   /** A knowledge pool containing the solutions found. */
   AlpsSolutionPool* solpool_;

   //  CoinSharedPtr<AlpsNodePool*> nodepool_;
   /** The next index to be assigned to a new search tree node */
   AlpsNodeIndex_t nextIndex_;

   /** The purpose of this method is to remove nodes that are not needed in
       the description of the subtree. The argument node must have status
       <code>fathomed</code>. First, the argument node is removed, and then
       the parent is examined to determine whether it has any children
       left. If it has none, then this function is called recursively on the
       parent. This removes all nodes that are no longer needed. */
   void removeDeadNodes(AlpsTreeNode*& node) throw(CoinError);

   /** This function replaces \c oldNode with \c newNode in the tree. */
   void replaceNode(AlpsTreeNode* oldNode, AlpsTreeNode* newNode);

 public:
   AlpsSubTree() : 
     root_(0), 
     nextIndex_(0), 
     nodepool_(new  AlpsNodePool), 
     solpool_(new  AlpsSolutionPool) { }
   virtual ~AlpsSubTree() { delete nodepool_; nodepool_ = NULL; }
 public:
  /** @name query and set member functions
   *
   */
  //@{
   inline int getNextIndex() { return nextIndex_; }
   inline void setNextIndex(int next) { nextIndex_ = next; }
 
   inline AlpsNodePool* getNodePool() { return nodepool_; }
   inline void setNodePool( AlpsNodePool* np) { nodepool_ = np; }
   
   inline AlpsSolutionPool* getSolutionPool() { return solpool_; }
   inline void setNodePool( AlpsSolutionPool* np) { solpool_ = np; }
   //@}

   /** Start to explore the subtree from \c root as the root of the
       subtree. This means iteratively pulling nodes out of the queue and
       performing the appropriate operation based on the node's status. */
   virtual void exploreSubTree(AlpsTreeNode* root) = 0;
   /** Explore the subtree for certain amount of work/time. */
   virtual void doOneUnitWork() = 0;

};

//#############################################################################
/** This derived class contains data and methods pertaining to a subtree that
    are only needed in the worker. */
//#############################################################################
//#############################################################################

class AlpsSubTreeWorker : public virtual AlpsSubTree { // Why virtual?

  // private:
 protected:
   /** This is the node that is currently being processed. Note that since
       this is the worker, there is only one. */
   AlpsTreeNode* activeNode_;

 protected:
   /** Get pointer to active node */
   inline AlpsTreeNode* getActiveNode() { return activeNode_; }

   /** Set pointer to active node */
   inline void setActiveNode(AlpsTreeNode *activeNode)
     { activeNode_ = activeNode; }

   /** */
   void createChildren(AlpsTreeNode* parent,
		       std::vector< CoinTriple<AlpsNodeDesc*, AlpsNodeStatus, 
		       double> >& children);

 public:
   /** Start to explore the subtree from \c root as the root of the
       subtree. This means iteratively pulling nodes out of the queue and
       performing the appropriate operation based on the node's status. */
   virtual void exploreSubTree(AlpsTreeNode* root);
   /** Explore the subtree for certain amount of work/time. */
   inline void doOneUnitWork();

};

//#############################################################################
/** This derived class from Worker. It contains data and methods 
    pertaining to a subtree that are only needed in the master. */
//#############################################################################
//#############################################################################

class AlpsSubTreeHub : public AlpsSubTreeWorker {

 public:
  /** Hub create enough nodes for workers and distribution them among
      workers. */
  void initializeSearch(AlpsTreeNode* root);
};

//#############################################################################
//#############################################################################


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
// The way to remove a child:
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
