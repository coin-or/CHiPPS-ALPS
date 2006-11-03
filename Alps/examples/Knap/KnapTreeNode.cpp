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

#include <iostream>
#include <utility>

//#include "CoinFloat.h"
#include "CoinUtility.hpp"

#include "AlpsKnowledgeBroker.h"
#include "AlpsKnowledge.h"

#include "KnapTreeNode.h"
#include "KnapSolution.h"

//#############################################################################

static const double KnapZeroTol = 1e-8;

//#############################################################################

AlpsTreeNode*
KnapTreeNode::createNewTreeNode(AlpsNodeDesc*& desc) const
{
    // Create a new tree node
    KnapNodeDesc* d = dynamic_cast<KnapNodeDesc*>(desc);
    KnapTreeNode* node = new KnapTreeNode(d);
    desc = 0;
    return(node);
}

//#############################################################################

int
KnapTreeNode::process(bool isRoot, bool rampUp)
{
    bool foundSolution = false;
    KnapNodeDesc* desc = dynamic_cast<KnapNodeDesc*>(desc_);
    const KnapModel* m = dynamic_cast<KnapModel*>(desc->getModel());

    //------------------------------------------------------
    // Here, we need to fill in the method for bounding.
    // Solve the lp relaxation.
    //------------------------------------------------------

    int cap = m->getCapacity() - desc->getUsedCapacity();
    int val = desc->getUsedValue();
    const KnapVarStatus* stati = 
	dynamic_cast<KnapNodeDesc*>(desc_)->getVarStati();
    int i;
    const int n = m->getNumItems();
#if defined NF_DEBUG_MORE
    std::cout << "Num of items = " << n << std::endl;
#endif
    for (i = 0; i < n; ++i) {
	if (stati[i] == KnapVarFree) {
	    cap -= m->getItem(i).first;
	    val += m->getItem(i).second;
	    if (cap <= 0)
		break;
	}
#if defined NF_DEBUG_MORE
	std::cout << m->getItem(i).second << " ";
#endif
    }
#if defined NF_DEBUG_MORE
    std::cout << std::endl;
#endif

    // get the best solution so far (for parallel, it is the incumbent)
    int bestval = 0;
    double valRelax = 0;

    // The quality of a solution is the negative of the objective value
    //  since Alps consideres sols with lower quality values better. 
    bestval = -static_cast<int>(getKnowledgeBroker()->getIncumbentValue());
  
    if (cap < 0) {
	// The last item dosn't fit fully, but some portion of it has been put
	// in (otherwise we'd have stopped at the previous i)
	const int size = m->getItem(i).first;
	const int cost = m->getItem(i).second;
	val -= cost;
	cap += size;
	valRelax = val + cost*cap/double(size);
    
	if (valRelax <= bestval)
	    setStatus(AlpsNodeStatusFathomed); // set the status to fathomed
	else {
	    branchedOn_ = i;
	    setStatus(AlpsNodeStatusPregnant);
	}
    } 
    else {
	// The last item made it in fully, or no item free to make it fully, 
	// find a feasible solution and fathom this node
	// save the solution if better than the best so far.
	valRelax = val;
	if (bestval < val) {                 // Find a better solution 
	    int* sol = new int[n];
	    for (i = 0; i < n; ++i) {
		sol[i] = stati[i] == KnapVarFixedToOne ? 1 : 0;
	    }
	    cap = m->getCapacity() - desc->getUsedCapacity();
	    for (i = 0; i < n; ++i) {
		if (stati[i] == KnapVarFree) {
		    sol[i] = 1;
		    cap -= m->getItem(i).first;
		    if (cap == 0)
			break;
		}
	    }
	    foundSolution = true;
	    KnapSolution* ksol = new KnapSolution(n, sol, val,
                            dynamic_cast<KnapModel*>(desc->getModel()));
	    getKnowledgeBroker()->addKnowledge(ALPS_SOLUTION, ksol, -val);
	    getKnowledgeBroker()->messageHandler()->
		message(ALPS_S_SEARCH_SOL, getKnowledgeBroker()->messages())
		    << (getKnowledgeBroker()->getProcRank()) 
		    << valRelax << CoinMessageEol;
	}    
	setStatus(AlpsNodeStatusFathomed);   // set the status to fathomed
    }

    quality_ = -valRelax;  // Set objective

    return foundSolution;
}

//#############################################################################

