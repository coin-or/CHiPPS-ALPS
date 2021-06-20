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


#ifndef KnapTreeNode_h_
#define KnapTreeNode_h_

// STL headers
#include <utility>
// ALPS headers
#include "AlpsTreeNode.h"

class KnapModel;
class KnapNodeDesc;

/*!
  Holds a Knapsack tree node.
 */

class KnapTreeNode : public AlpsTreeNode {
  /** The index of the branching variable */
  int branchedOn_;

public:
  KnapTreeNode(KnapModel * model);
  KnapTreeNode(KnapNodeDesc *& desc);
  virtual ~KnapTreeNode() { }

  void setBranchOn(int b) { branchedOn_ = b; }

  virtual AlpsTreeNode * createNewTreeNode(AlpsNodeDesc*& desc) const;

  virtual int process(bool isRoot = false, bool rampUp = false);

  virtual std::vector< CoinTriple<AlpsNodeDesc*, AlpsNodeStatus, double> >
  branch();

  using AlpsTreeNode::encode;
  /// Encode this into the given AlpsEncoded object.
  virtual AlpsReturnStatus encode(AlpsEncoded * encoded) const;
  /// Decode the given AlpsEncoded object into this.
  virtual AlpsReturnStatus decodeToSelf(AlpsEncoded & encoded);
  virtual AlpsKnowledge * decode(AlpsEncoded & encoded) const;

private:
  // NO: default constructor, copy constructor, assignment operator
  KnapTreeNode(KnapTreeNode const &);
  KnapTreeNode & operator=(KnapTreeNode const &);
};

#endif
