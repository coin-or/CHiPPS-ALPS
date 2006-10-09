#include "AlpsLicense.h"

#ifndef AlpsSubTree_h_
#define AlpsSubTree_h_

#include <queue>

#include "CoinError.hpp"
#include "CoinUtility.h"
#include "CoinBoostWrapper.h"

#include "AlpsKnowledge.h"
#include "AlpsSolution.h"
#include "AlpsTreeNode.h"


//#############################################################################
/** This class contains the data pertaining to a particular subtree in the
    search tree. In order to improve scalability, we will try to deal with
    entire subtrees as much as possible. They will be the basic unit of work
    that will be passed between processes. This class contains data 
    and methods pertaining to a subtree that are only needed in the worker.*/

class AlpSubTree : public AlpsKnowledge {

 protected:
   /** The root of the sub tree. */
   AlpsTreeNode* root_;
   
   /** A knowledge pool containing the leaf nodes awaiting processing. */
   AlpsNodePool* nodepool_;

   // Move to broker
   // /** A knowledge pool containing the solutions found. */
   // AlpsSolutionPool* solpool_;

   // CoinSharedPtr<AlpsNodePool*> nodepool_;
   /** The next index to be assigned to a new search tree node */
   AlpsNodeIndex_t nextIndex_;

   /** This is the node that is currently being processed. Note that since
      this is the worker, there is only one. */
   AlpsTreeNode* activeNode_;

   /** A quantity indicating how good this subtree is. */
   double priority_;

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
		       double> >& children);

 public:
   AlpSubTree() : 
     root_(0), 
     nextIndex_(0), 
     nodepool_(new  AlpsNodePool), 
     //    solpool_(new  AlpsSolutionPool),
     activeNode_(0),
     priority_(0.0) { }

   virtual ~AlpSubTree() { 
     delete nodepool_; nodepool_ = 0; 
     //     delete solpool_, solpool_ = 0;
   }

 public:

  /** @name query and set member functions
   *
   */
  //@{
   inline int getNextIndex() const { return nextIndex_; }
   inline void setNextIndex(int next) { nextIndex_ = next; }
 
   inline AlpsNodePool* getNodePool() { return nodepool_; }
   inline void setNodePool( AlpsNodePool* np) { nodepool_ = np; }
   
   // inline AlpsSolutionPool* getSolutionPool() { return solpool_; }
   // inline void setSolutionPool( AlpsSolutionPool* np) { solpool_ = np; }

   inline double getPriority() const { return priority_; };
   inline void calPriority() {
     if (nodepool_->hasKnowledge())
       priority_ = nodepool_->getKnowledge().second;
     else
       priority_ = 0.0;
   }
   //@}

   /** Start to explore the subtree from \c root as the root of the
       subtree. This means iteratively pulling nodes out of the queue and
       performing the appropriate operation based on the node's status. */
   void exploreSubTree(AlpsTreeNode* root);

   /** Explore the subtree for certain amount of work/time. */
   void doOneUnitWork();

   /** The method packs the subtree into a buffer and return this buffer. */
   char* packSubTree() const;

   /** The method that unpack a subtree from a buffer. */
   void unpackSubTree(char* buf);
};
 
//#############################################################################
/** This derived class from Hub. It contains data and methods 
    pertaining to a subtree that are needed for a hub. */

class AlpsSubTreeHub : public AlpSubTree {
 
 public:
  /** Hub create enough nodes for its worker. */
  virtual void rampUp();
};


//#############################################################################
/** This derived class from Hub. It contains data and methods 
    pertaining to a subtree that are needed for a master. */

class AlpsSubTreeMaster : public AlpsSubTreeHub {

 public:
  /** Master create enough nodes for hubs. */
  void rampUp(AlpsTreeNode* root);
};


#if 1
//#############################################################################
/** This class is used to implement the comparison for the priority queue.   */
//#############################################################################

class subTreeCompare {

 public:

   inline bool operator()(const AlpSubTree* st1,
			  const AlpSubTree* st2) const {
      return(st1->getPriority() > st2->getPriority());
   }
};

//#############################################################################
/** The subtree pool stores subtrees */
class AlpsSubTreePool {//: public AlpsKnowledgePool {

 private:
  AlpsSubTreePool(const AlpsSubTreePool&);
  AlpsSubTreePool& operator=(const AlpsSubTreePool&);
  AlpsPriorityQueue<AlpSubTree*, std::vector<AlpSubTree*>,
    subTreeCompare > subTreeList_;

 public:
  AlpsSubTreePool() {}
  virtual ~AlpsSubTreePool() {}
   
  /** Query the number of subtrees in the pool. */
  inline int getNumSubtrees() const { return subTreeList_.size(); }
  
  /** Check whether there is a subtree in the subtree pool. */
  inline bool hasSubTree() const{ return ! (subTreeList_.empty()); }

  /** Get a subtree from subtree pool, doesn't remove it from the pool*/
  inline std::pair<const AlpSubTree*, double> getSubTree() const {
    return std::make_pair( subTreeList_.top(), 
			   subTreeList_.top()->getPriority() );
  }

  /** Remove a subtree from the pool*/
  inline void popSubTree() {
    subTreeList_.pop();
  }

   /** Add a node to node pool. The node pool takes over the 
       ownership of the node */
  inline void addSubTree(const AlpSubTree* subTree, 
			    double priority=0) {
    const AlpSubTree * st = 
      dynamic_cast<const AlpSubTree*>(subTree);
    //     if(!nn) {
    AlpSubTree * nonst = const_cast<AlpSubTree* >(st);
    subTreeList_.push(nonst);
    //     }
    //    else 
    // std::cout << "Add node failed\n";
    //     else
    // throw CoinError();
  }

  inline const AlpsPriorityQueue< AlpSubTree*, 
    std::vector<AlpSubTree*>, subTreeCompare >& 
    getSubTreeList() const { return subTreeList_; }
  
};
#endif

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


