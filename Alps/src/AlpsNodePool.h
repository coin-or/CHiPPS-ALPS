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

#ifndef AlpsNodePool_h_
#define AlpsNodePool_h_

#include <vector>

#include "AlpsHelperFunctions.h"
#include "AlpsPriorityQueue.h"
#include "AlpsTreeNode.h"
#include "AlpsKnowledgePool.h"

/*!

  #AlpsNodePool is used to store the nodes to be processed.

*/

class AlpsNodePool: public AlpsKnowledgePool {
  /// Candidate list.
  AlpsPriorityQueue<AlpsTreeNode*> candidateList_;

public:
  ///@name Constructor and destructor.
  //@{
  /// Default constructor.
  AlpsNodePool();
  /// Destructor.
  virtual ~AlpsNodePool();
  //@}

  ///@name Querry methods
  //@{
  /// Query the number of nodes in the node pool.
  virtual int getNumKnowledges() const;
  /// Get the node with highest priority. Doesn't remove it from the pool.
  virtual std::pair<AlpsKnowledge*, double> getKnowledge() const;
  /// Check whether there are still nodes in the node pool.
  virtual bool hasKnowledge() const { return !(candidateList_.empty()); }
  /// Query the quantity limit of knowledges.
  virtual int getMaxNumKnowledges() const { return INT_MAX; };
  /// Query the best knowledge in the pool.
  virtual std::pair<AlpsKnowledge*, double> getBestKnowledge() const;
  /// Get a reference to all the knowledges in the pool.*/
  virtual void getAllKnowledges (std::vector<std::pair<AlpsKnowledge*,
                                 double> >& kls) const;
  //@}

  ///@name Knowledge manipulation
  //@{
  /// Add a node to node pool.
  virtual void addKnowledge(AlpsKnowledge* node, double priority);
  /// Remove the node with highest priority from the pool.
  virtual void popKnowledge() { candidateList_.pop(); }
  //@}

  ///@name Other functions
  //@{
  /// Set the quantity limit of knowledges that can be stored in the pool.
  virtual void setMaxNumKnowledges(int num);
  /// Get the "best value" of the nodes in node pool.
  double getBestKnowledgeValue() const;
  /// Get the "best" nodes in node pool.
  AlpsTreeNode * getBestNode() const;
  /// Get a constant reference to the priority queue that stores nodes.
  AlpsPriorityQueue<AlpsTreeNode*> const & getCandidateList() const;
  /// Set strategy and resort heap.
  void setNodeSelection(AlpsSearchStrategy<AlpsTreeNode*> & compare);
  /// Delete all the nodes in the pool and free memory.
  void deleteGuts();
  /// Remove all the nodes in the pool (does not free memory).
  void clear() { candidateList_.clear(); }
  //@}

private:
  /// Disable copy contructor.
  AlpsNodePool(AlpsNodePool const &);
  /// Disable copy assignment operator.
  AlpsNodePool & operator=(AlpsNodePool const &);
};

#endif
