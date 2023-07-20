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
 * Copyright (C) 2001-2023, Lehigh University, Yan Xu, Aykut Bulut, and      *
 *                          Ted Ralphs.                                      *
 * All Rights Reserved.                                                      *
 *===========================================================================*/


#ifndef AlpsSolutionPool_h_
#define AlpsSolutionPool_h_

#include "AlpsKnowledgePool.h"
#include "AlpsSolution.h"

// todo(aykut) we do want to allow solutions with the same priority, but do
// todo(aykut) we want to allow identical solutions?

/*!
  This class is a comtainer for the solutions found during the search.

  In the solution pool we assume that the lower the priority value the more
  desirable the solution is.
*/

class AlpsSolutionPool: public AlpsKnowledgePool {
  std::multimap<double, AlpsSolution*> solutions_;
  int maxNumSolutions_;

public:
  ///@name Constructor and Destructor.
  //@{
  AlpsSolutionPool(int maxsols=1);
  virtual ~AlpsSolutionPool();
  //@}

  ///@name Querry methods
  //@{
  /// Query the current number of solutions.
  virtual int getNumKnowledges() const;
  virtual std::pair<AlpsKnowledge*, double> getKnowledge() const;
  /// Return true if there are any solution stored in the solution pool.
  virtual bool hasKnowledge() const;
  /// query the maximum number of solutions.
  virtual int getMaxNumKnowledges() const { return maxNumSolutions_; }
  /// Return the best solution. The callee must not delete the returned
  /// pointer!
  virtual std::pair<AlpsKnowledge*, double> getBestKnowledge() const;
  /// Return all the solutions of the solution pool in the provided argument
  /// vector. The callee must not delete the returned pointers!
  virtual void getAllKnowledges (std::vector<std::pair<AlpsKnowledge*,
                                double> >& sols) const;
  //@}

  ///@name Knowledge manipulation
  //@{
  virtual void addKnowledge(AlpsKnowledge* sol, double priority);
  /// Remove a solution from the pool.
  virtual void popKnowledge();
  //@}

  ///@name Other functions
  //@{
  /// Set maximum number of solutions.
  virtual void setMaxNumKnowledges(int maxsols);
  /// Delete all the solutions in pool.
  void clean();
  //@}

private:
  /// Disable copy constructor.
  AlpsSolutionPool(const AlpsSolutionPool&);
  /// Disable copy assignment operator.
  AlpsSolutionPool& operator=(const AlpsSolutionPool&);
};

#define AlpsSolutionInterface(ref)					\
int getNumSolutions() const {						\
   (ref).getNumSolutions();						\
}									\
int getMaxNumSolutions() const {					\
   return (ref).getMaxNumSolutions();					\
}									\
void setMaxNumSolutions(int num) {					\
   (ref).setMaxNumSolutions(num);					\
}									\
bool hasSolution() const {						\
   return (ref).hasSolution();						\
}									\
std::pair<const AlpsSolution*, double> getBestSolution() const {	\
   return (ref).getBestSolution();					\
}									\
void getAllSolutions							\
   (std::vector<std::pair<const AlpsSolution*, double> >& sols) {	\
   return (ref).getAllSolutions(sols);					\
}									\
void addSolution(const AlpsSolution* sol, double priority) {		\
   (ref).addSolution(sol, priority);					\
}

#endif
