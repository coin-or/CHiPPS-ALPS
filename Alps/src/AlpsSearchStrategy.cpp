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
 * Copyright (C) 2001-2008, Lehigh University, Yan Xu, and Ted Ralphs.       *
 *===========================================================================*/

#include "AlpsSearchStrategy.h"

//#############################################################################
//#############################################################################

AlpsTreeNode* 
AlpsNodeSelection::selectNextNode(AlpsSubTree *subTree)
{
    AlpsTreeNode *node = subTree->activeNode();
    if (node == NULL) {
	node = dynamic_cast<AlpsTreeNode*>
	    (const_cast<AlpsKnowledge*>(subTree->nodePool()->getKnowledge().first) ); 
	subTree->nodePool()->popKnowledge();
        }           
    return node;
}

//#############################################################################

void 
AlpsNodeSelection::createNewNodes(AlpsSubTree *subTree, AlpsTreeNode *node) 
{
    std::vector< CoinTriple<AlpsNodeDesc*, AlpsNodeStatus, double> > 
	children = node->branch();
    subTree->createChildren(node, children);
    //subTree->setActiveNode(0);
}

//#############################################################################
//#############################################################################

bool 
AlpsTreeSelectionBest::compare(AlpsSubTree * x, AlpsSubTree * y) 
{
    return (x->getQuality() < y->getQuality());
}

//#############################################################################

bool 
AlpsTreeSelectionBreadth::compare(AlpsSubTree * x, AlpsSubTree * y)
{
    return (x->getRoot()->getDepth() > y->getRoot()->getDepth());
}

//#############################################################################

bool 
AlpsTreeSelectionDepth::compare(AlpsSubTree * x, AlpsSubTree * y) 
{
    return (x->getRoot()->getDepth() < y->getRoot()->getDepth());
}

//#############################################################################

bool 
AlpsTreeSelectionEstimate::compare(AlpsSubTree * x, AlpsSubTree * y)
{
    return (x->getSolEstimate() > y->getSolEstimate());
}

//#############################################################################
//#############################################################################

AlpsTreeNode*
AlpsNodeSelectionHybrid::selectNextNode(AlpsSubTree *subTree)
{
    AlpsTreeNode *node = subTree->activeNode();

    /* Check if dive too deep */
    if (node) {
        if (subTree->getDiveDepth() > 30) {
            /* Too deep, put nodes in dive pool to regular pool. */
            //std::cout << "++++ TOO DEEP: depth " << subTree->getDiveDepth() 
            //       << std::endl;
            subTree->reset();
            node = NULL;
        }
    }
    
    if (node) {
        subTree->incDiveDepth();
        node->setDiving(true);
    }
    else {
        subTree->setDiveDepth(0);
	if (subTree->diveNodePool()->getNumKnowledges() > 0) {
	    node = dynamic_cast<AlpsTreeNode*>(subTree->diveNodePool()->getKnowledge().first); 
	    node->setDiving(false);	    
	    subTree->diveNodePool()->popKnowledge();
	}
	else if (subTree->nodePool()->hasKnowledge()) {
	    node = dynamic_cast<AlpsTreeNode*>(subTree->nodePool()->getKnowledge().first); 
	    node->setDiving(false);
	    subTree->nodePool()->popKnowledge();
	}
	else {
	    assert(0);
	}
        
#if 0
        std::cout << "======= NOTE[" << node->getIndex() 
                  << "]: JUMP : depth = " << node->getDepth() 
                  << ", quality = " << node->getQuality()
                  << ", estimate = " << node->getSolEstimate()
                  << std::endl;
#endif
    }
    
    return node;
}

//#############################################################################

void 
AlpsNodeSelectionHybrid::createNewNodes(AlpsSubTree *subTree, AlpsTreeNode *node) 
{
    int numChildren = 0;
    AlpsTreeNode *tempNode, *activeNode = 0;

    while (subTree->diveNodePool()->getNumKnowledges() > 0) {
        tempNode = dynamic_cast<AlpsTreeNode *>
            (subTree->diveNodePool()->getKnowledge().first);
        subTree->diveNodePool()->popKnowledge();
        subTree->nodePool()->addKnowledge(tempNode, tempNode->getQuality());
    }
    
    std::vector< CoinTriple<AlpsNodeDesc*, AlpsNodeStatus, double> > 
        children = node->branch();
    
    subTree->createChildren(node, children, subTree->diveNodePool());
    numChildren = subTree->diveNodePool()->getNumKnowledges();
    
    if (numChildren > 0) {
        activeNode = dynamic_cast<AlpsTreeNode *>
            (subTree->diveNodePool()->getKnowledge().first);
        subTree->diveNodePool()->popKnowledge();
    }
    subTree->setActiveNode(activeNode);
}

//#############################################################################
//#############################################################################
