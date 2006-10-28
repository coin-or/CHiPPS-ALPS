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

#ifndef AlpsNodePool_h_
#define AlpsNodePool_h_

#include <vector>

#include "AlpsHelperFunctions.h"
#include "AlpsPriorityQueue.h"
#include "AlpsTreeNode.h"
#include "AlpsKnowledgePool.h"

//#############################################################################
/** Node pool is used to store the nodes to be processed. */
//#############################################################################

class AlpsNodePool : public AlpsKnowledgePool {
    
 private:
    AlpsNodePool(const AlpsNodePool&);
    AlpsNodePool& operator=(const AlpsNodePool&);
    
    AlpsPriorityQueue<AlpsTreeNode*> candidateList_;
    
 public:
    AlpsNodePool() {}
    virtual ~AlpsNodePool() {
	if (!candidateList_.empty()) {
	    clean();
	}
    }
    
    /** Query the number of nodes in the node pool. */
    inline int getNumKnowledges() const { return candidateList_.size(); }
    
    /** Get the "best value" of the nodes in node pool. */
    inline double getBestKnowledgeValue() const { 
        const std::vector<AlpsTreeNode *>& pool=candidateList_.getContainer();
        int k;
        int size = pool.size();
        double bestQuality = ALPS_OBJ_MAX;
        AlpsTreeNode * node = NULL;
        for (k = 0; k < size; ++k) {
            node = pool[k];
            if (node->getQuality() < bestQuality) {
                bestQuality = node->getQuality();
            }
        }
        return bestQuality;
    }

    /** Get the "best" nodes in node pool. */
    inline AlpsTreeNode *getBestNode() const { 
        const std::vector<AlpsTreeNode *>& pool=candidateList_.getContainer();
        int k;
        int size = pool.size();
        double bestQuality = ALPS_OBJ_MAX;
        AlpsTreeNode * bestNode = NULL;
        AlpsTreeNode * node = NULL;
        
        for (k = 0; k < size; ++k) {
            node = pool[k];
            if (node->getQuality() < bestQuality) {
                bestQuality = node->getQuality();
                bestNode = node;
            }
        }
        return bestNode;
    }
    
    /** Check whether there are still nodes in the node pool. */
    inline bool hasKnowledge() const{ return ! (candidateList_.empty()); }
    
    /** Get the node with highest priority. Doesn't remove it from the pool*/
    inline std::pair<AlpsKnowledge*, double> getKnowledge() const {
	return std::make_pair( candidateList_.top(), 
			       candidateList_.top()->getQuality() );
    }
    
    /** Remove the node with highest priority from the pool*/
    inline void popKnowledge() {
	candidateList_.pop();
    }

    /** Remove the node with highest priority from the pool and the elite 
	list*/
    /** Add a node to node pool. */
    inline void addKnowledge(AlpsKnowledge* node, double priority) {
	AlpsTreeNode * nn = dynamic_cast<AlpsTreeNode*>(node);
	//     if(!nn) {
	//AlpsTreeNode * nonnn = const_cast<AlpsTreeNode*>(nn);
	candidateList_.push(nn);
	//     }
	//    else 
	// std::cout << "Add node failed\n";
	//     else
	// throw CoinError();
    }

    /** Get a constant reference to the priority queue that stores nodes. */
    inline const AlpsPriorityQueue<AlpsTreeNode*>&
	getCandidateList() const { return candidateList_; }
    
    /** Set strategy and resort heap. */
    void setComparison(AlpsSearchStrategy<AlpsTreeNode*>& compare) {
	candidateList_.setComparison(compare);
    }

    /** Delete all the nodes in the pool. */
    void clean() {
	std::vector<AlpsTreeNode* > nodeVec = candidateList_.getContainer();
	for_each(nodeVec.begin(), nodeVec.end(), DeletePtrObject());
    }

    /** Remove all the nodes in the pool. */
    void clear() {
	candidateList_.clear();
    }

};

#endif

//#############################################################################
/** This class is used to implement the comparison for the priority queue.   */
//#############################################################################

//class nodeCompare {
//
// public:
//
//    inline bool operator()(const AlpsTreeNode* node1,
//			   const AlpsTreeNode* node2) const {
//	return(node1->getGoodness() > node2->getGoodness());
//    }
//};
