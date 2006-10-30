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

#include "AlpsCompareActual.h"

//#############################################################################

bool 
AlpsTreeSearchBest::compare(AlpsSubTree * x, AlpsSubTree * y) 
{
    return (x->getQuality() < y->getQuality());
}

//#############################################################################

bool 
AlpsTreeSearchBreadth::compare(AlpsSubTree * x, AlpsSubTree * y)
{
    return (x->getRoot()->getDepth() > y->getRoot()->getDepth());
}

//#############################################################################

bool 
AlpsTreeSearchDepth::compare(AlpsSubTree * x, AlpsSubTree * y) 
{
    return (x->getRoot()->getDepth() < y->getRoot()->getDepth());
}

//#############################################################################

bool 
AlpsTreeSearchEstimate::compare(AlpsSubTree * x, AlpsSubTree * y)
{
    return (x->getSolEstimate() > y->getSolEstimate());
}

//#############################################################################

AlpsTreeNode*
AlpsNodeSearchHybrid::selectNextNode(AlpsSubTree *subTree)
{
    AlpsTreeNode *node = subTree->activeNode();
    
    if (!node) {
        node = dynamic_cast<AlpsTreeNode*>(subTree->nodePool()->getKnowledge().first); 
        node->setDiving(false);
        
#if 0
        std::cout << "======= NOTE[" << node->getIndex() 
                  << "]: JUMP : depth = " << node->getDepth() 
                  << ", quality = " << node->getQuality()
                  << ", estimate = " << node->getSolEstimate()
                  << std::endl;
#endif
        subTree->nodePool()->popKnowledge();
	
    }
    else {
        node->setDiving(true);
    }
    
    return node;
}

//#############################################################################

void 
AlpsNodeSearchHybrid::createNewNodes(AlpsSubTree *subTree, AlpsTreeNode *node) 
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
