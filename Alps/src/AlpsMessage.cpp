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
    {ALPS_DONATE_AFTER, 11, 3, "Worker[%d] after donation %g nodes, %d subtrees"},
    {ALPS_DONATE_BEFORE, 14, 3, "Worker[%d] before donation %g nodes, %d subtrees"},
    {ALPS_DONATE_FAIL, 16, 3, "Worker[%d] fail to donate a subtree to %d, tag %d"},
    {ALPS_DONATE_SPLIT, 19, 3, "Worker[%d] donate a splitted subtree to %d, tag %d"},
    {ALPS_DONATE_WHOLE, 25, 3, "Worker[%d] donate a whole subtree to %d, tag %d"},
    {ALPS_DATAFILE, 30, 1, "Data file: %s"},
    {ALPS_KNOWLEDGE_GEN, 40, 1, "Master[%d] sent shared knowledge to hubs"},
    {ALPS_LAUNCH, 50, 1, "Launched %d processes"},
    {ALPS_LOADBAL_HUB, 60, 3, "Hub[%d] balanced the workload of its workers %d times"},
    {ALPS_LOADBAL_HUB_FAIL, 63, 1, "Hub[%d] failed to find a process to donate work to process %d"},
    {ALPS_LOADBAL_HUB_NO, 65, 1, "Hub[%d] balanced workload but do nothing since work load is %g"},
    {ALPS_LOADBAL_HUB_PERIOD, 67, 1, "Hub[%d] initially balances or report its workers every %.4f seconds"},
    {ALPS_LOADBAL_MASTER, 70, 3, "Master[%d] balanced the workloads of the hubs %d times"},
    {ALPS_LOADBAL_MASTER_NO, 72, 1, "Master[%d] balanced workload but do nothing since work load is %g"},
    {ALPS_LOADBAL_MASTER_PERIOD, 76, 1, "Master[%d] initially balances the hubs every %.4f seconds"},
    {ALPS_LOADBAL_WORKER_ASK, 80, 3, "Worker[%d] asks its hub (%d) for work"},
    {ALPS_LOADREPORT_MASTER, 90, 1, "Node %d: left %g, msg(s %d, r %d), inter(%d, %g), npt %g, unit %d, sol %g, %.0f sec."},
    {ALPS_LOADREPORT_MASTER_F, 92, 1, "Node %d: left(%g / %g), msg(s %d, r %d), inter %d, sol %g, %.0f sec."},
    {ALPS_LOADREPORT_MASTER_N, 94, 1, "Node %d: left %g, msg(s %d, r %d), inter(%d, %g), npt %g, unit %d, no sol, %.0f sec."},
    {ALPS_LOADREPORT_MASTER_F_N, 96, 1, "Node %d: left(%g / %g), msg(s %d, r %d), inter %d, no sol, %.0f sec."},
    {ALPS_MSG_HOW, 100, 3, "Process[%d] %s count %d in %s"},
    {ALPS_NODE_COUNT, 104, 3, "Worker[%d] processed %d nodes, %d nodes are left"},
    {ALPS_NODE_MEM_SIZE, 106, 1, "The memory size of a node is about %d bytes"},
    {ALPS_P_VERSION, 110, 1, "ALPS version 0.95.0 (Parallel, MPI)"},
    {ALPS_PARAMFILE, 120, 1, "Parameter file: %s"},
    {ALPS_PEAK_MEMORY, 125, 1, "Peak memory usage: %.2f M"},
    {ALPS_RAMPUP_HUB, 130, 1, "Hub[%d]'s rampup took %g seconds to process %d nodes. Node pool has %d nodes"},
    {ALPS_RAMPUP_HUB_FAIL, 132, 1, "Hub[%d] failed to generate enought subtrees (nodes) and finish search by itself"},
    {ALPS_RAMPUP_HUB_NODES, 133, 1, "Hub[%d] will generate %d nodes during rampup. Node processing time %g"},
    {ALPS_RAMPUP_HUB_NODES_AUTO, 134, 1, "Hub[%d] required %d nodes during rampup. Node processing time %g"},
    {ALPS_RAMPUP_HUB_RECV, 135, 3, "Hub[%d] received all subtrees (nodes) sent by the master(%d)"},
    {ALPS_RAMPUP_HUB_SOL, 136, 3, "Hub[%d] found a better solution %g during rampup"}, 
    {ALPS_RAMPUP_HUB_START, 138, 1, "Hub[%d] is creating nodes (%d) for its workers during rampup"},
    {ALPS_RAMPUP_MASTER, 140, 1, "Master[%d]'s rampup took %g seconds to process %d nodes. Node pool has %d nodes"},
    {ALPS_RAMPUP_MASTER_FAIL, 142, 1, "Master[%d] failed to generate enought subtrees (nodes) and finish search by itself"},
    {ALPS_RAMPUP_MASTER_NODES, 143, 1, "Master[%d] will generate %d nodes during rampup. Node processing time %g"},
    {ALPS_RAMPUP_MASTER_NODES_AUTO, 145, 1, "Master[%d] required %d nodes during rampup. Node processing time %g"},
    {ALPS_RAMPUP_MASTER_SOL, 146, 2, "Master[%d] found a better solution %g during rampup"},
    {ALPS_RAMPUP_MASTER_START, 148, 1, "Master[%d] is creating nodes (%d) for its hubs during rampup"},
    {ALPS_RAMPUP_WORKER_RECV, 150, 3, "Worker[%d] received all subtrees (nodes) sent by its hub(%d)"},
    {ALPS_SEARCH_WORKER_START, 152, 3, "Worker[%d] is searching solutions ..." },
    {ALPS_SOLUTION_COUNT, 160, 3, "Process[%d] has %d solutions"},
    {ALPS_SOLUTION_SEARCH, 162, 3, "Worker[%d] found a better solution %g during search"}, 
    {ALPS_TERM_FORCE_NODE, 170, 1, "Master asked other processes to stop searching due to reaching node limt %d"},
    {ALPS_TERM_FORCE_TIME, 172, 1, "Master asked other processes to stop searching due to reaching time limt %.2f seconds"},
    {ALPS_TERM_HUB_INFORM, 180, 3, "Hub[%d] got instruction to %s"},
    {ALPS_TERM_MASTER_START, 190, 1, "Master[%d] is doing termination check"},
    {ALPS_TERM_MASTER_INFORM, 192, 1, "Master[%d] asked other processes to %s"},
    {ALPS_TERM_WORKER_INFORM, 194, 3, "Worker[%d] got instruction to %s"},
    {ALPS_T_FAILED,198, 1, "Search failed, %d nodes processed, %d nodes left"},
    {ALPS_T_FEASIBLE,200, 1, "Incompleted search found feasible solutions, %d nodes processed, %d nodes left"},
    {ALPS_T_INFEASIBLE,202, 1, "Problem is infeasible. %d nodes processed, %d nodes left"},
    {ALPS_T_NODE_LIMIT,204, 1, "Reached node limit. %d nodes processed, %d nodes left"},
    {ALPS_T_NO_MEMORY,206, 1, "Out of memory. %d nodes processed, %d nodes left"},
    {ALPS_T_OPTIMAL,208, 1, "Found an optimal solution. %d nodes processed, %d nodes left"},
    {ALPS_T_TIME_LIMIT,230, 1, "Reached time limit. %d nodes processed, %d nodes left"},
    {ALPS_S_NODE_COUNT, 240, 1, "Processed %d nodes, has %d nodes, best relaxed %g, best feasible %g"},
    {ALPS_S_SEARCH_START, 250, 1, "Search solutions ..."},
    {ALPS_S_SEARCH_SOL, 255, 3, "Process %d found a better solution %g"},
    {ALPS_S_FINAL_SOL, 260, 1, "Quality of the best solution found: %g"},
    {ALPS_S_FINAL_NO_SOL, 264, 1, "No solution found"},
    {ALPS_S_FINAL_NODE_PROCESSED, 268, 1, "Number of nodes processed: %d"},
    {ALPS_S_FINAL_NODE_LEFT, 270, 1, "Number of nodes left: %d"},
    {ALPS_S_FINAL_DEPTH, 272, 1, "Tree depth: %d"},
    {ALPS_S_FINAL_CPU, 274, 1, "Search CPU time: %.2f seconds"},
    {ALPS_S_FINAL_WALLCLOCK, 278, 1, "Search wall-clock time: %.2f seconds"},
    {ALPS_S_VERSION, 300, 1, "ALPS version 0.95.0 (Serial)"},
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