std::vector< CoinTriple<AlpsNodeDesc*, AlpsNodeStatus, double> >
KnapTreeNode::branch()
{
    KnapNodeDesc* desc = 
	dynamic_cast<KnapNodeDesc*>(desc_);
    const int oldCap = desc->getUsedCapacity();
    const int oldVal = desc->getUsedValue();
    const KnapModel* m = dynamic_cast<KnapModel*>(desc->getModel());

    const int n = m->getNumItems();
    const KnapVarStatus* oldStati = desc->getVarStati();
    KnapVarStatus* newStati = new KnapVarStatus[n];
    std::copy(oldStati, oldStati+n, newStati);
    newStati[branchedOn_] = KnapVarFixedToZero;

    std::vector< CoinTriple<AlpsNodeDesc*, AlpsNodeStatus, double> > newNodes;

    int cap = oldCap;
    int val = oldVal;

    AlpsNodeDesc* child;
    child = new KnapNodeDesc(const_cast<KnapModel*>(m), newStati, cap, val);
    newNodes.push_back(CoinMakeTriple(child,
				      AlpsNodeStatusCandidate,
				      getQuality()));

    newStati = new KnapVarStatus[n];
    std::copy(oldStati, oldStati+n, newStati);
    newStati[branchedOn_] = KnapVarFixedToOne;

    cap = oldCap + m->getItem(branchedOn_).first;
    val = oldVal + m->getItem(branchedOn_).second;

    // *FIXME* : we could figure out if it's fathomed...// DONE, 
    // if used capacity is not larger than the knap capacity, then add
    // it newNodes...
    if ( cap <= m->getCapacity() ) {
	child = 
	    new KnapNodeDesc(const_cast<KnapModel* >(m), newStati, cap, val);
	newNodes.push_back(CoinMakeTriple(child,
					  AlpsNodeStatusCandidate,
					  getQuality()));
    }
    else {  // Need this
	child = 
	    new KnapNodeDesc(const_cast<KnapModel* >(m), newStati, cap, val);
	newNodes.push_back(CoinMakeTriple(child,
					  AlpsNodeStatusFathomed,
					  getQuality()));
    }

    return newNodes;
}

//#############################################################################

AlpsEncoded*
KnapTreeNode::encode() const 
{
    AlpsEncoded* encoded = new AlpsEncoded("ALPS_NODE");
    const KnapNodeDesc* desc = 
	dynamic_cast<const KnapNodeDesc*>(desc_);

    int num  = dynamic_cast<KnapModel*>(desc->getModel())->getNumItems();

    const KnapVarStatus* stati = desc->getVarStati();
    const int usedCap = desc->getUsedCapacity();
    const int usedVal = desc->getUsedValue();

    encoded->writeRep(explicit_);
    encoded->writeRep(num);
    encoded->writeRep(stati, num);
    encoded->writeRep(usedCap);
    encoded->writeRep(usedVal);
    encoded->writeRep(index_);
    encoded->writeRep(depth_);
    encoded->writeRep(quality_);
    encoded->writeRep(parentIndex_);
    encoded->writeRep(numChildren_);
    encoded->writeRep(status_);
    encoded->writeRep(sentMark_);
    encoded->writeRep(branchedOn_);

#if defined NF_DEBUG_MORE
    std::cout << "num = " << num << "; ";
    std::cout << "usedCap = " << usedCap << "; ";
    std::cout << "usedVal = " << usedVal << "; ";
    std::cout << "index_ = " << index_ << "; ";
    std::cout << "depth_ = " << depth_ << "; ";
    std::cout << "quality_ = " << quality_ << "; ";
    std::cout << "parentIndex_ = " << parentIndex_ << "; ";
    std::cout << "numChildren_ = " << numChildren_ << "; ";
    std::cout << "status_ = " << status_ << "; ";
    std::cout << "sentMark_ = " << sentMark_ << std::endl;
#endif

    return encoded;
}

AlpsKnowledge* 
KnapTreeNode::decode(AlpsEncoded& encoded) const
{
    int expli;
    int num;
    KnapVarStatus* stati;
    int usedCap;
    int usedVal;
    int index;
    int depth;
    double quality;
    int parentIndex;
    int numChildren;
    AlpsNodeStatus     nodeStatus;
    int sentMark;
    int branchedOn;
 
    encoded.readRep(expli);  // Check whether has full or partial
    encoded.readRep(num);
    encoded.readRep(stati, num);
    encoded.readRep(usedCap);
    encoded.readRep(usedVal);
    encoded.readRep(index);
    encoded.readRep(depth);
    encoded.readRep(quality);
    encoded.readRep(parentIndex);
    encoded.readRep(numChildren);
    encoded.readRep(nodeStatus);
    encoded.readRep(sentMark);
    encoded.readRep(branchedOn);

    KnapNodeDesc* nodeDesc = new KnapNodeDesc(
	dynamic_cast<KnapModel*>(desc_->getModel()), 
	stati, 
	usedCap, 
	usedVal
	);
 
    KnapTreeNode* treeNode = new KnapTreeNode(nodeDesc);
    treeNode->setIndex(index);
    treeNode->setDepth(depth);
    treeNode->setQuality(quality);
    treeNode->setParentIndex(parentIndex);
    treeNode->setNumChildren(numChildren);
    treeNode->setStatus(nodeStatus);
    treeNode->setSentMark(sentMark);
    treeNode->setBranchOn(branchedOn);

#if defined NF_DEBUG_MORE
    std::cout << "num = " << num << "; ";
    std::cout << "usedCap = " << usedCap << "; ";
    std::cout << "usedVal = " << usedVal << "; ";
    std::cout << "index = " << index << "; ";
    std::cout << "depth = " << depth << "; ";
    std::cout << "quality = " << quality << "; ";
    std::cout << "parentIndex = " << parentIndex << "; ";
    std::cout << "numChildren = " << numChildren << "; ";
    std::cout << "nodeStatus = " << nodeStatus << "; ";
    std::cout << "sentMark = " << sentMark << "treeNode->getSentMark() = " 
	      <<  treeNode->getSentMark() << std::endl;
#endif

    return treeNode;
}
