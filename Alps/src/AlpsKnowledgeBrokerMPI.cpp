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

#include <algorithm>
#include <cassert>
#include <cstring>
#include <exception>

#include "CoinError.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinTime.hpp"

#include "AlpsHelperFunctions.h"
#include "AlpsKnowledgeBrokerMPI.h"
#include "AlpsMessageTag.h"
#include "AlpsModel.h"
#include "AlpsNodePool.h"
#include "AlpsTreeNode.h"

using std::exception;

//#############################################################################

static int cluster2GlobalRank(int masterRank, int myHubRank, int clusterRank) 
{
    return (myHubRank - masterRank + clusterRank);
}

//#############################################################################

// Form binary tree to broadcast solutions.
static int rankToSequence(const int incRank, const int rank) 
{
    if (rank < incRank) return rank + 1;
    else if (rank == incRank) return 0;
    else return rank;
}

static int sequenceToRank(const int incRank, const int sequence)
{
    if (sequence == 0) return incRank;
    else if (sequence <= incRank) return sequence - 1;
    else return sequence;
}

#if 0 // not used
static int parentSequence(const int sequence, const int numProcesses)
{ 
    if (sequence <= 0) return -1;
    else return (sequence - 1) / 2; 
}
#endif

static int leftSequence(const int sequence, const int numProcesses)
{ 
    int seq =  2 * sequence + 1;
    if ( seq >= numProcesses ) return -1;
    else return seq;
}

static int rightSequence(const int sequence, const int numProcesses)
{ 
    int seq =  2 * sequence + 2;
    if ( seq >= numProcesses ) return -1;
    else return seq;  
}

//#############################################################################

static int computeUnitNodes(int oldUnitNodes,
                            double nodeProcessingTime)
{
    int unitNodes = 1;
    
    if (nodeProcessingTime < 1.0e-14) {
        nodeProcessingTime = 1.0e-5;
    }

    if (nodeProcessingTime > 30.0) {
	unitNodes = 1;
    }
    else if (nodeProcessingTime > 5.0) {
	unitNodes = 1;
    }
    else if (nodeProcessingTime > 1.0) {
	unitNodes = 2;
    }	     
    else if (nodeProcessingTime > 0.1) {
	unitNodes = 3;
    }
    else if (nodeProcessingTime > 0.05) {
	unitNodes = 4;
    }
    else if (nodeProcessingTime > 0.01) {
	unitNodes = 5;
    }
    else if (nodeProcessingTime > 0.005) {
	unitNodes = 7;
    }
    else if (nodeProcessingTime > 0.001) {
	unitNodes = 30;
    }
    else if (nodeProcessingTime > 0.0005) {
	unitNodes = 50;
    }
    else if (nodeProcessingTime > 0.0001) {
	unitNodes = 200;
    }
    else if (nodeProcessingTime > 0.00005) {
	unitNodes = 400;
    }
    else {
	unitNodes = 500;
    }

    if (oldUnitNodes > 0) {
        unitNodes = (int)(0.5 * (unitNodes + oldUnitNodes));
    }
    
#if 0
    std::cout << "$$$ Old unitNodes = " << oldUnitNodes 
              << "$$$ new unitWork = " << unitNodes
	      << ", nodeProcessingTime = " << nodeProcessingTime
	      << std::endl;
#endif

    return unitNodes;
}

//#############################################################################

