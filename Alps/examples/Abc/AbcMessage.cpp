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

#include "AbcMessage.h"

//#############################################################################

typedef struct {
    ABC_Message internalNumber;
    int externalNumber;              // or continuation
    char detail;
    const char * message;
} Abc_message;

//#############################################################################

static Abc_message us_english[]=
{
    {ABC_END_GOOD,1,1,"Search completed - best objective %g, took %d iterations and %d nodes"},
    {ABC_MAXNODES,3,1,"Exiting on maximum nodes"},
    {ABC_MAXTIME,20,1,"Exiting on maximum time"},
    {ABC_MAXSOLS,19,1,"Exiting on maximum solutions"},
    {ABC_SOLUTION,4,1,"Integer solution of %g found after %d iterations and %d nodes"},
    {ABC_END,5,1,"Partial search took %d iterations and %d nodes"},
    {ABC_INFEAS,6,1,"The LP relaxation is infeasible or too expensive"},
    {ABC_STRONG,7,3,"Strong branching on %d (%d), down %g (%d) up %g (%d) value %d"},
    {ABC_SOLINDIVIDUAL,8,2,"%d has value %g"},
    {ABC_INTEGERINCREMENT,9,1,"Objective coefficients multiple of %g"},
    {ABC_STATUS,10,1,"Process[%d]: after %d nodes, %d on tree, %g best solution, best possible %g"},
    {ABC_GAP,11,1,"Exiting as integer gap of %g less than %g"},
    {ABC_ROUNDING,12,1,"Integer solution of %g found by rounding after %d iterations and %d nodes"},
    {ABC_ROOT,13,3,"At root node, %d cuts changed objective from %g to %g in %d passes"},
    {ABC_GENERATOR,14,2,"Cut generator %d (%s) - %d row cuts (%d active), %d column cuts - new frequency is %d"},
    {ABC_BRANCH,15,3,"Node %d Obj %g Unsat %d depth %d"},
    {ABC_STRONGSOL,16,1,"Integer solution of %g found by strong branching after %d iterations and %d nodes"},
    {ABC_NOINT,3007,0,"No integer variables - nothing to do"},
    {ABC_VUB_PASS,17,1,"%d solved, %d variables fixed, %d tightened"},
    {ABC_VUB_END,18,1,"After tightenVubs, %d variables fixed, %d tightened"},
    {ABC_NOTFEAS1,21,2,"On closer inspection node is infeasible"},
    {ABC_NOTFEAS2,22,2,"On closer inspection objective value of %g above cutoff of %g"},
    {ABC_NOTFEAS3,23,2,"Allowing solution, even though largest row infeasibility is %g"},
    {ABC_CUTOFF_WARNING1,23,1,"Cutoff set to %g - equivalent to best solution of %g"},
    {ABC_CUTS,24,1, "At node %d, %d cuts changed objective from %g to %g in %d passes"},
    {ABC_BRANCHSOL,25,1,"Integer solution of %g found by branching after %d iterations and %d nodes"},
    {ABC_DUMMY_END, 999999, 0, ""}
};

//#############################################################################

/* Constructor */
AbcMessage::AbcMessage(Language language) 
    :
    CoinMessages(sizeof(us_english) / sizeof(Abc_message))
{
    language_ = language;
    strcpy(source_, "Abc");
    Abc_message * message = us_english;

    while (message->internalNumber != ABC_DUMMY_END) {
	CoinOneMessage oneMessage(message->externalNumber, message->detail,
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
	while (message->internalNumber != ABC_DUMMY_END) {
	    replaceMessage(message->internalNumber, message->message);
	    message++;
	}
    }
}
