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

class AlpsTreeSearchBest : public AlpsSearchStrategy<AlpsSubTree*> 
{
public:
    /** Default Constructor. */
    AlpsTreeSearchBest() {}

    /** Default Destructor. */
    virtual ~AlpsTreeSearchBest() {}
    
    /** This returns true if the quality of the subtree y is better
        (the less the better) than that the subtree x. */
    virtual bool compare(AlpsSubTree * x, AlpsSubTree * y);
};

class AlpsTreeSearchBreadth : public AlpsSearchStrategy<AlpsSubTree*> 
{
public:
    /** Default Constructor */
    AlpsTreeSearchBreadth() {}
    
    /** Default Destructor. */
    virtual ~AlpsTreeSearchBreadth() {}
    
    /** This returns true if the depth of the root node in subtree y
        is smaller than that of the root node in subtree x. */
    virtual bool compare(AlpsSubTree * x, AlpsSubTree * y);
};

class AlpsTreeSearchDepth : public AlpsSearchStrategy<AlpsSubTree*> 
{
public:
    /** Default Constructor */
    AlpsTreeSearchDepth() {}
    
    /** Default Destructor. */
    virtual ~AlpsTreeSearchDepth() {}
    
    /** This returns true if the depth of the root node in subtree y
        is greater than that of the root node in subtree x. */
    virtual bool compare(AlpsSubTree * x, AlpsSubTree * y);
};

class AlpsTreeSearchEstimate : public AlpsSearchStrategy<AlpsSubTree*> 
{
public:
    /** Default Constructor. */
    AlpsTreeSearchEstimate() {}

    /** Default Destructor. */
    virtual ~AlpsTreeSearchEstimate() {}
    
    /** This returns true if the estimated quality of the subtree y is better
        (the less the better) than that the subtree x. */
    virtual bool compare(AlpsSubTree * x, AlpsSubTree * y);
};

//#############################################################################

class AlpsNodeSearchBest : public AlpsSearchStrategy<AlpsTreeNode*> 
{
public:
    /** Default Constructor. */
    AlpsNodeSearchBest() {}

    /** Default Destructor. */
    virtual ~AlpsNodeSearchBest() {}

    /** This returns true if quality of node y is better (the less the better)
        than that of node x. */
    virtual bool compare(AlpsTreeNode * x, AlpsTreeNode * y) {
	return (x->getQuality() > y->getQuality());
    }
    
    /* Select the next node to be processed. */
    virtual AlpsTreeNode* selectNextNode(AlpsSubTree *subTree)
    {
        AlpsTreeNode *node = subTree->activeNode();
        if (node == NULL) {
            node = dynamic_cast<AlpsTreeNode*>
                (const_cast<AlpsKnowledge*>(subTree->nodePool()->getKnowledge().first) ); 
            subTree->nodePool()->popKnowledge();
        }           
        return node;
    }
    
    /* Create new nodes from pregnant node and store them in node pool. */
    virtual void createNewNodes(AlpsSubTree *subTree, AlpsTreeNode *node) 
    {
        std::vector< CoinTriple<AlpsNodeDesc*, AlpsNodeStatus, double> > 
            children = node->branch();
        subTree->createChildren(node, children);
        /* No active node now. */
        subTree->setActiveNode(0);
    }
};

class AlpsNodeSearchBreadth : public AlpsSearchStrategy<AlpsTreeNode*> 
{
public:
    /** Default Constructor. */
    AlpsNodeSearchBreadth() {}

    /** Default Destructor. */
    virtual ~AlpsNodeSearchBreadth() {};

    /** This returns true if the depth of node y is lesser
        than that of node x */
    virtual bool compare(AlpsTreeNode * x, AlpsTreeNode * y) {
	return x->getDepth() > y->getDepth();
    }

    /* Select the next node to be processed. */
    virtual AlpsTreeNode* selectNextNode(AlpsSubTree *subTree)
    {
        AlpsTreeNode *node = subTree->activeNode();
        if (node == NULL) {
            node = dynamic_cast<AlpsTreeNode*>
                (const_cast<AlpsKnowledge*>(subTree->nodePool()->getKnowledge().first) ); 
            subTree->nodePool()->popKnowledge();
        }           
        return node;
    }
    
    /* Create new nodes from pregnant node and store them in node pool. */
    virtual void createNewNodes(AlpsSubTree *subTree, AlpsTreeNode *node) 
    {
        std::vector< CoinTriple<AlpsNodeDesc*, AlpsNodeStatus, double> > 
            children = node->branch();
        subTree->createChildren(node, children);
        /* No active node now. */
        subTree->setActiveNode(0);
    }
};

class AlpsNodeSearchDepth : public AlpsSearchStrategy<AlpsTreeNode*> 
{
 public:
    /** Default Constructor. */
    AlpsNodeSearchDepth() {}

    /** Default Destructor. */
    virtual ~AlpsNodeSearchDepth() {};

    /** This returns true if the depth of node y is greater than 
        that of node x. */
    virtual bool compare(AlpsTreeNode * x, AlpsTreeNode * y) {
	return (x->getDepth() < y->getDepth());
    }

    /* Select the next node to be processed. */
    virtual AlpsTreeNode* selectNextNode(AlpsSubTree *subTree)
    {
        AlpsTreeNode *node = subTree->activeNode();
        if (node == NULL) {
            node = dynamic_cast<AlpsTreeNode*>
                (const_cast<AlpsKnowledge*>(subTree->nodePool()->getKnowledge().first) ); 
            subTree->nodePool()->popKnowledge();
        }           
        return node;
    }
    
    /* Create new nodes from pregnant node and store them in node pool. */
    virtual void createNewNodes(AlpsSubTree *subTree, AlpsTreeNode *node) 
    {
        std::vector< CoinTriple<AlpsNodeDesc*, AlpsNodeStatus, double> > 
            children = node->branch();
        subTree->createChildren(node, children);
        /* No active node now. */
        subTree->setActiveNode(0);
    }
};

class AlpsNodeSearchEstimate : public AlpsSearchStrategy<AlpsTreeNode*> 
{
 public:
    /** Default Constructor. */
    AlpsNodeSearchEstimate() {}

    /** Default Destructor. */
    virtual ~AlpsNodeSearchEstimate() {}

    /** This returns true if the estimate quality of node y is better
        (the lesser the better) than that of node x. */
    virtual bool compare (AlpsTreeNode * x, AlpsTreeNode * y) {
	return (x->getSolEstimate() > y->getSolEstimate());
    }

    /* Select the next node to be processed. */
    virtual AlpsTreeNode* selectNextNode(AlpsSubTree *subTree)
    {
        AlpsTreeNode *node = subTree->activeNode();
        if (node == NULL) {
            node = dynamic_cast<AlpsTreeNode*>
                (const_cast<AlpsKnowledge*>(subTree->nodePool()->getKnowledge().first) ); 
            subTree->nodePool()->popKnowledge();
        }           
        return node;
    }
    
    /* Create new nodes from pregnant node and store them in node pool. */
    virtual void createNewNodes(AlpsSubTree *subTree, AlpsTreeNode *node) 
    {
        std::vector< CoinTriple<AlpsNodeDesc*, AlpsNodeStatus, double> > 
            children = node->branch();
        subTree->createChildren(node, children);
        /* No active node now. */
        subTree->setActiveNode(0);
    }
};

class AlpsNodeSearchHybrid : public AlpsSearchStrategy<AlpsTreeNode*> 
{
public:
    /** Default Constructor. */
    AlpsNodeSearchHybrid() {}

    /** Default Destructor. */
    virtual ~AlpsNodeSearchHybrid() {}
    
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
