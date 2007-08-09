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
 * Copyright (C) 2001-2007, Lehigh University, Yan Xu, and Ted Ralphs.       *
 *===========================================================================*/

#ifndef AbcMessage_H_
#define AbcMessage_H_

//#############################################################################
// This file is modified from SbbMessage.hpp
//#############################################################################

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

/** This deals with Abc messages (as against Clp messages etc).
    CoinMessageHandler.hpp is the general part of message handling.
    All it has are enum's for the various messages.
    AbcMessage.cpp has text in various languages.
    
    It is trivial to use the .hpp and .cpp file as a basis for
    messages for other components.
 */

#include "CoinMessageHandler.hpp"

enum ABC_Message
{
  ABC_END_GOOD,
  ABC_MAXNODES,
  ABC_MAXTIME,
  ABC_MAXSOLS,
  ABC_SOLUTION,
  ABC_END,
  ABC_INFEAS,
  ABC_STRONG,
  ABC_SOLINDIVIDUAL,
  ABC_INTEGERINCREMENT,
  ABC_STATUS,
  ABC_GAP,
  ABC_ROUNDING,
  ABC_ROOT,
  ABC_GENERATOR,
  ABC_BRANCH,
  ABC_STRONGSOL,
  ABC_NOINT,
  ABC_VUB_PASS,
  ABC_VUB_END,
  ABC_NOTFEAS1,
  ABC_NOTFEAS2,
  ABC_NOTFEAS3,
  ABC_CUTOFF_WARNING1,
  ABC_CUTS,
  ABC_BRANCHSOL,
  ABC_DUMMY_END
};

class AbcMessage : public CoinMessages {

public:

  /**@name Constructors etc */
  //@{
  /** Constructor */
  AbcMessage(Language language=us_en);
  //@}

};

#endif
