/*===========================================================================*
 * This file is part of the Abstract Library for Parallel Search (ALPS).     *
 *                                                                           *
 * ALPS is distributed under the Common Public License as part of the        *
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
 * Copyright (C) 2001-2011, Lehigh University, Yan Xu, and Ted Ralphs.       *
 *===========================================================================*/

//#############################################################################
// This file is modified from SbbHeuristic.hpp
//#############################################################################

#ifndef AbcHeuristic_h_
#define AbcHeuristic_h_

#include <string>
#include <vector>
#include "CoinPackedMatrix.hpp"
#include "OsiCuts.hpp"

class OsiSolverInterface;
class AbcModel;

//#############################################################################

/** Heuristic base class */
class AbcHeuristic {
public:
  // Default Constructor 
  AbcHeuristic ();

  // Constructor with model - assumed before cuts
  AbcHeuristic (AbcModel & model);

  virtual ~AbcHeuristic();

  /// update model (This is needed if cliques update matrix etc)
  virtual void setModel(AbcModel * model);
  
  /// Clone
  virtual AbcHeuristic * clone() const=0;

  /** returns 0 if no solution, 1 if valid solution
      with better objective value than one passed in
      Sets solution values if good, sets objective value 
      This is called after cuts have been added - so can not add cuts
  */
  virtual int solution(double & objectiveValue,
		       double * newSolution)=0;

  /** returns 0 if no solution, 1 if valid solution, -1 if just
      returning an estimate of best possible solution
      with better objective value than one passed in
      Sets solution values if good, sets objective value (only if nonzero code)
      This is called at same time as cut generators - so can add cuts
      Default is do nothing
  */
  virtual int solution(double & objectiveValue,
		       double * newSolution,
		       OsiCuts & cs) {return 0;}

protected:

  /// Model
  AbcModel * model_;
private:
  
  /// Illegal Assignment operator 
  AbcHeuristic & operator=(const AbcHeuristic& rhs);
  
};

/** Rounding class
 */

class AbcRounding : public AbcHeuristic {
public:

  // Default Constructor 
  AbcRounding ();

  // Constructor with model - assumed before cuts
  AbcRounding (AbcModel & model);
  
  // Copy constructor 
  AbcRounding ( const AbcRounding &);
   
  // Destructor 
  ~AbcRounding ();
  
  /// Clone
  virtual AbcHeuristic * clone() const;

  /// update model (This is needed if cliques update matrix etc)
  virtual void setModel(AbcModel * model);
  
  /** returns 0 if no solution, 1 if valid solution
      with better objective value than one passed in
      Sets solution values if good, sets objective value (only if good)
      This is called after cuts have been added - so can not add cuts
  */
  virtual int solution(double & objectiveValue,
		       double * newSolution);


  /// Set seed
  void setSeed(int value)
  { seed_ = value;}

protected:
  // Data

  // Original matrix by column
  CoinPackedMatrix matrix_;

  // Original matrix by 
  CoinPackedMatrix matrixByRow_;

  // Seed for random stuff
  int seed_;

private:
  /// Illegal Assignment operator 
  AbcRounding & operator=(const AbcRounding& rhs);
};


#endif

