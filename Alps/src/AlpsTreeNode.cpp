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
 * Copyright (C) 2001-2011, Lehigh University, Yan Xu, and Ted Ralphs.       *
 *===========================================================================*/

#include <algorithm>

#include "AlpsKnowledgeBroker.h"
#include "AlpsTreeNode.h"
#include "AlpsSubTree.h"

//#############################################################################
//#############################################################################

void
AlpsTreeNode::removeChild(AlpsTreeNode*& child)
{

#if 0
    std::cout << "removeChild: Begin: numChildren_=" << numChildren_ 
              << ", index_=" << index_
              << ", depth_=" << depth_ << std::endl;
#endif

    assert(child);

#if 0
    // child_p is a pointer to AlpsTreeNode*
    AlpsTreeNode** child_p =  
        std::find(children_, children_ + numChildren_, child);

    if ( child_p == children_ + numChildren_ ) {
        // Could not find child.
        assert(0);
        throw CoinError("removeChild", "AlpsTreeNode",
                        "The argument is not a child of this node.");
    }

    // Put the last child in the slot to be deleted, 
    // so that can safely delete child and reduce num of children by 1.
    --numChildren_;

    // ERROR: can not do following, since child is an alias of
    // *child_p, later cause delete wrong child.
    *child_p = children_[numChildren_];
    assert(*child_p);
#else
    int i;
    for (i = 0; i < numChildren_; ++i) {
        if (child == children_[i]) break;
    }
    if (i == numChildren_) {
        // Could not find child.
        assert(0);
        throw CoinError("removeChild", "AlpsTreeNode",
                        "The argument is not a child of this node.");
    }
#endif

    AlpsTreeNode *childToDel = child;

    // Recursely delete its descendants.
    childToDel->removeDescendants();

#if 0
    std::cout << "removeChild: End: numChildren_="<<numChildren_ 
              << ", index_=" << index_
              << ", depth_=" << depth_ 
	      << ", children_[i] index = "<< children_[i]->getIndex() 
	      << std::endl;
#endif
    
    // Delete child node.
    delete children_[i];
    
    // Put the last child in the slot to be deleted, 
    // so that can safely delete child and reduce num of children by 1.
    --numChildren_;
    if (i != numChildren_) {
        children_[i] = children_[numChildren_];
        assert(children_[i]); 
    }
    
}

//#############################################################################

void
AlpsTreeNode::addChild(AlpsTreeNode*& child)
{
    //FIXME: Is this right? How does this node get deleted?
    // Can use only if enough memory is allocated.
    children_[numChildren_++] = child;
}

//#############################################################################

void
AlpsTreeNode::removeDescendants()
{
    // *FIXME* : Sanity Check. We might want to check whether the status of
    // this node is internal. Otherwise, we run the risk of just changing the
    // status on a node that is a candidate by accident. */

#ifdef NF_DEBUG_MORE
    std::cout << "removeDescendants: numChildren_="<<numChildren_ 
              << ", index_=" << index_ 
              << ", depth_=" << depth_ << std::endl;
#endif

    while ( numChildren_ > 0 ) {
        assert(children_[0]);
        removeChild(children_[0]);
    }
    status_ = AlpsNodeStatusFathomed;
}

//#############################################################################

/** Pack Alps portion of node into an encoded object. */
AlpsReturnStatus 
AlpsTreeNode::encodeAlps(AlpsEncoded *encoded) const 
{
    AlpsReturnStatus status = AlpsReturnStatusOk;
    
    encoded->writeRep(explicit_);
    encoded->writeRep(index_);
    encoded->writeRep(depth_);
    encoded->writeRep(solEstimate_);
    encoded->writeRep(quality_);
    encoded->writeRep(parentIndex_);
    encoded->writeRep(numChildren_);
    encoded->writeRep(status_);
    encoded->writeRep(sentMark_);
    
#ifdef NF_DEBUG
    std::cout << std::endl;
    std::cout << "index_ = " << index_ << "; ";
    std::cout << "depth_ = " << depth_ << "; ";
    std::cout << "quality_ = " << quality_ << "; ";
    std::cout << "parentIndex_ = " << parentIndex_ << "; ";
    std::cout << "numChildren_ = " << numChildren_ << std::endl;
    std::cout << "status_ = " << status_ << std::endl;
#endif
    
    return status;
}

//#############################################################################

/** Unpack Alps portion of node from an encoded object. */
AlpsReturnStatus 
AlpsTreeNode::decodeAlps(AlpsEncoded &encoded) 
{
    AlpsReturnStatus status = AlpsReturnStatusOk;
    
    encoded.readRep(explicit_);
    encoded.readRep(index_);
    encoded.readRep(depth_);
    encoded.readRep(solEstimate_);
    encoded.readRep(quality_);
    encoded.readRep(parentIndex_);
    encoded.readRep(numChildren_);
    encoded.readRep(status_);
    encoded.readRep(sentMark_);
    
#ifdef NF_DEBUG
    std::cout << std::endl;
    std::cout << "index_ = " << index_ << "; ";
    std::cout << "depth_ = " << depth_ << "; ";
    std::cout << "quality_ = " << quality_ << "; ";
    std::cout << "parentIndex_ = " << parentIndex_ << "; ";
    std::cout << "numChildren_ = " << numChildren_ << std::endl;
    std::cout << "status_ = " << status_ << std::endl;
#endif

    return status;
}    

//#############################################################################
