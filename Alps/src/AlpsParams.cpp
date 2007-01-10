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

#include "Alps.h"
#include "AlpsParams.h"

using std::make_pair;

void AlpsParams::createKeywordList() {

   //-------------------------------------------------------
   // Create the list of keywords for parameter file reading
   //-------------------------------------------------------

   //-------------------------------------------------------
   // CharPar
   //-------------------------------------------------------

   keys_.push_back(make_pair(std::string("Alps_deleteDeadNode"),
			     AlpsParameter(AlpsCharPar, 
					   deleteDeadNode)));
   keys_.push_back(make_pair(std::string("Alps_interClusterBalance"),
			    AlpsParameter(AlpsCharPar, 
					  interClusterBalance)));
   keys_.push_back(make_pair(std::string("Alps_intraClusterBalance"),
			    AlpsParameter(AlpsCharPar, 
					  intraClusterBalance)));
   keys_.push_back(make_pair(std::string("Alps_printSolution"),
                             AlpsParameter(AlpsCharPar, 
                                           printSolution)));
   //-------------------------------------------------------
   // BoolArrayPar (nothing here)
   //-------------------------------------------------------

   //-------------------------------------------------------
   // IntPar
   //-------------------------------------------------------

   keys_.push_back(make_pair(std::string("Alps_bufSpare"),
			     AlpsParameter(AlpsIntPar, 
					   bufSpare)));
   //
   keys_.push_back(make_pair(std::string("Alps_eliteSize"),
			     AlpsParameter(AlpsIntPar, 
					   eliteSize)));
   //
   keys_.push_back(make_pair(std::string("Alps_hubInitNodeNum"),
			     AlpsParameter(AlpsIntPar, 
					   hubInitNodeNum)));
   //
   keys_.push_back(make_pair(std::string("Alps_hubMsgLevel"),
			     AlpsParameter(AlpsIntPar, 
					   hubMsgLevel)));
   //
   keys_.push_back(make_pair(std::string("Alps_hubNum"),
			     AlpsParameter(AlpsIntPar, 
					   hubNum)));
   //
   keys_.push_back(make_pair(std::string("Alps_largeSize"),
			     AlpsParameter(AlpsIntPar, 
					   largeSize)));
   //
   keys_.push_back(make_pair(std::string("Alps_logFileLevel"),
			     AlpsParameter(AlpsIntPar, 
					   logFileLevel)));
   //
   keys_.push_back(make_pair(std::string("Alps_masterInitNodeNum"),
			     AlpsParameter(AlpsIntPar, 
					   masterInitNodeNum)));
   //
   keys_.push_back(make_pair(std::string("Alps_maxHubWorkSize"),
			     AlpsParameter(AlpsIntPar,
					   maxHubWorkSize)));
   //
   keys_.push_back(make_pair(std::string("Alps_masterReportInterval"),
			     AlpsParameter(AlpsIntPar, 
					   masterReportInterval)));
   //
   keys_.push_back(make_pair(std::string("Alps_mediumSize"),
			     AlpsParameter(AlpsIntPar, 
					   mediumSize)));
   //
   keys_.push_back(make_pair(std::string("Alps_msgLevel"),
			     AlpsParameter(AlpsIntPar, 
					   msgLevel)));
   //
   keys_.push_back(make_pair(std::string("Alps_nodeLimit"),
			     AlpsParameter(AlpsIntPar, 
					   nodeLimit)));
   //
   keys_.push_back(make_pair(std::string("Alps_nodeLogInterval"),
			     AlpsParameter(AlpsIntPar, 
					   nodeLogInterval)));
   //
   keys_.push_back(make_pair(std::string("Alps_processNum"),
			     AlpsParameter(AlpsIntPar, 
					   processNum)));
   //
   keys_.push_back(make_pair(std::string("Alps_searchStrategy"),
			     AlpsParameter(AlpsIntPar, 
					   searchStrategy)));
   //
   keys_.push_back(make_pair(std::string("Alps_smallSize"),
			     AlpsParameter(AlpsIntPar, 
					   smallSize)));
   //
   keys_.push_back(make_pair(std::string("Alps_solLimit"),
			     AlpsParameter(AlpsIntPar, 
					   solLimit)));
   //
   keys_.push_back(make_pair(std::string("Alps_unitWorkNodes"),
			     AlpsParameter(AlpsIntPar, 
					   unitWorkNodes)));
   //
   keys_.push_back(make_pair(std::string("Alps_workerMsgLevel"),
			     AlpsParameter(AlpsIntPar, 
					   workerMsgLevel)));

   //-------------------------------------------------------
   // DoublePar
   //-------------------------------------------------------

   keys_.push_back(make_pair(std::string("Alps_tolerance"),
			     AlpsParameter(AlpsDoublePar, 
					   tolerance)));
   //
   keys_.push_back(make_pair(std::string("Alps_unitWorkTime"),
			     AlpsParameter(AlpsDoublePar, 
					   unitWorkTime)));
   //
   keys_.push_back(make_pair(std::string("Alps_masterBalancePeriod"),
			     AlpsParameter(AlpsDoublePar, 
					   masterBalancePeriod)));

   //
   keys_.push_back(make_pair(std::string("Alps_hubReportPeriod"),
			     AlpsParameter(AlpsDoublePar, 
					   hubReportPeriod)));
   //
   keys_.push_back(make_pair(std::string("Alps_workerAskPeriod"),
			     AlpsParameter(AlpsDoublePar, 
					   workerAskPeriod)));
   //
   keys_.push_back(make_pair(std::string("Alps_rho"),
			     AlpsParameter(AlpsDoublePar, rho)));
   //
   keys_.push_back(make_pair(std::string("Alps_zeroLoad"),
			     AlpsParameter(AlpsDoublePar, zeroLoad)));
   //
   keys_.push_back(make_pair(std::string("Alps_needWorkThreshold"),
			     AlpsParameter(AlpsDoublePar, needWorkThreshold)));
   //
   keys_.push_back(make_pair(std::string("Alps_changeWorkThreshold"),
			     AlpsParameter(AlpsDoublePar, 
					   changeWorkThreshold)));
   //
   keys_.push_back(make_pair(std::string("Alps_donorThreshold"),
			     AlpsParameter(AlpsDoublePar, donorThreshold)));
   //
   keys_.push_back(make_pair(std::string("Alps_receiverThreshold"),
			     AlpsParameter(AlpsDoublePar, receiverThreshold)));
   //
   keys_.push_back(make_pair(std::string("Alps_timeLimit"),
			     AlpsParameter(AlpsDoublePar, timeLimit)));

   //-------------------------------------------------------
   // StringPar
   //-------------------------------------------------------

   keys_.push_back(make_pair(std::string("Alps_instance"),
			     AlpsParameter(AlpsStringPar, instance)));
   ///
   keys_.push_back(make_pair(std::string("Alps_logFile"),
			     AlpsParameter(AlpsStringPar, logFile)));
}

