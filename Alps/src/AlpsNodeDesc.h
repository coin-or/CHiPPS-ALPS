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


#ifndef AlpsNodeDesc_h_
#define AlpsNodeDesc_h_

#include "AlpsKnowledge.h"

class AlpsModel;

/*!
  This is an abstract base class for subproblem data to be stored in a tree
  node. An instance of this class is a member in #AlpsTreeNode.

  #AlpsTreeNode keeps data related to node's position within the tree and
  #AlpsNodeDesc holds data directly related to the corresponding
  subproblem. This design is prefered due to its simplicity and convenience of
  separating subproblem data from tree search related data.

 */

class AlpsNodeDesc: virtual public AlpsKnowledge {
 public:
  ///@name Constructor and Destructor.
  //@{
  /// Default constructor.
  AlpsNodeDesc(): AlpsKnowledge() {}
  AlpsNodeDesc(AlpsKnowledgeBroker * broker): AlpsKnowledge(AlpsKnowledgeTypeNodeDesc, broker) {}
  /// Destructor.
  virtual ~AlpsNodeDesc() {}
  //@}

  ///@name Encode/Decode inherited from #AlpsKnowledge.
  //@{
  /// Pack this into an #AlpsEncoded object and return a pointer to it.
  using AlpsKnowledge::encode;
  /// Pack this node description into the given #AlpsEncoded object.
  virtual AlpsReturnStatus encode(AlpsEncoded * encoded) const {
    return AlpsReturnStatusOk;
  }
  /// Unpack fields from the given #AlpsEncoded object.
  virtual AlpsReturnStatus decodeToSelf(AlpsEncoded & encoded) {
    return AlpsReturnStatusOk;
  }
};

#endif
