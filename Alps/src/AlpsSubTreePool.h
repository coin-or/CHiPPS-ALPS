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

#ifndef AlpsSubTreePool_h_
#define AlpsSubTreePool_h_

#include "AlpsHelperFunctions.h"
#include "AlpsSubTree.h"

//#############################################################################

/** The subtree pool is used to store subtrees */
class AlpsSubTreePool : public AlpsKnowledgePool {

 private:
    AlpsSubTreePool(const AlpsSubTreePool&);
    AlpsSubTreePool& operator=(const AlpsSubTreePool&);

    AlpsPriorityQueue<AlpsSubTree*> subTreeList_;

 public:
    AlpsSubTreePool() {}
    virtual ~AlpsSubTreePool() {
	if (!subTreeList_.empty()) {
	    deleteGuts();
	}
    }
   
    /** Query the number of subtrees in the pool. */
    inline int getNumKnowledges() const { return subTreeList_.size(); }
  
    /** Check whether there is a subtree in the subtree pool. */
    inline bool hasKnowledge() const{ return ! (subTreeList_.empty()); }
    
    /** Get a subtree from subtree pool, doesn't remove it from the pool*/
    inline std::pair<AlpsKnowledge*, double> getKnowledge() const {
	return std::make_pair( subTreeList_.top(), 
			       subTreeList_.top()->getQuality() );
    }

    /** Remove a subtree from the pool*/
    inline void popKnowledge() {
	subTreeList_.pop();
    }
  
    /** Add a subtree to the subtree pool. */
    inline void addKnowledge(AlpsKnowledge* subTree, double priority) {
	AlpsSubTree * st = dynamic_cast<AlpsSubTree* >(subTree);
	subTreeList_.push(st);
    }

    /** Return the container of subtrees. */  
    inline const AlpsPriorityQueue< AlpsSubTree*>&  
	getSubTreeList() const { return subTreeList_; }

    /** Set comparison function and resort heap. */
    void setComparison(AlpsSearchStrategy<AlpsSubTree*>& compare) {
	subTreeList_.setComparison(compare);
    }

    /** Delete the subtrees in the pool. */
    void deleteGuts() {
	std::vector<AlpsSubTree* > treeVec = subTreeList_.getContainer();
	for_each(treeVec.begin(), treeVec.end(), DeletePtrObject());
        subTreeList_.clear();
        assert(subTreeList_.size() == 0);
    }
};

#endif
