/*===========================================================================*
 * This file is part of the Abstract Library for Parallel Search (ALPS).     *
 *                                                                           *
 * ALPS is distributed under the Eclipse Public License as part of the       *
 * COIN-OR repository (http://www.coin-or.org).                              *
 *                                                                           *
 * Authors: Yan Xu, Lehigh University                                       *
 *          Ted Ralphs, Lehigh University                                    *
 *          Laszlo Ladanyi, IBM T.J. Watson Research Center                  *
 *          Matthew Saltzman, Clemson University                             *
 *                                                                           *
 *                                                                           *
 * Copyright (C) 2001-2015, Lehigh University, Yan Xu, and Ted Ralphs.       *
 *===========================================================================*/

#ifndef KnapNodeDesc_h_
#define KnapNodeDesc_h_

#include "AlpsNodeDesc.h"

enum KnapVarStatus {
  KnapVarFree,
  KnapVarFixedToOne,
  KnapVarFixedToZero
};

class KnapModel;

/*!

  Here, we need to fill in what the node description will look
  like. For now, we will not use differencing -- just explicitly
  represent it. Probably this means that we will just store the
  original problem data and a list of the variables that have been
  fixed.

 */

class KnapNodeDesc : public AlpsNodeDesc {
  KnapModel * model_;
  /// This array keeps track of which variables have been fixed by
  ///    branching and which are still free.
  // In the constructor, we should allocate this array to be the right
  // length.
  KnapVarStatus * varStatus_;
  /** The total size of the items fixed to be put into the knapsack */
  int usedCapacity_;
  int usedValue_;

public:
  KnapNodeDesc(KnapModel * model);
  KnapNodeDesc(KnapModel * model, KnapVarStatus *& st, int cap, int val);

  virtual ~KnapNodeDesc();

  void setVarStatus(const int i, const KnapVarStatus status);
  KnapVarStatus getVarStatus(const int i);
  KnapVarStatus const * getVarStati() const;


  inline int getUsedCapacity() const { return usedCapacity_; }
  inline int getUsedValue() const { return usedValue_; }

  KnapModel * model() { return model_; }
  KnapModel const * model() const { return model_; }
  ///@name Encode/Decode inherited from #AlpsKnowledge.
  //@{
  /// Grab encode function from #AlpsKnowledge
  using AlpsKnowledge::encode;
  /// Pack this node description into the given #AlpsEncoded object.
  virtual AlpsReturnStatus encode(AlpsEncoded * encoded) const;
  /// Unpack fields from the given #AlpsEncoded object.
  virtual AlpsReturnStatus decodeToSelf(AlpsEncoded & encoded);
  virtual AlpsNodeDesc * decode(AlpsEncoded & encoded) const;
private:
  /// Disable copy constructor
  KnapNodeDesc(KnapNodeDesc const & other);
  /// Disable copy assignment operator
  KnapNodeDesc & operator=(KnapNodeDesc const & rhs);
};

#endif
