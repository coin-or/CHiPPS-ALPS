/*===========================================================================*
 * This file is part of the Abstract Library for Parallel Search (ALPS).     *
 *                                                                           *
 * ALPS is distributed under the Eclipse Public License as part of the       *
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
 * Copyright (C) 2001-2017, Lehigh University, Yan Xu, and Ted Ralphs.       *
 *===========================================================================*/

#ifndef AlpsKnowledgePool_h_
#define AlpsKnowledgePool_h_

// STL headers
#include <climits>
#include <iostream>
#include <vector>

// CoinUtils headers
#include "CoinError.hpp"

// Alps headers
#include "AlpsKnowledge.h"

/*!
  This is an abstract base class, fixing an API for pool types of Alps,
  #AlpsNodePool, #AlpsSolutionPool, #AlpsSubTreePool.

 */

class AlpsKnowledgePool {
  AlpsKnowledgePoolType type_;

public:
  ///@name Constructor and Destructor.
  //@{
  /// Default constructor.
  AlpsKnowledgePool(AlpsKnowledgePoolType type) { type_ = type; }
  /// Destructor.
  virtual ~AlpsKnowledgePool() {}
  //@}

  ///@name Querry methods
  //@{
  /// Return size of the pool.
  virtual int getNumKnowledges() const = 0;
  /// Check the first item in the pool.
  virtual std::pair<AlpsKnowledge*, double> getKnowledge() const = 0;
  /// Check whether the pool is empty.
  virtual bool hasKnowledge() const = 0;
  /// Query the quantity limit of knowledges.
  virtual int getMaxNumKnowledges() const = 0;
  /// Query the best knowledge in the pool.
  virtual std::pair<AlpsKnowledge*, double> getBestKnowledge() const = 0;
  /// Get a reference to all the knowledges in the pool.*/
  virtual void getAllKnowledges (std::vector<std::pair<AlpsKnowledge*,
                                 double> >& kls) const = 0;
  //@}

  ///@name Knowledge manipulation
  //@{
  /// Add a knowledge to pool.
  virtual void addKnowledge(AlpsKnowledge * nk, double priority) = 0;
  /// Pop the first knowledge from the pool.
  virtual void popKnowledge() = 0;
  //@}

  ///@name Other functions
  //@{
  /// Set the quantity limit of knowledges that can be stored in the pool.
  virtual void setMaxNumKnowledges(int num) = 0;
  //@}

private:
  /// Disable copy constructor.
  AlpsKnowledgePool(AlpsKnowledgePool const &);
  /// Disable copy assignment operator.
  AlpsKnowledgePool & operator=(AlpsKnowledgePool const &);
};

#endif
