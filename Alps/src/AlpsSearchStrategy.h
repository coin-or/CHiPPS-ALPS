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

#ifndef AlpsSearchStrategy_h_
#define AlpsSearchStrategy_h_

#include "AlpsSearchStrategyBase.h"
#include "AlpsSubTree.h"
#include "AlpsTreeNode.h"

//#############################################################################
//#############################################################################

class AlpsTreeSelection : public AlpsSearchStrategy<AlpsSubTree*> 
{
public:
    /** Default Constructor. */
    AlpsTreeSelection() {}

    /** Default Destructor. */
    virtual ~AlpsTreeSelection() {}
    
    /** This returns true if the quality of the subtree y is better
        (the less the better) than that the subtree x. */
    virtual bool compare(AlpsSubTree * x, AlpsSubTree * y) = 0;
};

//#############################################################################

class AlpsNodeSelection : public AlpsSearchStrategy<AlpsTreeNode*> 
{
public:
    /** Default Constructor. */
    AlpsNodeSelection() {}
    
    /** Default Destructor. */
    virtual ~AlpsNodeSelection() {};
    
    /** This returns true if the depth of node y is lesser
	than that of node x */
    virtual bool compare(AlpsTreeNode * x, AlpsTreeNode * y) = 0;
    
    /* Select the next node to be processed. */
    virtual AlpsTreeNode* selectNextNode(AlpsSubTree *subTree);
    
    /* Create new nodes from pregnant node and store them in node pool. */
    virtual void createNewNodes(AlpsSubTree *subTree, AlpsTreeNode *node);
};

//#############################################################################
// SUBTREE SELECTION RULES
//#############################################################################

class AlpsTreeSelectionBest : public AlpsTreeSelection
{
public:
    /** Default Constructor. */
    AlpsTreeSelectionBest() { type_ = ALPS_SEARCH_BEST; }

    /** Default Destructor. */
    virtual ~AlpsTreeSelectionBest() {}
    
    /** This returns true if the quality of the subtree y is better
        (the less the better) than that the subtree x. */
    virtual bool compare(AlpsSubTree * x, AlpsSubTree * y);
};

//#############################################################################

class AlpsTreeSelectionBreadth : public AlpsTreeSelection
{
public:
    /** Default Constructor */
    AlpsTreeSelectionBreadth() { type_ = ALPS_SEARCH_BREATH; }
    
    /** Default Destructor. */
    virtual ~AlpsTreeSelectionBreadth() {}
    
    /** This returns true if the depth of the root node in subtree y
        is smaller than that of the root node in subtree x. */
    virtual bool compare(AlpsSubTree * x, AlpsSubTree * y);
};

//#############################################################################

class AlpsTreeSelectionDepth : public AlpsTreeSelection
{
public:
    /** Default Constructor */
    AlpsTreeSelectionDepth() { type_ = ALPS_SEARCH_DEPTH; }
    
    /** Default Destructor. */
    virtual ~AlpsTreeSelectionDepth() {}
    
    /** This returns true if the depth of the root node in subtree y
        is greater than that of the root node in subtree x. */
    virtual bool compare(AlpsSubTree * x, AlpsSubTree * y);
};

//#############################################################################

class AlpsTreeSelectionEstimate : public AlpsTreeSelection
{
public:
    /** Default Constructor. */
    AlpsTreeSelectionEstimate() { type_ = ALPS_SEARCH_BEST_EST; }

    /** Default Destructor. */
    virtual ~AlpsTreeSelectionEstimate() {}
    
    /** This returns true if the estimated quality of the subtree y is better
        (the less the better) than that the subtree x. */
    virtual bool compare(AlpsSubTree * x, AlpsSubTree * y);
};

//#############################################################################
// NODE SELECTION RULES
//#############################################################################

class AlpsNodeSelectionBest : public AlpsNodeSelection
{
public:
    /** Default Constructor. */
    AlpsNodeSelectionBest() { type_ = ALPS_SEARCH_BEST; }

    /** Default Destructor. */
    virtual ~AlpsNodeSelectionBest() {}

    /** This returns true if quality of node y is better (the less the better)
        than that of node x. */
    virtual bool compare(AlpsTreeNode * x, AlpsTreeNode * y) {
	return (x->getQuality() > y->getQuality());
    }
};

//#############################################################################

class AlpsNodeSelectionBreadth : public AlpsNodeSelection
{
public:
    /** Default Constructor. */
    AlpsNodeSelectionBreadth() { type_ = ALPS_SEARCH_BREATH; }

    /** Default Destructor. */
    virtual ~AlpsNodeSelectionBreadth() {};

    /** This returns true if the depth of node y is lesser
        than that of node x */
    virtual bool compare(AlpsTreeNode * x, AlpsTreeNode * y) {
	return x->getDepth() > y->getDepth();
    }
};

//#############################################################################

class AlpsNodeSelectionDepth : public AlpsNodeSelection
{
 public:
    /** Default Constructor. */
    AlpsNodeSelectionDepth() { type_ = ALPS_SEARCH_DEPTH; }

    /** Default Destructor. */
    virtual ~AlpsNodeSelectionDepth() {};

    /** This returns true if the depth of node y is greater than 
        that of node x. */
    virtual bool compare(AlpsTreeNode * x, AlpsTreeNode * y) {
	return (x->getDepth() < y->getDepth());
    }
};

//#############################################################################

class AlpsNodeSelectionEstimate : public AlpsNodeSelection
{
 public:
    /** Default Constructor. */
    AlpsNodeSelectionEstimate() { type_ = ALPS_SEARCH_BEST_EST; }

    /** Default Destructor. */
    virtual ~AlpsNodeSelectionEstimate() {}

    /** This returns true if the estimate quality of node y is better
        (the lesser the better) than that of node x. */
    virtual bool compare (AlpsTreeNode * x, AlpsTreeNode * y) {
	return (x->getSolEstimate() > y->getSolEstimate());
    }
};

//#############################################################################

class AlpsNodeSelectionHybrid : public AlpsNodeSelection
{
public:
    /** Default Constructor. */
    AlpsNodeSelectionHybrid() { type_ = ALPS_SEARCH_HYBRID; }

    /** Default Destructor. */
    virtual ~AlpsNodeSelectionHybrid() {}
    
    /** This returns true if the quality of node y is better (the lesser
        the better) than that of node x. */
    virtual bool compare(AlpsTreeNode * x, AlpsTreeNode * y) {
        // best first
        return (x->getQuality() > y->getQuality());
    }

    /* Select the next node to be processed. */
    virtual AlpsTreeNode* selectNextNode(AlpsSubTree *subTree);
    
    /* Create new nodes from pregnant node and store them in node pool. */
    virtual void createNewNodes(AlpsSubTree *subTree, AlpsTreeNode *node);
};

//#############################################################################
#endif
