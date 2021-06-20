/*===========================================================================*
 * This file is part of the Abstract Library for Parallel Search (ALPS).     *
 *                                                                           *
 * ALPS is distributed under the Eclipse Public License as part of the       *
 * COIN-OR repository (http://www.coin-or.org).                              *
 *                                                                           *
 * Authors:                                                                  *
 *                                                                           *
 *          Yan Xu, Lehigh University                                        *
 *          Aykut Bulut, Lehigh University                                   *
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
 * Copyright (C) 2001-2019, Lehigh University, Yan Xu, Aykut Bulut, and      *
 *                          Ted Ralphs.                                      *
 * All Rights Reserved.                                                      *
 *===========================================================================*/


#ifndef AbcTreeNode_h_
#define AbcTreeNode_h_

//#include <utility>
#include "AlpsKnowledgeBroker.h"
#include "AlpsTreeNode.h"

#include "AbcModel.h"
#include "AbcNodeDesc.h"

//#############################################################################

enum AbcVarStatus {
    AbcVarFree,
    AbcVarFixedToUB,
    AbcVarFixedToLB
};

//#############################################################################

class AbcTreeNode : public AlpsTreeNode {
 private:
    /** The index of the branching variable */
    int branchedOn_;

    /** The solution value (non-integral) of the branching variable. */
    double branchedOnVal_;

    /** Branching direction */
    int branchedDir_;

    /// Guessed satisfied Objective value
    double guessedObjectiveValue_;

    /// The number of objects unsatisfied at this node.
    int numberUnsatisfied_;

 public:
    AbcTreeNode()
        :
      AlpsTreeNode(),
        branchedOn_(-1),
        branchedOnVal_(ALPS_BND_MAX),
        branchedDir_(0),
        guessedObjectiveValue_(ALPS_OBJ_MAX),
        numberUnsatisfied_(0)
        {
          desc_ = new AbcNodeDesc();
        }

    AbcTreeNode(AbcNodeDesc*& desc)
        :
      AlpsTreeNode(),
        branchedOn_(-1),
        branchedOnVal_(ALPS_BND_MAX),
        branchedDir_(0),
        guessedObjectiveValue_(ALPS_OBJ_MAX),
        numberUnsatisfied_(0)
        {
            desc_ = desc;
            desc = 0;
            //At the time of registering node, that node hasn't set broker
            //desc_->setModel(getKnowledgeBroker()->getDataPool()->getModel());
        }

    virtual ~AbcTreeNode()
        {
        }

    virtual AlpsTreeNode* createNewTreeNode(AlpsNodeDesc*& desc) const;

    /** Performing the bounding operation. */
    virtual int process(bool isRoot = false, bool rampUp = false);

    /** Select the branch variable.
        Return value:
        <ul>
        <li>  0: A branching object has been installed
        <li> -1: A monotone object was discovered
        <li> -2: An infeasible object was discovered
        </ul>
    */
    int chooseBranch (AbcModel * model, bool& strongFound);

    /** Query/set the objective value (could be approximately or not exit)
        of the node. */
    ///@{
    inline double getObjValue() const { return quality_; }
    inline void setObjValue(const double objValue) { quality_ = objValue; }
    ///@}

    virtual std::vector< CoinTriple<AlpsNodeDesc*, AlpsNodeStatus, double> >
        branch();

    /// Get the number of objects unsatisfied at this node.
    inline int numberUnsatisfied() const
        { return numberUnsatisfied_; }

    /// Guessed objective value (for solution)
    inline double guessedObjectiveValue() const
        { return guessedObjectiveValue_; }
    ///
    inline void setGuessedObjectiveValue(double value)
        { guessedObjectiveValue_ = value; }
    ///
    void setBranchedOn(int b) { branchedOn_ = b; }
    ///
    void setBranchedDir(int d) { branchedDir_ = d; }
    ///
    void setBranchedOnValue(double b) { branchedOnVal_ = b; }
    ///
    int getBranchedOn() const { return branchedOn_; }
    ///
    int getBranchedDir() const { return branchedDir_; }
    ///
    double getBranchedOnValue() const { return branchedOnVal_; }
    ///

  using AlpsKnowledge::encode;
  /// Encode this into given object as input.
  virtual AlpsReturnStatus encode(AlpsEncoded * encoded) const;

  /// Decode the given AlpsEncoded object into a new #AbcTreeNode.
  virtual AlpsKnowledge * decode(AlpsEncoded & encoded) const;

  /// Decode the given AlpsEncoded object into this.
  virtual AlpsReturnStatus decodeToSelf(AlpsEncoded & encoded) {
    std::cerr << "Not implemented!" << std::endl;
    throw std::exception();
  }

private:
  /// Disable copy constructor.
  AbcTreeNode(AbcTreeNode const &);
  /// Disable copy assignment operator.
  AbcTreeNode & operator=(AbcTreeNode const &);

};

#endif
