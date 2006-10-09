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

#include "AlpsMessage.h"

//#############################################################################
//#############################################################################

typedef struct {
    ALPS_Message internalNumber;
    int externalNumber;
    char detail;
    const char * message;
} Alps_message;

//#############################################################################

static Alps_message us_english[] =
{
    {ALPS_DONATE_AFTER, 71, 3, "Worker[%d] after donation %g nodes, %d subtrees"},
    {ALPS_DONATE_BEFORE, 72, 3, "Worker[%d] before donation %g nodes, %d subtrees"},
    {ALPS_DONATE_FAIL, 73, 3, "Worker[%d] fail to donate a subtree to %d, tag %d"},
    {ALPS_DONATE_SPLIT, 74, 3, "Worker[%d] donate a splitted subtree to %d, tag %d"},
    {ALPS_DONATE_WHOLE, 75, 3, "Worker[%d] donate a whole subtree to %d, tag %d"},
    {ALPS_DATAFILE, 1, 1, "Data file: %s"},
    {ALPS_LAUNCH, 2, 1, "ALPS launched %d processes"},
    {ALPS_LOADBAL_HUB, 5, 3, "Hub[%d] balances the workload of its workers %d times"},
    {ALPS_LOADBAL_HUB_FAIL, 6, 3, "Hub[%d] failed to find a process to donate work to process %d"},
    {ALPS_LOADBAL_MASTER, 7, 3, "Master[%d] balanced workloads of the hubs %d times"},
    {ALPS_LOADBAL_MASTER_NO, 8, 3, "Master[%d] balanced workload but do nothing since work load is %g"},
    {ALPS_LOADREPORT_MASTER, 9, 1, "Master[%d]: nodes(processed %d, left quantity %g), msg counts(sent %d, received %d), incumbent %g"},
    {ALPS_LOADBAL_WORKER_ASK, 10, 3, "Worker[%d] asks its hub (%d) for work"},
    {ALPS_MSG_HOW, 13, 3, "Process[%d] %s count %d in %s"},
    {ALPS_NODE_COUNT, 15, 3, "Worker[%d] processed %d nodes, %d nodes are left"},
    {ALPS_PARAMFILE, 18, 1, "Parameter file: %s"},
    {ALPS_RAMPUP_HUB, 19, 1, "Hub[%d] took %g seconds, processed %d nodes, node pool has %d nodes"},
    {ALPS_RAMPUP_HUB_FAIL, 20, 1, "Hub[%d] failed to generate enought subtrees (nodes) and finish search by itself"},
    {ALPS_RAMPUP_HUB_RECV, 21, 3, "Hub[%d] received all subtrees (nodes) sent by the master(%d)"},
    {ALPS_RAMPUP_HUB_SOL, 22, 3, "Hub[%d] found a better solution %g during rampup"}, 
    {ALPS_RAMPUP_HUB_START, 23, 1, "Hub[%d] is creating subtrees (node) for its workers"},
    {ALPS_RAMPUP_MASTER, 24, 1, "Master[%d] took %g seconds, processed %d nodes, node pool has %d nodes"},
    {ALPS_RAMPUP_MASTER_FAIL, 25, 1, "Master[%d] failed to generate enought subtrees (nodes) and finish search by itself"},
    {ALPS_RAMPUP_MASTER_SOL, 26, 2, "Master[%d] found a better solution %g during rampup"},
    {ALPS_RAMPUP_MASTER_START, 27, 1, "Master[%d] is creating subtrees (node) for hubs"},
    {ALPS_RAMPUP_WORKER_RECV, 28, 3, "Worker[%d] received all subtrees (nodes) sent by its hub(%d)"},
    {ALPS_SEARCH_WORKER_START, 35, 3, "Worker[%d] start to search" },
    {ALPS_SOLUTION_COUNT, 40, 3, "Process[%d] has %d solutions"},
    {ALPS_SOLUTION_SEARCH, 41, 3, "Worker[%d] found a better solution %g during search"}, 
    {ALPS_TERM_HUB_INFORM, 45, 3, "Hub[%d] got instruction to %s"},
    {ALPS_TERM_MASTER_START, 46, 1, "Master[%d] checks termination"},
    {ALPS_TERM_MASTER_INFORM, 47, 1, "Master[%d] tells other processes to %s"},
    {ALPS_TERM_WORKER_INFORM, 48, 3, "Worker[%d] got instruction to %s"},
    {ALPS_T_FEASIBLE,50, 1, "Partial search found feasible solutions, %d nodes processed, %d nodes left"},
    {ALPS_T_INFEASIBLE,51, 1, "Exiting due to infeasibility, %d nodes processed, %d nodes left"},
    {ALPS_T_NODE_LIMIT,52, 1, "Exiting on maximum nodes, %d nodes processed, %d nodes left"},
    {ALPS_T_OPTIMAL,53, 1, "Search completed, %d nodes processed, %d nodes left"},
    {ALPS_T_TIME_LIMIT,54, 1, "Exiting on maximum time, %d nodes processed, %d nodes left"},
    {ALPS_S_NODE_COUNT, 60, 1, "Processed %d nodes, has %d nodes, best relaxed %g, best feasible %g"},
    {ALPS_S_SEARCH_START, 61, 1, "Start to search ..."},
    {ALPS_S_SEARCH_SOL, 62, 3, "Process %d found a better solution %g"},
    {ALPS_DUMMY_END, 999999, 0, ""}
};

//#############################################################################
// Constructor
AlpsMessage::AlpsMessage(Language language) 
    :
    CoinMessages(sizeof(us_english)/sizeof(Alps_message))
{
    language_ = language;
    strcpy(source_, "Alps");
    Alps_message * message = us_english;
    
    while (message->internalNumber != ALPS_DUMMY_END) {
	CoinOneMessage oneMessage(message->externalNumber, 
				  message->detail,
				  message->message);
	addMessage(message->internalNumber, oneMessage);
	message++;
    }

    // now override any language ones
    switch (language) {
	
    default:
	message = NULL;
	break;
    }
    
    // replace if any found
    if (message) {
	while (message->internalNumber != ALPS_DUMMY_END) {
	    replaceMessage(message->internalNumber, message->message);
	    message++;
	}
    }
}
