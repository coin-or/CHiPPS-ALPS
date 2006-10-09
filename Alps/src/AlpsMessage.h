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
 * Corporation, Lehigh University, Yan Xu, Ted Ralphs, Matthew Salzman and   *
 *===========================================================================*/

#ifndef AlpsMessage_h_
#define AlpsMessage_h_

#include "CoinMessageHandler.hpp"

//#############################################################################
/** A list of Alps print out messages. Note this is not the messages sent
    round among processes. */
enum ALPS_Message
{
    ALPS_DONATE_AFTER,
    ALPS_DONATE_BEFORE,
    ALPS_DONATE_FAIL,
    ALPS_DONATE_SPLIT,
    ALPS_DONATE_WHOLE,
    ALPS_DATAFILE,
    ALPS_LAUNCH,
    ALPS_LOADBAL_HUB,
    ALPS_LOADBAL_HUB_FAIL,
    ALPS_LOADBAL_MASTER,
    ALPS_LOADBAL_WORKER_ASK,
    ALPS_LOADREPORT_MASTER,
    ALPS_LOADBAL_MASTER_NO,
    ALPS_MSG_HOW,
    ALPS_NODE_COUNT,
    ALPS_PARAMFILE,
    ALPS_RAMPUP_HUB,
    ALPS_RAMPUP_HUB_FAIL,
    ALPS_RAMPUP_HUB_RECV,
    ALPS_RAMPUP_HUB_SOL,
    ALPS_RAMPUP_HUB_START,
    ALPS_RAMPUP_MASTER,
    ALPS_RAMPUP_MASTER_FAIL,
    ALPS_RAMPUP_MASTER_SOL,
    ALPS_RAMPUP_MASTER_START,
    ALPS_RAMPUP_WORKER_RECV,
    ALPS_SOLUTION_COUNT,
    ALPS_SOLUTION_SEARCH,
    ALPS_SEARCH_WORKER_START,
    ALPS_TERM_HUB_INFORM,
    ALPS_TERM_MASTER_START,
    ALPS_TERM_MASTER_INFORM,
    ALPS_TERM_WORKER_INFORM,
    ALPS_T_FEASIBLE,
    ALPS_T_INFEASIBLE,
    ALPS_T_NODE_LIMIT,
    ALPS_T_OPTIMAL,
    ALPS_T_TIME_LIMIT,
    // Following are for serial only
    ALPS_S_NODE_COUNT,
    ALPS_S_SEARCH_START,
    ALPS_S_SEARCH_SOL,
    ALPS_DUMMY_END
};

//#############################################################################

class AlpsMessage : public CoinMessages {

 public:
    /**@name Constructors etc */
    //@{
    AlpsMessage(Language language=us_en);
    //@}
};

#endif
