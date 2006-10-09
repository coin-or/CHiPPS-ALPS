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

#ifndef AlpsCompareActual_h_
#define AlpsCompareActual_h_

#include "AlpsCompareBase.h"
#include "AlpsSubTree.h"
#include "AlpsTreeNode.h"

//#############################################################################

class AlpsCompareSubTreeBreadth : public AlpsCompareBase<AlpsSubTree*> 
{
 public:
    // Default Constructor 
    AlpsCompareSubTreeBreadth() {}
    ~AlpsCompareSubTreeBreadth() {};
    
    // This returns true if the depth of the root node in subtree x
    // is greater than depth of the root node in subtree y
    virtual bool test(AlpsSubTree * x, AlpsSubTree * y);
};

//#############################################################################

class AlpsCompareSubTreeBest : public AlpsCompareBase<AlpsSubTree*> 
{
 public:
    // Default Constructor 
    AlpsCompareSubTreeBest() {}
    ~AlpsCompareSubTreeBest() {};
    
    // This returns true if the quality of the subtree x 
    // is better than the quality of the subtree y
    virtual bool test(AlpsSubTree * x, AlpsSubTree * y);
};

//#############################################################################

class AlpsCompareSubTreeQuantity : public AlpsCompareBase<AlpsSubTree*> 
{
 public:
    // Default Constructor 
    AlpsCompareSubTreeQuantity() {}
    ~AlpsCompareSubTreeQuantity() {};
    
    // This returns true if the num of nodes in the subtree x 
    // is less than the num of nodes in the subtree y
    virtual bool test(AlpsSubTree * x, AlpsSubTree * y);
  
};

//#############################################################################

class AlpsCompareTreeNodeDepth : public AlpsCompareBase<AlpsTreeNode*> 
{
 public:
    // Default Constructor 
    AlpsCompareTreeNodeDepth() {}
    ~AlpsCompareTreeNodeDepth() {};

    // This returns true if the depth of node x is lesser than depth of node y
    virtual bool test (AlpsTreeNode * x, AlpsTreeNode * y) {
	return x->getDepth() < y->getDepth();
    }
};

//#############################################################################

class AlpsCompareTreeNodeBest : public AlpsCompareBase<AlpsTreeNode*> 
{
 public:
    // Default Constructor 
    AlpsCompareTreeNodeBest() {}
    ~AlpsCompareTreeNodeBest() {}

    // This returns true if the objective value (assume minization) of node x 
    // is greater than objective of node y
    virtual bool test (AlpsTreeNode * x, AlpsTreeNode * y) {
	return x->getQuality() > y->getQuality();
    }
};

//#############################################################################

class AlpsCompareTreeNodeEstimate : public AlpsCompareBase<AlpsTreeNode*> 
{
 public:
    // Default Constructor 
    AlpsCompareTreeNodeEstimate() {}
    ~AlpsCompareTreeNodeEstimate() {}

    // This returns true if the objective value (assume minization) of node x 
    // is greater than objective of node y
    virtual bool test (AlpsTreeNode * x, AlpsTreeNode * y) {
	return x->getSolEstimate() > y->getSolEstimate();
    }
};

//#############################################################################

class AlpsCompareTreeNodeDefault : public AlpsCompareBase<AlpsTreeNode*> 
{
 public:
    // Default Constructor 
    AlpsCompareTreeNodeDefault() {}
    ~AlpsCompareTreeNodeDefault() {}
    
    // This returns true if the quality (assume minization) of node x 
    // is greater than objective of node y
    virtual bool test(AlpsTreeNode * x, AlpsTreeNode * y) {
	if (weight_ == -1.0) {
	    // depth first
	    return x->getDepth() < y->getDepth();
	}
	else {
	    //best first
	    return x->getQuality() > y->getQuality();
	}
    }
};

//#############################################################################

class AlpsCompareTreeNodeBreadth : public AlpsCompareBase<AlpsTreeNode*> 
{
 public:
    // Default Constructor 
    AlpsCompareTreeNodeBreadth() {}
    ~AlpsCompareTreeNodeBreadth() {};

    // This returns true if the depth of node x is greater than depth of node y
    virtual bool test (AlpsTreeNode * x, AlpsTreeNode * y) {
	return x->getDepth() > y->getDepth();
    }
};

#endif
