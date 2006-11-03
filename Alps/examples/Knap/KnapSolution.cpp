/*===========================================================================*
 * This file is part of the Abstract Library for Parallel Search (ALPS).     *
 *                                                                           *
 * ALPS is distributed under the Common Public License as part of the        *
 * COIN-OR repository (http://www.coin-or.org).                              *
 *                                                                           *
 * Authors: Yan Xu, SAS Institute Inc.                                       *
 *          Ted Ralphs, Lehigh University                                    *
 *          Laszlo Ladanyi, IBM T.J. Watson Research Center                  *
 *          Matthew Saltzman, Clemson University                             *
 *                                                                           * 
 *                                                                           *
 * Copyright (C) 2001-2006, Lehigh University, Yan Xu, and Ted Ralphs.       *
 *===========================================================================*/

#include <iostream>
#include <set>

#include "KnapModel.h"
#include "KnapSolution.h"

//#############################################################################

void 
KnapSolution::print(std::ostream& os) const
{
    const int* seq = getModel()->getSequence(); 

    int i;
    std::set<int> solu;
    for (i = 0; i < size_; ++i) {
	if (solution_[i] == 1) 
	    solu.insert( seq[i]+1 );
    }

    i = 0;
    std::set<int>::iterator pos;
    //os << "Items in knapsack are:\n\n"; 
    for (pos = solu.begin(); pos != solu.end(); ++pos) {
	os << *pos;
	if (i != 0 && !((++i)%5)) 
	    os << "\n";
	else 
	    os << "\t";
    }
    os << std::endl;
}

//#############################################################################

AlpsEncoded*
KnapSolution::encode() const 
{ 
    //  AlpsEncoded* encoded = new AlpsEncoded(typeid(*this).name());
    AlpsEncoded* encoded = new AlpsEncoded("ALPS_SOLUTION");

    encoded->writeRep(value_);
    encoded->writeRep(size_);     // Base operand of `->' has non-pointer type
    encoded->writeRep(solution_, size_);

    return encoded; 
}

//#############################################################################

// Note: write and read sequence MUST same! 
AlpsKnowledge* 
KnapSolution::decode(AlpsEncoded& encoded) const
{ 
    int s, v;
    int* sol = 0;
    // sol = new int[s];       // By default, don't need to allocate memory
    encoded.readRep(v);        
    encoded.readRep(s);        // s must immediately before sol
    encoded.readRep(sol, s);

    return new KnapSolution(s, sol, v, getModel()); 
}

//#############################################################################
