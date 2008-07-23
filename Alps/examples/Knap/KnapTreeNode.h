/*===========================================================================*
 * This file is part of the Abstract Library for Parallel Search (ALPS).     *
 *                                                                           *
 * ALPS is distributed under the Common Public License as part of the        *
 * COIN-OR repository (http://www.coin-or.org).                              *
 *                                                                           *
 * Authors: Yan Xu, Lehigh University                                       *
 *          Ted Ralphs, Lehigh University                                    *
 *          Laszlo Ladanyi, IBM T.J. Watson Research Center                  *
 *          Matthew Saltzman, Clemson University                             *
 *                                                                           * 
 *                                                                           *
 * Copyright (C) 2001-2008, Lehigh University, Yan Xu, and Ted Ralphs.       *
 *===========================================================================*/

#ifndef KnapTreeNode_h_
#define KnapTreeNode_h_

#include <utility>

#include "AlpsTreeNode.h"

#include "KnapModel.h"


//#############################################################################

enum KnapVarStatus {
    KnapVarFree,
    KnapVarFixedToOne,
    KnapVarFixedToZero
};

//#############################################################################

class KnapNodeDesc : public AlpsNodeDesc {
  
 private:

    /* Here, we need to fill in what the node description will look
       like. For now, we will not use differencing -- just explicitly
       represent it. Probably this means that we will just store the
       original problem data and a list of the variables that have been
       fixed. */
  
    /** This array keeps track of which variables have been fixed by
	branching and which are still free. */
    // In the constructor, we should allocate this array to be the right 
    // length.
    KnapVarStatus* varStatus_;

    /** The total size of the items fixed to be put into the knapsack */
    int usedCapacity_;
    int usedValue_;

 public:
#if 0
    KnapNodeDesc() 
	:
	usedCapacity_(0), 
	usedValue_(0) 
	{
	    const int n = 
		dynamic_cast<const KnapModel*>(model_)->getNumItems();
	    varStatus_ = new KnapVarStatus[n];
	    std::fill(varStatus_, varStatus_ + n, KnapVarFree);
	}
#endif

    KnapNodeDesc(KnapModel* m) 
	:
	AlpsNodeDesc(m),
	usedCapacity_(0), 
	usedValue_(0) 
	{
	    //model_ = m;      // data member declared in Alps
	    const int n = dynamic_cast<KnapModel* >(model_)->getNumItems();
	    varStatus_ = new KnapVarStatus[n];
	    std::fill(varStatus_, varStatus_ + n, KnapVarFree);
	}
  
    KnapNodeDesc(KnapModel* m, KnapVarStatus *& st, int cap, int val) 
	:
	//      model_(m), varStatus_(0), usedCapacity_(cap), usedValue_(val) {
	AlpsNodeDesc(m),
	varStatus_(0), 
	usedCapacity_(cap), 
	usedValue_(val) 
	{
	    // model_ = m;
	    std::swap(varStatus_, st);
	}
  
    virtual ~KnapNodeDesc() { delete[] varStatus_; }
  
    // inline KnapModel* getModel() { return model_; }
    // inline const KnapModel* getModel() const { return model_; }// move Alps
  
    inline void setVarStatus(const int i, const KnapVarStatus status)
	{ varStatus_[i] = status; }
  
    inline KnapVarStatus getVarStatus(const int i)
	{ return varStatus_[i]; }
  
    inline const KnapVarStatus* getVarStati() const
	{ return varStatus_; }
  
    inline int getUsedCapacity() const { return usedCapacity_; }
    inline int getUsedValue() const { return usedValue_; }
};

//#############################################################################

class KnapTreeNode : public AlpsTreeNode {
 private:
    // NO: default constructor, copy constructor, assignment operator
    KnapTreeNode(const KnapTreeNode&);
    KnapTreeNode& operator=(const KnapTreeNode&);

 private:
    /** The index of the branching variable */
    int branchedOn_;
  
 public:
    KnapTreeNode() : branchedOn_(-1) {
	desc_ = new KnapNodeDesc(dynamic_cast<KnapModel* >
				 (getKnowledgeBroker()->getModel()));
    }

    KnapTreeNode(KnapModel* model) : branchedOn_(-1) {
	desc_ = new KnapNodeDesc(model);
    }

    KnapTreeNode(KnapNodeDesc*& desc) : branchedOn_(-1) {
	desc_ = desc;
	desc = 0;
	// At the time of registering node, that node hasn't set broker
	// desc_->setModel(getKnowledgeBroker()->getDataPool()->getModel());
    }

    ~KnapTreeNode() { }

    void setBranchOn(int b) { branchedOn_ = b; }

    virtual AlpsTreeNode* createNewTreeNode(AlpsNodeDesc*& desc) const;
  
    virtual int process(bool isRoot = false, bool rampUp = false);
  
    virtual std::vector< CoinTriple<AlpsNodeDesc*, AlpsNodeStatus, double> > 
	branch();
  
    // We only need these for parallel execution I think...   
    virtual AlpsEncoded* encode() const;
 
    virtual AlpsKnowledge* decode(AlpsEncoded&) const;
};

#endif
