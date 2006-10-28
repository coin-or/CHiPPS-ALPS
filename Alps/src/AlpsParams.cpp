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

#include "AlpsParams.h"

using std::make_pair;

void AlpsParams::createKeywordList() {

   //-------------------------------------------------------
   // Create the list of keywords for parameter file reading
   //-------------------------------------------------------

   //-------------------------------------------------------
   // CharPar
   //-------------------------------------------------------

   keys_.push_back(make_pair(std::string("Alps_inputFromFile"),
			    AlpsParameter(AlpsCharPar, 
					  inputFromFile)));
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

   keys_.push_back(make_pair(std::string("Alps_logFileLevel"),
			     AlpsParameter(AlpsIntPar, 
					   logFileLevel)));
   //
   keys_.push_back(make_pair(std::string("Alps_msgLevel"),
			     AlpsParameter(AlpsIntPar, 
					   msgLevel)));
   //
   keys_.push_back(make_pair(std::string("Alps_processNum"),
			     AlpsParameter(AlpsIntPar, 
					   processNum)));
   //
   keys_.push_back(make_pair(std::string("Alps_hubNum"),
			     AlpsParameter(AlpsIntPar, 
					   hubNum)));
   //
   keys_.push_back(make_pair(std::string("Alps_masterInitNodeNum"),
			     AlpsParameter(AlpsIntPar, 
					   masterInitNodeNum)));
   //
   keys_.push_back(make_pair(std::string("Alps_hubInitNodeNum"),
			     AlpsParameter(AlpsIntPar, 
					   hubInitNodeNum)));
   //
   keys_.push_back(make_pair(std::string("Alps_unitWorkNodes"),
			     AlpsParameter(AlpsIntPar, 
					   unitWorkNodes)));
   //
   keys_.push_back(make_pair(std::string("Alps_maxNumSolustion"),
			     AlpsParameter(AlpsIntPar, 
					   maxNumSolustion)));
   //
   keys_.push_back(make_pair(std::string("Alps_smallSize"),
			     AlpsParameter(AlpsIntPar, 
					   smallSize)));
   //
   keys_.push_back(make_pair(std::string("Alps_mediumSize"),
			     AlpsParameter(AlpsIntPar, 
					   mediumSize)));
   //
   keys_.push_back(make_pair(std::string("Alps_largeSize"),
			     AlpsParameter(AlpsIntPar, 
					   largeSize)));
   //
   keys_.push_back(make_pair(std::string("Alps_bufSpare"),
			     AlpsParameter(AlpsIntPar, 
					   bufSpare)));
   //
   keys_.push_back(make_pair(std::string("Alps_minNodeNum"),
			     AlpsParameter(AlpsIntPar, 
					   minNodeNum)));
   //
   keys_.push_back(make_pair(std::string("Alps_nodeLogInterval"),
			     AlpsParameter(AlpsIntPar, 
					   nodeLogInterval)));
   //
   keys_.push_back(make_pair(std::string("Alps_eliteSize"),
			     AlpsParameter(AlpsIntPar, 
					   eliteSize)));
   //
   keys_.push_back(make_pair(std::string("Alps_searchStrategy"),
			     AlpsParameter(AlpsIntPar, 
					   searchStrategy)));
   //
   keys_.push_back(make_pair(std::string("Alps_masterReportInterval"),
			     AlpsParameter(AlpsIntPar, 
					   masterReportInterval)));
   //
   keys_.push_back(make_pair(std::string("Alps_nodeLimit"),
			     AlpsParameter(AlpsIntPar, 
					   nodeLimit)));
   //
   keys_.push_back(make_pair(std::string("Alps_maxHubWorkSize"),
			     AlpsParameter(AlpsIntPar,
					   maxHubWorkSize)));

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
  setEntry(printSolution, false);
  setEntry(inputFromFile, true);
  setEntry(deleteDeadNode, true);
  setEntry(interClusterBalance, true);
  setEntry(intraClusterBalance, true);

  // IntPar
  setEntry(logFileLevel, 0);
  setEntry(msgLevel, 2);
  setEntry(processNum, 2);
  setEntry(hubNum, 1);
  setEntry(masterInitNodeNum, 2);
  setEntry(hubInitNodeNum, 2);
  setEntry(unitWorkNodes, 50);
  setEntry(maxNumSolustion, 100000);
  setEntry(smallSize, 256);      // 2^8
  setEntry(mediumSize, 4096);    // 2^12
  setEntry(largeSize, 1048576);  // 2^21
  setEntry(bufSpare, 256);
  setEntry(minNodeNum, 1);
  setEntry(nodeLogInterval, 100);
  setEntry(eliteSize, 5);
  setEntry(searchStrategy, 0);
  setEntry(masterReportInterval, 10);
  setEntry(nodeLimit, 210000000);
  setEntry(maxHubWorkSize, 0); // Hub never works now

  // DoublePar
  setEntry(tolerance, 1.0e-6);
  setEntry(unitWorkTime, 0.5);
  setEntry(masterBalancePeriod, 0.05);
  setEntry(hubReportPeriod, 0.5);
  setEntry(workerAskPeriod, 0.5);
  setEntry(rho, 0.0);
  setEntry(zeroLoad, 1.0e-6);
  setEntry(needWorkThreshold, 2);
  setEntry(changeWorkThreshold, 0.05);
  setEntry(donorThreshold, 0.10);
  setEntry(receiverThreshold, 0.10);
  setEntry(timeLimit, 1.0e75);

  // StringPar
  setEntry(instance, "NONE");
  setEntry(logFile, "Alps.log");
}

//#############################################################################
