#include "AlpsLicense.h"

#ifndef AlpsSubTree_h_
#define AlpsSubTree_h_

#include <queue>

#include "CoinError.hpp"
#include "CoinUtility.h"

#include "AlpsSolution.h"
#include "AlpsTreeNode.h"

//#############################################################################
/** This class is used to implement the comparison for the priority queue.   */
//#############################################################################

class nodeCompare {

 public:

   inline bool operator()(const AlpsTreeNode* node1,
			  const AlpsTreeNode* node2) const {
      return(node1->getPriority() > node2->getPriority());
   }
};

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

   /** A priority queue containing the leaf nodes awaiting processing. */
   //FIXME: We should implement our own priority queue, but for now, use
   //the built-in one. 
   std::priority_queue < AlpsTreeNode*,
                         std::vector<AlpsTreeNode*>,
                         nodeCompare > candidateList_;

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
   void replaceNode(AlpsTreeNode* node1, AlpsTreeNode* node2);

 public:
   AlpsSubTree() : root_(0), nextIndex_(0) {}
   virtual ~AlpsSubTree() {}
 public:
   int getNextIndex() { return nextIndex_; }
   void setNextIndex(int next) { nextIndex_ = next; }

};

//#############################################################################
/** This derived class contains data and methods pertaining to a subtree that
    are only needed in the worker. */
//#############################################################################
//#############################################################################

class AlpsSubTreeWorker : public virtual AlpsSubTree {

 private:
   /** This is the node that is currently being processed. Note that since
       this is the worker, there is only one. */
   AlpsTreeNode* activeNode_;

#if 0
   double priorityThreshold_; 
#endif

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
#if 0
   /** Takes the explicit description of the current active node and decides
       whether or not it should be fathomed. */
   virtual bool fathom();
#endif
 public:
   /** Start to explore the subtree from \c root as the root of the
       subtree. This means iteratively pulling nodes out of the queue and
       performing the appropriate operation based on the node's status. */
   virtual void exploreSubTree(AlpsTreeNode* root);

#if 0
   double getPriorityThreshold()
     { return priorityThreshold_; }

   void setPriorityThreshold(double threshold)
     { priorityThreshold_ = threshold; }
#endif
};

//#############################################################################
//#############################################################################

#endif