static double computeBalancePeriod(bool hasUserInput,
                                   double oldBalancePeriod,
				   double nodeProcessingTime)
{
    if (hasUserInput) return oldBalancePeriod;
    
    double newPeriod = 0.1;
    
    if (nodeProcessingTime < 1.0e-14) {
        nodeProcessingTime = 1.0e-5;    
    }
    
    if (nodeProcessingTime > 30.0) {
	newPeriod = nodeProcessingTime;
    }
    else if (nodeProcessingTime > 5.0) {
	newPeriod = nodeProcessingTime;
    }
    else if (nodeProcessingTime > 1.0) {
	newPeriod = nodeProcessingTime;
    }
    else if (nodeProcessingTime > 0.5) {
	newPeriod = nodeProcessingTime;
    }
    else if (nodeProcessingTime > 0.1) {
	newPeriod = nodeProcessingTime;
    }
    else if (nodeProcessingTime > 0.05) {
	newPeriod = nodeProcessingTime * 2;
    }
    else if (nodeProcessingTime > 0.01) {
	newPeriod = nodeProcessingTime * 3;
    }
    else if (nodeProcessingTime > 0.005) {
        newPeriod = nodeProcessingTime * 10;
    }
    else if (nodeProcessingTime > 0.001) {
        newPeriod = nodeProcessingTime * 40;
    }
    else if (nodeProcessingTime > 0.0005) {
	newPeriod = nodeProcessingTime * 100;
    }
    else if (nodeProcessingTime > 0.0001) {
	newPeriod = nodeProcessingTime * 200;
    }
    else if (nodeProcessingTime > 0.00005) {
	newPeriod = nodeProcessingTime * 600;
    }
    else if (nodeProcessingTime > 0.00001) {
	newPeriod = nodeProcessingTime * 1500;
    }
    else {
	newPeriod = 0.03;
    }

    if (oldBalancePeriod > 0.0) {
        newPeriod = 0.5 * (newPeriod + oldBalancePeriod);
    }
    
#if 0
    std::cout << "$$$ newPeriod = " << newPeriod
              << ", oldBalancePeriod = " << oldBalancePeriod
	      << ", nodeProcessingTime = " << nodeProcessingTime
	      << std::endl;
#endif
    return newPeriod;
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::masterMain(AlpsTreeNode* root)
{
    int i;
    int preSysSendCount = 0;
    int masterCheckCount = 0;
    int hubCheckCount = 0;
    int reportInterval = 0;
    int reportIntervalCount = 0;
 
    double elaspeTime  = 0.0;
    
    char reply = 'C';
    
    bool allWorkerReported = false;
    bool terminate = false;

    //------------------------------------------------------
    // Get parameters.
    //------------------------------------------------------
    
    const int staticBalanceScheme = 
	model_->AlpsPar()->entry(AlpsParams::staticBalanceScheme);
    
    //const bool clockType = 
    //model_->AlpsPar()->entry(AlpsParams::clockType);
    const bool interCB = 
	model_->AlpsPar()->entry(AlpsParams::interClusterBalance);
    const bool intraCB = 
	model_->AlpsPar()->entry(AlpsParams::intraClusterBalance);
    const int smallSize = 
	model_->AlpsPar()->entry(AlpsParams::smallSize);
    const int nodeLimit = 
	model_->AlpsPar()->entry(AlpsParams::nodeLimit);
    const double zeroLoad = 
	model_->AlpsPar()->entry(AlpsParams::zeroLoad);

    masterBalancePeriod_ =
        model_->AlpsPar()->entry(AlpsParams::masterBalancePeriod);
    if (masterBalancePeriod_ > 0.0) {
        userBalancePeriod_ = true;
    }
    else {
        masterBalancePeriod_ = -masterBalancePeriod_ ;
    }
    
    largeSize_ = model_->AlpsPar()->entry(AlpsParams::largeSize);

    //------------------------------------------------------
    // Initialization and setup.
    //------------------------------------------------------

    masterTimer_.setClockType(AlpsClockTypeWallClock);

    hubNodeProcesseds_ = new int [hubNum_];
    hubWorkQualities_ = new double [hubNum_];
    hubWorkQuantities_ = new double [hubNum_];
    hubReported_ = new bool [hubNum_];
    workerNodeProcesseds_ = new int [clusterSize_];
    workerWorkQualities_ = new double [clusterSize_];
    workerWorkQuantities_ = new double [clusterSize_];
    workerReported_ = new bool [clusterSize_];
  
    for (i = 0; i < hubNum_; ++i) {
	hubNodeProcesseds_[i] = 0;
	hubWorkQualities_[i] = 0.0;
	hubWorkQuantities_[i] = 0.0;
	hubReported_[i] = false;
    }
    for (i = 0; i < clusterSize_; ++i) {
	workerNodeProcesseds_[i] = 0;
	workerWorkQualities_[i] = ALPS_OBJ_MAX;
	workerWorkQuantities_[i] = 0.0;
	workerReported_[i] = false;
    }

    workerNodeProcesseds_[0] = nodeProcessedNum_;
    workerWorkQualities_[0] = workQuality_;
    workerWorkQuantities_[0] = workQuantity_;

    root->setKnowledgeBroker(this); 
    root->setQuality(ALPS_OBJ_MAX);
    root->setDepth(0);
    root->setIndex(0);
    root->setExplicit(1); // Always true for root.

    //------------------------------------------------------
    // Estimate a tree node size and send it to other hubs.
    //------------------------------------------------------
    
    AlpsEncoded* encSize = root->encode();
    setNodeMemSize(static_cast<int>(encSize->size() * 4.0));
    delete encSize;
    
    // Adjust largeSize to avoid extreme cases.
    largeSize_ = CoinMax(largeSize_, nodeMemSize_ * 3);

    // Allocate msg buffers
    largeBuffer_ = new char [largeSize_];
    largeBuffer2_ = new char [largeSize_];
    smallBuffer_ = new char [smallSize];

    for (i = 0; i < hubNum_; ++i) {
	if (hubRanks_[i] != globalRank_) {
	    MPI_Send(&nodeMemSize_, 1, MPI_INT, i, AlpsMsgNodeSize, hubComm_);
	}
    }
    
    if (msgLevel_ > 0) {
	messageHandler()->message(ALPS_NODE_MEM_SIZE, messages())
	    << nodeMemSize_ << CoinMessageEol;
    }

    //------------------------------------------------------
    // Send estimated node size to master's workers.
    //------------------------------------------------------

    for (i = 0; i < clusterSize_; ++i) {
	if (i != clusterRank_) {
	    MPI_Send(&nodeMemSize_, 1, MPI_INT, i, AlpsMsgNodeSize, 
		     clusterComm_);
	}
    }

    MPI_Barrier(MPI_COMM_WORLD); // Sync before rampup    

    //======================================================
    // Master's Ramp-up.
    //======================================================
    
    // Start to measure rampup
    masterTimer_.start();

    setPhase(AlpsPhaseRampup);
    switch (staticBalanceScheme) {
    case AlpsRootInit:
	rootInitMaster(root);
	break;
    case AlpsSpiral:
	spiralMaster(root);
	break;
    default:
	throw CoinError("Unknown static balance scheme", "masterMain", 
			"AlpsKnowledgeBrokerMPI");
    }

    rampUpTime_ = masterTimer_.getTime();

    //======================================================
    // Search for solutions.
    //======================================================

    setPhase(AlpsPhaseSearch);

    //------------------------------------------------------
    // MASTER SCHEDULER: 
    // a. Listen and process messages periodically. 
    // b. Do termination check if the conditions are statisfied, 
    //    otherwise, balances the workload of hubs.
    //------------------------------------------------------

    masterBalancePeriod_ = computeBalancePeriod(userBalancePeriod_,
                                                masterBalancePeriod_,
                                                nodeProcessingTime_);

    if (msgLevel_ > 0 && interCB) {
	messageHandler()->message(ALPS_LOADBAL_MASTER_PERIOD, messages())
	    << globalRank_ <<  masterBalancePeriod_ << CoinMessageEol;
    }
    
    MPI_Request request;
    MPI_Status status;
    int flag;

    MPI_Irecv(largeBuffer_, largeSize_, MPI_PACKED, MPI_ANY_SOURCE, 
	      MPI_ANY_TAG, MPI_COMM_WORLD, &request);

    while (true) {
	
	//**------------------------------------------------
	// Listen and process msg for a period. 
	//**------------------------------------------------

	masterTimer_.start();
	elaspeTime = 0.0;

        //masterBalancePeriod_ = 0.50;  // Test frequency
        //std::cout << ", masterBalancePeriod_ = " << masterBalancePeriod_
        //        << std::endl;
        
	while (elaspeTime < masterBalancePeriod_) {
	    MPI_Test(&request, &flag, &status);
	    if (flag) { // Receive a msg
		processMessages(largeBuffer_, status, request);
	    }
	    elaspeTime = masterTimer_.getTime();
	    //std::cout << "***** elaspeTime = " << elaspeTime
	    //      << ", masterBalancePeriod_ = " << masterBalancePeriod_
	    //      << std::endl;
	}
	
	//**------------------------------------------------
	// Check if all workers in this cluster have reported their status.
	//**------------------------------------------------
    
	if (!allWorkerReported) {	
	    workerReported_[masterRank_] = true;
	    allWorkerReported = true;
	    for (i = 0; i < clusterSize_; ++i) {
		if (workerReported_[i] == false) {
		    allWorkerReported = false;
		    break;
		}
	    }
	}

	//**------------------------------------------------
	// Check whether all hubs have reported their status.
	//**------------------------------------------------
    
	if (!allHubReported_) {
	    allHubReported_ = true;
	    // NOTE: The position of this hub is 0.
	    hubReported_[0] = true;
	    for (i = 0; i < hubNum_; ++i) {
		if ( hubReported_[i] != true ) {
		    allHubReported_ =  false;
		    break;
		}
	    }
	}

	//**------------------------------------------------
	// Master add the status of its cluster to system status.
	//**------------------------------------------------

	refreshSysStatus();

	//**------------------------------------------------
	// Check if force terminate.
	// FIXME: clean up msg and memory before leaving.
	//**------------------------------------------------

	// NOTE: Use wall clock time for parallel.
        if (!forceTerminate_) {
            if (allWorkerReported && allHubReported_ && 
                timer_.reachWallLimit()) {

                forceTerminate_ = true;
                setExitStatus(AlpsExitStatusTimeLimit);
                masterForceHubTerm();
                hubForceWorkerTerm();
		if (msgLevel_ > 0) {
		    if (incumbentValue_ < ALPS_INFINITY) {
			messageHandler()->message(ALPS_LOADREPORT_MASTER_F, messages())
			    << systemNodeProcessed_ 
			    << systemWorkQuantity_
			    << systemWorkQuantityForce_ 
			    << systemSendCount_ << systemRecvCount_ 
                            << masterCheckCount
			    << incumbentValue_ 
			    << timer_.getWallClock()
			    << CoinMessageEol; 
		    }
		    else {
			messageHandler()->message(ALPS_LOADREPORT_MASTER_F_N, messages())
			    << systemNodeProcessed_ 
			    << systemWorkQuantity_
			    << systemWorkQuantityForce_ 
			    << systemSendCount_ << systemRecvCount_ 
                            << masterCheckCount
			    << timer_.getWallClock()
			    << CoinMessageEol; 
		    }
		    
		  messageHandler()->message(ALPS_TERM_FORCE_TIME, messages())
		    << timer_.limit_ << CoinMessageEol;
		}
                systemWorkQuantityForce_ = systemWorkQuantity_;
            }
            else if (allWorkerReported && allHubReported_ && 
                     (systemNodeProcessed_ >= nodeLimit)) {
                forceTerminate_ = true;
                setExitStatus(AlpsExitStatusNodeLimit);
                masterForceHubTerm();
                hubForceWorkerTerm();
		if (msgLevel_ > 0) {
		    if (incumbentValue_ < ALPS_INFINITY) {
			messageHandler()->message(ALPS_LOADREPORT_MASTER_F, messages())
			    << systemNodeProcessed_ 
			    << systemWorkQuantity_ 
			    << systemWorkQuantityForce_
			    << systemSendCount_ << systemRecvCount_ 
                            << masterCheckCount
			    << incumbentValue_
			    << timer_.getWallClock()
			    << CoinMessageEol; 
		    }
		    else {
			messageHandler()->message(ALPS_LOADREPORT_MASTER_F_N, messages())
			    << systemNodeProcessed_ 
			    << systemWorkQuantity_ 
			    << systemWorkQuantityForce_
			    << systemSendCount_ << systemRecvCount_ 
                            << masterCheckCount
			    << timer_.getWallClock()
			    << CoinMessageEol; 
		    }
		   messageHandler()->message(ALPS_TERM_FORCE_NODE, messages())
		     << nodeLimit << CoinMessageEol;
		}
                systemWorkQuantityForce_ = systemWorkQuantity_;
            }
        }
	
	//**------------------------------------------------
        // Print workload information to screen.
	//**------------------------------------------------

	reportInterval = 
            model_->AlpsPar()->entry(AlpsParams::masterReportInterval);

	assert(reportInterval > 0);

	if (reportIntervalCount % reportInterval == 0) {
            if (msgLevel_ > 0) {
                if (forceTerminate_) {
		    //d::cout << "******systemWorkQuantityForce_ = " << systemWorkQuantityForce_<< std::endl;
		    if (incumbentValue_ < ALPS_INFINITY) {
			messageHandler()->message(ALPS_LOADREPORT_MASTER_F, messages())
			    << systemNodeProcessed_ 
			    << systemWorkQuantity_ 
			    << systemWorkQuantityForce_
			    << systemSendCount_ << systemRecvCount_ 
                            << masterCheckCount
			    << incumbentValue_ 
			    << timer_.getWallClock()
			    << CoinMessageEol;   
		    }
		    else {
			messageHandler()->message(ALPS_LOADREPORT_MASTER_F_N, messages())
			    << systemNodeProcessed_ 
			    << systemWorkQuantity_ 
			    << systemWorkQuantityForce_
			    << systemSendCount_ << systemRecvCount_ 
                            << masterCheckCount
			    << timer_.getWallClock()
			    << CoinMessageEol;  
		    }
                }
                else {
		    if (incumbentValue_ < ALPS_INFINITY) {
			messageHandler()->message(ALPS_LOADREPORT_MASTER, messages())
			    << systemNodeProcessed_ 
			    << systemWorkQuantity_ 
			    << systemSendCount_ << systemRecvCount_ 
                            << masterCheckCount << masterBalancePeriod_ 
                            << nodeProcessingTime_ << unitWorkNodes_
			    << incumbentValue_ 
			    << timer_.getWallClock()
			    << CoinMessageEol;
		    }
		    else {
			messageHandler()->message(ALPS_LOADREPORT_MASTER_N, messages())
			    << systemNodeProcessed_ 
			    << systemWorkQuantity_ 
			    << systemSendCount_ << systemRecvCount_
                            << masterCheckCount << masterBalancePeriod_ 
                            << nodeProcessingTime_ << unitWorkNodes_
			    << timer_.getWallClock()
			    << CoinMessageEol;
		    }
		}
            }
            
#if 0
	    std::cout << "Master: ";
	    for (i = 0; i < hubNum_; ++i) {
		std::cout << "hub[" << i << "]=" <<
		    hubWorkQuantities_[i] << ", ";
	    }
	    for (i = 0; i < clusterSize_; ++i) {
		std::cout << "w[" << i << "]=" <<
		    workerWorkQuantities_[i] << ", ";
	    }
	    std::cout << std::endl;
#endif
	}
	++reportIntervalCount;
        
	//**------------------------------------------------
	// If terminate check can be activated.
	//**------------------------------------------------
        
	if ( allWorkerReported && 
	     allHubReported_ && 
	     (systemWorkQuantity_ < zeroLoad ) && 
	     (systemSendCount_ == systemRecvCount_) ) {

	    preSysSendCount = systemSendCount_;
	    blockTermCheck_ = false;   // Activate termination check
	}

#if 0
	std::cout << "blockTermCheck_=" << blockTermCheck_
		  << ", allWorkerReported=" << allWorkerReported
		  << ", allHubReported_=" << allHubReported_
		  << ", systemWorkQuantity_=" << systemWorkQuantity_
		  << ", preSysSendCount=" << preSysSendCount
		  << ", systemSendCount_=" << systemSendCount_
		  << ", systemRecvCount_=" << systemRecvCount_
		  << std::endl;
#endif
	
	//**------------------------------------------------
	// Do termination checking if activated.
	// Note: All msgs during termination checking are not counted.
	//**------------------------------------------------

	if ( ! blockTermCheck_ ) {
            if (msgLevel_ > 0) {
                messageHandler()->message(ALPS_TERM_MASTER_START, messages())
                  << globalRank_ << CoinMessageEol;
            }
	    // Ask other hubs to do termination check.
	    for (i = 0; i < hubNum_; ++i) {
		if (hubRanks_[i] != globalRank_) {
		    MPI_Send(smallBuffer_, 0, MPI_PACKED, hubRanks_[i], 
			     AlpsMsgAskHubPause, MPI_COMM_WORLD);
#ifdef NF_DEBUG
		    std::cout << "Master["<< masterRank_ << "] ask hub["
			      << hubRanks_[i] << "] to do termination check."
			      << std::endl;
#endif
		}
	    }

	    // Ask master's workers to do termination check.
	    for (i = 0; i < clusterSize_; ++i) {
		//if (i != clusterRank_) {
		if (i != globalRank_) {
#ifdef NF_DEBUG
		    std::cout << "Master["<< masterRank_ << "] ask its worker["
			      << i << "] to do termination check."<< std::endl;
#endif
		    MPI_Send(smallBuffer_, 0, MPI_PACKED, i, 
			     AlpsMsgAskPause, MPI_COMM_WORLD);
		}
	    }
	    

	    MPI_Request termReq;
	    MPI_Status termSta;
	    
	    // Update the cluster status to which the Master belongs
	    for(i = 1; i < clusterSize_; ++i) {
		MPI_Irecv(smallBuffer_, smallSize, MPI_PACKED, MPI_ANY_SOURCE,
			  AlpsMsgWorkerTermStatus, clusterComm_, &termReq);
		MPI_Wait(&termReq, &termSta);
		hubUpdateCluStatus(smallBuffer_, &termSta, clusterComm_);
	    }

#ifdef NF_DEBUG
	    std::cout << "Master: TERM: finished updating its cluster." 
                      << std::endl;
#endif
	    // clusterWorkQuality_ += workQuality_;
	    clusterWorkQuantity_ += workQuantity_;// workQuantity = 0
	    clusterSendCount_ += sendCount_;
	    clusterRecvCount_ += recvCount_;
	    sendCount_ = recvCount_ = 0;

	    // Update system status
	    for(i = 1; i < hubNum_; ++i) {
		MPI_Irecv(smallBuffer_, smallSize, MPI_PACKED, MPI_ANY_SOURCE,
			  AlpsMsgHubTermStatus, hubComm_, &termReq);
		MPI_Wait(&termReq, &termSta);
		masterUpdateSysStatus(smallBuffer_, &termSta, hubComm_);
	    }
	    
#ifdef NF_DEBUG
	    std::cout << "Master: TERM: finished updating hubs." <<std::endl;
#endif
	    systemWorkQuantity_ += clusterWorkQuantity_;
	    systemSendCount_ += clusterSendCount_;
	    systemRecvCount_ += clusterRecvCount_;
	    clusterSendCount_ = clusterRecvCount_ = 0;

#ifdef NF_DEBUG
	    std::cout << "Master: TERM: Quantity_ = " << systemWorkQuantity_
		      << ", systemSendCount_ = " << systemSendCount_
		      << ", systemRecvCount_ = " << systemRecvCount_ 
		      << "; preSysSendCount = " << preSysSendCount
		      << std::endl;
#endif
            
	    if ( (systemWorkQuantity_ < zeroLoad) && 
		 (preSysSendCount == systemSendCount_) ) {
		terminate = true;
	    }
	    else {
		terminate = false;
                // Do forget to deduct! refeshSysStatus will add clusterWorkQuantitY_
                systemWorkQuantity_ -= clusterWorkQuantity_;
	    }
	    
#ifdef NF_DEBUG
	    std::cout << "Master: TERM: terminate=" << terminate <<std::endl;
#endif
	    // True idle, tell others to terminate.
	    if (terminate) {
                if (msgLevel_ > 0) {
                    messageHandler()->message(ALPS_TERM_MASTER_INFORM,messages())
                        << globalRank_ << "stop searching" << CoinMessageEol;
                }
		
		// Send instruction to my hubs (as the master)
		for (i = 0; i < hubNum_; ++i) {
		    if (hubRanks_[i] != globalRank_) {
                        // i is not Master cluster. 
			reply = 'T';
			MPI_Send(&reply, 1, MPI_CHAR, i, AlpsMsgContOrTerm, 
                                 hubComm_);
		    }	
		}

		// Send instruction to my works (as a hub)
		for (i = 0; i < clusterSize_; ++i) {
		    if (i != globalRank_) {
			reply = 'T';
#ifdef NF_DEBUG
			std::cout << "Master["<< masterRank_ 
                                  << "] ask its worker["
				  << i << "] to terminate."
				  << " clusterRank_=" <<  clusterRank_
				  << std::endl;
#endif
			MPI_Send(&reply, 1, MPI_CHAR, i, AlpsMsgContOrTerm,
				 clusterComm_);
		    }
		}
	    } 
	    else {  // Not true idle yet
                if (msgLevel_ > 0) {
                    messageHandler()->message(ALPS_TERM_MASTER_INFORM, messages())
                        << globalRank_ << "continue searching" << CoinMessageEol;
                }
                
                blockTermCheck_ = true;

		// Send instruction to the hubs (as the master)
		for (i = 0; i < hubNum_; ++i) {
		    if (hubRanks_[i] != globalRank_) { // i is not Master 
			reply = 'C';
			MPI_Send(&reply, 1, MPI_CHAR, i,AlpsMsgContOrTerm,
                                 hubComm_);
		    }
		}
		// Send instruction to the works (as a hub)
		for (i = 1; i < clusterSize_; ++i) {
		    reply = 'C';
		    MPI_Send(&reply, 1, MPI_CHAR, i, AlpsMsgContOrTerm,
			     clusterComm_);
		}
	    }

	    if (msgLevel_ > 0) {
                if (forceTerminate_) {
                    if (incumbentValue_ < ALPS_INFINITY) {
                        messageHandler()->message(ALPS_LOADREPORT_MASTER_F, messages())
                            << systemNodeProcessed_ 
                            << systemWorkQuantity_ 
                            << systemWorkQuantityForce_ 
                            << systemSendCount_ << systemRecvCount_
                            << masterCheckCount 
                            << incumbentValue_
                            << timer_.getWallClock()
                            << CoinMessageEol;
                    }
                    else {
                        messageHandler()->message(ALPS_LOADREPORT_MASTER_F_N, messages())
                            << systemNodeProcessed_ 
                            << systemWorkQuantity_ 
                            << systemWorkQuantityForce_ 
                            << systemSendCount_ << systemRecvCount_ 
                            << masterCheckCount
                            << timer_.getWallClock()
                            << CoinMessageEol;
                    }
                }
                else {
                    if (incumbentValue_ < ALPS_INFINITY) {
                        messageHandler()->message(ALPS_LOADREPORT_MASTER, messages())
                            << systemNodeProcessed_ 
                            << systemWorkQuantity_ 
                            << systemSendCount_ << systemRecvCount_ 
                            << masterCheckCount << masterBalancePeriod_ 
                            << nodeProcessingTime_ << unitWorkNodes_
                            << incumbentValue_ 
                            << timer_.getWallClock()
                            << CoinMessageEol;
                    }
                    else {
                        messageHandler()->message(ALPS_LOADREPORT_MASTER_N, messages())
                            << systemNodeProcessed_ 
                            << systemWorkQuantity_ 
                            << systemSendCount_ << systemRecvCount_ 
                            << masterCheckCount << masterBalancePeriod_
                            << nodeProcessingTime_ << unitWorkNodes_ 
                            << timer_.getWallClock()
                            << CoinMessageEol;
                    }
                }
	    }

            if (terminate){
                // Break after printing msg
                break;  // Break *,  Master terminates
            }
	}   
      
	//**------------------------------------------------
	// Master balances work load of hubs if
	// (1) not terminate,
	// (2) not force terminate,
	// (3) all hubs reported, and
	// (4) previous balance has been completed.
	//**------------------------------------------------

        if (msgLevel_ > 10) {
            std::cout << "masterDoBalance_ = " << masterDoBalance_ 
                      << ", allHubReported_ = " << allHubReported_
                      << std::endl;
        }
        
	if ( !terminate && !forceTerminate_ && 
	     allHubReported_ && (masterDoBalance_ == 0) ) {
	    if (hubNum_ > 1 && interCB) {
		masterBalanceHubs();
		++masterCheckCount;
		if (masterCheckCount % 10 == 0) {
                    if (msgLevel_ > 10) {
                        messageHandler()->message(ALPS_LOADBAL_MASTER, messages())
                            << globalRank_ << masterCheckCount << CoinMessageEol;
                    }
		}
	    }
	}
        
        if (msgLevel_ > 10) {
            std::cout << "++++ hubDoBalance_ = " << hubDoBalance_
                      << ", allWorkerReported = " << allWorkerReported
                      << std::endl;
        }
        
	if ( !terminate && !forceTerminate_ && 
	     allWorkerReported && hubDoBalance_ == 0 ) {
	    if (clusterSize_ > 2 && intraCB) {
		hubBalanceWorkers();
		++hubCheckCount;
		if (hubCheckCount % 10 == 0) {
                    if (msgLevel_ > 10) {
                        messageHandler()->message(ALPS_LOADBAL_HUB, messages())
                            << globalRank_ << hubCheckCount << CoinMessageEol;
                    }
		}
	    }
	}
    }

    //------------------------------------------------------
    // Clean up before leaving.
    //------------------------------------------------------

    int cancelNum = 0;

    MPI_Cancel(&request);
    MPI_Wait(&request, &status);

    MPI_Test_cancelled(&status, &flag);
    if(flag) {  // Cancel succeeded
	++cancelNum;
    }
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::hubMain()
{
    int i;
    int hubCheckCount = 0;
    int errorCode = 0;
    
    char reply = 'C';

    double elaspeTime = 0.0;

    bool allWorkerReported = false;   // Workers report load at least once
    bool terminate = false;
    bool deletedSTs = false;
    bool comUnitWork = false;
    
    int numComUnitWork = 0;
    
    AlpsReturnStatus rCode = AlpsReturnStatusOk;
    AlpsExitStatus exitStatus = AlpsExitStatusInfeasible;
    MPI_Status status;    

    //------------------------------------------------------
    // Get parameters.
    //------------------------------------------------------

    const int staticBalanceScheme = 
	model_->AlpsPar()->entry(AlpsParams::staticBalanceScheme);
    const int smallSize = 
	model_->AlpsPar()->entry(AlpsParams::smallSize);
    const bool intraCB = 
	model_->AlpsPar()->entry(AlpsParams::intraClusterBalance);
    const double zeroLoad = 
	model_->AlpsPar()->entry(AlpsParams::zeroLoad);
    double unitTime =
	model_->AlpsPar()->entry(AlpsParams::unitWorkTime);
    if (unitTime <= 0.0) {
        // Not set by user
        unitTime  = ALPS_DBL_MAX;
    }

    double changeWorkThreshold = model_->AlpsPar()->
	entry(AlpsParams::changeWorkThreshold);

    hubReportPeriod_ = model_->AlpsPar()->entry(AlpsParams::hubReportPeriod);
    if (hubReportPeriod_ > 0.0) {
        userBalancePeriod_ = true;
    }
    else {
        hubReportPeriod_ = -hubReportPeriod_;
    }

    largeSize_ = model_->AlpsPar()->entry(AlpsParams::largeSize);

    unitWorkNodes_ = model_->AlpsPar()->entry(AlpsParams::unitWorkNodes);
    if (unitWorkNodes_ <= 0) {
        comUnitWork = true;
    }
    
    //------------------------------------------------------
    // Initialization and setup.
    //------------------------------------------------------

    hubTimer_.setClockType(AlpsClockTypeWallClock);

    workerNodeProcesseds_ = new int [clusterSize_];
    workerWorkQualities_ = new double [clusterSize_];
    workerWorkQuantities_ = new double [clusterSize_];
    workerReported_ = new bool [clusterSize_];

    for (i = 0; i < clusterSize_; ++i) {
	workerNodeProcesseds_[i] = 0;
	workerWorkQualities_[i] = ALPS_OBJ_MAX;
	workerWorkQuantities_[i] = 0.0;
    }
    
    workerNodeProcesseds_[0] = nodeProcessedNum_;
    workerWorkQualities_[0] = workQuality_;
    workerWorkQuantities_[0] = workQuantity_ = 0.0;

    //------------------------------------------------------
    // Recv tree node size and send it to my worker.
    // NOTE: master's rank is always 0 in hubComm_.
    //------------------------------------------------------
    
    MPI_Recv(&nodeMemSize_, 1, MPI_INT, 0, AlpsMsgNodeSize, hubComm_, &status);
    for (i = 0; i < clusterSize_; ++i) {
	if (i != clusterRank_) {
	    MPI_Send(&nodeMemSize_, 1, MPI_INT, i, AlpsMsgNodeSize, 
                     clusterComm_);
	}
    }

    // Adjust largeSize to avoid extreme cases.
    largeSize_ = CoinMax(largeSize_, nodeMemSize_ * 3);  

    largeBuffer_ = new char [largeSize_];
    largeBuffer2_ = new char [largeSize_];
    smallBuffer_ = new char [smallSize];

    MPI_Barrier(MPI_COMM_WORLD); // Sync before rampup    
    
    //======================================================
    // Hub's Ramp-up.
    //======================================================

    // Start to measure rampup
    hubTimer_.start();

    setPhase(AlpsPhaseRampup);
    switch (staticBalanceScheme) {
    case AlpsRootInit:
	rootInitHub();
	break;
    case AlpsSpiral:
	spiralHub();
	break;
    default:
	throw CoinError("Unknown static balance scheme", "hubMain", 
			"AlpsKnowledgeBrokerMPI");
    }

    rampUpTime_ = hubTimer_.getTime();

    //======================================================
    // Hub's search
    //======================================================

    setPhase(AlpsPhaseSearch);
    
    //------------------------------------------------------
    // HUB SCHEDULER:
    // (1) Listen and process messages periodically.
    // (2) If required, do one unit of work.
    // (3) Send work quality, quantity, and msg counts to Master.
    // (4) Balance workload quality of my workers.
    // (5) Do termination check if requred.
    //------------------------------------------------------

    hubReportPeriod_ = computeBalancePeriod(userBalancePeriod_,
                                            hubReportPeriod_, 
                                            nodeProcessingTime_);

    if (hubMsgLevel_ > 5 && intraCB) {
	messageHandler()->message(ALPS_LOADBAL_HUB_PERIOD, messages())
	    << globalRank_ << hubReportPeriod_ << CoinMessageEol;
    }
    
    MPI_Request request;
    int flag = 1;

    MPI_Irecv(largeBuffer_, largeSize_, MPI_PACKED, MPI_ANY_SOURCE, 
	      MPI_ANY_TAG, MPI_COMM_WORLD, &request);

    while (true) {

        flag = 1;

	//**------------------------------------------------
	// Listen and process msg for a period. 
	//**------------------------------------------------

	hubTimer_.start();	
	elaspeTime = 0.0;
	while (elaspeTime < hubReportPeriod_ ) {
	    MPI_Test(&request, &flag, &status);
	    if (flag) { // Received a msg
	      processMessages(largeBuffer_, status, request);
	    }
	    elaspeTime = hubTimer_.getTime();
	}

	//std::cout << "++++++ hubReportPeriod_ = "
	//    << hubReportPeriod_ << std::endl;
    
        
	//**------------------------------------------------
        // if forceTerminate_ == true;
	//**------------------------------------------------
        
	if (forceTerminate_) {
            // Delete all subtrees if hub work.
            if (hubWork_) {   
                deleteSubTrees();
                updateWorkloadInfo();
                assert(!(subTreePool_->hasKnowledge()));
            }
            if (!deletedSTs) {
                hubForceWorkerTerm();
            }

            deletedSTs = true;
        }
        
	//**------------------------------------------------
	// Check if all my workers have reported.
	//**------------------------------------------------

	if (!allWorkerReported) {
	    workerWorkQualities_[masterRank_] = workQuality_;
	    workerWorkQuantities_[masterRank_] = workQuantity_;
	    workerReported_[masterRank_] = true;
	    allWorkerReported = true;
	    for (i = 0; i < clusterSize_; ++i) {
		if (workerReported_[i] == false) {
		    allWorkerReported = false;
		    break;

		}
	    }
	}

	//**------------------------------------------------
	// If hub work, do one unit of work.
	//**------------------------------------------------

	if ( !haltSearch_ && hubWork_ &&
             (workingSubTree_ != 0 || hasKnowledge(AlpsKnowledgeTypeSubTree))){

	    //reportCount = hubReportFreqency;
            
	    // NOTE: will stop when there is a better solution.
	    bool betterSolution = true;
            int thisNumNodes = 0;
            
            // Compute unit work based on node processing time
            if (comUnitWork) {
                unitWorkNodes_ = computeUnitNodes(unitWorkNodes_, nodeProcessingTime_);
                if (++numComUnitWork > 200) {
                    comUnitWork = false;
                }
            }
            try {
                rCode = doOneUnitWork(unitWorkNodes_, 
                                      unitTime, 
                                      exitStatus,
                                      thisNumNodes,
                                      treeDepth_, 
                                      betterSolution);
                nodeProcessedNum_ += thisNumNodes;
            }
            catch (std::bad_alloc&) {
                errorCode = 1;
            }
            catch(CoinError& er) {
                errorCode = 2;
            }
            catch(...) {
                errorCode = 3;          
            }

            if (errorCode) {
                // Do we want to free some memory?
                if (hubWork_) {
                    deleteSubTrees();
                    updateWorkloadInfo();
                    assert(!(subTreePool_->hasKnowledge()));   
                }
                haltSearch_ = true;
                sendErrorCodeToMaster(errorCode);
            }

	    // Update work load quantity and quality info.
	    updateWorkloadInfo();
	    
	    // Adjust workingSubTree_ if it 'much' worse than the best one.
            changeWorkingSubTree(changeWorkThreshold);

#if 0
	    std::cout << "******** HUB [" << globalRank_ << "]: "
		      << " quality = " << workQuality_
		      << "; quantity = " << workQuantity_ << std::endl;
#endif

	    if (betterSolution) {
		// Double check if better. If yes, update and sent it to Master
		double incVal = 
		   getBestKnowledge(AlpsKnowledgeTypeSolution).second;
		if(incVal < incumbentValue_) {  // Minimization
		    incumbentValue_ = incVal;
		    incumbentID_ = globalRank_;
                    sendKnowledge(AlpsKnowledgeTypeSolution, //useful
                                  globalRank_,
                                  0,
                                  smallBuffer_,
                                  0,
                                  MPI_ANY_TAG,
                                  MPI_COMM_WORLD,
                                  false);
		    //sendIncumbent();
                    if (hubMsgLevel_ > 0) {
                        messageHandler()->message(ALPS_SOLUTION_SEARCH, 
						  messages())
                            << globalRank_ << incVal << CoinMessageEol;
                    }
		} 
	    }
	}// EOF if hub work

	//**------------------------------------------------
	// Add into the hub's status to cluster's status. 
        // Need to check if report.
	//**------------------------------------------------
        
	refreshClusterStatus();
        
#if 0
	std::cout << "HUB "<< globalRank_ 
		  << ": clusterWorkQuality_  = " << clusterWorkQuality_
		  << ": clusterWorkQuantity_  = " << clusterWorkQuantity_
		  << ", sendCount_ = " << sendCount_
		  << ", recvCount_ = " << recvCount_
		  << ", allWorkerReported = " << allWorkerReported
		  << ", blockHubReport_ = " << blockHubReport_
		  << ", blockTermCheck_ = " << blockTermCheck_
		  << std::endl; 
#endif

	//**------------------------------------------------
	// If needed, report cluster status, and check if should 
        // block report.
	//**------------------------------------------------

	if ((clusterSendCount_ || clusterRecvCount_ || !blockHubReport_)) {
	    //&& reportCount == hubReportFreqency) {
            
	    //reportCount = 0;
	    incSendCount("hubMain()-hubReportStatus");
            
            if (hubWork_) {
                updateWorkloadInfo();
            }
	    refreshClusterStatus();  // IMPORTANT: report latest state
	    hubReportStatus(AlpsMsgHubPeriodReport, MPI_COMM_WORLD);
	    
	    if (clusterWorkQuantity_ < zeroLoad) {
		blockHubReport_ = true;
#ifdef NF_DEBUG_MORE
		std::cout << "HUB[" << globalRank_ << "]: blockHubReport"
			  << std::endl;
#endif
	    }
	    else {
		blockHubReport_ = false;
#ifdef NF_DEBUG_MORE
		std::cout << "HUB[" << globalRank_ << "]: unblockHubReport"
			  << std::endl;
#endif
	    }
	}

	//**------------------------------------------------
	// If master ask hub to do termination check.
	//**------------------------------------------------

	if (!blockTermCheck_) {

	    // Ask hub's workers to check termination
	    for (i = 0; i < clusterSize_; ++i) {
		if (i != clusterRank_) {
		    int workRank = globalRank_ - masterRank_ + i;
		    MPI_Send(smallBuffer_, 0, MPI_PACKED, workRank, 
			     AlpsMsgAskPause, MPI_COMM_WORLD);
#ifdef NF_DEBUG
		    std::cout << "HUB[" << globalRank_ << "]: ask its worker "
			      << workRank << " to do TERM check."
			      << ", clusterRank_=" << clusterRank_
			      << ", i=" << i << std::endl;
#endif
		}
	    }

	    // Recv workers' stati
	    MPI_Request termReq;
	    MPI_Status termSta;
	    for(i = 1; i < clusterSize_; ++i) {
		MPI_Irecv(smallBuffer_, smallSize, MPI_PACKED, MPI_ANY_SOURCE,
			  AlpsMsgWorkerTermStatus, clusterComm_, &termReq);
		MPI_Wait(&termReq, &termSta);
		hubUpdateCluStatus(smallBuffer_, &termSta, clusterComm_);
	    }
            
	    // No workQuantity_ and workQuality for the hub
	    //clusterWorkQuality_ += workQuality_;
	    //clusterWorkQuantity_ += workQuantity_;
	    clusterSendCount_ += sendCount_;
	    clusterRecvCount_ += recvCount_;
	    sendCount_ = recvCount_ = 0;

	    // Report my status to master
	    hubReportStatus(AlpsMsgHubTermStatus, hubComm_);
	    
#ifdef NF_DEBUG
	    std::cout << "HUB[" << globalRank_ << "]: reported  TERM status to "
		      << "master " << masterRank_ << std::endl;
#endif

	    // Get termination instruction from Master
	    // NOTE: master's rank is always 0 in hubComm_.
	    MPI_Irecv(&reply, 1, MPI_CHAR, 0, AlpsMsgContOrTerm, hubComm_,
                      &termReq);
	    MPI_Wait(&termReq, &termSta);
	    
#ifdef NF_DEBUG
	    std::cout << "HUB[" << globalRank_ << "]: received TERM instruction "
		      << reply << " from master " << masterRank_ << std::endl;
#endif


	    if(reply == 'T') {
                if (hubMsgLevel_ > 0) {
                    messageHandler()->message(ALPS_TERM_HUB_INFORM, messages())
                        << globalRank_<< "exit" << CoinMessageEol;
                }
                
		// Send instruction to my workers
		for (i = 0; i < clusterSize_; ++i) {
		    if (i != clusterRank_) {
			reply = 'T';
			MPI_Send(&reply, 1, MPI_CHAR, i, AlpsMsgContOrTerm,
				 clusterComm_);
#ifdef NF_DEBUG
			std::cout << "HUB[" << globalRank_ << "]: ask its worker "
				  << i << " to TERM." << std::endl;
#endif
		    }
		}
		terminate = true;
		break;    // Break * and terminate
	    }
	    else {
                if (hubMsgLevel_ > 0) {
                    messageHandler()->message(ALPS_TERM_HUB_INFORM, messages())
                        << globalRank_<< "continue" << CoinMessageEol;
                }
		for (i = 0; i < clusterSize_; ++i) {
		    if (i != clusterRank_) {
			reply = 'C';
			MPI_Send(&reply, 1, MPI_CHAR, i, AlpsMsgContOrTerm,
				 clusterComm_);
#ifdef NF_DEBUG
			std::cout << "HUB[" << globalRank_ << "]: ask its worker "
				  << i << " to continue." << std::endl;
#endif
		    }
		}
		terminate = false;
		blockTermCheck_ = true;
		blockHubReport_ = false;
	    }
	}

	//**------------------------------------------------
	// Hub balances the workload of its workers if
	// (1) not terminate,
	// (2) not force terminate,
	// (3) all workers have reported, and
	// (4) previous balance has been completed.
	//**------------------------------------------------

	if ( !terminate && !forceTerminate_ && 
	     allWorkerReported && hubDoBalance_ == 0 ) {
	    if (clusterSize_ > 2 && intraCB) {
		hubBalanceWorkers();
		++hubCheckCount;
		if (hubCheckCount % 10 == 0) {
                    if (hubMsgLevel_ > 0) {
                        messageHandler()->message(ALPS_LOADBAL_HUB, messages())
                            << globalRank_ << hubCheckCount << CoinMessageEol;
                    }
		}
	    }
	}
	
    } // EOF while


    //------------------------------------------------------
    // Clean up before leaving.
    //------------------------------------------------------

    int cancelNum = 0;
    MPI_Cancel(&request);
    MPI_Wait(&request, &status);
    MPI_Test_cancelled(&status, &flag);
    if(flag) {  // Cancel succeeded
	++cancelNum;
    }
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::workerMain()
{
    bool isIdle = false;
    int thisNumNodes = 0;
    int errorCode = 0;
    
    char reply = 'C';
    char* tempBuffer = 0;

    bool terminate = false;
    bool allMsgReceived = false;
    bool deletedSTs = false;
    bool comUnitWork = false;
    
    int numComUnitWork = 0;
    
    MPI_Status status;
    AlpsReturnStatus rCode = AlpsReturnStatusOk;
    AlpsExitStatus exitStatus = AlpsExitStatusInfeasible;

    AlpsTimer msgTimer;

    //------------------------------------------------------
    // Get parameters.
    //------------------------------------------------------
    
    const int staticBalanceScheme = 
	model_->AlpsPar()->entry(AlpsParams::staticBalanceScheme);
    const int nodeLogInterval =
	model_->AlpsPar()->entry(AlpsParams::nodeLogInterval);
    const int smallSize = 
	model_->AlpsPar()->entry(AlpsParams::smallSize);
    const int workerMsgLevel = 
        model_->AlpsPar()->entry(AlpsParams::workerMsgLevel);
    const double zeroLoad = 
	model_->AlpsPar()->entry(AlpsParams::zeroLoad); 
    double changeWorkThreshold = 
	model_->AlpsPar()->entry(AlpsParams::changeWorkThreshold);
    const double needWorkThreshold = 
	model_->AlpsPar()->entry(AlpsParams::needWorkThreshold);
    const bool intraCB = 
	model_->AlpsPar()->entry(AlpsParams::intraClusterBalance);
    double unitTime  =
	model_->AlpsPar()->entry(AlpsParams::unitWorkTime);
    if (unitTime <= 0.0) {
        unitTime  = ALPS_DBL_MAX;
    }

    unitWorkNodes_ = model_->AlpsPar()->entry(AlpsParams::unitWorkNodes);
    if (unitWorkNodes_ <= 0) {
        comUnitWork = true;
    }

    largeSize_ = model_->AlpsPar()->entry(AlpsParams::largeSize);

    workerTimer_.setClockType(AlpsClockTypeWallClock);
    msgTimer.setClockType(AlpsClockTypeWallClock);

    //------------------------------------------------------
    // Recv node memory size from hub.
    //------------------------------------------------------
    
    MPI_Recv(&nodeMemSize_, 1, MPI_INT, masterRank_, /* masterRank_ is 0 or 1*/
	     AlpsMsgNodeSize, clusterComm_, &status);

    // Adjust largeSize to avoid extreme cases.
    largeSize_ = CoinMax(largeSize_, nodeMemSize_ * 3);
    
    largeBuffer_ = new char [largeSize_];
    largeBuffer2_ = new char [largeSize_];
    smallBuffer_ = new char [smallSize];

    MPI_Barrier(MPI_COMM_WORLD); // Sync before rampup    

    //======================================================
    // Worker's Ramp-up.
    //======================================================

    workerTimer_.start();

    setPhase(AlpsPhaseRampup);
    switch (staticBalanceScheme) {
    case AlpsRootInit:
	rootInitWorker();
	break;
    case AlpsSpiral:
	spiralWorker();
	break;
    default:
	throw CoinError("Unknown static balance scheme", "workerMain", 
			"AlpsKnowledgeBrokerMPI");
    }

    rampUpTime_ = workerTimer_.getTime();

    //======================================================
    // Worker start to search.
    //======================================================

    if (workerMsgLevel > 0) {
        messageHandler()->message(ALPS_SEARCH_WORKER_START, messages())
            <<globalRank_ << CoinMessageEol;
    }

    setPhase(AlpsPhaseSearch);

    //------------------------------------------------------
    // WORKER SCHEDULER:
    // (1) Listen and process messages until no message is in msg queue.
    // (2) Do one unit of work.
    // (3) Report status or check termination.
    //------------------------------------------------------

    MPI_Request request;    
    int flag;

    MPI_Irecv(largeBuffer_, largeSize_, MPI_PACKED, MPI_ANY_SOURCE, 
	      MPI_ANY_TAG, MPI_COMM_WORLD, &request);
  
    while (true) {
	
	blockTermCheck_ = true;
	
	//**------------------------------------------------
	// Listen and process until no message.
	//**------------------------------------------------
	
        msgTimer.start();
	while (true) {
	    MPI_Test(&request, &flag, &status);
	    if (flag) { // Receive a msg
		allMsgReceived = false;
		processMessages(largeBuffer_, status, request);
	    } 
	    else {
		allMsgReceived = true; 
		break;
	    }
	}
        msgTime_ += msgTimer.getTime();

	//**------------------------------------------------
        // if forceTerminate_ == true;
	//**------------------------------------------------

        //if (forceTerminate_ && !deletedSTs) {
	// Don't know why error on limits
        if (forceTerminate_) {

            deletedSTs = true;

            // Remove nodes from node pools
            deleteSubTrees();
            assert(!(subTreePool_->hasKnowledge()));
            updateWorkloadInfo();
        }
	
	//**------------------------------------------------
	// If don't check termination, do one unit of work.
	//**------------------------------------------------
	
	if (blockTermCheck_) {
	    
	    bool betterSolution = false;

            // Check whether need ask for node index before doing work
            if (!haltSearch_) {
                if (getMaxNodeIndex() - getNextNodeIndex() < (unitWorkNodes_ + 5)) {
                    workerAskIndices();
                    haltSearch_ = true;
                }
            }
            
	    if ( !haltSearch_ && 
                 (workingSubTree_ || hasKnowledge(AlpsKnowledgeTypeSubTree))) {
		if(isIdle) {
		  // Since has work now, set isIdle to false
		  idleTime_ += workerTimer_.getTime();
		  isIdle = false;
		}
		
		// Need check better solution.
		betterSolution = true;

                // Compute unit work based on node processing time
                if (comUnitWork) {
                    unitWorkNodes_ = computeUnitNodes(unitWorkNodes_, 
                                                nodeProcessingTime_);
                    if (++numComUnitWork > 20) {
                        comUnitWork = false;
                    }
                }
                
                try {
                    rCode = doOneUnitWork(unitWorkNodes_, 
                                          unitTime,
                                          exitStatus,
                                          thisNumNodes,
                                          treeDepth_, 
                                          betterSolution);
                    nodeProcessedNum_ += thisNumNodes;
                }
                catch (std::bad_alloc&) {
                    errorCode = 1;   
                }
                catch(CoinError& er) {
                    errorCode = 2;   
                }
                catch(...) {
                    errorCode = 3;   
                }
                if (errorCode) {
                    haltSearch_ = true;
                    deleteSubTrees();
                    assert(!(subTreePool_->hasKnowledge()));
                    updateWorkloadInfo();
                    sendErrorCodeToMaster(errorCode);   
                }                
		
		// Adjust workingSubTree_ if it 'much' worse than the best one
                changeWorkingSubTree(changeWorkThreshold);
		
		// Print tree size 
		if ( (msgLevel_ == 1) && 
		     (nodeProcessedNum_ % nodeLogInterval == 0) ) {
                    if (workerMsgLevel > 0) {
                        messageHandler()->message(ALPS_NODE_COUNT, messages())
                            << globalRank_<<nodeProcessedNum_
			    <<updateNumNodesLeft()
                            << CoinMessageEol;
                    }
		}
		
		// Share generated knowledge.
		sendModelKnowledge(MPI_COMM_WORLD, // Comm
                                   globalRank_);   // Receiver(no use)
	    } 
	    else {  
                // Worker is idle.
		if (!isIdle) {
		    workerTimer_.start();
		    isIdle = true;
		}
	    }
            
	    // If has better solution, check whether need to send it
	    if (betterSolution) {
		// Double check if better. If yes, update and sent it to Master
		double incVal = getBestKnowledge(AlpsKnowledgeTypeSolution).second;
		if(incVal < incumbentValue_) {  // Minimization
		    incumbentValue_ = incVal;
		    incumbentID_ = globalRank_;
#ifdef NF_DEBUG
		    std::cout << "\nWORKDER[" << globalRank_ <<
			"]: send a better solution. Quality = "
			      << incumbentValue_ << std::endl;
#endif 
                    sendKnowledge(AlpsKnowledgeTypeSolution, //useful
                                  globalRank_,
                                  0,
                                  smallBuffer_,
                                  0,
                                  MPI_ANY_TAG,
                                  MPI_COMM_WORLD,
                                  false);
                   //sendIncumbent();
                    if (workerMsgLevel > 0) {
                        messageHandler()->message(ALPS_SOLUTION_SEARCH, 
						  messages())
                            << globalRank_ << incVal << CoinMessageEol;
                    }
		} 
	    }

	    // Report its status to hub periodically if msg counts are not 
	    // zero or not blocked. If no load, worker will block report 

            updateWorkloadInfo();

	    if (sendCount_ || recvCount_ || !blockWorkerReport_) {
		incSendCount("workerMain() - workerReportStatus");
		workerReportStatus(AlpsMsgWorkerStatus, MPI_COMM_WORLD);
                
		if (workQuantity_ < zeroLoad) {
		    blockWorkerReport_ = true;
		}
		if ( intraCB && !forceTerminate_ &&
		     (workQuantity_ < needWorkThreshold) && 
		     (blockAskForWork_ == false) ) {

                    ++(psStats_.workerAsk_);
                    
		    MPI_Send(tempBuffer, 0, MPI_PACKED, myHubRank_, 
			     AlpsMsgWorkerNeedWork, MPI_COMM_WORLD);
		    incSendCount("workerAskForWork");
		    blockAskForWork_ = true;
		}
                
#if 0
		std::cout << "WORKER: " << globalRank_ 
			  << " after updateWorkloadInfo() = "
		    	  << workQuantity_ << std::endl;
#endif
	    }
	} 
	else { /* Do termination check. */
#if 0
	    std::cout << "WORKER[" << globalRank_ << "] check termination."
		      << std::endl;
#endif
	    // Record rampdown
	    rampDownTime_ = workerTimer_.getTime();
	    
	    // Report my latest status
	    updateWorkloadInfo();
	    workerReportStatus(AlpsMsgWorkerTermStatus, clusterComm_);
	    
#ifdef NF_DEBUG
	    std::cout << "WORKER[" << globalRank_ << "] reported TEMR status."
		      << std::endl;
#endif
	    // Get instruction from my hub
	    MPI_Request termReq;
	    MPI_Status termSta;
	    MPI_Irecv(&reply, 1, MPI_CHAR, masterRank_, AlpsMsgContOrTerm, 
		      clusterComm_, &termReq);
	    MPI_Wait(&termReq, &termSta);
	    
	    if(reply == 'T') {
		terminate = true;
                if (workerMsgLevel > 0) {
                    messageHandler()->message(ALPS_TERM_WORKER_INFORM, messages())
                        << globalRank_ << "exit" << CoinMessageEol;	
                }
		break;  // Break * and terminate
	    }
	    else {
                if (workerMsgLevel > 0) {
                    messageHandler()->message(ALPS_TERM_WORKER_INFORM, messages())
                        << globalRank_ << "continue" << CoinMessageEol;
                }
                blockWorkerReport_ = false;
		blockTermCheck_ = true;
		terminate = false;
	    }
	}
    }

    //------------------------------------------------------
    // How many node left? 
    //------------------------------------------------------

    updateNumNodesLeft();
    
    //------------------------------------------------------
    // Cancel MPI_Irecv before leaving.
    //------------------------------------------------------

    int cancelNum = 0;
    MPI_Cancel(&request);
    MPI_Wait(&request, &status);
    MPI_Test_cancelled(&status, &flag);
    if(flag) {  // Cancel succeeded
	++cancelNum;
    }
}

//#############################################################################

// Process messages. 
void 
AlpsKnowledgeBrokerMPI::processMessages(char *&bufLarge, 
                                        MPI_Status &status, 
                                        MPI_Request &request)
{
    int count;
    bool success = false;
    
    incRecvCount("Processing msg");
    switch (status.MPI_TAG) {
	
	//--------------------------------------------------
	// Following are master's msgs.
	//--------------------------------------------------
    case AlpsMsgErrorCode:
        recvErrorCode(bufLarge);
        masterForceHubTerm();
        hubForceWorkerTerm();
        break;
    case AlpsMsgHubFailFindDonor:
        --masterDoBalance_;
        break;
    case AlpsMsgHubPeriodReport:
	masterUpdateSysStatus(bufLarge, &status, MPI_COMM_WORLD);
	break;
    case AlpsMsgTellMasterRecv:
	--masterDoBalance_;
	break;
    case AlpsMsgWorkerAskIndices:
	masterSendIndices(bufLarge);
	break;
       
	//-------------------------------------------------
	// Following are hub's msgs.
	//-------------------------------------------------

    case AlpsMsgWorkerNeedWork:
	hubSatisfyWorkerRequest(bufLarge, &status);
	break;
    case AlpsMsgAskHubShare:
	hubsShareWork(bufLarge, &status);
	break;
    case AlpsMsgAskHubPause: 
        // Do not count messages during terminate checking 
	decRecvCount("hub periodical listening: AlpsMsgAskPause"); 
	blockTermCheck_ = false;
	break;
    case AlpsMsgTellHubRecv:
	--hubDoBalance_;
	break;
    case AlpsMsgWorkerStatus:
	hubUpdateCluStatus(bufLarge, &status, MPI_COMM_WORLD);
	blockHubReport_ = false;
	break;
	
	//--------------------------------------------------
	// Following are worker's msg.
	//-------------------------------------------------

    case AlpsMsgAskDonate:
	donateWork(bufLarge, AlpsMsgSubTree, &status);
	incSendCount("worker listening - AlpsMsgAskDonate");
	break;
    case AlpsMsgAskDonateToHub:
	donateWork(bufLarge, AlpsMsgSubTreeByMaster, &status);
	incSendCount("worker listening - AlpsMsgAskDonateToHub");
	break;
    case AlpsMsgAskDonateToWorker:
	donateWork(bufLarge, AlpsMsgSubTreeByWorker, &status);
	incSendCount("worker listening - AlpsMsgAskDonateToWorker");
	break;
    case AlpsMsgAskPause:
        // Do not count messages during terminate checking 
	decRecvCount("worker listening - AlpsMsgAskPause");
	blockTermCheck_ = false;
	break;
    case AlpsMsgIndicesFromMaster:
        workerRecvIndices(bufLarge);
        break;
    case AlpsMsgSubTree:
	receiveSubTree(bufLarge, status.MPI_SOURCE, &status);
	tellHubRecv();
	MPI_Get_count(&status, MPI_PACKED, &count);
	if (count > 0){
	    blockWorkerReport_ = false;
	    blockAskForWork_  = false;
	}
	break;
    case AlpsMsgSubTreeByWorker:
	receiveSubTree(bufLarge, status.MPI_SOURCE, &status);
	MPI_Get_count(&status, MPI_PACKED, &count);
	if (count > 0){
	    blockWorkerReport_ = false;
	    blockAskForWork_  = false;
	}
	break;

	//--------------------------------------------------
	// Following are common msgs.
	//--------------------------------------------------

    case AlpsMsgModelGenSearch:
        receiveModelKnowledge(MPI_COMM_WORLD);
        forwardModelKnowledge();
        break;
    case AlpsMsgIncumbentTwo:
	success = unpackSetIncumbent(bufLarge, &status);
	if (success) {
	    sendIncumbent();
	    //updateIncumbent_ = false;
	}
	break;
    case AlpsMsgSubTreeByMaster:
	if (globalRank_ == masterRank_) {
	    hubAllocateDonation(bufLarge, &status);
	    MPI_Get_count(&status, MPI_PACKED, &count);
	    --masterDoBalance_;
	}
	else if (globalRank_ == myHubRank_) {
	    hubAllocateDonation(bufLarge, &status);
	    tellMasterRecv();
	    MPI_Get_count(&status, MPI_PACKED, &count);
	    if (count > 0) {
		blockHubReport_ = false;
	    }
	}
	else {
	    receiveSubTree(bufLarge, status.MPI_SOURCE, &status);
	    MPI_Get_count(&status, MPI_PACKED, &count);
	    if (count > 0) blockWorkerReport_ = false;
	    break;
	}
	
	break;
    case AlpsMsgForceTerm:
        forceTerminate_ = true;
        break;
    default:
	std::cout << "PROC " << globalRank_ << " : processMessages"
		  << " : received UNKNOWN message. tag = " 
		  << status.MPI_TAG <<  std::endl; 
	throw CoinError("Unknown message type", "workermain", 
			"AlpsKnowledgeBrokerMPI");
    }

    MPI_Irecv(bufLarge, largeSize_, MPI_PACKED, MPI_ANY_SOURCE, 
	      MPI_ANY_TAG, MPI_COMM_WORLD, &request);

}

//#############################################################################

// Master ask the donor hub to send work to the receiver hub
void 
AlpsKnowledgeBrokerMPI::masterAskHubDonate(int donorID, 
					   int receiverID, 
					   double receiverWorkload)
{
    int smallSize = 
	model_->AlpsPar()->entry(AlpsParams::smallSize);
    int pos       = 0;

    MPI_Pack(&receiverID, 1, MPI_INT, smallBuffer_, smallSize, &pos, MPI_COMM_WORLD);
    MPI_Pack(&receiverWorkload, 1, MPI_DOUBLE, smallBuffer_, smallSize, &pos,
             MPI_COMM_WORLD);

#if 0
    std::cout << "masterAskHubDonate(): donor is " << donorID << std::endl;
#endif

    MPI_Send(smallBuffer_,pos, MPI_PACKED, donorID, AlpsMsgAskHubShare, MPI_COMM_WORLD);
    incSendCount("masterAskHubDonate()");

}

//#############################################################################

// Master ask the donor hub to send work to the receiver hub
void 
AlpsKnowledgeBrokerMPI::hubAskWorkerDonate(int donorID, 
					   int receiverID, 
					   double receiverWorkload)
{
    int smallSize = model_->AlpsPar()->entry(AlpsParams::smallSize);
    int pos = 0;
    
    MPI_Pack(&receiverID, 1, MPI_INT, smallBuffer_, smallSize, &pos, 
             MPI_COMM_WORLD);
    MPI_Pack(&receiverWorkload, 1, MPI_DOUBLE, smallBuffer_, smallSize, &pos,
             MPI_COMM_WORLD);
    
#ifdef NF_DEBUG_MORE
    std::cout << "hubAskHubDonate() send to " << donorID << std::endl;
#endif

    MPI_Send(smallBuffer_, pos, MPI_PACKED, donorID, AlpsMsgAskDonate, 
             MPI_COMM_WORLD);
    incSendCount("hubAskWorkerDonate()");
}

//#############################################################################

// Calculate the quality and quantity of workload on this process
void
AlpsKnowledgeBrokerMPI::updateWorkloadInfo()
{
    int count;
    
    workQuality_ = ALPS_OBJ_MAX;  // Worst ever possible
    workQuantity_ = 0.0;          // No work
    
    if ( (workingSubTree_ == NULL) && 
         (subTreePool_->hasKnowledge() == false) ) {
	return;
    }
    
    std::vector<AlpsSubTree* > subTreeVec = 
	subTreePool_->getSubTreeList().getContainer();
    std::vector<AlpsSubTree* >::iterator pos1, pos2;
    pos1 = subTreeVec.begin();
    pos2 = subTreeVec.end();

    workQuality_ = subTreePool_->getBestQuality();

    //    if (pos1 != pos2) {
    //        workQuality_ = (*pos1)->getQuality();//Best in pool
    //    }

    for (; pos1 != pos2; ++pos1) {
	if ((*pos1)->getNumNodes() > 5) {
	    //count = 5;
            count = (*pos1)->getNumNodes();
	}
	else {
	    count = (*pos1)->getNumNodes();
	}
	workQuantity_ += count;
    }

    if (workingSubTree_ != 0) {
	workingSubTree_->calculateQuality();
	if (workQuality_ > workingSubTree_->getQuality()) {
	    workQuality_ = workingSubTree_->getQuality();
	}
	if (workingSubTree_->getNumNodes() > 5) {
	    //count = 5;
	    count = workingSubTree_->getNumNodes();
	}
	else {
	    count = workingSubTree_->getNumNodes();
	}
	workQuantity_ += count;
    }

#ifdef NF_DEBUG_MORE
    std::cout << "PROC[" << globalRank_  << "] incumbentValue_ = " 
	      << incumbentValue_ << "; workQuality_ = " 
	      << workQuality_<< "; workQuantity_ = "
	      << workQuantity_ << std::endl;
#endif
}

//#############################################################################

// A worker donates a subtree to another worker(whose info is in bufLarge)
void 
AlpsKnowledgeBrokerMPI::donateWork(char*& anyBuffer,
				   int tag,
				   MPI_Status* status,  
				   int recvID,
				   double recvWL)
{
    int size       = 
	model_->AlpsPar()->entry(AlpsParams::smallSize);
    int pos        = 0;
    int receiverID = 0;
    int treeSize;

    char* dummyBuf = 0;
    double receiverWorkload = 0.0;
    bool sentSuccessful = false;
    AlpsSubTree* aSubTree = 0;    

#ifdef NF_DEBUG_MORE
    updateWorkloadInfo();
    messageHandler()->message(ALPS_DONATE_BEFORE, messages()) 
	<< globalRank_ << workQuantity_ << subTreePool_->getNumKnowledges() 
	<< CoinMessageEol;
#endif

    //------------------------------------------------------
    // Find out to which process I should donate.
    //------------------------------------------------------

    if (recvID == -1) {
	MPI_Unpack(anyBuffer, size, &pos, &receiverID, 1, MPI_INT, MPI_COMM_WORLD);
	MPI_Unpack(anyBuffer, size, &pos, &receiverWorkload, 1, MPI_DOUBLE, 
                   MPI_COMM_WORLD);
    }
    else {
	receiverID = recvID;
	receiverWorkload = recvWL;
    }
    

    //------------------------------------------------------
    // Check if previous sending subtree has completed.
    //------------------------------------------------------

    int alreadySent = true;
    if (subTreeRequest_ != MPI_REQUEST_NULL){
	MPI_Status sentStatus;
	MPI_Test(&subTreeRequest_, &alreadySent, &sentStatus);
#if 0        
	if (alreadySent) {
	    std::cout << "++++ Process[" << globalRank_ 
		      << "] have completed sending a subtree." << std::endl;
	}
	else {
	    std::cout << "++++ Process[" << globalRank_
		      << "] haven't completed sending a subtree." << std::endl;
	}
#endif
    }
    else {
#if 0
        std::cout << "++++ Process[" << globalRank_ 
                  << "] ready to sent a subtree." << std::endl;
#endif
    }
    
    //------------------------------------------------------
    // Case 1: If subTreePool has subtrees, send the best one.
    //         if the size of the best one is big large, split it.
    // Case 2: If subTreePool has no subtrees AND workingSubTree_ does not 
    //         point to NULL, split it and send one part of it.
    // Case 3: Otherwise, sent a empty msg.
    //------------------------------------------------------

    if (alreadySent) {
	if (subTreePool_->hasKnowledge()) {   // Case 1
	    aSubTree = dynamic_cast<AlpsSubTree* >
		(subTreePool_->getKnowledge().first);
	    sentSuccessful = sendSubTree(receiverID, aSubTree, tag);
        
	    if (sentSuccessful) {
		++(psStats_.subtreeWhole_);
		
		subTreePool_->popKnowledge();
		// Since sent to other process, delete it.
		delete aSubTree;
		aSubTree = NULL;
		if (msgLevel_ > 100) {
		    messageHandler()->message(ALPS_DONATE_WHOLE, messages()) 
			<< globalRank_ << receiverID 
			<< status->MPI_TAG << CoinMessageEol;
		}
	    }
	    else {
		// Split subtree.
		aSubTree = aSubTree->splitSubTree(treeSize);
		if (treeSize > ALPS_GEN_TOL) {
		    ++(psStats_.subtreeSplit_);
		    sentSuccessful = sendSubTree(receiverID, aSubTree, tag);
		    assert(sentSuccessful == true);
		    // Since sent to other process, delete it.
		    delete aSubTree;
		    aSubTree = NULL;
		}

		if (msgLevel_ > 100) {
		    messageHandler()->message(ALPS_DONATE_SPLIT, messages())
			<< globalRank_ << receiverID << status->MPI_TAG 
			<< CoinMessageEol;
		}
	    }
	}
	else if (workingSubTree_ != 0) {     // Case 2
	    aSubTree = workingSubTree_->splitSubTree(treeSize);
	    if (treeSize > ALPS_GEN_TOL) {
		++(psStats_.subtreeSplit_);
		
		sentSuccessful = sendSubTree(receiverID, aSubTree, tag);
		assert(sentSuccessful == true);
		
		// Since sent to other process, delete it.
		delete aSubTree;
		aSubTree = NULL;
		
		if (msgLevel_ > 100) {
		    messageHandler()->message(ALPS_DONATE_SPLIT, messages()) 
			<< globalRank_ << receiverID << status->MPI_TAG 
			<< CoinMessageEol;
		}
	    }
	}   
    }

    if (!sentSuccessful || !alreadySent) {               // Case 3
#if 0
	std::cout << "donateWork(): " << globalRank_ << "has nothing send to " 
		  << receiverID <<"; alreadySent "<<alreadySent<< std::endl;
#endif
        ++(psStats_.donateFail_);

	//MPI_Send(dummyBuf, 0, MPI_PACKED, receiverID, tag, MPI_COMM_WORLD);
	MPI_Isend(dummyBuf, 0, MPI_PACKED, receiverID, tag, 
		  MPI_COMM_WORLD, &subTreeRequest_); // request is overrided.

        if (msgLevel_ > 100) {
            messageHandler()->message(ALPS_DONATE_FAIL, messages()) 
                << globalRank_ << receiverID << status->MPI_TAG 
		<< CoinMessageEol;
        }
    }
    else {
        ++(psStats_.donateSuccess_);
    }

#ifdef NF_DEBUG_MORE
    updateWorkloadInfo();
    messageHandler()->message(ALPS_DONATE_AFTER, messages()) 
	<< globalRank_ << workQuantity_ << subTreePool_->getNumKnowledges() 
	<< CoinMessageEol;
#endif
}

//#############################################################################

// The hub allocate the donated subtree to the worker who has the worst 
// workload. Note the donated subtree is from another cluster.
void 
AlpsKnowledgeBrokerMPI::hubAllocateDonation(char*& bufLarge, 
					    MPI_Status* status)
{
    int i;
    int count = 0;

    int worstRank = -1;
    double worst = -ALPS_OBJ_MAX; // Minimization: best possible

    //------------------------------------------------------
    // Get the number of elements in bufLarge.
    //------------------------------------------------------

    MPI_Get_count(status, MPI_PACKED, &count);

    //------------------------------------------------------
    // Find the worker who has worst work quality.
    //------------------------------------------------------
    
    for (i = 0; i < clusterSize_; ++i) {
	if (!hubWork_) {
	    if (i == clusterRank_) continue;
	}
	if (workerWorkQuantities_[i] > worst) {
	    worst = workerWorkQualities_[i];
	    worstRank = globalRank_ + i;
	}
    }

    //------------------------------------------------------
    // Forward the bufLarge to this worst worker.
    //------------------------------------------------------

    if (worstRank != -1) {
	
#ifdef NF_DEBUG_MORE
	std::cout << "hubAllocateDonation() send to " 
		  << worstRank << std::endl;
#endif
	if (worstRank != globalRank_) {
	    MPI_Send(bufLarge, count, MPI_PACKED, worstRank, 
		     AlpsMsgSubTreeByMaster, MPI_COMM_WORLD);
	    incSendCount("hubAllocateDonation");
	}
	else { // Hub self
	    //std::cout << "Allocate to myself" << std::endl;
	    receiveSubTree(bufLarge, status->MPI_SOURCE, status);
	}
    }
    else {
	std::cout << "ERROR: worstRank == -1" << std::endl; 
	throw CoinError("worstRank == -1", 
			"hubAllocateDonation", "AlpsKnowledgeBrokerMPI");
    }
}

//#############################################################################

// Hub balances the workload of its workers. Check if there are workers has
// no workload. If there is, do quantity balance. If not, do quality balance.
void
AlpsKnowledgeBrokerMPI::hubBalanceWorkers()
{
    const double zeroQuantity = 
	model_->AlpsPar()->entry(AlpsParams::zeroLoad);
    if (clusterWorkQuantity_ < zeroQuantity) {
        if (msgLevel_ > 100) {
            messageHandler()->message(ALPS_LOADBAL_HUB_NO, messages())
                //<< globalRank_ << systemWorkQuantity_ << CoinMessageEol;
                << globalRank_ << clusterWorkQuantity_ << CoinMessageEol;
        }
	return;
    }

    int i;
    char* dummyBuf = NULL;

    std::vector<std::pair<double, int> > loadIDVector;
    loadIDVector.reserve(hubNum_);
    std::multimap<double, int, std::greater<double> > receivers;         
    std::multimap<double, int> donors; 

    const double donorSh      = 
	model_->AlpsPar()->entry(AlpsParams::donorThreshold);
    const double receiverSh   = 
	model_->AlpsPar()->entry(AlpsParams::receiverThreshold);
    const double needWorkThreshold = 
    	model_->AlpsPar()->entry(AlpsParams::needWorkThreshold);
    assert(needWorkThreshold > 0.0);

    ++(psStats_.intraBalance_);
    
    // Indentify donors and receivers and decide to do quantity or quality.
    // Do quantity balance if any hubs do not have work
    bool quantityBalance = false;
    
    for (i = 0; i < clusterSize_; ++i) { // NOTE: i start from 1
#ifdef NF_DEBUG
	std::cout << "Hub["<<globalRank_ <<"] : HUB BALANCE: worker " 
		  << i << ", quantitity = " << workerWorkQuantities_[i] 
		  << ", quality = " << workerWorkQualities_[i] 
		  << ", needWorkThreshold = " << needWorkThreshold << std::endl;
#endif
	if ( i == clusterRank_ ) continue;
        
	if (workerWorkQuantities_[i] <= needWorkThreshold) {
	    receivers.insert(std::pair<double, int>(workerWorkQualities_[i], 
						    globalRank_ - masterRank_ + i));
	    quantityBalance = true;
	}
    }
    
    if (quantityBalance) {  // Quantity balance
        ++(psStats_.quantityBalance_);
        
	for (i = 0; i < clusterSize_; ++i) {
	    if ( i == clusterRank_ ) continue;

	    if (workerWorkQuantities_[i] > needWorkThreshold) {
		donors.insert(std::pair<double, int>(workerWorkQualities_[i], 
						     globalRank_-masterRank_ + i));
	    }
	}
    }
    else {   // Quality balance
        ++(psStats_.qualityBalance_);
        
	double averQuality = 0.0;
	for (i = 0; i < clusterSize_; ++i) {
	    if ( i == clusterRank_ ) continue;
	    averQuality += workerWorkQualities_[i];
	}
	averQuality /= (clusterSize_ - 1);
	
	for (i = 0; i < clusterSize_; ++i) {
	    if ( i == clusterRank_ ) continue;
	    double diff = workerWorkQualities_[i] - averQuality;
	    double diffRatio = fabs(diff / averQuality);
	    if (diff < 0 && diffRatio > donorSh) {  // Donor candidates
		donors.insert(std::pair<double, int>(workerWorkQualities_[i], 
						     globalRank_ -masterRank_ + i));
	    }
	    else if (diff > 0 && diffRatio > receiverSh){// Receiver candidates
		receivers.insert(std::pair<double, int>(
                                     workerWorkQualities_[i],
                                     globalRank_ - masterRank_ + i));
	    }
	}
    }
  
    // Instruct donor workers to send nodes to receiver workers 
    const int numDonor    = donors.size();
    const int numReceiver = receivers.size();
    const int numPair     = CoinMin(numDonor, numReceiver);

#ifdef NF_DEBUG
    std::cout << "Hub[" << globalRank_ << "]: num of donors = " << numDonor 
	      << ", num of receivers = " << numReceiver << std::endl;
#endif

    int donorID;
    int receiverID;
    
    std::multimap<double, int, std::greater<double> >::iterator posD = 
	donors.begin();
    std::multimap<double, int>::iterator posR = receivers.begin();

    for(i = 0; i < numPair; ++i) {
	donorID = posD->second;
	receiverID = posR->second;
        
#ifdef NF_DEBUG
	std::cout << "Hub["<<globalRank_ <<"] : HUB BALANCE: receiver worker ID = " 
		  << receiverID << std::endl;
	std::cout << "Hub["<<globalRank_ <<"] : donor worker ID = " 
		  << donorID << std::endl;
#endif

	assert(receiverID < processNum_ && donorID < processNum_);
	
	++hubDoBalance_;
	if (donorID == globalRank_) { // Hub self is donor
	    MPI_Status status;
	    donateWork(dummyBuf, AlpsMsgSubTree, &status, 
		       receiverID, posR->first);
	    incSendCount("hubBalanceWorkers");
	}
	else {
	    hubAskWorkerDonate(donorID, receiverID, posR->first);
	}
	
    	++posD;
	++posR;
    }
}

//#############################################################################

// Upon receiving request for workload from a worker, this hub asks its most 
// loaded worker to send a subtree to the worker
void 
AlpsKnowledgeBrokerMPI::hubSatisfyWorkerRequest(char*& bufLarge, MPI_Status* status)
{
    int i;
    int requestorRank;
    int pos = 0;
    int donorRank = -1;
    int donorGlobalRank = -1;      // Global rank
    int size = model_->AlpsPar()->entry(AlpsParams::smallSize);

    double bestQuality = ALPS_OBJ_MAX;  // Worst could be

    //------------------------------------------------------
    // Indentify the requestor's cluster rank.
    //------------------------------------------------------

    int requestor = status->MPI_SOURCE;
    requestorRank = requestor % userClusterSize_;

    //------------------------------------------------------
    // Find the worker having best quality in this cluster.
    //------------------------------------------------------

    for (i = 0; i < clusterSize_; ++i) {
	if (i == clusterRank_ || i == requestorRank){
	    // Skip hub and the requestor.
	    // TODO: do not skip hub if hub works.
	    continue;
	}
	if (workerWorkQualities_[i] < bestQuality) {
	    bestQuality = workerWorkQualities_[i];
	    donorRank = i;
	    donorGlobalRank = globalRank_ - masterRank_ + i;
	}
    }

    //------------------------------------------------------
    // Ask the worker has best qaulity to share work with requestor.
    //------------------------------------------------------

    double temp = 0.0;

    if ( (donorGlobalRank != -1) && (donorRank != requestorRank) ) {
	// Send the requestor rank and quality to donor.
	MPI_Pack(&requestor, 1, MPI_INT, smallBuffer_, size, &pos, 
                 MPI_COMM_WORLD);
	MPI_Pack(&temp, 1, MPI_DOUBLE, smallBuffer_, size, &pos, 
                 MPI_COMM_WORLD);
	MPI_Send(smallBuffer_, pos, MPI_PACKED, 
		 donorGlobalRank, 
		 AlpsMsgAskDonateToWorker,
		 MPI_COMM_WORLD);

#ifdef NF_DEBUG_MORE
	std::cout << "HUB " << globalRank_ << "ask worker " <<donorGlobalRank
		  << "to donate workload with " << requestor << std::endl;
#endif
    }
    else {
	// Failed to find a donor, send a empty buffer to requestor.
        ++(psStats_.donateFail_);
    
	MPI_Send(smallBuffer_, 0, MPI_PACKED, 
		 requestor,
		 AlpsMsgSubTreeByWorker, 
		 MPI_COMM_WORLD);
        if (msgLevel_ > 100) {
            messageHandler()->message(ALPS_LOADBAL_HUB_FAIL, messages())
                << globalRank_ << requestor << CoinMessageEol;
        }
    }
    
    incSendCount("hubSatisfyWorkerRequest");
} 

//#############################################################################

// Report cluster work quality, quantity, and msg counts.
void 
AlpsKnowledgeBrokerMPI::hubReportStatus(int tag, MPI_Comm comm)
{
    int pos   = 0;
    int size = model_->AlpsPar()->entry(AlpsParams::smallSize);

    int receiver;

    if (comm == MPI_COMM_WORLD) {
	receiver = masterRank_;
    }
    else if (comm == hubComm_) {
	// NOTE: master's rank is always 0 in hubComm_.
	receiver = 0;
    }
    else {
	std::cout << "HUB " << globalRank_ 
		  <<" : Unkown Comm in hubReportStatus" << std::endl;
	throw CoinError("Unkown Comm", "hubReportStatus", 
			"AlpsKnowledgeBrokerMPI");
    }

#if defined(NF_DEBUG_MORE)
    std::cout << "HUB " << globalRank_ 
	      << " : quantity = " << clusterWorkQuantity_ << ", ";
    for (int i = 0; i < clusterSize_; ++i) {
	std::cout << "w[" << globalRank_+i << "]=" 
		  << workerWorkQuantities_[i] << ", ";
    }
    std::cout << std::endl;
    std::cout << "HUB " << globalRank_ 
	      << " : quality = " << clusterWorkQuality_ << ", ";
    for (int i = 0; i < clusterSize_; ++i) {
	std::cout << "w[" << globalRank_+i << "]=" 
		  << workerWorkQualities_[i] << ", ";
    }
    std::cout << std::endl;
#endif

    MPI_Pack(&clusterNodeProcessed_, 1, MPI_INT, smallBuffer_,size,&pos, comm);
    MPI_Pack(&clusterWorkQuality_, 1, MPI_DOUBLE, smallBuffer_,size,&pos,comm);
    MPI_Pack(&clusterWorkQuantity_, 1, MPI_DOUBLE,smallBuffer_,size,&pos,comm);
    MPI_Pack(&clusterSendCount_, 1, MPI_INT, smallBuffer_, size, &pos, comm);
    MPI_Pack(&clusterRecvCount_, 1, MPI_INT, smallBuffer_, size, &pos, comm);
    MPI_Pack(&nodeProcessingTime_, 1, MPI_DOUBLE, smallBuffer_, size, &pos,
             comm);
    MPI_Pack(&unitWorkNodes_, 1, MPI_INT, smallBuffer_, size, &pos, comm);    

    MPI_Send(smallBuffer_, pos, MPI_PACKED, receiver, tag, comm);

    clusterSendCount_ = clusterRecvCount_ = 0;  // Only count new msg
}

//#############################################################################

// After receiving status (in buf) from a worker, a hub updates its 
// cluster's status
void 
AlpsKnowledgeBrokerMPI::hubUpdateCluStatus(char*& bufLarge, 
					   MPI_Status* status, 
					   MPI_Comm comm)
{
    int msgSendNum, msgRecvNum;
    int position = 0;
    int sender;
    int size = model_->AlpsPar()->entry(AlpsParams::smallSize);

    if (comm == MPI_COMM_WORLD)
	sender = (status->MPI_SOURCE) % userClusterSize_;
    else if (comm == clusterComm_)
	sender = status->MPI_SOURCE;
    else {
	std::cout << "unknown sender in hubUpdateCluStatus()" << std::endl;
	throw CoinError("unknown sender", 
			"hubUpdateCluStatus()", "AlpsKnowledgeBrokerMPI");
    }

    workerReported_[sender] = true;
    
    int preNodeProcessed = workerNodeProcesseds_[sender];
    int curNodeProcessed;
    //double preQuality = workerWorkQualities_[sender];
    double curQuality;
    double preQuantity = workerWorkQuantities_[sender];
    double curQuantity;
    double npTime;

    MPI_Unpack(bufLarge, size, &position, &curNodeProcessed, 1, MPI_INT, comm);
    MPI_Unpack(bufLarge, size, &position, &curQuality, 1, MPI_DOUBLE, comm);
    MPI_Unpack(bufLarge, size, &position, &curQuantity, 1, MPI_DOUBLE, comm);
    MPI_Unpack(bufLarge, size, &position, &msgSendNum, 1, MPI_INT, comm);
    MPI_Unpack(bufLarge, size, &position, &msgRecvNum, 1, MPI_INT, comm);
    MPI_Unpack(bufLarge, size, &position, &npTime, 1, MPI_DOUBLE, comm);
    MPI_Unpack(bufLarge, size, &position, &unitWorkNodes_, 1, MPI_INT, comm);
    
    workerNodeProcesseds_[sender] = curNodeProcessed;
    workerWorkQualities_[sender] = curQuality;
    workerWorkQuantities_[sender] = curQuantity;
    
    clusterNodeProcessed_ -= preNodeProcessed;
    clusterNodeProcessed_ += curNodeProcessed;

    clusterWorkQuantity_ -= preQuantity;
    clusterWorkQuantity_ += curQuantity;

    if (clusterWorkQuantity_ < ALPS_QUALITY_TOL)
	clusterWorkQuality_ = ALPS_OBJ_MAX;
    else {
#if 0  // Have problems
	// NOTE: no work means quality is ALPS_OBJ_MAX.
	if (hubWork_) {
	    clusterWorkQuality_ *= userClusterSize_;
	}
	else {
	    clusterWorkQuality_ *= (userClusterSize_ - 1);
	}
	    
	clusterWorkQuality_ -= preQuality;
	clusterWorkQuality_ += curQuality;
	if (hubWork_) {
	    clusterWorkQuality_ /= userClusterSize_;
	}
	else {
	    clusterWorkQuality_ /= (userClusterSize_ - 1);
	}
#endif
	clusterWorkQuality_ = std::min(clusterWorkQuality_, curQuality);
	
    }
    
    clusterSendCount_ += msgSendNum;
    clusterRecvCount_ += msgRecvNum;

    if (npTime != ALPS_NODE_PROCESS_TIME && npTime > 1.0e-8) {
        if (nodeProcessingTime_ == ALPS_NODE_PROCESS_TIME) {
            nodeProcessingTime_ = npTime;
        }
        else {
            nodeProcessingTime_ = 0.5 * (nodeProcessingTime_ + npTime);
        }

        if (globalRank_ == masterRank_) {
            masterBalancePeriod_ = computeBalancePeriod(userBalancePeriod_,
                                                        masterBalancePeriod_,
                                                        nodeProcessingTime_);
        }
        else {
            hubReportPeriod_ = computeBalancePeriod(userBalancePeriod_,
                                                    hubReportPeriod_,
                                                    nodeProcessingTime_);
        }
        
	if (hubMsgLevel_ > 5) {
            messageHandler()->message(ALPS_LOADBAL_HUB_PERIOD, messages())
                << globalRank_ << hubReportPeriod_ << CoinMessageEol;
	}

#if 0
        std::cout << "HUB[" << globalRank_ << "]: After updateSystem(),"
                  << "hubReportPeriod_ =  " << masterBalancePeriod_
                  << ", npTime = " << npTime
                  << ", nodeProcessingTime_ = " << nodeProcessingTime_
                  << std::endl;
#endif

    }
    
#if 0
    std::cout << "HUB["<< globalRank_ <<"]: after hubUpdateCluStatus(): " 
	      << "curQuality = " << curQuality 
              << ", sender = " << sender
	      << ", curQuantity = " << curQuantity 
	      << ", preQuantity = " << preQuantity 
	      << ", clusterWorkQuantity_  = " << clusterWorkQuantity_
	      << ", clusterWorkQuality_  = " << clusterWorkQuality_
	      << ", clusterSendCount_ = " << clusterSendCount_
	      << ", clusterRecvCount_ = " << clusterRecvCount_
	      << ", clusterSize_ = " << clusterSize_
	      << ", status->MPI_SOURCE = " << status->MPI_SOURCE
	      << std::endl;
#endif

}

//#############################################################################

// After receive master's require to donate work, this hub finds its most 
// loaded worker and ask it to donate some work to another hub, whose
// information (id and load) is in bufLarge.
void 
AlpsKnowledgeBrokerMPI::hubsShareWork(char*& bufLarge,
				      MPI_Status* status)
{
    int i;
    int pos = 0;
    int receiverID = 0;
    int maxLoadRank = -1;            // Global rank
    double maxLoad  = ALPS_DBL_MAX;
    int size = model_->AlpsPar()->entry(AlpsParams::smallSize);

    int skipRank;
    
    double receiverWorkload = 0.0;

    //------------------------------------------------------
    // Indentify the receiver and its current workload.
    //------------------------------------------------------

    MPI_Unpack(bufLarge, size, &pos, &receiverID, 1, MPI_INT, MPI_COMM_WORLD);
    MPI_Unpack(bufLarge, size, &pos, &receiverWorkload, 1, MPI_DOUBLE, 
               MPI_COMM_WORLD);

    //------------------------------------------------------
    // Find the donor worker in hub's cluster.
    //------------------------------------------------------

    if (hubWork_) {
	skipRank = -1;
    }
    else {
	skipRank = clusterRank_;
    }
    
    for (i = 0; i < clusterSize_; ++i) {
	if (i != skipRank) {
	    if ( (workerWorkQuantities_[i] > ALPS_QUALITY_TOL) &&
		 (workerWorkQualities_[i] < maxLoad) ) {
		maxLoad = workerWorkQualities_[i];
		maxLoadRank = globalRank_ - masterRank_ + i;
	    }
	}
    }

    //------------------------------------------------------
    // Ask the donor to share work (send a subtree to receiving hub).
    //------------------------------------------------------

    if (maxLoadRank != -1) {
#if 0
	std::cout << "HUB[" << globalRank_ << "] : will ask process " 
                  << maxLoadRank
		  << " to donate workload to process " << receiverID 
                  << std::endl;
#endif
	
	if(maxLoadRank != globalRank_) { // Not hub 
	    // max loaded is NOT this hub.

	    pos = 0;                   // Reset position to pack
	    MPI_Pack(&receiverID, 1, MPI_INT, smallBuffer_, size,
                     &pos, MPI_COMM_WORLD);
	    MPI_Pack(&receiverWorkload, 1, MPI_DOUBLE, smallBuffer_, size,
                     &pos, MPI_COMM_WORLD);
	    MPI_Send(smallBuffer_, pos, MPI_PACKED, maxLoadRank, 
		     AlpsMsgAskDonateToHub, MPI_COMM_WORLD);
	    incSendCount("hubsShareWork");
	}
	else {
	    // Max loaded is this hub. 
	    // NOTE: smallBuffer_ is NOT useful in this case.
	    donateWork(smallBuffer_, 
		       AlpsMsgSubTreeByMaster,
		       status, 
		       receiverID,
		       receiverWorkload);
	    incSendCount("hubsShareWork");
	}
    }
    else {
#if 0
	std::cout << "HUB[" << globalRank_ 
                  << "] :can not find a process to donate workload to process "
                  << receiverID << std::endl;
#endif
        if (msgLevel_ > 100) {
            messageHandler()->message(ALPS_LOADBAL_HUB_FAIL, messages())
                << globalRank_ << receiverID << CoinMessageEol;
        }
        
        // See a empty msg to master to reduce masterDoBalance_ by 1
        MPI_Send(smallBuffer_, 0, MPI_PACKED, masterRank_,
                 AlpsMsgHubFailFindDonor, MPI_COMM_WORLD);
        incSendCount("hubsShareWork");
    }
}

//#############################################################################

// Master balance the workload of hubs. Check if there are hubs has
// no workload. If there is, do quantity balance. If not, do quality balance.
void
AlpsKnowledgeBrokerMPI::masterBalanceHubs()
{
    const double zeroQuantity = 
	model_->AlpsPar()->entry(AlpsParams::zeroLoad);
    if (systemWorkQuantity_ < zeroQuantity) {
        if (msgLevel_ > 100) {
            messageHandler()->message(ALPS_LOADBAL_MASTER_NO, messages())
                << globalRank_ << systemWorkQuantity_ << CoinMessageEol;
        }
	return;
    }
    
    int i;
    int size = model_->AlpsPar()->entry(AlpsParams::smallSize);
    
    std::vector<std::pair<double, int> > loadIDVector;

    loadIDVector.reserve(hubNum_);

    std::multimap<double, int, std::greater<double> > receivers;         
    std::multimap<double, int> donors; 

    const double donorSh      =
	model_->AlpsPar()->entry(AlpsParams::donorThreshold);
    const double receiverSh   =
	model_->AlpsPar()->entry(AlpsParams::receiverThreshold);

    //------------------------------------------------------
    // Identify donors and receivers and decide do quality or quantity.
    // Do quantity balance immediately if any hubs do not have work.
    //------------------------------------------------------

    ++(psStats_.interBalance_);

    bool quantityBalance = false;
    for (i = 0; i < hubNum_; ++i) {
	if (hubWorkQuantities_[i] < ALPS_QUALITY_TOL) {    // Have not work
#if 0
            std::cout << "++++ master balance: find donor hub " << hubRanks_[i]
                      << "; quantity = " << hubWorkQuantities_[i] 
                      << std::endl;
#endif
	    receivers.insert(std::pair<double, int>(hubWorkQualities_[i], 
						    hubRanks_[i]));
	    quantityBalance = true;
	}
    }
    
    if (quantityBalance) {  // Quantity balance
        ++(psStats_.quantityBalance_);

	for (i = 0; i < hubNum_; ++i) {
	    if (hubWorkQuantities_[i] > ALPS_QUALITY_TOL) {
#if 0
                std::cout << "++++ master balance: find donor hub " 
                          << hubRanks_[i] << "; quantity = " 
                          << hubWorkQuantities_[i] << std::endl;
#endif
		donors.insert(std::pair<double, int>(hubWorkQualities_[i], 
						     hubRanks_[i]));
	    }
	}
    }
    else {   // Quality balance
        ++(psStats_.qualityBalance_);

	double averQuality  = 0.0;
	for (i = 0; i < hubNum_; ++i) {
	    averQuality += hubWorkQualities_[i];
	}
	averQuality /= hubNum_;
	
	for (i = 0; i < hubNum_; ++i) {
	    double diff = hubWorkQualities_[i] - averQuality;
	    double diffRatio = fabs(diff / (averQuality + 1.0));
	    if (diff < 0 && diffRatio > donorSh) {  // Donor candidates
#if 0
                std::cout << "++++ master balance: find donor hub " 
                          << hubRanks_[i] << "; quality = " 
                          << hubWorkQualities_[i] << std::endl;
#endif           
		donors.insert(std::pair<double, int>(hubWorkQualities_[i], 
						     hubRanks_[i]));
	    }
	    else if (diff > 0 && diffRatio > receiverSh){// Receiver candidates
#if 0
                std::cout << "++++ master balance: quality: find receiver hub "
                          << hubRanks_[i] << "; quality = " 
                          << hubWorkQualities_[i] << std::endl;
#endif
		receivers.insert(std::pair<double, int>(hubWorkQualities_[i], 
							hubRanks_[i]));
	    }
	}
    }
    
    //------------------------------------------------------
    // Tell the donor hubs to send subtree to receiver hubs.
    //------------------------------------------------------

    const int numDonor    = donors.size();
    const int numReceiver = receivers.size();
    const int numPair     = CoinMin(numDonor, numReceiver);

#if 0
    std::cout << "Master: donors size = " << numDonor 
              << ", receiver size = " << numReceiver 
              << ", numPair = " << numPair << std::endl;
#endif

    int donorID;
    int receiverID;
 
    std::multimap<double, int, std::greater<double> >::iterator posD = 
	donors.begin();
    std::multimap<double, int>::iterator posR = receivers.begin();
    
    for(i = 0; i < numPair; ++i) {
	donorID = posD->second;
	receiverID = posR->second;
   
#if 0
	std::cout << "MASTER : receiver hub ID = " << receiverID 
		  << "; quality = " << posR->first << std::endl;
	std::cout << "MASTER : donor hub ID = " << donorID 
		  << "; quality = " << posD->first << std::endl;
#endif
	
	if (CoinAbs(receiverID) >= processNum_ || 
	    CoinAbs(donorID) >= processNum_) {
	    std::cout << "ERROR: receiverID = " << receiverID <<  std::endl;
	    std::cout << "ERROR : donorID = " << donorID << std::endl;
	    throw
		CoinError("receiverID>=processNum_ || donorID >= processNum_", 
			  "masterBalanceHubs", "AlpsKnowledgeBrokerMPI");
	    
	}

	if (donorID != masterRank_) {
	    ++masterDoBalance_;
	    masterAskHubDonate(donorID, receiverID, posR->first);
	} 
	else {  // donor is the master
	    double maxLoad = ALPS_DBL_MAX;
	    int maxLoadRank = -1;
	    int pos = 0;
	    double recvLoad = posR->first;

	    // Find the donor worker in hub's cluster
	    for (int k = 0; k < clusterSize_; ++k) {
		if (k != clusterRank_) {
		    if (workerWorkQualities_[k] < maxLoad) {
			maxLoad = workerWorkQualities_[k];
			maxLoadRank = globalRank_ - masterRank_ + k;
		    }
		}
	    }
	    
	    // FIXME: how many workload to share? Righ now just 1 subtree. 
	    // Ask the donor worker to send a subtree to receiving hub)
	    if (maxLoadRank != -1) {
		MPI_Pack(&receiverID, 1, MPI_INT, smallBuffer_, size, &pos,
			 MPI_COMM_WORLD);
		MPI_Pack(&recvLoad, 1, MPI_DOUBLE, smallBuffer_, size, &pos, 
			 MPI_COMM_WORLD);
                
		incSendCount("masterBalanceHubs()");
		MPI_Send(smallBuffer_, pos, MPI_PACKED, maxLoadRank,
			 AlpsMsgAskDonateToHub, MPI_COMM_WORLD);
		++masterDoBalance_;
		
#ifdef NF_DEBUG_MORE
		std::cout << "MASTER : ask its worker " << maxLoadRank
			  << " to donate load to hub"
			  << receiverID << std::endl;
#endif
	    }      
	}
	++posD;
	++posR;
    }
}

//#############################################################################

// After recieve status (in buf) of a hub, update system status
void 
AlpsKnowledgeBrokerMPI::masterUpdateSysStatus(char*& bufLarge, 
					      MPI_Status* status, 
					      MPI_Comm comm)
{
    int position = 0;
    int msgSendNum, msgRecvNum;
    int sender = -1;
    int size = model_->AlpsPar()->entry(AlpsParams::smallSize);

    if (comm == MPI_COMM_WORLD) {
        //Be careful with / or %
	sender = (int)(status->MPI_SOURCE) / userClusterSize_;
    }
    else if (comm == hubComm_) {
	sender = (int)(status->MPI_SOURCE);
    }
    else {
	std::cout << "unknown COMM in masterUpdateSysStatus" 
		  << std::endl;
	throw CoinError("unknown sender", 
			"masterUpdateSysStatus", "AlpsKnowledgeBrokerMPI");
    }

    int preNodeProcessed = hubNodeProcesseds_[sender];
    int curNodeProcessed = 0;
    //double preQuality = hubWorkQualities_[sender];
    double curQuality;
    double preQuantity = hubWorkQuantities_[sender];
    double curQuantity;
    double npTime;

    MPI_Unpack(bufLarge, size, &position, &curNodeProcessed, 1, MPI_INT, comm);
    MPI_Unpack(bufLarge, size, &position, &curQuality, 1, MPI_DOUBLE, comm);
    MPI_Unpack(bufLarge, size, &position, &curQuantity, 1, MPI_DOUBLE, comm);
    MPI_Unpack(bufLarge, size, &position, &msgSendNum, 1, MPI_INT, comm);
    MPI_Unpack(bufLarge, size, &position, &msgRecvNum, 1, MPI_INT, comm);
    MPI_Unpack(bufLarge, size, &position, &npTime, 1, MPI_DOUBLE, comm);
    MPI_Unpack(bufLarge, size, &position, &unitWorkNodes_, 1, MPI_INT, comm);

    // Update the hub's status
    hubNodeProcesseds_[sender] = curNodeProcessed;
    hubWorkQualities_[sender] = curQuality;
    hubWorkQuantities_[sender] = curQuantity;

    // Update system status
    systemSendCount_ += msgSendNum;
    systemRecvCount_ += msgRecvNum;
    
    systemNodeProcessed_ += curNodeProcessed;
    systemNodeProcessed_ -= preNodeProcessed;
    
    systemWorkQuantity_ += curQuantity;
    systemWorkQuantity_ -= preQuantity;

    if (systemWorkQuantity_ < ALPS_QUALITY_TOL) {
	systemWorkQuality_ = ALPS_OBJ_MAX;
    }
    else {
	systemWorkQuality_ = std::min(systemWorkQuality_, curQuality);
    }
    
    if ( hubReported_[sender] != true ) {
	hubReported_[sender] = true;
    }

    int large = ALPS_INT_MAX/10;
    if (systemSendCount_ > large || systemRecvCount_ > large) {
	int minCount = std::min(systemSendCount_, systemRecvCount_);
	systemSendCount_ -= minCount;
	systemRecvCount_ -= minCount;
    }

    if (npTime != ALPS_NODE_PROCESS_TIME && npTime > 1.0e-10) {
        if (nodeProcessingTime_ == ALPS_NODE_PROCESS_TIME) {
            nodeProcessingTime_ = npTime;
        }
        else {
            nodeProcessingTime_ = 0.5 * (nodeProcessingTime_ + npTime);
        }

        masterBalancePeriod_ = computeBalancePeriod(userBalancePeriod_,
                                                    masterBalancePeriod_,
                                                    nodeProcessingTime_);
#if 0
        std::cout << "MASTER[" << globalRank_ << "]: After updateSystem(),"
                  << "masterBalancePeriod_ =  " << masterBalancePeriod_
                  << std::endl;
#endif
    }

#ifdef NF_DEBUG
    std::cout << "MASTER[" << globalRank_ 
              << "]: After updateSystem() : curQuality " 
	      << curQuality << " preQuality = " << preQuality 
	      << ", hubReported_[" << sender << "] = "
	      << hubReported_[sender]
	      << ", systemSendCount_ = " << systemSendCount_
	      << ", systemRecvCount_ = " << systemRecvCount_
	      << ", systemWorkQuality_ = " << systemWorkQuality_
	      << std::endl; 
#endif

}

//#############################################################################

// Add master and master cluster status to system status
void
AlpsKnowledgeBrokerMPI::refreshSysStatus()
{
    //------------------------------------------------------
    // Add master's quantity (0 anyway) to hub 1.
    //------------------------------------------------------

    workerWorkQuantities_[masterRank_] = workQuantity_;
    clusterWorkQuantity_ += workerWorkQuantities_[masterRank_];

    //------------------------------------------------------
    // Add master's node processed num to hub 1.
    //------------------------------------------------------

    int preMasterNodeP = workerNodeProcesseds_[masterRank_];
    workerNodeProcesseds_[masterRank_] = nodeProcessedNum_;
    clusterNodeProcessed_ += workerNodeProcesseds_[masterRank_];
    clusterNodeProcessed_ -= preMasterNodeP;

    // Note: Nothing need to do about master's quality. 

    //------------------------------------------------------
    // Add hub1(master) cluster's quantity into system.
    //------------------------------------------------------

    double preHub1QT = hubWorkQuantities_[0];
    hubWorkQuantities_[0] = clusterWorkQuantity_;
    systemWorkQuantity_  += hubWorkQuantities_[0];
    systemWorkQuantity_  -= preHub1QT;

    //------------------------------------------------------
    // Add hub1(master) cluster's number of nodes processed into system.
    //------------------------------------------------------

    int preHub1NodeP = hubNodeProcesseds_[0];
    hubNodeProcesseds_[0] = clusterNodeProcessed_;
    systemNodeProcessed_ += hubNodeProcesseds_[0];
    systemNodeProcessed_ -= preHub1NodeP;

    //------------------------------------------------------
    // Add hub1(master) cluster's quality into system.
    //------------------------------------------------------

    hubWorkQualities_[0] = clusterWorkQuality_;
    if (systemWorkQuantity_ < ALPS_QUALITY_TOL) {
	systemWorkQuality_ = ALPS_OBJ_MAX;
    }
    else {
	systemWorkQuality_ = std::min(systemWorkQuality_,hubWorkQualities_[0]);
    }
   
#if 0
    int i;
    
    std::cout << "WORKLOAD: ";
    for (i = 0; i < clusterSize_; ++i) {
	std::cout << "worker[" <<i<< "] = " 
		  << workerWorkQuantities_[i] << ", ";
    }
    std::cout << std::endl;
    for (i = 0; i < hubNum_; ++i) {
	std::cout << "hub[" <<i<< "] = " << hubWorkQuantities_[i]
		  << "; ";
    }
    std::cout << std::endl;
#endif    

    //------------------------------------------------------
    // Add master' msg counts to system.
    //------------------------------------------------------

    systemSendCount_ += sendCount_; 
    systemRecvCount_ += recvCount_; 
    sendCount_ = recvCount_ = 0;

    //------------------------------------------------------
    // Add cluster's msg counts to system.
    //------------------------------------------------------

    systemSendCount_ += clusterSendCount_;
    systemRecvCount_ += clusterRecvCount_;
    clusterSendCount_ = clusterRecvCount_ = 0;
}

//#############################################################################

// Add the state of the hub to cluster state
void
AlpsKnowledgeBrokerMPI::refreshClusterStatus()
{   
    //------------------------------------------------------
    // Add hub's msg counts into cluster counts.
    //------------------------------------------------------

    clusterSendCount_ += sendCount_;
    clusterRecvCount_ += recvCount_;
    sendCount_ = recvCount_ = 0;        // IMPORTANT!  

    //------------------------------------------------------
    // Add hub's quantity into cluster. 
    //------------------------------------------------------

    double preWorkQuantity = workerWorkQuantities_[masterRank_];
    workerWorkQuantities_[masterRank_] = workQuantity_;
    clusterWorkQuantity_ -= preWorkQuantity;
    clusterWorkQuantity_ += workQuantity_;

    //------------------------------------------------------
    // Incorporate hub's quality.
    //------------------------------------------------------

    workerWorkQualities_[masterRank_] = workQuality_;
    clusterWorkQuality_ = std::min(clusterWorkQuality_, workQuality_);

    // Add hub's node processed num into cluster
    int PreNodeP = workerNodeProcesseds_[masterRank_];
    workerNodeProcesseds_[masterRank_] = nodeProcessedNum_;
    clusterNodeProcessed_ += workerNodeProcesseds_[masterRank_];
    clusterNodeProcessed_ -= PreNodeP;

#ifdef NF_DEBUG_MORE
    std::cout << "Hub[" << globalRank_ << "]: clusterWorkQuality = " 
	      << clusterWorkQuality_ << std::endl;
    for (int i = 0; i < clusterSize_; ++i) {
	std::cout << "wQL[" <<i + globalRank_ << "] = " 
		  << workerWorkQualities_[i] << ", ";
    }
    std::cout << std::endl;
#endif 
}

//#############################################################################

// Unpack received incumbent value and process ID from bufLarge.
// Update incumbent value and ID stored on this process.
bool
AlpsKnowledgeBrokerMPI::unpackSetIncumbent(char*& bufLarge, MPI_Status* status)
{
    bool accept = false;
    int size     = 
	model_->AlpsPar()->entry(AlpsParams::smallSize);
    int position = 0;
    double incVal= 0.0;
    int incID    = 0;
 
    // Unpack incumbent value from bufLarge
    MPI_Unpack(bufLarge, size, &position, &incVal, 1, MPI_DOUBLE, 
               MPI_COMM_WORLD);
    MPI_Unpack(bufLarge, size, &position, &incID, 1, MPI_INT, 
               MPI_COMM_WORLD);

    if (incID == globalRank_) { 
	
#ifdef NF_DEBUG_MORE
	std::cout << "PROC " << globalRank_ 
		  << " : unpack and ingore incVal = " << incVal << " at PROC" 
		  << incID << ", since I am " << incID
		  << std::endl;
#endif
	return false;  // Do nothing
    }

    // Assume minimization
    if (incVal < incumbentValue_) {
        // Better solution 
	incumbentValue_ = incVal;
	incumbentID_ = incID;
	accept = true;

        if (globalRank_ == masterRank_) {
            bestSolNode_ = systemNodeProcessed_;
        }
	
#ifdef NF_DEBUG_MORE
	std::cout << "PROC " << globalRank_ << " : accept incVal = " 
		  << incVal << " from PROC " << incID << std::endl;
#endif

	//updateIncumbent_ = true;        // The incumbent value is updated. 
    } 
    else if(incVal == incumbentValue_ ) {
	   
	if (incID < incumbentID_) {     // So that all process consistant 
	    incumbentValue_ = incVal;
	    incumbentID_ = incID;
	    accept = true;
	} 
	else{
	    accept = false;

#ifdef NF_DEBUG_MORE
	    std::cout << "PROC " << globalRank_ 
		      << " : recv but discard incVal = " << incVal  
		      << " from PROC (SAME) " << incID << std::endl;
#endif  
	} 
    } 
    else {   // >
	accept = false;
	
#ifdef NF_DEBUG_MORE
	std::cout << "PROC " << globalRank_ 
		  << " : recv but discard incVal = " << incVal  
		  << " from PROC (WORSE)" << incID << std::endl;
#endif  
    }
    
    return accept;
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::workerReportStatus(int tag, 
					   MPI_Comm comm)
{
    int pos = 0;
    int size = model_->AlpsPar()->entry(AlpsParams::smallSize);
  
    int receiver;
    
    if (comm == MPI_COMM_WORLD)	{
	receiver = myHubRank_;
    } 
    else if (comm == clusterComm_) {
	receiver = masterRank_;
    } 
    else {
	std::cout << "WORKER " << globalRank_ 
		  <<" : Unkown Comm in workerReportStatus" << std::endl;
	throw CoinError("Unkown Comm", 
			"workerReportStatus",
                        "AlpsKnowledgeBrokerMPI");
    }

    MPI_Pack(&nodeProcessedNum_, 1, MPI_INT, smallBuffer_, size, &pos, comm);
    MPI_Pack(&workQuality_, 1, MPI_DOUBLE, smallBuffer_, size, &pos, comm);
    MPI_Pack(&workQuantity_, 1, MPI_DOUBLE, smallBuffer_, size, &pos, comm);
    MPI_Pack(&sendCount_, 1, MPI_INT, smallBuffer_, size, &pos, comm);
    MPI_Pack(&recvCount_, 1, MPI_INT, smallBuffer_, size, &pos, comm);
    MPI_Pack(&nodeProcessingTime_, 1, MPI_DOUBLE, smallBuffer_, size, &pos,
             comm);
    MPI_Pack(&unitWorkNodes_, 1, MPI_INT, smallBuffer_, size, &pos, comm);

    MPI_Send(smallBuffer_, pos, MPI_PACKED, receiver, tag, comm);
   
#if 0
    std::cout << "WORKER " << globalRank_ 
	      << " : report quality = " << workQuality_ 
	      << " : report quantity = " << workQuantity_ 
	      << " to hub " 
	      << myHubRank_ << "; Tag = " << tag << "; sendCount_ = "
	      << sendCount_ << "; recvCount_ = " << recvCount_ 
              << "unitWorkNodes_ = " << unitWorkNodes_
              << std::endl;
#endif

    sendCount_ = recvCount_ = 0;      // Only count new message
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::workerRecvIndices(char *&bufLarge) 
{
    int position = 0;
    int size = 100;
    int nextIndex;
    int maxIndex;
    
    MPI_Unpack(bufLarge, size, &position, &nextIndex, 1, MPI_INT, 
               MPI_COMM_WORLD);
    MPI_Unpack(bufLarge, size, &position, &maxIndex, 1, MPI_INT, 
               MPI_COMM_WORLD);
    
#if 0
    std::cout << "++++ worker[" << globalRank_ 
              << "] received indices from master"
              << ", nextIndex = " << nextIndex
              << ", maxIndex = " << maxIndex
              << std::endl;
#endif
    
    if (nextIndex < 0 || maxIndex < 0 || nextIndex > maxIndex) {
        haltSearch_ = true;
    }
    else {
        haltSearch_ = false;
	setNextNodeIndex(nextIndex);
	setMaxNodeIndex(maxIndex);
    }
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::workerAskIndices()
{
    int position = 0;
    int size = model_->AlpsPar()->entry(AlpsParams::smallSize);

#if 0
    std::cout << "++++ worker[" << globalRank_ << "] ask indices from master."
              << std::endl;
#endif
    
    MPI_Pack(&globalRank_, 1, MPI_INT, smallBuffer_, size, &position,
             MPI_COMM_WORLD);
    MPI_Send(smallBuffer_, position, MPI_PACKED, masterRank_, 
             AlpsMsgWorkerAskIndices, MPI_COMM_WORLD);    
    incSendCount("workerAskIndices()");
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::masterSendIndices(char *&bufLarge)
{
    int nextIndex = getNextNodeIndex();
    int maxIndex = getMaxNodeIndex();
    int leftIndex = maxIndex - nextIndex;

    int pos = 0;
    int size = 100;    
    int recvWorker = -1;

    if (leftIndex >  masterIndexBatch_) {
        maxIndex = nextIndex + masterIndexBatch_;
	setNextNodeIndex(maxIndex + 1);
    }
    else if (leftIndex > 100) {
	setNextNodeIndex(maxIndex);
    }
    else {
        // No index
        forceTerminate_ = true;
        nextIndex = -1;
        maxIndex = -1;
    }

    // Unpack the rank of worker.
    MPI_Unpack(bufLarge, size, &pos, &recvWorker, 1, MPI_INT, MPI_COMM_WORLD);
    
#if 0
    std::cout << "++++ master[" << globalRank_ << "] send indices to worker "
              << recvWorker << ", nextIndex = " << nextIndex
              << ", maxIndex = " << maxIndex
              << std::endl;
#endif
    
    // Pack nextIndex and maxIndex into small buffer and send it.
    pos = 0;
    MPI_Pack(&nextIndex, 1, MPI_INT, smallBuffer_, size, &pos,
             MPI_COMM_WORLD);
    MPI_Pack(&maxIndex, 1, MPI_INT, smallBuffer_, size, &pos,
             MPI_COMM_WORLD);
    MPI_Send(smallBuffer_, pos, MPI_PACKED, recvWorker, 
             AlpsMsgIndicesFromMaster, MPI_COMM_WORLD);
    incSendCount("masterSendIndices");        
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::broadcastModel(const int id, const int sender)
{
    char* modelBuffer = 0;
    int size = -1;
    int position = 0;  // Initialize to 0
    AlpsEncoded* encodedModel = NULL;

    //------------------------------------------------------
    // Encode matrix and pack into modelBuffer.
    //------------------------------------------------------
    
    if (id == sender) {
        // Pack model 
        encodedModel = model_->encode();
	packEncoded(encodedModel, modelBuffer, size, position, MPI_COMM_WORLD);

#ifdef NF_DEBUG
	std::cout << "MASTER: packed model. size = " 
		  << size << std::endl; 
#endif
    }

    //------------------------------------------------------
    // Broadcost the size of matrix.
    //------------------------------------------------------

    // Broadcast modelBuffer size first
    MPI_Bcast(&size, 1, MPI_INT, sender, MPI_COMM_WORLD);  

    if (id != sender) {
	// Process except master receive the size of model.
	if (size <= 0) {
	    throw CoinError("Msg size <= 0", 
			    "broadcastModel", 
			    "AlpsKnowledgeBrokerMPI");
	}
	if( modelBuffer != NULL ) {
	    delete [] modelBuffer;  
	    modelBuffer = NULL;
	}
	modelBuffer = new char[size + 100];
    }

    //------------------------------------------------------
    // Broadcost matrix.
    //------------------------------------------------------

    MPI_Bcast(modelBuffer, size, MPI_CHAR, sender, MPI_COMM_WORLD);
    
    if (id != sender) {

	//--------------------------------------------------
	// Unpack to encoded model.
	//--------------------------------------------------

        position = 0;
	encodedModel = unpackEncoded(modelBuffer, 
                                     position, 
                                     MPI_COMM_WORLD,
                                     size+100);
	
#ifdef NF_DEBUG
	std::cout << "PROCESS[" <<id<< "]: start to decode model."
		  << ", knowledge type="<< encodedModel->type() << std::endl;
#endif

	//--------------------------------------------------
	// Note AlpsDataPool have a application model, do not need to 
	// create more than one model. Just need to fill in model data.
	//--------------------------------------------------

	model_->decodeToSelf(*encodedModel);
	
#ifdef NF_DEBUG
	std::cout << "PROCESS[" <<id<< "]: finished decoding model." 
		  << std::endl;
#endif

	//--------------------------------------------------
	// Set up self.
	//--------------------------------------------------

	//(modifyDataPool()->getModel())->setupSelf();

	model_->setupSelf();
    }

    if (modelBuffer) {
        delete [] modelBuffer;
        modelBuffer = 0;
    }
    if (encodedModel) {
        delete encodedModel;
        encodedModel = 0;
    }
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::sendIncumbent()
{
    int position = 0;
    int size = 
	model_->AlpsPar()->entry(AlpsParams::smallSize);
    
    int mySeq = rankToSequence(incumbentID_, globalRank_);
    int leftSeq = leftSequence(mySeq, processNum_);
    int rightSeq = rightSequence(mySeq, processNum_);

    if (mySeq >= processNum_ || leftSeq >= processNum_ 
	|| rightSeq >= processNum_) {
	std::cout << "sequence is Wrong !!!" << std::endl;
	abort();
    }

    MPI_Pack(&incumbentValue_, 1, MPI_DOUBLE, smallBuffer_, size, &position,
             MPI_COMM_WORLD);
    MPI_Pack(&incumbentID_, 1, MPI_INT, smallBuffer_, size, &position, 
             MPI_COMM_WORLD);
    
    if (leftSeq != -1) {
	int leftRank = sequenceToRank(incumbentID_, leftSeq);
#if 0
	std::cout << "PROC " <<  globalRank_
		  <<" : init send a solution - L,  value = " 
		  << incumbentValue_ << " to "<< leftRank << std::endl; 
#endif
	MPI_Isend(smallBuffer_, position, MPI_PACKED, leftRank, 
                  AlpsMsgIncumbentTwo, MPI_COMM_WORLD, &forwardRequestL_);
	incSendCount("sendIncumbent()");
    }
    
    if (rightSeq != -1) {
	int rightRank = sequenceToRank(incumbentID_, rightSeq);
#if 0
	std::cout << "PROC " <<  globalRank_
		  <<" : init send a solution - R,  value = " 
		  << incumbentValue_ << " to "<< rightRank << std::endl; 
#endif
	MPI_Isend(smallBuffer_, position, MPI_PACKED, rightRank, 
                  AlpsMsgIncumbentTwo, MPI_COMM_WORLD, &forwardRequestR_);
	incSendCount("sendIncumbent()");
    }
}

//#############################################################################

void
AlpsKnowledgeBrokerMPI::collectBestSolution(int destination)
{
    MPI_Status status;
    int sender = incumbentID_;
    int position = 0;

#ifdef NF_DEBUG
    std::cout << "CollectBestSolution: sender=" << sender 
	      << ", destination=" << destination << std::endl;
#endif

    if ( sender == destination ) {
	// DO NOTHING since the best solution is in the solution of its pool
    }
    else {
	if (globalRank_ == sender) {                 // Send solu
	    char* senderBuf = NULL;  // MUST init to NULL, otherwise 
	    int size = -1;           // packEncoded(..) crashes. 

	    const AlpsSolution* solu = static_cast<const AlpsSolution* >
		(getBestKnowledge(AlpsKnowledgeTypeSolution).first);
  
	    double value = getBestKnowledge(AlpsKnowledgeTypeSolution).second;

	    AlpsEncoded* enc = solu->encode();
	    packEncoded(enc, senderBuf, size, position, MPI_COMM_WORLD);
         
	    sendSizeBuf(senderBuf, size, position, destination, 
			AlpsMsgIncumbent, MPI_COMM_WORLD);
	    MPI_Send(&value, 1, MPI_DOUBLE, destination, AlpsMsgIncumbent, 
		     MPI_COMM_WORLD);      
	    if(senderBuf) {
		delete [] senderBuf; 
		senderBuf = NULL;
	    }

            delete enc;
            enc = NULL;
            
#ifdef NF_DEBUG
	    std::cout << "CollectBestSolution: sender " << sender 
		      << " sent solution " << value << std::endl;
#endif
	}
	else if (globalRank_ == destination) {            // Recv solu
	    double value = 0.0;
	    char* destBuf = NULL;  //new char [ls];

	    receiveSizeBuf(destBuf, sender, AlpsMsgIncumbent, 
			   MPI_COMM_WORLD, &status);
	    MPI_Recv(&value, 1, MPI_DOUBLE, sender, AlpsMsgIncumbent,
		     MPI_COMM_WORLD, &status);
            
            position = 0;
	    AlpsEncoded* encodedSolu = unpackEncoded(destBuf, 
                                                     position, 
                                                     MPI_COMM_WORLD);
	    AlpsSolution* bestSolu = static_cast<AlpsSolution* >
		( decoderObject(encodedSolu->type())->decode(*encodedSolu) );
	    
	    addKnowledge(AlpsKnowledgeTypeSolution, bestSolu, value);
	    
	    if(destBuf) {
		delete [] destBuf;     
		destBuf = NULL;
	    }
	    if (encodedSolu) {
		delete encodedSolu;
		encodedSolu = NULL;
	    }
#ifdef NF_DEBUG
	    std::cout << "CollectBestSolution: destination " <<destination 
		      << " received solution " << value << std::endl;
#endif
	}
    }
}

//#############################################################################

void
AlpsKnowledgeBrokerMPI::tellMasterRecv()
{
    char* dummyBuf = 0;
    MPI_Send(dummyBuf, 0, MPI_CHAR, masterRank_, AlpsMsgTellMasterRecv, 
	     MPI_COMM_WORLD);
    incSendCount("tellMasterRecv()");
}

//#############################################################################

void
AlpsKnowledgeBrokerMPI::tellHubRecv()
{
    char* dummyBuf = 0;
    if (globalRank_ == myHubRank_) 
	--hubDoBalance_;
    else {
	MPI_Send(dummyBuf, 0, MPI_CHAR, myHubRank_, AlpsMsgTellHubRecv, 
		 MPI_COMM_WORLD);
	incSendCount("tellHubRecv()");
    }
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::packEncoded(AlpsEncoded* enc, 
				    char*& packBuffer,
				    int& size, 
				    int& position, 
                                    MPI_Comm comm) 
{
    const int bufSpare = model_->AlpsPar()->entry(AlpsParams::bufSpare);
    
    int type = enc->type();
    int repSize = enc->size();

    if(!packBuffer) {
        size = repSize + 2*sizeof(int) + bufSpare;
        packBuffer = new char[size];
    }
    
    // Pack repSize, type_, representation_ of enc
    MPI_Pack(&type, 1, MPI_INT, packBuffer, size, &position, comm);
    MPI_Pack(&repSize, 1, MPI_INT, packBuffer, size, &position, comm);
    //MPI_Pack(const_cast<char*>(type), typeSize, MPI_CHAR, 
    //packBuffer, size, &position, comm); 
    MPI_Pack(const_cast<char*>(enc->representation()), repSize, MPI_CHAR, 
	     packBuffer, size, &position, comm);
}

//#############################################################################

AlpsEncoded* 
AlpsKnowledgeBrokerMPI::unpackEncoded(char*& unpackBuffer, 
                                      int& position, 
                                      MPI_Comm comm,
                                      int size) 
{
    int type, repSize;
    AlpsEncoded *encoded = NULL;
    
    if (size <= 0) {
	size = largeSize_;
    }
    
    // Unpack typeSize, repSize, type and rep from unpackBuffer_
    MPI_Unpack(unpackBuffer, size, &position, &type, 1, MPI_INT, comm);
    
#if defined(NF_DEBUG_MORE)
    std::cout << "PROC "<< globalRank_ <<" : typeSize is " 
	      << typeSize << ";\tsize = "<< size <<  std::endl;
#endif

    MPI_Unpack(unpackBuffer, size, &position, &repSize, 1, MPI_INT, comm);
  
    char *rep = new char[repSize + 1];    
    MPI_Unpack(unpackBuffer, size, &position, rep, repSize, MPI_CHAR, comm);
    rep[repSize] = '\0';

#if defined(NF_DEBUG_MORE)
    std::cout << "PROC "<< globalRank_ << ": type = " << type 
	      << "; repSize = " << repSize << std::endl;
#endif

    // NOTE: Take over the memory of rep, but not type.
    encoded = new AlpsEncoded(type, repSize, rep );

    return encoded;
}

//#############################################################################

void
AlpsKnowledgeBrokerMPI::receiveSizeBuf(char*& buf, 
				       int sender, 
				       int tag, 
				       MPI_Comm comm, 
				       MPI_Status* status) 
{
    int size = -1;

    // First recv the msg size
    MPI_Recv(&size, 1, MPI_INT, sender, MPI_ANY_TAG, comm, status);
    if (status->MPI_TAG == AlpsMsgFinishInit)
	return;
  
    if (size < 0) {
	throw CoinError("size < 0", "receiveSizeBuf", 
			"AlpsKnowledgeBrokerMPI");
    }

    // Second, allocate memory for buf
    if (buf != 0) {
	delete [] buf;  buf = 0;
    }
    buf = new char [size];

    // Third, receive MPI packed data and put in buf
    MPI_Recv(buf, size, MPI_PACKED, sender, tag, comm, status);
}

//#############################################################################

// Block receive a node and store in rampUpSubTree_
// Used in rampup
void 
AlpsKnowledgeBrokerMPI::receiveRampUpNode(int sender,
					  MPI_Comm comm, 
					  MPI_Status* status) 
{
    char* buf = 0;
    int position = 0;

    receiveSizeBuf(buf, sender, AlpsMsgNode, comm, status);
 
    if (status->MPI_TAG == AlpsMsgFinishInit ) {
	//std::cout << "PROC: " << globalRank_ 
	//	  <<" : rec AlpsMsgFinishInit... STOP INIT." << std::endl;
    } 
    else if (status->MPI_TAG == AlpsMsgNode) {
	AlpsEncoded* encodedNode = unpackEncoded(buf, position, comm);
 
#ifdef NF_DEBUG
	std::cout << "WORKER: received and unpacked a node." << std::endl; 
	std::cout << "WORKER: type() is " << encodedNode->type() << std::endl;
	std::cout << "WORKER: finish unpacking node." << std::endl; 
	std::cout << "WORKER: start to decode node." << std::endl; 

	//    const AlpsTreeNode* bugNode = dynamic_cast<const AlpsTreeNode* >(
	//	  AlpsKnowledge::decoderObject(encodedNode->type()));
	const AlpsTreeNode* bugNode = dynamic_cast<const AlpsTreeNode* >
	    ( decoderObject(encodedNode->type()) );

	std::cout <<"WORKER: bugNode's Priority = " 
		  << bugNode->getQuality() << std::endl;
	std::cout << "WORKER: finish just decoding node." << std::endl; 
#endif

	AlpsTreeNode* node = dynamic_cast<AlpsTreeNode* >
	    ( decoderObject(encodedNode->type())->decode(*encodedNode) );

#ifdef NF_DEBUG_MORE
	std::cout << "WORKER " << globalRank_ << " : received a node from " 
		  << sender << std::endl; 

#endif
	// Make the node as a subtree root, then add it to the 
	// local node pool

        //node->setSubTree(st);
	node->setKnowledgeBroker(this); 
	node->modifyDesc()->setModel(model_);
	node->setParent(NULL);

	// Do not want to do this, parent index is unique.
	//node->setParentIndex(-1);

	// Can not do this since there are other status.
	// node->setStatus(AlpsNodeStatusCandidate);

	rampUpSubTree_->nodePool()->addKnowledge(node, node->getQuality());
	assert(rampUpSubTree_->getNumNodes() > 0);
	if ( (rampUpSubTree_->nodePool()->getNumKnowledges() ) == 1) {
	    // Make the first node as root.
	    rampUpSubTree_->setRoot(node);
        }

	if (encodedNode) {
	    delete encodedNode;
	    encodedNode = 0;
	}
        node = NULL;
    } 
    else {
	std::cout << "PROC " << globalRank_ << " : receiveRampUpNode"
		  << " : received UNKNOWN message: tag = " << status->MPI_TAG 
		  << "; sender = " << status->MPI_SOURCE <<std::endl;
	throw CoinError("Unknow message type", 
			"receiveSizeNode()", "AlpsKnowledgeBrokerMPI");
    }

    if (buf != 0) {
	delete [] buf; 
	buf = 0;
    }

}

//#############################################################################

void  
AlpsKnowledgeBrokerMPI::receiveSubTree(char*& bufLarge,
				       int sender,
				       MPI_Status* status)
{
#ifdef NF_DEBUG_MORE
    std::cout << "WORKER " << globalRank_ 
	      << " : start to receive a subtree from " << sender 
	      << "; num of subtrees = " 
	      << getNumKnowledges(AlpsKnowledgeTypeSubTree)
              << "; hasKnowlege() = "
	      << subTreePool_->hasKnowledge() <<std::endl;
#endif

    int count = 0;
    int position = 0;
    
    MPI_Get_count(status, MPI_PACKED, &count);

    if (count <= 0) {

#ifdef NF_DEBUG_MORE
	std::cout << "PROC " << globalRank_ 
		  << " : ask for a subtree but receive nothing from "
		  << status->MPI_SOURCE << std::endl;
#endif
	return;
    }
    
    if (status->MPI_TAG == AlpsMsgSubTree ||
	status->MPI_TAG == AlpsMsgSubTreeByMaster ||
	status->MPI_TAG == AlpsMsgSubTreeByWorker) {
	
	AlpsEncoded* encodedST = unpackEncoded(bufLarge, 
                                               position, 
                                               MPI_COMM_WORLD);
	AlpsSubTree* tempST = dynamic_cast<AlpsSubTree*>
	    (const_cast<AlpsKnowledge *>
	     (decoderObject(AlpsKnowledgeTypeSubTree)))->
	    newSubTree();

	tempST->setKnowledgeBroker(this);
	tempST->setNodeSelection(nodeSelection_);	
        
	AlpsSubTree* subTree = 
	    dynamic_cast<AlpsSubTree* >(tempST->decode(*encodedST) );

	assert(subTree->getNumNodes() > 0);

	subTree->calculateQuality();
	addKnowledge(AlpsKnowledgeTypeSubTree, subTree, subTree->getQuality());


#if 0
	std::cout << "WORKER " << globalRank_ << " : received a subtree from " 
		  << sender << " of size " << count
		  << "; num of subtrees = "
		  << getNumKnowledges(AlpsKnowledgeTypeSubTree) <<std::endl; 
#endif

	if (encodedST) {
	    delete encodedST;
	    encodedST = 0;
	}
	if (tempST) {
	    delete tempST;
	    tempST = 0;
	}
	subTree = 0;
	
    } 
    else {
	std::cout << "PROC " << globalRank_ << " : receiveSubTree"
		  <<" : received UNKNOWN message, tag = " << status->MPI_TAG 
		  << ", source = " << status->MPI_SOURCE <<std::endl; 
	throw CoinError("Unknow message type", 
			"receiveSubTree()", "AlpsKnowledgeBrokerMPI");
    }

}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::sendSizeBuf(char*& buf, 
				    int size, 
				    int position,
				    const int target, 
				    const int tag, 
				    const MPI_Comm comm) 
{
    // If packing successfully, send buf size and buf to target
    if (size >= 0) {
	MPI_Send(&size, 1, MPI_INT, target, AlpsMsgSize, comm);
	MPI_Send(buf, position, MPI_PACKED, target, tag, comm); 
    }
    else
	throw CoinError("Msg size is < 0", "send", "AlpsKnowledgeBrokerMPI");
}

//#############################################################################

// Send a node from rampUpSubTree's node pool.
// NOTE: comm can be hubComm_ or clusterComm_. 
void 
AlpsKnowledgeBrokerMPI::sendRampUpNode(const int receiver, MPI_Comm comm) 
{
    char* buf = 0;
    int size = 0;
    int position = 0;

    AlpsTreeNode* node = dynamic_cast<AlpsTreeNode* >
	(rampUpSubTree_->nodePool()->getKnowledge().first);

    AlpsEncoded* enc = node->encode();
    
    rampUpSubTree_->nodePool()->popKnowledge();

    delete node;   // Since sending to other process
    
    packEncoded(enc, buf, size, position, comm);
    sendSizeBuf(buf, size, position, receiver, AlpsMsgNode, comm);

    if (buf) {
	delete [] buf; 
	buf = 0;
    }

    if (enc) {
	delete enc;  
	enc = 0;                 // Allocated in encode()
    }
}

//#############################################################################

// Send a node from rampUpSubTree's node pool and generated model knowledge
void 
AlpsKnowledgeBrokerMPI::sendNodeModelGen(int receiver, int doUnitWork) 
{
    int position = 0;
    AlpsEncoded* enc = NULL;
    
    // Pack if doUnitWork
    MPI_Pack(&doUnitWork, 1, MPI_INT, largeBuffer_, largeSize_, &position,
             MPI_COMM_WORLD);
    
    // Pack a node
    AlpsTreeNode* node = dynamic_cast<AlpsTreeNode* >
	(rampUpSubTree_->nodePool()->getKnowledge().first);
    enc = node->encode();
    rampUpSubTree_->nodePool()->popKnowledge();
    delete node;   // Since sending to other process
    
    packEncoded(enc, largeBuffer_, largeSize_, position, MPI_COMM_WORLD);
    delete enc; 
    enc = NULL;

    // Pack generated model knowledge
    int hasKnowledge = 0;
    enc = model_->packSharedKnowlege();
    if (enc) {
        hasKnowledge = 1;
        // Pack flag indicating that there is model knowledge
        MPI_Pack(&hasKnowledge, 1, MPI_INT, largeBuffer_, largeSize_, 
                 &position, MPI_COMM_WORLD);
        // pack knowledge
        packEncoded(enc, largeBuffer_, largeSize_, position, MPI_COMM_WORLD);
        delete enc; 
        enc = NULL;
    }
    else {
        // Pack flag indicating that there is no model knowledge
        MPI_Pack(&hasKnowledge, 1, MPI_INT, largeBuffer_, largeSize_, 
                 &position, MPI_COMM_WORLD);
    }
    
    MPI_Send(largeBuffer_, position, MPI_PACKED, receiver, AlpsMsgNode, 
             MPI_COMM_WORLD);
    
}

//#############################################################################

// return sent or not.
bool
AlpsKnowledgeBrokerMPI::sendSubTree(const int receiver,
				    AlpsSubTree*& st,
				    int tag)
{
#ifdef NF_DEBUG
    std::cout << "WORKER["<< globalRank_ 
	      << "]: start to donate a subtree to PROC " << receiver 
              << std::endl;
#endif

    bool success = false;
    char* buf = 0;
    int size = 0;
    int position = 0;

    AlpsEncoded* enc = st->encode();
    packEncoded(enc, buf, size, position, MPI_COMM_WORLD);

#if 0
    std::cout << "WORKER["<< globalRank_ 
	      << "]: donor a subtree to PROC " << receiver
	      << "; buf pos = " << position 
	      << "; buf size = " << size 
	      << "; largeSize_ = " << largeSize_ <<  std::endl; 
#endif

    assert(size > 0);
	
    if (size <= largeSize_) {
	// Attach buffer 
#if 0
	//MPI_Bsend(buf, position, MPI_PACKED, receiver, tag, MPI_COMM_WORLD);
	MPI_Send(buf, position, MPI_PACKED, receiver, tag, MPI_COMM_WORLD);
#endif
	if (!attachBuffer_) {
	    int attachSize = largeSize_* 6 + MPI_BSEND_OVERHEAD;
	    attachBuffer_ =  new char [attachSize];
	    MPI_Buffer_attach(attachBuffer_, attachSize);
	}
	MPI_Ibsend(buf, position, MPI_PACKED, receiver, tag, 
		   MPI_COMM_WORLD, &subTreeRequest_);

        success = true;
    }
    else {
        // msg size is larger than buffer size. Must not happen.
	success = false;
	std::cout << "WARNING: Subtree size is larger than message buffer size, will split it." << std::endl;
    }
    
    if (buf) {
	delete [] buf;
	buf = 0;
    }
    if (enc) {
	delete enc;  
	enc = 0;                 // Allocated in encode()
    }

    return success;
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::sendFinishInit(const int receiver, 
				       MPI_Comm comm) 
{
    char* dummyBuf = 0;
    MPI_Send(dummyBuf, 0, MPI_PACKED, receiver, AlpsMsgFinishInit, comm);
}

//#############################################################################

void AlpsKnowledgeBrokerMPI::deleteSubTrees()
{
    if (workingSubTree_) {
        delete workingSubTree_;
        workingSubTree_ = NULL;
        needWorkingSubTree_ = true;
    }
    subTreePool_-> deleteGuts();
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::initializeSearch(int argc, 
					 char* argv[], 
					 AlpsModel& model) 
{

    //------------------------------------------------------
    // Store a pointer to model.
    //------------------------------------------------------

    model.setKnowledgeBroker(this);
    model_ = &model;
    
    //------------------------------------------------------
    // Init msg env.
    //------------------------------------------------------

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &globalRank_);
    MPI_Comm_size(MPI_COMM_WORLD, &processNum_);

    // CORRECTME
    // NOTE: masterRank_ is 0 or 1 (debug). Must smaller than cluster size.
    masterRank_ = 0;
 
    int color, i;
    int key   = globalRank_;

    //------------------------------------------------------
    // Register knowledge before broadcasting model.
    //------------------------------------------------------

    model.registerKnowledge();

    //------------------------------------------------------
    // Master read in parameters and model data.
    //------------------------------------------------------

    if (globalRank_ == masterRank_) {

	//--------------------------------------------------
	// Read in params.
	//--------------------------------------------------
	
	model.readParameters(argc, argv);
        
        msgLevel_ = model_->AlpsPar()->entry(AlpsParams::msgLevel);
        hubNum_ = model_->AlpsPar()->entry(AlpsParams::hubNum);

        if (msgLevel_ > 0) {
            messageHandler()->message(ALPS_P_VERSION, messages())
                << CoinMessageEol;
            messageHandler()->message(ALPS_LAUNCH, messages())
                << processNum_ << CoinMessageEol;
        }

        // 12/20/06, do we need print parameter file name?
        if (msgLevel_ > 100) {
            messageHandler()->message(ALPS_PARAMFILE, messages())
                << argv[2] << CoinMessageEol;
        }
        
	//--------------------------------------------------
	// Set up logfile if requred.
	//--------------------------------------------------

	logFileLevel_ = model_->AlpsPar()->entry(AlpsParams::logFileLevel);
	if (logFileLevel_ > 0) {    // Require log file
	    logfile_ = model_->AlpsPar()->entry(AlpsParams::logFile);
	}

	//--------------------------------------------------	
	// Read in model data if required.
	//--------------------------------------------------

	if (argc == 2) {
            // FIXME: only work for mpich, not lam
            model_->AlpsPar()->setEntry(AlpsParams::instance, argv[1]);
	}
    
        std::string dataFile = model_->AlpsPar()->entry(AlpsParams::instance);
        
        if (dataFile != "NONE") {   
            if (msgLevel_ > 0) {
                messageHandler()->message(ALPS_DATAFILE, messages())
                    << dataFile.c_str() << CoinMessageEol;
            }
	    
	    model.readInstance(dataFile.c_str());
	    
	    if (logFileLevel_ > 0) {
		
		std::string fileDir = dataFile;
		std::string::size_type pos1 = 
		    fileDir.rfind ('/', std::string::npos);
		if (pos1 == std::string::npos) {
		    // No /, put at beginning.
		    pos1 = 0;
		    //std::cout << "No /, pos1 = "<< pos1 << std::endl;
		}
		else {
		    //std::cout << "Found /, pos1 = "<< pos1 << std::endl;
		    ++pos1;
		}
		
		std::string::size_type pos2 = fileDir.find (".mps", pos1);
		
		if (pos2 == std::string::npos) {
		    // No .mps
		    //std::cout << "No .mps" << std::endl;
		    
		    pos2 = fileDir.find (".gz", pos1); 
		    if (pos2 == std::string::npos) {
			// No .gz
			pos2 = fileDir.length();
			//std::cout << "No .gz, pos2 = "<< pos2 << std::endl;
		    }
		    else { 
			//std::cout << "Found .gz, pos2 = "<< pos2 << std::endl;
		    }
		}
		
		// Sub-string from pos1 to pos2(not included)
		int length = pos2 - pos1;
		// std::cout << "pos1=" << pos1 <<", pos2="<< pos2
		//        << ", lenght=" << length << std::endl;

		instanceName_ = fileDir.substr(pos1, length);    
		logfile_ = instanceName_ + ".log";
		
		model_->AlpsPar()->setEntry(AlpsParams::logFile,
					    logfile_.c_str());

		std::ofstream logFout(logfile_.c_str());

		logFout << "\n\n================================================"
			<< std::endl;
		logFout << "Problem = " << instanceName_ << std::endl;
		logFout << "Data file = " << dataFile << std::endl;
		logFout << "Log file = " << logfile_ << std::endl;
		std::cout << "Problem = " << instanceName_ << std::endl;
		std::cout << "Log file = " << logfile_ << std::endl;
	    }
	}

	// modifyDataPool()->setAppParams(&userParams);// setupself needs userPar

	model.preprocess();
	model.setupSelf();
	//std::cout << "Here1" << std::endl;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    //------------------------------------------------------
    // Broadcast model.
    //------------------------------------------------------

    broadcastModel(globalRank_, masterRank_);
    
    MPI_Barrier(MPI_COMM_WORLD);

    //------------------------------------------------------
    // Deside the cluster size and actual hubNum_.
    //------------------------------------------------------

    msgLevel_ = model_->AlpsPar()->entry(AlpsParams::msgLevel);
    hubMsgLevel_ = model_->AlpsPar()->entry(AlpsParams::hubMsgLevel);
    workerMsgLevel_ = model_->AlpsPar()->entry(AlpsParams::workerMsgLevel);
    messageHandler()->setLogLevel(msgLevel_);
    hubNum_ = model_->AlpsPar()->entry(AlpsParams::hubNum);

    timer_.limit_ = model_->AlpsPar()->entry(AlpsParams::timeLimit);    

    while(true) {
	if (hubNum_ > 0) {
	    userClusterSize_ = 1;
	    while (userClusterSize_ * hubNum_ < processNum_) {
		++userClusterSize_;
	    }
            // [0,...,cluSize-1] in group 1
	    color = globalRank_ / userClusterSize_; 
	} 
	else {
	    std::cout << "hubNum_ <= 0" << std::endl;
	    throw CoinError("hubNum_ <= 0", 
			    "initSearch",
			    "AlpsKnowledgeBrokerMPI");
	}

        // more than 1 proc in the last cluser
	if (processNum_- userClusterSize_ * (hubNum_ - 1) > 1) {
	    break;
	}
	else {
	    --hubNum_;
	}
    }
    if ( (globalRank_ == masterRank_) && (msgLevel_ > 0) ) {
        messageHandler()->message(ALPS_HUB_NUM, messages())
            << hubNum_ << CoinMessageEol;
    }

    //------------------------------------------------------
    // Create clusterComm_.
    //------------------------------------------------------

    MPI_Comm_split(MPI_COMM_WORLD, color, key, &clusterComm_);
    MPI_Comm_rank(clusterComm_, &clusterRank_);
    MPI_Comm_size(clusterComm_, &clusterSize_);

#if 0
    std::cout << "+++ masterRank_ = " << masterRank_
              << ", clusterSize_ = " << clusterSize_
              << ", userClusterSize_ = " << userClusterSize_ << std::endl;
#endif    

    //------------------------------------------------------
    // Create hubGroup_ and hubComm_.
    //------------------------------------------------------

    hubRanks_ = new int [hubNum_];
    int k = 0;
    for (i = 0; i < processNum_; i += userClusterSize_) {
	hubRanks_[k++] = i + masterRank_;
    }

    MPI_Group worldGroup;
    MPI_Comm_group(MPI_COMM_WORLD, &worldGroup);
    MPI_Group_incl(worldGroup, hubNum_, hubRanks_, &hubGroup_);
    MPI_Comm_create(MPI_COMM_WORLD, hubGroup_, &hubComm_);
    
    //------------------------------------------------------
    // Allcoate index, classify process types, set up knowledge pools
    //   Master  :  [0, INT_MAX/5]
    //   Others  :  divide [INT_MAX/5 + 1, INT_MAX]
    //------------------------------------------------------

    int workerDown = -1, workerUp = -1;
    int masterDown = 0;
    int masterUp = static_cast<int>(ALPS_INT_MAX / 5);
    int workerIndexR = static_cast<int>( static_cast<double>(ALPS_INT_MAX - masterUp)/(processNum_-1));
    workerIndexR -= 2;  // leave some buffer
    
    if (globalRank_ < masterRank_) {
        workerDown = masterUp + globalRank_ * workerIndexR;
        workerUp = workerDown + workerIndexR;
    }
    else if (globalRank_ > masterRank_) {
        workerDown = masterUp + (globalRank_-1) * workerIndexR;        
        workerUp = workerDown + workerIndexR;
    }

    masterIndexBatch_ = masterUp / (processNum_ * 2);

    //------------------------------------------------------
    // Decide process type.
    //------------------------------------------------------

    if (globalRank_ == masterRank_) {
	processType_ = AlpsProcessTypeMaster;
	setNextNodeIndex(masterDown);
	setMaxNodeIndex(masterUp);

#if 0
        std::cout << "masterDown = " << masterDown 
                  << "; masterUp = " << masterUp
                  << "; workerDown = " << workerDown
                  << "; workerUp = " << workerUp
                  << "; masterIndexBatch_ = " << masterIndexBatch_
                  << std::endl;
#endif
    } 
    else if ( (globalRank_ % userClusterSize_) == masterRank_ ) {
	processType_ = AlpsProcessTypeHub;
	setNextNodeIndex(workerDown);
	setMaxNodeIndex(workerUp);
    }
    else {
	processType_ = AlpsProcessTypeWorker;
	setNextNodeIndex(workerDown);
	setMaxNodeIndex(workerUp);
    }

    // Determine if hubs process nodes.
    int hubWorkClusterSizeLimit = 
        model_->AlpsPar()->entry(AlpsParams::hubWorkClusterSizeLimit);
    if (userClusterSize_ > hubWorkClusterSizeLimit) {
        hubWork_ = false;
    }
    else {
        hubWork_ = true;
    }   

    if (!processTypeList_) {
	delete [] processTypeList_;
	processTypeList_ = NULL;
    }
    processTypeList_ = new AlpsProcessType [processNum_];
    for (i = 0; i < processNum_; ++i) {
	if (i == masterRank_) {
	    processTypeList_[i] = AlpsProcessTypeMaster;
	}
	else if ( (i % userClusterSize_) == masterRank_) {
	    processTypeList_[i] = AlpsProcessTypeHub;
	}
	else {
	    processTypeList_[i] = AlpsProcessTypeWorker;
	}
    }

    //------------------------------------------------------
    // Set hub's global rank for workers and hubs.
    //------------------------------------------------------

    if (processType_ == AlpsProcessTypeWorker ||
	processType_ == AlpsProcessTypeHub) {
	
	myHubRank_ = 
            (globalRank_ / userClusterSize_) * userClusterSize_ + masterRank_;
	
#ifdef NF_DEBUG
	std::cout << "PROCESS[" << globalRank_ << "] : my hub rank = " 
		  << myHubRank_ << std::endl; 
#endif
    }

    //------------------------------------------------------
    // Set up knowledge pools.
    //------------------------------------------------------
    
    setupKnowledgePools();
    MPI_Barrier(MPI_COMM_WORLD);

    //------------------------------------------------------
    // Set max number of solutions.
    //------------------------------------------------------

    const int mns = model_->AlpsPar()->entry(AlpsParams::solLimit);
    setMaxNumKnowledges(AlpsKnowledgeTypeSolution, mns);

    //------------------------------------------------------
    // Set clock type
    //------------------------------------------------------

    const bool clockType = 
      model_->AlpsPar()->entry(AlpsParams::clockType);
    
    timer_.setClockType(clockType);
    subTreeTimer_.setClockType(clockType);
    tempTimer_.setClockType(clockType);

    //------------------------------------------------------
    // Allocate memory. TODO.
    //------------------------------------------------------
}

//#############################################################################

/** Search best solution for a given model. */
void AlpsKnowledgeBrokerMPI::search(AlpsModel *model) 
{
    AlpsTreeNode* root = NULL;
    if (getProcRank() == masterRank_) {
	// Only master need create root.
	// NOTE: masterRank_ has been assigned in intializeSearch().
	root = model->createRoot();
    }
    
    rootSearch(root);
}

//#############################################################################

void AlpsKnowledgeBrokerMPI::rootSearch(AlpsTreeNode* root)
{  

    timer_.start();
    
    //------------------------------------------------------
    // Call main functions.
    //------------------------------------------------------

    if (processType_ == AlpsProcessTypeMaster) {
	masterMain(root);
    } 
    else if(processType_ == AlpsProcessTypeHub) {
	hubMain();
    } 
    else if (processType_ == AlpsProcessTypeWorker){
	workerMain();
    }
    else {
        assert(0);
    }

    //------------------------------------------------------
    // Collect best solution.
    //------------------------------------------------------

    MPI_Barrier(MPI_COMM_WORLD);
    collectBestSolution(masterRank_);

    // Search to end.
    if (processType_ == AlpsProcessTypeMaster && 
	exitStatus_ == AlpsExitStatusUnknown) {
        if (hasKnowledge(AlpsKnowledgeTypeSolution)) {
            setExitStatus(AlpsExitStatusOptimal);
        }
        else {
            setExitStatus(AlpsExitStatusInfeasible);
        }
    }
    
    timer_.stop();
    
    //------------------------------------------------------
    // log statistics.
    //------------------------------------------------------

    searchLog();
    if (processType_ == AlpsProcessTypeMaster) {
	model_->postprocess();
	model_->modelLog();
    }
}

//#############################################################################

// Master tell hubs to terminate due to reaching limits or other reason.
void 
AlpsKnowledgeBrokerMPI::masterForceHubTerm()
{
    char* buf = 0;
    forceTerminate_ = true;
    for (int i = 0; i < hubNum_; ++i) {
	if (i != masterRank_) {
	    MPI_Send(buf, 0, MPI_PACKED, hubRanks_[i], 
		     AlpsMsgForceTerm, MPI_COMM_WORLD);
            incSendCount("masterForceHubTerm");
	}
    }
}

//#############################################################################

// Hub tell workers to terminate due to reaching limits or other reason.
void 
AlpsKnowledgeBrokerMPI::hubForceWorkerTerm()
{
    char* buf = 0;
    forceTerminate_ = true;
    for (int i = 0; i < clusterSize_; ++i) {
	if (i != clusterRank_) {
	    MPI_Send(buf, 0, MPI_PACKED, globalRank_+i, 
		     AlpsMsgForceTerm, MPI_COMM_WORLD);
            incSendCount("hubForceWorkerTerm");
	}
    }
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::changeWorkingSubTree(double & changeWorkThreshold)
{
    if ( workingSubTree_ && subTreePool_->hasKnowledge() ) {
        //std::cout << "Process[" << globalRank_ << "here 1" << std::endl;
        workingSubTree_->calculateQuality();
        //std::cout << "Process[" << globalRank_ << "here 2" << std::endl;

        double curQuality = workingSubTree_->getQuality();
        double topQuality = subTreePool_->getBestQuality();
        //double topQuality = subTreePool_->getKnowledge().second;
	
        if (curQuality > topQuality) {
            double perDiff = ALPS_FABS(curQuality - topQuality)/
                (ALPS_FABS(curQuality) + 1.0e-9);
            if (perDiff > changeWorkThreshold) {
                // Need put node in regular node pool.
                workingSubTree_->reset();

                AlpsSubTree* tempST = workingSubTree_;
                workingSubTree_ = dynamic_cast<AlpsSubTree* >(
                    subTreePool_->getKnowledge().first);
                subTreePool_->popKnowledge();
                addKnowledge(AlpsKnowledgeTypeSubTree, tempST, curQuality);
                ++(psStats_.subtreeChange_);
                
                // Avoid too much change.
                if (psStats_.subtreeChange_ / 10 == 0) {
                    changeWorkThreshold *= 2.0;
                }
#if 0
                std::cout << "Process[" << globalRank_ 
                          << "]: change subtree " << curQuality
                          << " to " << topQuality 
                          << ", new working tree nodes " 
                          << workingSubTree_->getNumNodes() 
                          << std::endl;
#endif

            }
        }
    }
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::searchLog()
{
    int i, j;
    int* tSizes = 0;
    int* tLefts = 0;
    int* tDepths = 0;
    double* idles = 0;
    double* msgTimes = 0;
    double* rampUps = 0;
    double* rampDowns = 0;
    double *cpuTimes =  NULL;
    double *wallClocks = NULL;

    int *qualityBalance = 0;
    int *quantityBalance = 0;
    int *interBalance = 0;
    int *intraBalance = 0;
    int *workerAsk = 0;
    int *donateSuccess = 0;
    int *donateFail = 0;
    int *subtreeSplit = 0;
    int *subtreeWhole = 0;
    int *subtreeChange = 0;
    
    if (globalRank_ == masterRank_) {
	tSizes = new int [processNum_];
	tLefts = new int [processNum_];
	tDepths = new int [processNum_];
	idles = new double [processNum_];
	msgTimes = new double [processNum_];
	rampUps = new double [processNum_];
	rampDowns = new double [processNum_];
	cpuTimes = new double [processNum_];
	wallClocks = new double [processNum_];
        qualityBalance = new int [processNum_];
        quantityBalance = new int [processNum_];
        interBalance = new int [processNum_];
        intraBalance = new int [processNum_];
        workerAsk = new int [processNum_];
        donateSuccess = new int [processNum_];
        donateFail = new int [processNum_];
        subtreeSplit = new int [processNum_];
        subtreeWhole = new int [processNum_];
        subtreeChange = new int [processNum_];
    }  

    MPI_Gather(&nodeProcessedNum_, 1, MPI_INT, tSizes, 1, MPI_INT, 
	       masterRank_, MPI_COMM_WORLD);
    MPI_Gather(&nodeLeftNum_, 1, MPI_INT, tLefts, 1, MPI_INT, 
	       masterRank_, MPI_COMM_WORLD);
    MPI_Gather(&treeDepth_, 1, MPI_INT, tDepths, 1, MPI_INT, masterRank_, 
	       MPI_COMM_WORLD);
    MPI_Gather(&idleTime_, 1, MPI_DOUBLE, idles, 1, MPI_DOUBLE, masterRank_,
	       MPI_COMM_WORLD);
    MPI_Gather(&msgTime_, 1, MPI_DOUBLE, msgTimes, 1, MPI_DOUBLE, masterRank_,
	       MPI_COMM_WORLD);
    MPI_Gather(&rampUpTime_, 1, MPI_DOUBLE, rampUps, 1, MPI_DOUBLE, 
	       masterRank_, MPI_COMM_WORLD);
    MPI_Gather(&rampDownTime_, 1, MPI_DOUBLE, rampDowns, 1, MPI_DOUBLE, 
	       masterRank_, MPI_COMM_WORLD);
    MPI_Gather(&(timer_.cpu_), 1, MPI_DOUBLE, cpuTimes, 1, MPI_DOUBLE, 
	       masterRank_, MPI_COMM_WORLD);
    MPI_Gather(&(timer_.wall_), 1, MPI_DOUBLE, wallClocks, 1, MPI_DOUBLE, 
	       masterRank_, MPI_COMM_WORLD);
    MPI_Gather(&(psStats_.qualityBalance_), 1, MPI_INT, qualityBalance, 1,
               MPI_INT, masterRank_, MPI_COMM_WORLD);
    MPI_Gather(&(psStats_.quantityBalance_), 1, MPI_INT, quantityBalance, 1,
                MPI_INT, masterRank_, MPI_COMM_WORLD);
    MPI_Gather(&(psStats_.interBalance_), 1, MPI_INT, interBalance, 1,
               MPI_INT, masterRank_, MPI_COMM_WORLD);
    MPI_Gather(&(psStats_.intraBalance_), 1, MPI_INT, intraBalance, 1,
               MPI_INT, masterRank_, MPI_COMM_WORLD);
    MPI_Gather(&(psStats_.workerAsk_), 1, MPI_INT, workerAsk, 1,
               MPI_INT, masterRank_, MPI_COMM_WORLD);
    MPI_Gather(&(psStats_.donateSuccess_), 1, MPI_INT, donateSuccess, 1,
               MPI_INT, masterRank_, MPI_COMM_WORLD);
    MPI_Gather(&(psStats_.donateFail_), 1, MPI_INT, donateFail, 1,
               MPI_INT, masterRank_, MPI_COMM_WORLD);
    MPI_Gather(&(psStats_.subtreeSplit_), 1, MPI_INT, subtreeSplit, 1,
               MPI_INT, masterRank_, MPI_COMM_WORLD);
    MPI_Gather(&(psStats_.subtreeWhole_), 1, MPI_INT, subtreeWhole, 1,
               MPI_INT, masterRank_, MPI_COMM_WORLD);
    MPI_Gather(&(psStats_.subtreeChange_), 1, MPI_INT, subtreeChange, 1,
               MPI_INT, masterRank_, MPI_COMM_WORLD);

    if (processType_ == AlpsProcessTypeMaster) {
	int numWorkers = 0;   // Number of process are processing nodes.
	
	int sumSize = 0, aveSize = 0, maxSize = 0, minSize = ALPS_INT_MAX;
	int sumDep = 0, aveDep = 0, maxDep = 0, minDep = ALPS_INT_MAX; 
	
	double sumIdle = 0.0, aveIdle = 0.0;
	double maxIdle = 0.0, minIdle = ALPS_DBL_MAX;

	double sumMsgTime = 0.0, aveMsgTime = 0.0;
	double maxMsgTime = 0.0, minMsgTime = ALPS_DBL_MAX;
	
	double sumRampUp = 0.0, aveRampUp = 0.0;
	double maxRampUp = 0.0, minRampUp = ALPS_DBL_MAX;
	
	double sumRampDown = 0.0, aveRampDown = 0.0;
	double maxRampDown = 0.0, minRampDown = ALPS_DBL_MAX;
	
	double sumCpuTime = 0.0, aveCpuTime = 0.0;
	double maxCpuTime = 0.0, minCpuTime = ALPS_DBL_MAX;
	
	double sumWallClock = 0.0, aveWallClock = 0.0;
	double maxWallClock = 0.0, minWallClock = ALPS_DBL_MAX;
	
	double varSize = 0.0, stdSize = 0.0;
	double varDep = 0.0, stdDep = 0.0;
	double varIdle = 0.0, stdIdle = 0.0;
	double varMsgTime = 0.0, stdMsgTime = 0.0;
	double varRampUp = 0.0, stdRampUp = 0.0;
	double varRampDown = 0.0, stdRampDown = 0.0;
	
	int totalTreeSize = 0;
	int totalNodeLeft = 0;

        int sumQualityBalance = 0;
        int sumQuantityBalance = 0;
        int sumInterBalance = 0;
        int sumIntraBalance = 0;
        int sumWorkerAsk = 0;
        int sumDonateSuccess = 0;
        int sumDonateFail = 0;
        int sumSubtreeSplit = 0;
        int sumSubtreeWhole = 0;
        int sumSubtreeChange = 0;
    
	if(logFileLevel_ > 0 || msgLevel_ > 0) {
	    for(i = 0; i < processNum_; ++i) {
		totalTreeSize += tSizes[i];
		totalNodeLeft += tLefts[i];
		
		sumRampUp += rampUps[i];
		sumCpuTime += cpuTimes[i];
		sumWallClock += wallClocks[i];

                sumQualityBalance += qualityBalance[i];
                sumQuantityBalance += quantityBalance[i];
                sumInterBalance += interBalance[i];
                sumIntraBalance += intraBalance[i];
                sumWorkerAsk += workerAsk[i];
                sumDonateSuccess += donateSuccess[i];
                sumDonateFail += donateFail[i];
                sumSubtreeSplit += subtreeSplit[i];
                sumSubtreeWhole += subtreeWhole[i];
                sumSubtreeChange += subtreeChange[i];
                
		// Only valid for workers.
		if (processTypeList_[i] == AlpsProcessTypeWorker) {
		    ++numWorkers;
		    
		    sumSize += tSizes[i];
		    sumDep += tDepths[i];
		    sumIdle += idles[i];
		    sumMsgTime += msgTimes[i];
		    sumRampDown += rampDowns[i];
		    
		    if (tSizes[i] > maxSize) maxSize = tSizes[i];
		    if (tSizes[i] < minSize) minSize = tSizes[i];
                    
		    if (tDepths[i] > maxDep) maxDep = tDepths[i];
		    if (tDepths[i] < minDep) minDep = tDepths[i];

		    if (idles[i] > maxIdle) maxIdle = idles[i];
		    if (idles[i] < minIdle) minIdle = idles[i];

		    if (msgTimes[i] > maxMsgTime) maxMsgTime = msgTimes[i];
		    if (msgTimes[i] < minMsgTime) minMsgTime = msgTimes[i];

		    if (rampDowns[i] > maxRampDown) maxRampDown = rampDowns[i];
		    if (rampDowns[i] < minRampDown) minRampDown = rampDowns[i];
		}
		
		if (cpuTimes[i] > maxCpuTime) maxCpuTime = cpuTimes[i];
		if (cpuTimes[i] < minCpuTime) minCpuTime = cpuTimes[i];
		
		if (wallClocks[i] > maxWallClock) maxWallClock = wallClocks[i];
		if (wallClocks[i] < minWallClock) minWallClock = wallClocks[i];
		
		if (rampUps[i] > maxRampUp) maxRampUp = rampUps[i];
		if (rampUps[i] < minRampUp) minRampUp = rampUps[i];
	    }
           
#if 0
            // Does not make sense, 2/24/07
	    for(i = 0; i < processNum_; ++i) { // Adjust idle and rampDown 
		if (tSizes[i] > 0) {           // due to periodically checking.
		    //idles[i] -= minIdle;
		    //rampDowns[i] -= minRampDown;
		}
	    }
	    sumIdle -= minIdle * numWorkers;
	    sumRampDown -= minRampDown * numWorkers;
	    maxIdle -= minIdle;
	    minIdle = 0.0;
	    maxRampDown -= minRampDown;
	    minRampDown = 0.0;	    
#endif            

	    if( numWorkers != 0) {
		aveSize = sumSize / numWorkers; 
		aveDep = sumDep / numWorkers;
		aveIdle = sumIdle / numWorkers;
		aveMsgTime = sumMsgTime / numWorkers;
		aveRampDown = sumRampDown / numWorkers;
	    }
            aveRampUp = sumRampUp / processNum_;
            
	    for (i = 0; i < processNum_; ++i) {
		if (processTypeList_[i] == AlpsProcessTypeWorker) {
		    varSize += pow(tSizes[i] - aveSize, 2);
		    varDep += pow(tDepths[i] - aveDep, 2);
		    varIdle = pow(idles[i] - aveIdle, 2);
		    varMsgTime = pow(msgTimes[i] - aveMsgTime, 2);
		    varRampDown = pow(rampDowns[i] - aveRampDown, 2);
		}
		varRampUp = pow(rampUps[i] - aveRampUp, 2);
	    }
	    if( numWorkers != 0) {
		varSize /= numWorkers;
		stdSize = sqrt(varSize);
		varDep /= numWorkers;
		stdDep = sqrt(varDep);
		varIdle /= numWorkers;
		stdIdle = sqrt(varIdle);
		varMsgTime /= numWorkers;
		stdMsgTime = sqrt(varMsgTime);
		varRampDown /= numWorkers;
		stdRampDown = sqrt(varRampDown);
		varRampUp /= numWorkers;
	    }
	    stdRampUp = sqrt(varRampUp);
            aveCpuTime = sumCpuTime/processNum_;
            aveWallClock = sumWallClock/processNum_;
        }

        if (logFileLevel_ > 0) {
	    std::ofstream logFout(logfile_.c_str(), std::ofstream::app);
	    
	    logFout << "Number of processes = " << processNum_ << std::endl;
	    logFout << "Number of hubs = " << hubNum_ << std::endl;

	    //----------------------------------------------
	    // Tree size and depth.
	    //----------------------------------------------

	    logFout << "Number of nodes processed = " 
		    << totalTreeSize << std::endl;
            logFout << "Number of nodes left = " 
		    << totalNodeLeft << std::endl;
	    logFout << "Max number of nodes processed by a worker = "
		    << maxSize << std::endl;
	    logFout << "Min number of nodes processed by a worker = "
		    << (minSize != ALPS_INT_MAX ? minSize : 0) << std::endl;
	    logFout << "Std Dev of number of nodes processed by a worker = "
		    << stdSize << std::endl;
	    logFout << std::endl << "Max Tree Depth on workers = " 
		    << maxDep << std::endl;
	    logFout << "Min Tree Depth on workers = " 
		    << (minDep != ALPS_INT_MAX ? minDep : 0) << std::endl;
	    logFout << "Std Dev of Tree Depth on workers = " 
		    << stdDep << std::endl;
            
	    if (logFileLevel_ > 0) {
		j = 0;
		logFout << std::endl 
			<< "Numbers of Nodes processed by processes: " 
			<< std::endl;
		for (i = 0; i < processNum_; ++i) { 
		    ++j;
		    if (j % 5 == 0) logFout << std::endl;
		    logFout << tSizes[i] << "\t";
		}

		j = 0;
		logFout << std::endl << std::endl 
			<< "Tree depths on processes: " 
			<< std::endl;
		for (i = 0; i < processNum_; ++i) {
		    ++j;
		    if (j % 5 == 0) logFout << std::endl;
		    logFout << tDepths[i] << "\t";
		}
		logFout << std::endl << std::endl;
	    } // Log if logFileLevel_ > 0
	
	    //----------------------------------------------
	    // Times.
	    //----------------------------------------------

	    // Ramp up.
	    logFout << "Average RampUp = " << aveRampUp << std::endl;
	    logFout << "Max RampUp = " << maxRampUp << std::endl;
	    logFout << "Min RampUp = " 
		    << (minRampUp != ALPS_DBL_MAX ? minRampUp : 0.0) 
		    << std::endl;
	    logFout << "Std Dev of RampUp = " << stdRampUp << std::endl;

	    if (logFileLevel_ > 0) {
		logFout << std::endl << "Ramp ups of processes: " << std::endl;
		for (i = 0; i < processNum_; ++i) { 
		    if (i % 5 == 0)
			logFout << std::endl;
		    logFout << rampUps[i] << "\t";
		}
		logFout << std::endl;
	    }

	    // Idle.
	    logFout << "Average Idle = " << aveIdle << std::endl;
	    logFout << "Max Idle = " << maxIdle << std::endl;
	    logFout << "Min Idle = " 
		    << (minIdle != ALPS_DBL_MAX ? minIdle : 0.0) << std::endl;
	    logFout << "Std Dev of Idle = " << stdIdle << std::endl;

	    // Msg time.
	    logFout << "Average MsgTime = " << aveMsgTime << std::endl;
	    logFout << "Max MsgTime = " << maxMsgTime << std::endl;
	    logFout << "Min MsgTime = " 
		    << (minMsgTime != ALPS_DBL_MAX ? minMsgTime : 0.0) 
                    << std::endl;
	    logFout << "Std Dev of MsgTime = " << stdMsgTime << std::endl;
	    
	    // Ramp down.
	    logFout << "Average RampDown = " << aveRampDown << std::endl;
	    logFout << "Max RampDown = " << maxRampDown << std::endl;
	    logFout << "Min RampDown = " 
		    << (minRampDown != ALPS_DBL_MAX ? minRampDown : 0.0) 
		    << std::endl;
	    logFout << "Std Dev of RampDown = " << stdRampDown << std::endl;
	    
	    // Overall. 
	    logFout << "Search CPU time = " << timer_.getCpuTime() <<" seconds"
		    << ", max = " << maxCpuTime
		    << ", min = "<< minCpuTime 
		    << ", total CPU time = " << sumCpuTime << std::endl;
	    logFout <<"Search wallclock  = "<<timer_.getWallClock()<<" seconds"
		    << ", max = " << maxWallClock
		    << ", min = "<< minWallClock 
		    <<", total wallclock = " << sumWallClock << std::endl;

	    //----------------------------------------------
	    // Solution.
	    //----------------------------------------------

	    logFout << "Best solution quality = " << getBestQuality()
                    << " ; node required = " << bestSolNode_;
            logFout << std::endl;
	    if (hasKnowledge(AlpsKnowledgeTypeSolution) ) {
		dynamic_cast<AlpsSolution* >
		    (getBestKnowledge(AlpsKnowledgeTypeSolution).first)->print(logFout);
	    }
            
	}  // Log if logFileLevel_ > 0

        if (msgLevel_ > 0) {
            if (getSolStatus() == AlpsExitStatusOptimal) {
                messageHandler()->message(ALPS_T_OPTIMAL, messages())
                    << systemNodeProcessed_
		    << static_cast<int>(systemWorkQuantity_) 
                    << CoinMessageEol;
            }
            else if (getSolStatus() == AlpsExitStatusNodeLimit) {
		if (forceTerminate_) {
		    messageHandler()->message(ALPS_T_NODE_LIMIT, messages())
			<< systemNodeProcessed_ 
			<< static_cast<int>(systemWorkQuantityForce_) 
			<< CoinMessageEol;
		}
		else {
		    messageHandler()->message(ALPS_T_NODE_LIMIT, messages())
			<< systemNodeProcessed_ 
			<< static_cast<int>(systemWorkQuantity_) 
			<< CoinMessageEol;
		}
            }
            else if (getSolStatus() == AlpsExitStatusTimeLimit) {
#if 0
		std::cout << "------ forceTerminate_=" << forceTerminate_ 
			  << ", systemWorkQuantityForce_ = "
			  << static_cast<int>(systemWorkQuantityForce_) 
			  << std::endl;
#endif
		
		if (forceTerminate_) {
		    messageHandler()->message(ALPS_T_TIME_LIMIT, messages())
			<< systemNodeProcessed_ 
			<< static_cast<int>(systemWorkQuantityForce_)
			<< CoinMessageEol; 
		}
		else  {
		  messageHandler()->message(ALPS_T_TIME_LIMIT, messages())
		      << systemNodeProcessed_ 
			<< static_cast<int>(systemWorkQuantity_)
			<< CoinMessageEol;
		}
            }
            else if (getSolStatus() == AlpsExitStatusFeasible) {
                messageHandler()->message(ALPS_T_FEASIBLE, messages())
                    << systemNodeProcessed_ 
		    << static_cast<int>(systemWorkQuantity_)
                    << CoinMessageEol;
            }
            else if (getSolStatus() == AlpsExitStatusNoMemory) {
                messageHandler()->message(ALPS_T_NO_MEMORY, messages())
                    << systemNodeProcessed_
		    << static_cast<int>(systemWorkQuantity_)
                    << CoinMessageEol;
            }
            else if (getSolStatus() == AlpsExitStatusFailed) {
                messageHandler()->message(ALPS_T_FAILED, messages())
                    << systemNodeProcessed_
		    << static_cast<int>(systemWorkQuantity_)
                    << CoinMessageEol;
            }
            else {
                messageHandler()->message(ALPS_T_INFEASIBLE, messages())
                    << systemNodeProcessed_ 
		    << static_cast<int>(systemWorkQuantity_)
                    << CoinMessageEol;
            }

            std::cout << "\n******************* SEARCH RESULTS ******************"
                      << std::endl;

            //----------------------------------------------
	    // Tree size, depth, overheads.
	    //----------------------------------------------

	    std::cout << "Number of nodes processed = " 
                      << totalTreeSize << std::endl;
            std::cout << "Number of nodes left = " 
                      << totalNodeLeft << std::endl;
	    std::cout << "Max number of nodes processed by a worker = "
                      << maxSize << std::endl;
	    std::cout << "Min number of nodes processed by a worker = "
                      << (minSize != ALPS_INT_MAX ? minSize : 0) << std::endl;
	    std::cout << "Std Dev of number of nodes processed by a worker = "
                      << stdSize << std::endl;
            std::cout << "----------------------------------" << std::endl;

            std::cout << "Average tree depth = " << aveDep << std::endl;
            std::cout << "Max tree depth on workers = " << maxDep << std::endl;
	    std::cout << "Min tree depth on workers = "
                      << (minDep != ALPS_INT_MAX ? minDep : 0) << std::endl;
	    std::cout << "Std Dev of tree depth on workers = " << stdDep 
                      << std::endl;
            std::cout << "----------------------------------" << std::endl;

	    // Ramp up.
	    std::cout << "Average RampUp = " << aveRampUp << std::endl;
	    std::cout << "Max RampUp = " << maxRampUp << std::endl;
	    std::cout << "Min RampUp = " 
                      << (minRampUp != ALPS_DBL_MAX ? minRampUp : 0.0) 
                      << std::endl;
	    std::cout << "Std Dev of RampUp = " << stdRampUp << std::endl;
            std::cout << "----------------------------------" << std::endl;

	    // Idle.
	    std::cout << "Average Idle = " << aveIdle << std::endl;
	    std::cout << "Max Idle = " << maxIdle << std::endl;
	    std::cout << "Min Idle = " 
                      << (minIdle != ALPS_DBL_MAX ? minIdle : 0.0) 
                      << std::endl;
	    std::cout << "Std Dev of Idle = " << stdIdle << std::endl;
            std::cout << "----------------------------------" << std::endl;

	    // MsgTime.
	    std::cout << "Average MsgTime = " << aveMsgTime << std::endl;
	    std::cout << "Max MsgTime = " << maxMsgTime << std::endl;
	    std::cout << "Min MsgTime = " 
                      << (minMsgTime != ALPS_DBL_MAX ? minMsgTime : 0.0) 
                      << std::endl;
	    std::cout << "Std Dev of MsgTime = " << stdMsgTime << std::endl;
            std::cout << "----------------------------------" << std::endl;
	    
	    // Ramp down.
	    std::cout << "Average RampDown = " << aveRampDown << std::endl;
	    std::cout << "Max RampDown = " << maxRampDown << std::endl;
	    std::cout << "Min RampDown = " 
                      << (minRampDown != ALPS_DBL_MAX ? minRampDown : 0.0) 
                      << std::endl;
	    std::cout << "Std Dev of RampDown = " << stdRampDown << std::endl;
            std::cout << "----------------------------------" << std::endl;

            //----------------------------------------------
	    // Load balancing
	    //----------------------------------------------

            std::cout << "Quality balance = " << sumQualityBalance 
                      << std::endl;
            std::cout << "Quantity balance = " << sumQuantityBalance 
                      << std::endl;
            std::cout << "Inter-balance = " << sumInterBalance 
                      << std::endl;
            std::cout << "Intra-balance = " << sumIntraBalance 
                      << std::endl;
            std::cout << "Worker asked workload = " << sumWorkerAsk 
                      << std::endl;
            std::cout << "Worker donate success = " << sumDonateSuccess 
                      << std::endl;
            std::cout << "Worker donate fail = " << sumDonateFail 
                      << std::endl;
            std::cout << "Subtree split = " << sumSubtreeSplit
                      << std::endl;
            std::cout << "Subtree whole = " << sumSubtreeWhole 
                      << std::endl;
            std::cout << "Subtree change = " << sumSubtreeChange 
                      << std::endl;
            
            std::cout << "----------------------------------" << std::endl;

            //----------------------------------------------
	    // Overall. 
	    //----------------------------------------------           

	    std::cout << "Search CPU time = "<<timer_.getCpuTime() <<" seconds"
		      << ", max = " << maxCpuTime
		      << ", min = "<< minCpuTime 
		      << ", total CPU time = " << sumCpuTime << std::endl;
	    std::cout << "Search wallclock  = "<<timer_.getWallClock()
                      <<" seconds"
		      << ", max = " << maxWallClock
		      << ", min = "<< minWallClock 
		      <<", total wallclock = " << sumWallClock << std::endl;
            
            std::cout << "Best solution quality = " << getBestQuality()
                      << " ; node required = " << bestSolNode_
                      << std::endl;
            std::cout << "*****************************************************"
                      << std::endl;
        } // Print msg if msgLevel_ > 0

    }  // Only master log

    if (globalRank_ == masterRank_) {
	delete [] tSizes;     tSizes = 0;
	delete [] tLefts;     tLefts = 0;
	delete [] tDepths;    tDepths = 0;
	delete [] idles;      idles = 0;
	delete [] rampUps;    rampUps = 0;
	delete [] rampDowns;  rampDowns = 0;
        delete [] cpuTimes;
        delete [] wallClocks;
        delete [] qualityBalance;
        delete [] quantityBalance;
        delete [] interBalance;
        delete [] intraBalance;
        delete [] workerAsk;
        delete [] donateSuccess;
        delete [] donateFail;
        delete [] subtreeSplit;
        delete [] subtreeWhole;
        delete [] subtreeChange;
    }
}

//#############################################################################

// Explore a subtree from subtree pool for certain units of work and time. 
// Return how many nodes have been processed. The same subtree will be
// explored next time if it still have unexplored nodes. 
AlpsReturnStatus
AlpsKnowledgeBrokerMPI::doOneUnitWork(int unitWork,
                                      double unitTime,
                                      AlpsExitStatus & exitStatus,
                                      int & numNodesProcessed,
                                      int & depth,
                                      bool & betterSolution)
{
    
    AlpsReturnStatus rCode = AlpsReturnStatusOk;

    numNodesProcessed = 0; 
    
    if( !workingSubTree_ && !(subTreePool_->hasKnowledge()) ) {
        return rCode;
    }
    
    if (forceTerminate_) {
        // Subtree from load balancing
        deleteSubTrees();
        return rCode;
    }
    
    if ( ! needWorkingSubTree_ )  {
        // Already has a subtree working on.
        
        assert(workingSubTree_);
        rCode = workingSubTree_->exploreUnitWork(true, /* leaveAsIt*/
						 unitWork,
                                                 unitTime,
                                                 exitStatus,
                                                 numNodesProcessed,
                                                 treeDepth_,
                                                 betterSolution);
        
        if ( !(workingSubTree_->getNumNodes()) ) {
            delete workingSubTree_;  // Empty subtree
            workingSubTree_ = NULL;
            needWorkingSubTree_ = true;
        }
    } 
    else if( needWorkingSubTree_ && (subTreePool_->hasKnowledge()) ) {
        
        workingSubTree_ = dynamic_cast<AlpsSubTree* >
            (subTreePool_->getKnowledge().first);  
        
        subTreePool_->popKnowledge();     // Remove from pool
        needWorkingSubTree_ = false;      // Mark alread have one
        
#ifdef NF_DEBUG
        std::cout << "pop a subtree." << std::endl;
#endif
        
        rCode = workingSubTree_->exploreUnitWork(true,
						 unitWork,
                                                 unitTime,
                                                 exitStatus,
                                                 numNodesProcessed,
                                                 treeDepth_,
                                                 betterSolution);
        
        if ( !( workingSubTree_->getNumNodes()) ) {
            delete workingSubTree_;   // Empty subtree
            workingSubTree_ = 0;
            needWorkingSubTree_ = true;
        }
    }
    else {   // need subtree, but system has no workload
        if (workingSubTree_ != 0) {
            delete workingSubTree_;
            workingSubTree_ = 0;
        }
    }

    return rCode;
}

//#############################################################################

/** Initialize member data. */
void 
AlpsKnowledgeBrokerMPI::init() 
{
    processNum_ = 0;
    globalRank_ = -1;
    clusterComm_ = MPI_COMM_NULL;
    hubComm_ = MPI_COMM_NULL;
    hubGroup_ = MPI_GROUP_NULL;
    clusterSize_ = 0;
    userClusterSize_ = 0;
    clusterRank_ = -1;
    hubRanks_ = 0;
    myHubRank_ = -1;
    masterRank_ = -1;
    processType_ = AlpsProcessTypeAny;
    processTypeList_ = NULL;
    hubWork_ = false;
    subTreeRequest_ = MPI_REQUEST_NULL;
    solRequestL_ = MPI_REQUEST_NULL;
    solRequestR_ = MPI_REQUEST_NULL;
    modelKnowRequestL_ = MPI_REQUEST_NULL;
    modelKnowRequestR_ = MPI_REQUEST_NULL;
    forwardRequestL_ = MPI_REQUEST_NULL;
    forwardRequestR_ = MPI_REQUEST_NULL;
    incumbentValue_ = ALPS_INC_MAX;
    incumbentID_ = 0;
    updateIncumbent_ = false;
    workQuality_ = ALPS_OBJ_MAX;     // Worst possible
    clusterWorkQuality_ = ALPS_OBJ_MAX;
    systemWorkQuality_ = ALPS_OBJ_MAX;
    hubWorkQualities_ = 0;
    workerWorkQualities_ = 0;
    workQuantity_ = 0.0;
    clusterWorkQuantity_ = 0.0;
    systemWorkQuantity_ = 0.0;
    systemWorkQuantityForce_ = 0.0;
    hubWorkQuantities_ = 0;
    workerWorkQuantities_ = 0;
    workerReported_ = 0;
    hubReported_ = 0;
    allHubReported_  = false; 
    masterDoBalance_ = 0;
    hubDoBalance_ = 0;
    workerNodeProcesseds_ = 0;
    clusterNodeProcessed_ = 0;
    hubNodeProcesseds_ = 0;
    systemNodeProcessed_ = 0;
    sendCount_ = 0;
    recvCount_ = 0;
    clusterSendCount_ = 0;
    clusterRecvCount_ = 0;
    systemSendCount_ = 0;
    systemRecvCount_ = 0;
    masterIndexBatch_ = 0;
    rampUpTime_ = 0.0;
    rampDownTime_ = 0.0;
    idleTime_ = 0.0;
    msgTime_ = 0.0;

    psStats_.qualityBalance_ = 0;
    psStats_.quantityBalance_ = 0;
    psStats_.interBalance_ = 0;
    psStats_.intraBalance_ = 0;
    psStats_.workerAsk_ = 0;
    psStats_.donateSuccess_ = 0;
    psStats_.donateFail_ = 0;
    psStats_.subtreeSplit_ = 0;
    psStats_.subtreeWhole_ = 0;
    psStats_.subtreeChange_ = 0;

    forceTerminate_ = false;
    blockTermCheck_ = true;
    blockHubReport_ = false;
    blockWorkerReport_ = false;
    blockAskForWork_   = false;    
    attachBuffer_ = 0;
    largeBuffer_ = 0;
    largeBuffer2_ = 0;
    smallBuffer_ = 0;
    masterBalancePeriod_ = 0.01;
    hubReportPeriod_ = 0.01;
    modelGenID_ = -1;
    modelGenPos_ = -1;

    rampUpSubTree_ = 0;
    unitWorkNodes_ = 0;
    haltSearch_ = false;

    userBalancePeriod_ = false;
}

//#############################################################################

/** Destructor. */
AlpsKnowledgeBrokerMPI::~AlpsKnowledgeBrokerMPI() 
{
    if (workerWorkQualities_) {
	delete [] workerWorkQualities_;
	workerWorkQualities_ = 0;
    }
    
    //	std::cout << "Here1 -- " << globalRank_ << std::endl;
    if (hubRanks_) {
	delete [] hubRanks_; hubRanks_ = 0;
    }
    //	std::cout << "Here2 -- " << globalRank_ << std::endl;
    if (processTypeList_) {
	delete [] processTypeList_; processTypeList_ = NULL;
    }
    //	std::cout << "Here3 -- " << globalRank_ << std::endl;
    if (hubWorkQualities_) {
	delete [] hubWorkQualities_; hubWorkQualities_ = 0;
    }
    //	std::cout << "Here4 -- " << globalRank_ 
    //		  << "c size = " << clusterSize_ << std::endl;

    if (hubWorkQuantities_) {
	delete [] hubWorkQuantities_; hubWorkQuantities_ = 0;
    }
    if (workerWorkQuantities_){
	delete [] workerWorkQuantities_;
	workerWorkQuantities_ = 0;
    }
    //	std::cout << "Here5 -- " << globalRank_ << std::endl;
    
    if (workerReported_) {
	delete [] workerReported_; workerReported_ = 0;
    }
    if (hubReported_) {
	delete [] hubReported_; hubReported_ = 0;
    }
    if (workerNodeProcesseds_) {
	delete [] workerNodeProcesseds_; 
	workerNodeProcesseds_ = 0;
    }
    if (hubNodeProcesseds_) {
	delete [] hubNodeProcesseds_;
	hubNodeProcesseds_ = 0;
    }
#if 0
    if (attachBuffer_) {
        delete [] attachBuffer_;
        attachBuffer_ = 0;
    }
#endif
    if (largeBuffer_) {
        delete [] largeBuffer_;
        largeBuffer_ = 0;
    }
    if (largeBuffer2_) {
        delete [] largeBuffer2_;
        largeBuffer2_ = 0;
    }
    if (smallBuffer_) {
        delete [] smallBuffer_;
        smallBuffer_ = 0;
    }

    if (rampUpSubTree_) {
        delete rampUpSubTree_;
        rampUpSubTree_ = 0;
    }
    // Terminate MPI environment.
    MPI_Finalize();
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::printBestSolution(char* outputFile) const 
{
    if (globalRank_ == masterRank_) {

	if (msgLevel_ < 1) return;

	if (getNumKnowledges(AlpsKnowledgeTypeSolution) <= 0) {
	    std::cout << "\nALPS did not find a solution."
		      << std::endl;
	    return;
	}
	if (outputFile != 0) {               
	    // Write to outputFile
	    std::ofstream os(outputFile);
	    os << "Quality = " << getBestQuality();
	    os << std::endl;
	    dynamic_cast<AlpsSolution* >
		(getBestKnowledge(AlpsKnowledgeTypeSolution).first)->print(os);
	}
	else {             
	    // Write to std::cout
	    std::cout << "Quality = " << getBestQuality();
	    std::cout << "\nBest solution: " << std::endl;
	    dynamic_cast<AlpsSolution* >
		(getBestKnowledge(AlpsKnowledgeTypeSolution).first)->print(std::cout);
	}
    }
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::incSendCount(const char* how, int s) 
{ 
    if (msgLevel_ > 100) {
        messageHandler()->message(ALPS_MSG_HOW, messages())
            << globalRank_ << "increment send" << s << how << CoinMessageEol;
    }
    
    sendCount_ += s; 
}

//#############################################################################

/** Decrement the number of sent message. */
void
AlpsKnowledgeBrokerMPI::decSendCount(const char* how, int s)
{ 
    if (msgLevel_ > 100) {
        messageHandler()->message(ALPS_MSG_HOW, messages())
            << globalRank_ << "decrement send" << s << how << CoinMessageEol;
    }
    
    sendCount_ -= s; 
}

//#############################################################################

/** Increment the number of received message. */
void 
AlpsKnowledgeBrokerMPI::incRecvCount(const char* how, int s) 
{
    if (msgLevel_ > 100) {
        messageHandler()->message(ALPS_MSG_HOW, messages())
            << globalRank_ << "increment recieve" << s << how <<CoinMessageEol;
    }
    
    recvCount_ += s; 
}
    
//#############################################################################

/** Decrement the number of sent message. */
void 
AlpsKnowledgeBrokerMPI::decRecvCount(const char* how, int s) 
{ 
    if (msgLevel_ > 100) {
        messageHandler()->message(ALPS_MSG_HOW, messages())
            << globalRank_  << "decrement recieve" <<s<< how<< CoinMessageEol;
    }
    
    recvCount_ -= s;
}

//#############################################################################

/** Set knowlege to receiver. */
void 
AlpsKnowledgeBrokerMPI::sendKnowledge(AlpsKnowledgeType type, 
                                      int sender,
                                      int receiver,
                                      char *& msgBuffer,
                                      int msgSize,
                                      int msgTag,
                                      MPI_Comm comm,
                                      bool blocking)
{
    switch (type) {
    case AlpsKnowledgeTypeModel:
        break;
    case AlpsKnowledgeTypeModelGen:
        sendModelKnowledge(comm, receiver);
        break;
    case AlpsKnowledgeTypeNode:
        sendRampUpNode(receiver, comm);
        break;
    case AlpsKnowledgeTypeSolution:
        sendIncumbent();
        break;
    case AlpsKnowledgeTypeSubTree:
        break;
    default:
        assert(0);
	throw CoinError("Unknown knowledge type", "sendKnowledge", 
			"AlpsKnowledgeBrokerMPI");
    }
}

//#############################################################################

/** Receive knowlege from sender. */
void
AlpsKnowledgeBrokerMPI::receiveKnowledge(AlpsKnowledgeType type, 
                                         int sender,
                                         int receiver,
                                         char *& msgBuffer,
                                         int msgSize,
                                         int msgTag,
                                         MPI_Comm comm,
                                         MPI_Status* status,
                                         bool blocking)
{
    switch (type) {
    case AlpsKnowledgeTypeModel:
        break;
    case AlpsKnowledgeTypeModelGen:
        receiveModelKnowledge(comm);
        break;
    case AlpsKnowledgeTypeNode:
        receiveRampUpNode(sender, comm, status);
        break;
    case AlpsKnowledgeTypeSolution:
        break;
    case AlpsKnowledgeTypeSubTree:
        break;
    default:
        assert(0);
	throw CoinError("Unknown knowledge type", "receiveKnowledge", 
			"AlpsKnowledgeBrokerMPI");
    }
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::requestKnowledge(AlpsKnowledgeType type, 
                                         int sender,
                                         int receiver,
                                         char *& msgBuffer,
                                         int msgSize,
                                         int msgTag,
                                         MPI_Comm comm,
                                         bool blocking)
{
    switch (type) {
    case AlpsKnowledgeTypeModel:
        break;
    case AlpsKnowledgeTypeModelGen:
        break;
    case AlpsKnowledgeTypeNode:
        break;
    case AlpsKnowledgeTypeSolution:
        break;
    case AlpsKnowledgeTypeSubTree:
        break;
    default:
        assert(0);
	throw CoinError("Unknown knowledge type", "requestKnowledge", 
			"AlpsKnowledgeBrokerMPI");
    }
}

//#############################################################################

/** Set generated knowlege (related to model) to receiver. */
// NOTE: comm is hubComm_ or MPI_COMM_WORLD.
void 
AlpsKnowledgeBrokerMPI::forwardModelKnowledge()
{
    assert(modelGenPos_ > 0);

    // Binary forewarding
    int mySeq = rankToSequence(modelGenID_, globalRank_);
    int leftSeq = leftSequence(mySeq, processNum_);
    int rightSeq = rightSequence(mySeq, processNum_);
    
#if 0
    std::cout << "++++ forwardModelKnowledge: mySeq = " << mySeq
              << ", leftSeq = " << leftSeq
              << ", rightSeq = " << rightSeq 
              << ", modelGenID_ = " << modelGenID_
              << ", globalRank_ = " << globalRank_
              << std::endl;
#endif

    if (leftSeq != -1) {
	if (forwardRequestL_ != MPI_REQUEST_NULL) {
	    // Test if previous sending has completed.
	    int alreadySent = false;
	    MPI_Status sentStatus;
	    MPI_Test(&forwardRequestL_, &alreadySent, &sentStatus);
	    if (!alreadySent) { // Previous sending hasn't complete
		std::cout << "++++ Previous left forwardModelKnowledge hasn't finished." 
			  << std::endl;
		return;
	    }
	}

        // Note: modelGenPos_ is set by recieveModelKnowledge
        int leftRank = sequenceToRank(modelGenID_, leftSeq);
        MPI_Isend(largeBuffer_, modelGenPos_, MPI_PACKED, leftRank, 
		  AlpsMsgModelGenSearch, MPI_COMM_WORLD, &forwardRequestL_);
        std::cout << "++++ forwardModelKnowledge: leftRank = " 
                  << leftRank << std::endl;
        
        incSendCount("forwardModelKnowledge during search");        
    }
    
    if (rightSeq != -1) {
	if (forwardRequestR_ != MPI_REQUEST_NULL) {
	    // Test if previous sending has completed.
	    int alreadySent = false;
	    MPI_Status sentStatus;
	    MPI_Test(&forwardRequestR_, &alreadySent, &sentStatus);
	    if (!alreadySent) { // Previous sending hasn't complete
		std::cout << "++++ Previous right forwardModelKnowledge hasn't finished." 
			  << std::endl;
		return;
	    }
	}
        int rightRank = sequenceToRank(modelGenID_, rightSeq);
        std::cout << "++++ forwardModelKnowledge: rightRank = " 
                  << rightRank << std::endl;
        MPI_Isend(largeBuffer_, modelGenPos_, MPI_PACKED, rightRank, 
		  AlpsMsgModelGenSearch, MPI_COMM_WORLD, &forwardRequestR_);
        incSendCount("forwardModelKnowledge during search");
    }
}

//#############################################################################

/** Set generated knowlege (related to model) to receiver. */
// NOTE: comm is hubComm_ or MPI_COMM_WORLD.
// During search, need use buffered send since other process process may send
// this process at the same time, then will be deadlock.
void 
AlpsKnowledgeBrokerMPI::sendModelKnowledge(MPI_Comm comm, int receiver)
{
    int size = largeSize_;
    int position = 0;
    
    bool hasKnowledge = false;

    assert(largeBuffer2_);

    // Pack original sender's global rank
    position = 0;
    MPI_Pack(&globalRank_, 1, MPI_INT, largeBuffer2_, largeSize_, 
             &position, comm);
    
    //std::cout << "----- 1. position = " << position << std::endl;
    
    // Retrieve and pack generated model knowledge
    AlpsEncoded *encoded = model_->packSharedKnowlege();
    
    if (encoded) {
        hasKnowledge = true;
        // Pack into local buffer
        packEncoded(encoded, largeBuffer2_, size, position, comm);
    }
#if 0
    else {
        std::cout << "---- Process[" << globalRank_ 
                  << "]: have no mode knowledge to share." << std::endl;
    }
#endif
    
    //std::cout << "----- 2. position = " << position
    //        << ", size = " << size << std::endl;
    delete encoded;
    
    if (phase_ == AlpsPhaseRampup) {
        assert(comm == clusterComm_ || comm == hubComm_);
        assert(receiver > -1);
        
        if (hasKnowledge) {
            // Send packed knowledge of size "position"
            MPI_Send(largeBuffer2_, position, MPI_PACKED, receiver,
                     AlpsMsgModelGenRampUp, comm);
            incSendCount("sendModelKnowledge during rampup");
        }
        else {
            // Send an empty message
            MPI_Send(largeBuffer2_, 0, MPI_PACKED, receiver, 
                     AlpsMsgModelGenRampUp, comm);
            incSendCount("sendModelKnowledge during rampup");
        }
    }
    else if ( hasKnowledge && (phase_ == AlpsPhaseSearch) ) {
        assert(comm == MPI_COMM_WORLD);
#if 0
	// Attach buffer 
	if (!attachBuffer_) {
	    int attachSize = 2 * (largeSize_  + MPI_BSEND_OVERHEAD );
	    attachBuffer_ =  new char [attachSize];
	    MPI_Buffer_attach(attachBuffer_, attachSize);
	}
#endif	

#if 0   
        std::cout << "---- Process[" << globalRank_ 
                  << "]: Share mode knowledge by buffer sending, "
		  << "position " << position << std::endl;
#endif
   
        // During search, binary sending
        int mySeq = rankToSequence(globalRank_, globalRank_);
        int leftSeq = leftSequence(mySeq, processNum_);
        int rightSeq = rightSequence(mySeq, processNum_);
        
        if (leftSeq != -1) {
	    if (modelKnowRequestL_ != MPI_REQUEST_NULL) {
		// Test if previous sending has completed.
		int alreadySent = false;
		MPI_Status sentStatus;
		MPI_Test(&modelKnowRequestL_, &alreadySent, &sentStatus);
		if (!alreadySent) { // Previous sending hasn't complete
		    return;
		}
	    }
            int leftRank = sequenceToRank(globalRank_, leftSeq);
	    // Buffered send
	    //MPI_Bsend(largeBuffer2_, position, MPI_PACKED, leftRank, 
	    //      AlpsMsgModelGenSearch, comm);
	    //MPI_Send(largeBuffer2_, position, MPI_PACKED, leftRank, 
	    //     AlpsMsgModelGenSearch, comm);
	    MPI_Isend(largeBuffer2_, position, MPI_PACKED, leftRank,
		      AlpsMsgModelGenSearch, comm, &modelKnowRequestL_);
            incSendCount("sendModelKnowledge during search");        
        }
        
        if (rightSeq != -1) {
	    if (modelKnowRequestR_ != MPI_REQUEST_NULL) {
		// Test if previous sending has completed.
		int alreadySent = false;
		MPI_Status sentStatus;
		MPI_Test(&modelKnowRequestR_, &alreadySent, &sentStatus);
		if (!alreadySent) { // Previous sending hasn't complete
		    return;
		}
	    }
            int rightRank = sequenceToRank(globalRank_, rightSeq);
	    // Buffered send
 	    //MPI_Bsend(largeBuffer2_, position, MPI_PACKED, rightRank, 
	    //      AlpsMsgModelGenSearch, comm);
 	    //MPI_Send(largeBuffer2_, position, MPI_PACKED, rightRank, 
	    //     AlpsMsgModelGenSearch, comm);
	    MPI_Isend(largeBuffer2_, position, MPI_PACKED, rightRank,
		      AlpsMsgModelGenSearch, comm, &modelKnowRequestR_);
	    incSendCount("sendModelKnowledge during search");
        }
    }
}

//#############################################################################

/** Receive generated knowlege (related to model) from sender. */
// NOTE: comm is hubComm_ or MPI_COMM_WORLD.
void 
AlpsKnowledgeBrokerMPI::receiveModelKnowledge(MPI_Comm comm)
{
    int position = 0;
    int count = 0;
    MPI_Status status;
    bool hasKnowledge = true;

    char *localBuffer = 0;// FIXME: add a largebuffer2_
    
    if (phase_ == AlpsPhaseRampup) {
        localBuffer = new char [largeSize_];
        
        // std::cout << "PROCESS[" << globalRank_ << "] : B: recive gen model"
        //        << std::endl;
        
        // Receive
        MPI_Recv(localBuffer, largeSize_, MPI_PACKED, MPI_ANY_SOURCE,
                 AlpsMsgModelGenRampUp, comm, &status);

        MPI_Get_count(&status, MPI_PACKED, &count);

        if (count < 1) {
            hasKnowledge = false;
        }

        incRecvCount("receiveModelKnowledge during rampup");

        //std::cout << "PROCESS[" << globalRank_ << "] : A: recive gen model"
	//  << ", count = " << count << std::endl;    
    }
    else if (phase_ == AlpsPhaseSearch) {
        localBuffer = largeBuffer_;
    }
    else {
        assert(0);
    }
    
    if (hasKnowledge) {
        // Upack original sender's global rank
        position = 0;  // Start from position 0 in local buffer
        MPI_Unpack(localBuffer, largeSize_, &position, &modelGenID_, 1,
                   MPI_INT, comm);
        
        //std::cout << "PROCESS[" << globalRank_ << "] : recive gen model"
	//  << ", count = " << count << ", from " << modelGenID_
	//        << std::endl;
        
        // Upack knowledge from buffer.
        AlpsEncoded* encodedModelGen = unpackEncoded(localBuffer,
                                                     position,
                                                     comm,
                                                     largeSize_);
        
        // Record actuall buffer size for broadcasting
        modelGenPos_ = position;
        
        // Upack and store knowledge from larger buffer.
        model_->unpackSharedKnowledge(*encodedModelGen);

        delete encodedModelGen; 
        encodedModelGen = 0;
    }

    if ( (phase_ == AlpsPhaseRampup) && localBuffer ) {
        delete [] localBuffer;
    }
    localBuffer = 0;
}

//#############################################################################

void
AlpsKnowledgeBrokerMPI::sendErrorCodeToMaster(int errorCode)
{
    int size = model_->AlpsPar()->entry(AlpsParams::smallSize);
    int pos = 0;
    MPI_Pack(&errorCode, 1, MPI_INT, smallBuffer_, size, &pos, MPI_COMM_WORLD);
    MPI_Send(smallBuffer_, pos, MPI_PACKED, masterRank_, AlpsMsgErrorCode, 
             MPI_COMM_WORLD);
    incSendCount("sendErrorCodeToMaster");
}

//#############################################################################

void
AlpsKnowledgeBrokerMPI::recvErrorCode(char *& bufLarge)
{
    int size = 10;
    int pos = -1;
    int errorCode = 0;
    
    MPI_Unpack(bufLarge, size, &pos, &errorCode, 1, MPI_INT, MPI_COMM_WORLD);
    if (errorCode == 1) {
        // out of memory
        exitStatus_ = AlpsExitStatusNoMemory;
    }
    else {
        // error
        exitStatus_ = AlpsExitStatusFailed;
    }
    
    forceTerminate_ = true;
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::rootInitMaster(AlpsTreeNode* root)
{
    int i = 0;
    int requiredNumNodes = 
        model_->AlpsPar()->entry(AlpsParams::masterInitNodeNum);
    double rampUpTimeMaster = 0.0;

    AlpsNodePool* tempNodePool= NULL;

    if (msgLevel_ > 0) {
	messageHandler()->message(ALPS_STATIC_BALANCE_BEG, messages())
	    << "the Two-level Root Initialization" << CoinMessageEol;
    }

    //------------------------------------------------------
    // Create required number of subtrees(nodes) for hubs.
    //------------------------------------------------------

    if (msgLevel_ > 0 && requiredNumNodes > 0) {
	messageHandler()->message(ALPS_RAMPUP_MASTER_START, messages())
	    << globalRank_ << requiredNumNodes << CoinMessageEol;
    }

    rampUpSubTree_ = dynamic_cast<AlpsSubTree*>
	(const_cast<AlpsKnowledge *>(decoderObject(AlpsKnowledgeTypeSubTree)))
	->newSubTree();
    rampUpSubTree_->setKnowledgeBroker(this);
    rampUpSubTree_->setNodeSelection(rampUpNodeSelection_);
    rampUpSubTree_->setNextIndex(1); // One more than root's index
    
    nodeProcessedNum_ += rampUpSubTree_->rampUp(hubNum_,
                                                requiredNumNodes, 
                                                treeDepth_, 
                                                root);
    
    int numGenNodes = rampUpSubTree_->nodePool()->getNumKnowledges();

    //------------------------------------------------------
    // Spawn hub/worker processes (For dynamic process management only) 
    // Nothing now. FILL IN HERE IF NEEDED LATER.
    //------------------------------------------------------
 
    //------------------------------------------------------
    // Distribute subtrees(nodes) to hubs in Round-robin way.
    //------------------------------------------------------

    // Temporary store node for hub 0(master)
    tempNodePool = new AlpsNodePool;
    tempNodePool->setNodeSelection(*rampUpNodeSelection_);	

    if (numGenNodes <= 0) {
        if (msgLevel_ > 0) {
            messageHandler()->message(ALPS_RAMPUP_MASTER_FAIL, messages())
                << globalRank_ << CoinMessageEol;
        }
    }
    else {
	int numSent = 0;
	while (numSent < numGenNodes) {
	    for (i = 0; i < hubNum_; ++i) {
		if(numSent >= numGenNodes) {
		    break; // Break in the sending round
		}
		if (hubRanks_[i] != globalRank_) { 
                    sendKnowledge(AlpsKnowledgeTypeNode,
                                  globalRank_,
                                  i,
                                  smallBuffer_, 
                                  0,
                                  MPI_ANY_TAG,
                                  hubComm_,
                                  true);

		    // NOTE: master's rank is always 0 in hubComm_.
                    if (msgLevel_ > 100) {
                        std::cout << "master send a node to hub " 
				  << hubRanks_[i]
                                  << std::endl;
                    }
		}
		else {
		    AlpsTreeNode* nodeM = dynamic_cast<AlpsTreeNode* >
			(rampUpSubTree_->nodePool()->getKnowledge().first);
		    rampUpSubTree_->nodePool()->popKnowledge();
		    tempNodePool->addKnowledge(nodeM, nodeM->getQuality());
		}
		++numSent;
	    }
	}
    }

    //------------------------------------------------------
    // Move nodes from tempNodePool to rampUpSubTree_'s node pool.
    // NOTE: These nodes do not necessary form a subtree. We want to use
    //       subtree's functions to generates more nodes for workers.
    //------------------------------------------------------

    while (tempNodePool->hasKnowledge()) {
	AlpsTreeNode* nodeT = dynamic_cast<AlpsTreeNode* > 
	    (tempNodePool->getKnowledge().first); 
	double valT = tempNodePool->getKnowledge().second;
	rampUpSubTree_->nodePool()->addKnowledge(nodeT, valT);
	tempNodePool->popKnowledge();
    }

    //------------------------------------------------------
    // Tell hubs no more nodes to send.
    //------------------------------------------------------

    for (i = 0; i < hubNum_; ++i) {
	if (hubRanks_[i] != globalRank_) sendFinishInit(i, hubComm_);
    }

    //------------------------------------------------------
    // Sent generated model knowledge to hubs.
    //------------------------------------------------------

    for (i = 0; i < hubNum_; ++i) {
	if (hubRanks_[i] != globalRank_) {
            sendKnowledge(AlpsKnowledgeTypeModelGen,
                          masterRank_,
                          i,            // receiver
                          largeBuffer2_,
                          0,
                          AlpsMsgModelGenRampUp,
                          hubComm_,     // comm
                          true);
            //sendModelKnowledge(genBuf, AlpsMsgModelGenRampUp, hubComm_, i);
        }
    }
    if (msgLevel_ > 0) {
        messageHandler()->message(ALPS_KNOWLEDGE_GEN, messages())
            << globalRank_ << CoinMessageEol;
    }

    // Master's rampup stops.
    rampUpTimeMaster = masterTimer_.getTime();

    //------------------------------------------------------
    // If have solution, broadcast its value and process id.
    //------------------------------------------------------

    if (hasKnowledge(AlpsKnowledgeTypeSolution)) {
	double incVal = getBestKnowledge(AlpsKnowledgeTypeSolution).second;
	if(incVal < incumbentValue_) {   // Assume Minimization
	    incumbentValue_ = incVal;
	    incumbentID_ = globalRank_;
            sendKnowledge(AlpsKnowledgeTypeSolution, //useful
                          globalRank_,
                          0,
                          smallBuffer_,
                          0,
                          MPI_ANY_TAG,
                          MPI_COMM_WORLD,
                          false);
	    //sendIncumbent();
            if (msgLevel_ > 0) {
                messageHandler()->message(ALPS_RAMPUP_MASTER_SOL, messages())
                    << globalRank_ << incVal << CoinMessageEol;
            }
	}
    }
    
    //------------------------------------------------------
    // Print out statistics about root ramp up.
    //------------------------------------------------------
    
    if (msgLevel_ > 0) {
        messageHandler()->message(ALPS_RAMPUP_MASTER, messages())
            << globalRank_ << rampUpTimeMaster 
            << nodeProcessedNum_ << numGenNodes
            << CoinMessageEol;
    }
 
    if (nodeProcessedNum_) {
        nodeProcessingTime_ = rampUpTimeMaster/nodeProcessedNum_;
    }
    
    //------------------------------------------------------
    // Generate and send required number of nodes for master's workers.
    //------------------------------------------------------
    
    requiredNumNodes = model_->AlpsPar()->entry(AlpsParams::hubInitNodeNum);
    
    if (msgLevel_ > 0 && requiredNumNodes > 0) {
        messageHandler()->message(ALPS_RAMPUP_HUB_START, messages())
            << globalRank_ << requiredNumNodes << CoinMessageEol;
    }
    
    int treeSizeByHub = rampUpSubTree_->rampUp(clusterSize_ - 1, 
                                               requiredNumNodes, 
                                               treeDepth_);

    nodeProcessedNum_ += treeSizeByHub;
    const int numNode2 = rampUpSubTree_->nodePool()->getNumKnowledges();
 
    if (numNode2 == 0) {
        if (msgLevel_ > 0) {
            messageHandler()->message(ALPS_RAMPUP_HUB_FAIL, messages())
                << globalRank_ << CoinMessageEol;
        }
    }
    else {    // Send nodes to my workers
	int numSent2 = 0;
	while (numSent2 < numNode2) {
	    for (i = 0; i < clusterSize_; ++i) {
		if(numSent2 >= numNode2) {
		    break;
		}
		if (i != clusterRank_) {
                    sendKnowledge(AlpsKnowledgeTypeNode,
                                  globalRank_,
                                  i,
                                  smallBuffer_, 
                                  0,
                                  MPI_ANY_TAG,
                                  clusterComm_,
                                  true);

		    ++numSent2;
#ifdef NF_DEBUG
		    std::cout << "MASTER/HUB " << clusterRank_
			      <<" : sent nodes to Worker " 
			      << i << std::endl; 
#endif
		}
	    }
	}
    }

    assert(rampUpSubTree_->getNumNodes() == 0);
    rampUpSubTree_->nullRootActiveNode();
    
    //------------------------------------------------------
    // Tell its workers no more nodes to send.
    //------------------------------------------------------

    for (i = 0; i < clusterSize_; ++i) {
	if (i != clusterRank_) {
	    sendFinishInit(i, clusterComm_);
	}
    }

    //------------------------------------------------------
    // Sent generated model knowledge to its workers.
    //------------------------------------------------------

    for (i = 0; i < clusterSize_; ++i) {
	if (i != clusterRank_) {
            sendKnowledge(AlpsKnowledgeTypeModelGen,
                          clusterRank_,
                          i,             // receiver
                          largeBuffer2_,
                          0,
                          AlpsMsgModelGenRampUp,
                          clusterComm_,  // comm
                          true);
	    //sendModelKnowledge(genBuf, AlpsMsgModelGenRampUp, clusterComm_, i);
	}
    }
    
    //------------------------------------------------------
    // If have better solution, broadcast its value and process id.
    //------------------------------------------------------

    if (hasKnowledge(AlpsKnowledgeTypeSolution)) {
	double incVal = getBestKnowledge(AlpsKnowledgeTypeSolution).second;
	if(incVal < incumbentValue_) {   // Assume Minimization
	    incumbentValue_ = incVal;
	    incumbentID_ = globalRank_;
            sendKnowledge(AlpsKnowledgeTypeSolution, //useful
                          globalRank_,
                          0,
                          smallBuffer_,
                          0,
                          MPI_ANY_TAG,
                          MPI_COMM_WORLD,
                          false);
	    //sendIncumbent();
            if (msgLevel_ > 0) {
                messageHandler()->message(ALPS_RAMPUP_HUB_SOL, messages())
                    << globalRank_ << incVal << CoinMessageEol;
            }
	} 
    }

    rampUpTime_ = masterTimer_.getTime();
    
    if (msgLevel_ > 0) {
        messageHandler()->message(ALPS_RAMPUP_HUB, messages())
            << globalRank_ << (rampUpTime_ - rampUpTimeMaster) 
            << treeSizeByHub << numNode2 << CoinMessageEol;
    }

    if (treeSizeByHub) {
        nodeProcessingTime_=0.5*(nodeProcessingTime_ +
                                 (rampUpTime_-rampUpTimeMaster)/treeSizeByHub);
    }   

    // Reset to normal selection.
    rampUpSubTree_->setNodeSelection(nodeSelection_);    

    // Free pool.
    delete tempNodePool;

    if (msgLevel_ > 0) {
	messageHandler()->message(ALPS_STATIC_BALANCE_END, messages())
	    << "the Two-level Root Initialization" << CoinMessageEol;
    }
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::rootInitHub()
{
    int i;
    
    MPI_Status status;

    int requiredNumNodes = 
        model_->AlpsPar()->entry(AlpsParams::hubInitNodeNum);

    rampUpSubTree_ = dynamic_cast<AlpsSubTree*>
        (const_cast<AlpsKnowledge *>
	 (decoderObject(AlpsKnowledgeTypeSubTree)))->newSubTree();
    rampUpSubTree_->setKnowledgeBroker(this);
    rampUpSubTree_->setNodeSelection(rampUpNodeSelection_);

    // Make sure the number of nodes created is larger than cluster size.
    int minNumNodes = 0;
    if (hubWork_) {
        minNumNodes = clusterSize_;
    }
    else {
        minNumNodes = clusterSize_ - 1;
    }

    //------------------------------------------------------
    // Receive subtrees (nodes) sent by Master.
    // NOTE: Put all nodes in rampUpSubtree_. We just want to use the 
    //       subtree's functions to generates more nodes.
    //------------------------------------------------------

    while(true) {
	// NOTE: master's rank is always 0 in hubComm_.
        receiveKnowledge(AlpsKnowledgeTypeNode, 
                         0,           // sender,
                         globalRank_, // receiver,
                         smallBuffer_,
                         0,
                         MPI_ANY_TAG, 
                         hubComm_,
                         &status,
                         true);

        if (hubMsgLevel_ > 100) {
            std::cout << "HUB " << globalRank_ << ": received a node from master "
                      << masterRank_ << std::endl;
        }
	
	if (status.MPI_TAG == AlpsMsgFinishInit) {
            if (hubMsgLevel_ > 0) {
                messageHandler()->message(ALPS_RAMPUP_HUB_RECV, messages())
                    << globalRank_ << masterRank_ << CoinMessageEol;
            }
            
	    break;
	}
    }

    //------------------------------------------------------
    // Receive generated knowledge form the master.
    //------------------------------------------------------
    
    receiveKnowledge(AlpsKnowledgeTypeModelGen,
                     masterRank_, // sender
                     globalRank_, // receiver,
                     smallBuffer_,
                     0,
                     MPI_ANY_TAG,
                     hubComm_,    // hub comm
                     &status,
                     true);

    //receiveModelKnowlege(hubComm_);
    
    //------------------------------------------------------
    // Generate and send required number of nodes(subtree) for hub's workers.
    //------------------------------------------------------

    if (hubMsgLevel_ > 0 && requiredNumNodes > 0) {
        messageHandler()->message(ALPS_RAMPUP_HUB_START, messages())
            << globalRank_ << requiredNumNodes << CoinMessageEol;
    }

    nodeProcessedNum_ += rampUpSubTree_->rampUp(minNumNodes,
                                                requiredNumNodes, 
                                                treeDepth_);
    
    const int numNode = rampUpSubTree_->nodePool()->getNumKnowledges();
    
    if (numNode == 0) {
        if (hubMsgLevel_ > 0) {
            messageHandler()->message(ALPS_RAMPUP_HUB_FAIL, messages())
                << globalRank_ << CoinMessageEol;
        }
    }
    else {    // Distribute nodes
	int numSent = 0;
	while ( numSent < numNode) {
	    for (i = 0; i < clusterSize_; ++i) {
		if (numSent >= numNode ) break;

                if (i == clusterRank_) {
                    if (hubWork_) {
                        // Hub need work, so Keep a node for self
                        AlpsSubTree* myTree = dynamic_cast<AlpsSubTree*>
                            (const_cast<AlpsKnowledge *> (decoderObject
				(AlpsKnowledgeTypeSubTree)))->newSubTree();
                        myTree->setKnowledgeBroker(this);
                        myTree->setNodeSelection(nodeSelection_);
                        
                        AlpsTreeNode* node = static_cast<AlpsTreeNode* >
                            (rampUpSubTree_->nodePool()->getKnowledge().first);
                        rampUpSubTree_->nodePool()->popKnowledge();
                        
                        node->setKnowledgeBroker(this);
                        node->modifyDesc()->setModel(model_);
                        node->setParent(NULL);
                        node->setParentIndex(-1);
                        node->setNumChildren(0);
                        node->setStatus(AlpsNodeStatusCandidate);
                        myTree->nodePool()->addKnowledge(node, 
                                                         node->getQuality());
                        assert(myTree->getNumNodes() == 1);
                        myTree->setRoot(node); // Don't forget!
                        myTree->calculateQuality();
                        addKnowledge(AlpsKnowledgeTypeSubTree, myTree, 
				     myTree->getQuality());
                        ++numSent;
                    }
                }
                else {
                    // Send a node to work_i
                    sendKnowledge(AlpsKnowledgeTypeNode,
                                  globalRank_,
                                  i,
                                  smallBuffer_, 
                                  0,
                                  MPI_ANY_TAG,
                                  clusterComm_,
                                  true);

                    ++numSent;
                }
	    } // EOF of for loop
	} // EOF of while loop
    }
    
    assert(rampUpSubTree_->getNumNodes() == 0);
    rampUpSubTree_->nullRootActiveNode();

    //------------------------------------------------------
    // Sent finish initialization tag to workers so that they know to
    // stop receiving subtrees.
    //------------------------------------------------------

    for (i = 0; i < clusterSize_; ++i) {
	if (i != clusterRank_) {
	    sendFinishInit(i, clusterComm_);
	}
    }
    
    //------------------------------------------------------
    // Sent generated model knowledge to its workers.
    //------------------------------------------------------

    for (i = 0; i < clusterSize_; ++i) {
	if (i != clusterRank_) {
            sendKnowledge(AlpsKnowledgeTypeModelGen,
                          globalRank_,
                          i,             // receiver
                          largeBuffer2_,
                          0,
                          AlpsMsgModelGenRampUp,
                          clusterComm_,  // comm
                          true);
            // sendModelKnowledge(genBuf, AlpsMsgModelGenRampUp, clusterComm_, i);
	}
    }

    //------------------------------------------------------
    // If have better solution, broadcast its value and process id.
    //------------------------------------------------------
    
    if (hasKnowledge(AlpsKnowledgeTypeSolution)) {
	double incVal = getBestKnowledge(AlpsKnowledgeTypeSolution).second;
	if(incVal < incumbentValue_) {    // Assume Minimization
	    incumbentValue_ = incVal;
	    incumbentID_ = globalRank_;
            sendKnowledge(AlpsKnowledgeTypeSolution, //useful
                          globalRank_,
                          0,
                          smallBuffer_,
                          0,
                          MPI_ANY_TAG,
                          MPI_COMM_WORLD,
                          false);
	    //sendIncumbent();
            if (hubMsgLevel_ > 0) {
                messageHandler()->message(ALPS_RAMPUP_HUB_SOL, messages())
                    << globalRank_ << incVal << CoinMessageEol;
            }
	}
    }

    rampUpTime_ = hubTimer_.getTime();

    if (hubMsgLevel_ > 0) {
        messageHandler()->message(ALPS_RAMPUP_HUB, messages())
            << globalRank_ << rampUpTime_ << nodeProcessedNum_ << numNode
            << CoinMessageEol;
    }
    
    if (nodeProcessedNum_) {
        nodeProcessingTime_ = rampUpTime_/nodeProcessedNum_;
    }

    /* Reset to normal selection. */
    rampUpSubTree_->setNodeSelection(nodeSelection_);
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::rootInitWorker()
{
    int numTrees = 0;
    MPI_Status status;
    const int workerMsgLevel = 
        model_->AlpsPar()->entry(AlpsParams::workerMsgLevel);
 
    //------------------------------------------------------
    // Recv subtrees(nodes) sent by my hub.
    //------------------------------------------------------

#ifdef NF_DEBUG
    std::cout << "WORKER["<< globalRank_ << "]: before rampup." << std::endl;
#endif

    while(true) {

	rampUpSubTree_ = dynamic_cast<AlpsSubTree*>
	    (const_cast<AlpsKnowledge *>
	     (decoderObject(AlpsKnowledgeTypeSubTree)))->
	    newSubTree();
	rampUpSubTree_->setKnowledgeBroker(this);
	rampUpSubTree_->setNodeSelection(nodeSelection_);
        
	// NOTE: received subtree is decoded into rampUpSubTree_
        receiveKnowledge(AlpsKnowledgeTypeNode, 
                         masterRank_,  // sender,
                         globalRank_,  // receiver,
                         smallBuffer_,
                         0,
                         MPI_ANY_TAG, 
                         clusterComm_, // cluster comm
                         &status,
                         true);

	if (status.MPI_TAG == AlpsMsgFinishInit) {
            if (workerMsgLevel > 0) {
                messageHandler()->message(ALPS_RAMPUP_WORKER_RECV, messages())
                    << globalRank_ << myHubRank_ << CoinMessageEol;
            }
	    delete rampUpSubTree_;
	    rampUpSubTree_ = NULL;
	    break;
	}

	assert(rampUpSubTree_->getNumNodes() > 0);

	rampUpSubTree_->calculateQuality();

	addKnowledge(AlpsKnowledgeTypeSubTree,
                     rampUpSubTree_,
                     rampUpSubTree_->getQuality()); 
        
        rampUpSubTree_ = NULL;

        ++numTrees;
        assert(numTrees == subTreePool_->getNumKnowledges());
    }

    //------------------------------------------------------    
    // Receive generated knowledge form its hub.
    //------------------------------------------------------
    
    receiveKnowledge(AlpsKnowledgeTypeModelGen,
                     myHubRank_,  // sender
                     globalRank_, // receiver,
                     smallBuffer_,
                     0,
                     MPI_ANY_TAG,
                     clusterComm_,// cluster comm
                     &status,
                     true);
    // receiveModelKnowlege(clusterComm_);

}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::spiralMaster(AlpsTreeNode *root)
{
    // Create initial number of nodes (large than 1)
    int k;
    int requiredNumNodes = 2;
    int receiver, pos, sender;
    int numSent = 0;
    int numLoaded = 0;
    int doUnitWork = 1;
    int donorNodes = 0;
    int numFinishSiginal = 1; // Count master itself

    bool stopSearch = false;
    bool inRampUp = true;
    double incVal;
    
    MPI_Status status;

    const int smallSize = model_->AlpsPar()->entry(AlpsParams::smallSize);

    if (msgLevel_ > 0) {
	messageHandler()->message(ALPS_STATIC_BALANCE_BEG, messages())
	    << "the Spiral Initialization" << CoinMessageEol;
    }

    if (msgLevel_ > 0 && requiredNumNodes > 0) {
	messageHandler()->message(ALPS_RAMPUP_MASTER_START, messages())
	    << globalRank_ << requiredNumNodes << CoinMessageEol;
    }
    
    rampUpSubTree_ = dynamic_cast<AlpsSubTree*>
	(const_cast<AlpsKnowledge *>(decoderObject(AlpsKnowledgeTypeSubTree)))
	->newSubTree();
    rampUpSubTree_->setKnowledgeBroker(this);
    rampUpSubTree_->setNodeSelection(rampUpNodeSelection_);
    rampUpSubTree_->setNextIndex(1); // One more than root's index
    
    nodeProcessedNum_ += rampUpSubTree_->rampUp(1,  // minimum number
                                                requiredNumNodes, 
                                                treeDepth_, 
                                                root);
    
    // Distribute created nodes to other processes
    int numGenNodes = rampUpSubTree_->nodePool()->getNumKnowledges();

#if 0
    std::cout << "master: spiral: numGenNodes = " << numGenNodes<< std::endl;
#endif

    if (numGenNodes == 0) {
        // Send initialization complete signal to other processes.
	for (k = 0; k < processNum_; ++k) {
	    if (k != globalRank_) {
		MPI_Send(smallBuffer_, 0, MPI_PACKED, k,
			 AlpsMsgFinishInit, MPI_COMM_WORLD);
	    }
	}
        stopSearch = true;
        goto TERM_SPIRAL_MASTER;
    }

    numSent = 0;
    numLoaded = 0;
    doUnitWork = 1;
    if (numGenNodes > processNum_ - 2) {
        doUnitWork = 0;
    }

    while (numSent < numGenNodes) {
	for (k = 0; k < processNum_; ++k) {
	    if(numSent == numGenNodes) {
		break; // Distributed all nodes
	    }
	    if (k != globalRank_) {
                sendNodeModelGen(k, doUnitWork);
		++numSent;
	    }
	    ++numLoaded;
	}
    }

    assert(rampUpSubTree_->getNumNodes() == 0);
    rampUpSubTree_->nullRootActiveNode();
    
    // Manage rampup
    inRampUp = true;
    donorNodes = 0;
    numFinishSiginal = 1; // Count master itself

    if (doUnitWork) {
        // If ask other process do unit work, then they will report back.
        while (inRampUp) {
            MPI_Recv(&donorNodes, 1, MPI_INT, MPI_ANY_SOURCE, 
                     AlpsMsgRampUpLoad, MPI_COMM_WORLD, &status);
            sender = status.MPI_SOURCE;
            if ((donorNodes > 1) && (numLoaded < processNum_)) {
                // Ask the sender to donate a node to process numLoaded;
                receiver = numLoaded;
                pos = 0;
                MPI_Pack(&receiver, 1, MPI_INT, smallBuffer_, smallSize, &pos,
                         MPI_COMM_WORLD);
                MPI_Send(smallBuffer_, pos, MPI_PACKED, sender, 
                         AlpsMsgRampUpDonate, MPI_COMM_WORLD);
                ++numLoaded;
            }
            else {
                MPI_Send(smallBuffer_, 0, MPI_PACKED, sender,
                         AlpsMsgFinishInit, MPI_COMM_WORLD);
                ++numFinishSiginal;
            }
            
            // If send all "loaded process" finish signial, then exit.
            if (numFinishSiginal == numLoaded) {
                inRampUp = false;
            } 
        }
    }
    else {
        // Just ask them to exit from rampup
        for (k = 0; k < processNum_; ++k) {
	    if (k != globalRank_) {
                MPI_Send(smallBuffer_, 0, MPI_PACKED, k,
                         AlpsMsgFinishInit, MPI_COMM_WORLD);
	    }
	}
    }


TERM_SPIRAL_MASTER:
    
    MPI_Barrier(MPI_COMM_WORLD);
    
    //------------------------------------------------------
    // If have better solution, broadcast its value and process id.
    //------------------------------------------------------

    if (!stopSearch && hasKnowledge(AlpsKnowledgeTypeSolution)) {
	incVal = getBestKnowledge(AlpsKnowledgeTypeSolution).second;
	if(incVal < incumbentValue_) {   // Assume Minimization
	    incumbentValue_ = incVal;
	    incumbentID_ = globalRank_;
            sendKnowledge(AlpsKnowledgeTypeSolution, //useful
                          globalRank_,
                          0,
                          smallBuffer_,
                          0,
                          MPI_ANY_TAG,
                          MPI_COMM_WORLD,
                          false);
	    //sendIncumbent();
            if (msgLevel_ > 0) {
                messageHandler()->message(ALPS_RAMPUP_MASTER_SOL, messages())
                    << globalRank_ << incVal << CoinMessageEol;
            }
	} 
    }

    if (msgLevel_ > 0) {
	messageHandler()->message(ALPS_STATIC_BALANCE_END, messages())
	    << "the Spiral Initialization" << CoinMessageEol;
    }
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::spiralHub()
{
    bool inRampUp = true;
    MPI_Status status;
    
    while(inRampUp) {
	MPI_Recv(largeBuffer_, largeSize_, MPI_PACKED, MPI_ANY_SOURCE, 
                 MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	switch(status.MPI_TAG) {
	case AlpsMsgNode:
	    // Unpack the node, explore it and send load info to master
	    spiralRecvProcessNode();
	    break;
	case AlpsMsgRampUpDonate:
	    // Unpack msg and donate a node 
	    spiralDonateNode();
	    break;
	case AlpsMsgFinishInit:
            inRampUp = false;
            break;
	default:
	    std::cout << "PROC " << globalRank_ 
		      << " : spiralHub: received UNKNOWN message. tag = " 
		      << status.MPI_TAG <<  std::endl; 
            assert(0);
	    throw CoinError("Unknown message type", "spiralHubWorker", 
			    "AlpsKnowledgeBrokerMPI");
	}
    }

    if (hubMsgLevel_ > 10) {
        std::cout << "hub[" << globalRank_ 
                  << "]: spiral: exited from while." << std::endl;
    }
    
    if (hubWork_) {
        // Put rampup subtree in pool
        if (rampUpSubTree_) {
            rampUpSubTree_->calculateQuality();
            addKnowledge(AlpsKnowledgeTypeSubTree,
                         rampUpSubTree_,
                         rampUpSubTree_->getQuality());
            rampUpSubTree_ = NULL;
        }
    }
    else {
	// hub send its nodes to its workers.
        int i;
        const int numNode = rampUpSubTree_->nodePool()->getNumKnowledges();
        int numSent = 0;
        int receiver = -1;
        
	while ( numSent < numNode) {
	    for (i = 0; i < clusterSize_; ++i) {
		if (numSent == numNode ) break;
                if (i != clusterRank_) {
                    // Send a node to work receiver
                    receiver = cluster2GlobalRank(masterRank_, myHubRank_, i);
                    sendNodeModelGen(receiver, 0); // Just recv, no unit work
                    ++numSent;
                }
	    } 
	} // EOF of while loop
        assert(rampUpSubTree_->getNumNodes() == 0);
        rampUpSubTree_->nullRootActiveNode();
        
        // Send hub finish tag
        char* dummyBuf = 0;
        for (i = 0; i < clusterSize_; ++i) {
            if (i != clusterRank_) {
                receiver = cluster2GlobalRank(masterRank_, myHubRank_, i);
                MPI_Send(dummyBuf, 0, MPI_PACKED, receiver, 
                         AlpsMsgFinishInitHub, MPI_COMM_WORLD);
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    
    if (hubMsgLevel_ > 10) {
        std::cout << "hub: spiral: finished." << std::endl;
    }

    //------------------------------------------------------
    // If have better solution, broadcast its value and process id.
    //------------------------------------------------------

    if (hasKnowledge(AlpsKnowledgeTypeSolution)) {
	double incVal = getBestKnowledge(AlpsKnowledgeTypeSolution).second;
	if(incVal < incumbentValue_) {   // Assume Minimization
	    incumbentValue_ = incVal;
	    incumbentID_ = globalRank_;
            sendKnowledge(AlpsKnowledgeTypeSolution, //useful
                          globalRank_,
                          0,
                          smallBuffer_,
                          0,
                          MPI_ANY_TAG,
                          MPI_COMM_WORLD,
                          false);
	    //sendIncumbent();
            if (msgLevel_ > 0) {
                messageHandler()->message(ALPS_RAMPUP_HUB_SOL, messages())
                    << globalRank_ << incVal << CoinMessageEol;
            }
	} 
    }
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::spiralWorker()
{
    // Exit when receive instruction from Hub and master
    int exitCount = 2;
    int count = 0;
    bool inRampUp = true;

    if (hubWork_ || (globalRank_ < clusterSize_)) {
        // Hub won't send it node to me since it works.
        // If in master's cluster, then exit now.
        exitCount = 1;
    }
    
    // Note the sequence to receive AlpsMsgFinishInit or
    // AlpsMsgFinishInitHub is not deterministic.
    MPI_Status status;
    while(inRampUp) {
	MPI_Recv(largeBuffer_, largeSize_, MPI_PACKED, MPI_ANY_SOURCE, 
                 MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	switch(status.MPI_TAG) {
	case AlpsMsgNode:
	    // Unpack the node, explore it and send load info to master
	    spiralRecvProcessNode();
	    break;
	case AlpsMsgRampUpDonate:
	    // Unpack msg and donate a node 
	    spiralDonateNode();
	    break;
	case AlpsMsgFinishInit:
            ++count;
            if (count == exitCount) {
                inRampUp = false;
            }
            break;
	case AlpsMsgFinishInitHub:
            ++count;
            if (count == exitCount) {
                inRampUp = false;
            }
	    break;
	default:
	    std::cout << "PROC " << globalRank_ 
		      << " : spiralWorker: received UNKNOWN message. tag = " 
		      << status.MPI_TAG <<  std::endl; 
            assert(0);
	    throw CoinError("Unknown message type", "spiralHubWorker", 
			    "AlpsKnowledgeBrokerMPI");
	}
    }

#if 0
    std::cout << "worker[" << globalRank_ 
              << "]: spiral: exited from while." << std::endl;
#endif

    // Put rampup subtree in pool
    if (rampUpSubTree_) {
        rampUpSubTree_->calculateQuality();
        addKnowledge(AlpsKnowledgeTypeSubTree,
                     rampUpSubTree_,
                     rampUpSubTree_->getQuality());
        rampUpSubTree_ = NULL;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    //------------------------------------------------------
    // If have better solution, broadcast its value and process id.
    //------------------------------------------------------

    if (hasKnowledge(AlpsKnowledgeTypeSolution)) {
	double incVal = getBestKnowledge(AlpsKnowledgeTypeSolution).second;
	if(incVal < incumbentValue_) {   // Assume Minimization
	    incumbentValue_ = incVal;
	    incumbentID_ = globalRank_;
            sendKnowledge(AlpsKnowledgeTypeSolution, //useful
                          globalRank_,
                          0,
                          smallBuffer_,
                          0,
                          MPI_ANY_TAG,
                          MPI_COMM_WORLD,
                          false);
	    //sendIncumbent();
            if (msgLevel_ > 0) {
                messageHandler()->message(ALPS_RAMPUP_WORKER_SOL, messages())
                    << globalRank_ << incVal << CoinMessageEol;
            }
	} 
    }

#if 0
    std::cout << "worker[" << globalRank_<<"]: spiral: finished." << std::endl;
#endif

}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::spiralRecvProcessNode()
{
    //--------------------------------------------
    // Unpack if doUnitWork
    //--------------------------------------------

    int pos = 0;
    int doUnitWork = 0;
    MPI_Unpack(largeBuffer_, largeSize_, &pos, &doUnitWork, 1, 
	       MPI_INT, MPI_COMM_WORLD);
#if 0
    std::cout << "PROCESS " << globalRank_ << " : received a node, doUnitWork "
              << doUnitWork << std::endl;    
#endif

    //--------------------------------------------
    // Unpack a node
    //--------------------------------------------

    rampUpSubTree_ = dynamic_cast<AlpsSubTree*>
        (const_cast<AlpsKnowledge *>(decoderObject(AlpsKnowledgeTypeSubTree)))->newSubTree();
    rampUpSubTree_->setKnowledgeBroker(this);
    rampUpSubTree_->setNodeSelection(nodeSelection_);
    
    AlpsEncoded* encodedNode = unpackEncoded(largeBuffer_,pos,MPI_COMM_WORLD);
    AlpsTreeNode* node = dynamic_cast<AlpsTreeNode* >
	( decoderObject(encodedNode->type())->decode(*encodedNode) );
    
    node->setKnowledgeBroker(this);
    node->modifyDesc()->setModel(model_);
    node->setParent(NULL);
    
    rampUpSubTree_->nodePool()->addKnowledge(node, node->getQuality());
    assert(rampUpSubTree_->getNumNodes() > 0);
    if ( (rampUpSubTree_->nodePool()->getNumKnowledges() ) == 1) {
	// Make the first node as root.
	rampUpSubTree_->setRoot(node);
    }
    
    if (encodedNode) {
	delete encodedNode;
	encodedNode = 0;
    }
    node = NULL;

    //--------------------------------------------
    // Unpack model knowledge
    //--------------------------------------------
    int hasKnowledge = 0;
    MPI_Unpack(largeBuffer_, largeSize_, &pos, &hasKnowledge, 1, 
	       MPI_INT, MPI_COMM_WORLD);
    if (hasKnowledge) {
        // Upack knowledge from buffer.
        AlpsEncoded* encodedModelGen = unpackEncoded(largeBuffer_,
                                                     pos,
                                                     MPI_COMM_WORLD,
                                                     largeSize_);
        // Upack and store knowledge from larger buffer.
        model_->unpackSharedKnowledge(*encodedModelGen);
        delete encodedModelGen;
        encodedModelGen = NULL;
    }
    
    //--------------------------------------------
    // Do one unit work or store the subtree
    //--------------------------------------------

    if (doUnitWork) {
        // Do one unit of work and report load info
        int requiredNumNodes = 2;
        nodeProcessedNum_ += rampUpSubTree_->rampUp(1,  // minimum number
                                                    requiredNumNodes, 
                                                    treeDepth_, 
                                                    node);
    
        // Send load infor to master
        int numNodes = rampUpSubTree_->getNumNodes();
        //std::cout << "numNodes = " << numNodes << std::endl;
        
        const int smallSize = model_->AlpsPar()->entry(AlpsParams::smallSize);

        pos = 0;
        MPI_Pack(&numNodes, 1, MPI_INT, smallBuffer_, smallSize, &pos,
                 MPI_COMM_WORLD);
        MPI_Send(smallBuffer_, pos, MPI_PACKED, masterRank_,
                 AlpsMsgRampUpLoad, MPI_COMM_WORLD);
        if (numNodes == 0) {
            delete rampUpSubTree_;
            rampUpSubTree_ = NULL;
        }
    }
    else {
        // Put rampup subtree in subtree pool
        rampUpSubTree_->calculateQuality();
	addKnowledge(AlpsKnowledgeTypeSubTree,
                     rampUpSubTree_,
                     rampUpSubTree_->getQuality());
        rampUpSubTree_ = NULL;
    }
}

//#############################################################################

// TODO: With model knowledge?
void 
AlpsKnowledgeBrokerMPI::spiralDonateNode()
{
    // Get receiver rank
    int i;
    int position = 0;
    int receiver = -1;

    MPI_Unpack(largeBuffer_, largeSize_, &position, &receiver, 1, 
	       MPI_INT, MPI_COMM_WORLD);

    // Get a node for receiver
    AlpsTreeNode* node = dynamic_cast<AlpsTreeNode* >
	(rampUpSubTree_->nodePool()->getKnowledge().first);
    AlpsEncoded* enc = node->encode();
    
    rampUpSubTree_->nodePool()->popKnowledge();
    
    // Update parent linkage
    AlpsTreeNode *parent = node->getParent();
    int numChildren = -1;
    if (parent) {
        numChildren = parent->getNumChildren();
        for (i = 0; i < numChildren; ++i) {
            if (parent->getChild(i) == node) break;
        }
        parent->setChild(i, parent->getChild(numChildren - 1));
        parent->setChild(numChildren - 1, NULL);
        parent->modifyNumChildren(-1);  // A child have gone
    }

    // Update children's linkage
    numChildren = node->getNumChildren();
    AlpsTreeNode *child = NULL;
    for (i = 0; i < numChildren; ++i) {
        child = node->getChild(i);
        child->setParent(NULL);
        child->setParentIndex(-1);
    }
    
    // Delete the node since it been sent to other process
    delete node;
    node = NULL;

    // Send the encoded node
    position = 0;
    int doUnitWork = 1;
    int hasKnowledge = 0;

    // Pack if doUnitWork
    MPI_Pack(&doUnitWork, 1, MPI_INT, largeBuffer_, largeSize_, &position,
             MPI_COMM_WORLD);
    packEncoded(enc, largeBuffer_, largeSize_, position, MPI_COMM_WORLD);
    MPI_Send(largeBuffer_, position, MPI_PACKED, receiver, AlpsMsgNode, 
             MPI_COMM_WORLD); 
    MPI_Pack(&hasKnowledge, 1, MPI_INT, largeBuffer_, largeSize_, &position,
             MPI_COMM_WORLD);
    if (enc) {
	delete enc;  
	enc = 0;                 // Allocated in encode()
    }
    
    // Do one unit of work
    int requiredNumNodes = 2;
    nodeProcessedNum_ += rampUpSubTree_->rampUp(1,  // minimum number
                                                requiredNumNodes, 
                                                treeDepth_, 
                                                NULL);
    
    // Send load infor to master
    int numNodes = rampUpSubTree_->getNumNodes();    
    const int smallSize = model_->AlpsPar()->entry(AlpsParams::smallSize);
    int pos = 0;
    MPI_Pack(&numNodes, 1, MPI_INT, smallBuffer_, smallSize, &pos,
             MPI_COMM_WORLD);
    MPI_Send(smallBuffer_, pos, MPI_PACKED, masterRank_, AlpsMsgRampUpLoad, 
             MPI_COMM_WORLD);

    if (numNodes == 0) {
        delete rampUpSubTree_;
        rampUpSubTree_ = NULL;
    }
}

//#############################################################################