//#############################################################################

void AlpsParams::setDefaultEntries() {

  // CharPar
  setEntry(deleteDeadNode, true);
  setEntry(interClusterBalance, true);
  setEntry(intraClusterBalance, true);
  setEntry(printSolution, false);

  // IntPar
  setEntry(bufSpare, 256);
  setEntry(eliteSize, 1);
  setEntry(hubInitNodeNum, 2);
  setEntry(hubMsgLevel, 0);
  setEntry(hubNum, 1);
  setEntry(largeSize, 10485760);  // 2^21
  setEntry(logFileLevel, 0);
  setEntry(masterInitNodeNum, 2);
  setEntry(masterReportInterval, 10);
  setEntry(maxHubWorkSize, 0);   // Hub never works by default
  setEntry(mediumSize, 4096);    // 2^12
  setEntry(msgLevel, 2);
  setEntry(nodeLimit, ALPS_INT_MAX);
  setEntry(nodeLogInterval, 100);
  setEntry(processNum, 2);
  setEntry(searchStrategy, 4);  
  setEntry(smallSize, 1024);      // 2^10
  setEntry(solLimit, ALPS_INT_MAX);
  setEntry(unitWorkNodes, 20);
  setEntry(workerMsgLevel, 0);

  // DoublePar
  setEntry(changeWorkThreshold, 0.02);
  setEntry(donorThreshold, 0.02);
  setEntry(hubReportPeriod, 0.01);
  setEntry(masterBalancePeriod, 0.03);
  setEntry(needWorkThreshold, 2);
  setEntry(receiverThreshold, 0.02);
  setEntry(rho, 0.0);
  setEntry(timeLimit, ALPS_DBL_MAX);
  setEntry(tolerance, 1.0e-6);
  setEntry(unitWorkTime, 0.03);
  setEntry(workerAskPeriod, 0.5);
  setEntry(zeroLoad, 1.0e-6);

  // StringPar
  setEntry(instance, "NONE");
  setEntry(logFile, "Alps.log");
}

//#############################################################################
