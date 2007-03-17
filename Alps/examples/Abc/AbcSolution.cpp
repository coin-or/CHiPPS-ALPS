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
 * Copyright (C) 2001-2006, Lehigh University, Yan Xu, and Ted Ralphs.       *
 *===========================================================================*/

#include <iomanip>
#include <iostream>
#include <set>

//#include "AlpsDataPool.h" 
#include "AbcModel.h"
#include "AbcSolution.h"

//#############################################################################

void 
AbcSolution::print(std::ostream& os) const
{
    os <<std::setiosflags(std::ios::fixed|std::ios::showpoint)<<std::setw(14);

    os << "-------------------------" <<std::endl;
    for (int i = 0; i < size_; ++i) {
	if (fabs(value_[i]) > 1.0e-7) {
	    os << std::setw(6) << i << " " << value_[i] << std::endl;
	}
    }
    os << "-------------------------" <<std::endl;
    os <<std::resetiosflags(std::ios::fixed|std::ios::showpoint|std::ios::scientific);

}

/** The method that encodes the node into a encoded object. */
AlpsEncoded*
AbcSolution::encode() const 
{ 
  //  AlpsEncoded* encoded = new AlpsEncoded(typeid(*this).name());
  AlpsEncoded* encoded = new AlpsEncoded("ALPS_SOLUTION");
  
  encoded->writeRep(size_);     // Base operand of `->' has non-pointer type
  encoded->writeRep(value_, size_);
  encoded->writeRep(objective_);

  return encoded; 
}

//#############################################################################

/** The method that decodes the node from a encoded object. */
// Note: write and read sequence MUST same! 
AlpsKnowledge* 
AbcSolution::decode(AlpsEncoded& encoded) const 
{ 
  int s;
  double obj;
  double* val = 0;
  encoded.readRep(s);        
  encoded.readRep(val, s);        // s must immediately before sol
  encoded.readRep(obj);
  AbcSolution* sol = new AbcSolution(s, val, obj);

  if (val != 0) {
      delete [] val;
      val = 0;
  }
  
  return sol;
}

//#############################################################################
