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

#include <algorithm>
#include <cassert>
#include <cstring>

#include "CoinError.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinTime.hpp"

#include "AlpsHelperFunctions.h"
#include "AlpsKnowledgeBrokerMPI.h"
#include "AlpsMessageTag.h"
#include "AlpsModel.h"
#include "AlpsNodePool.h"
#include "AlpsTreeNode.h"

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

static int parentSequence(const int sequence, const int numProcesses)
{ 
    if (sequence <= 0) return -1;
    else return (sequence - 1) / 2; 
}

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

void 
AlpsKnowledgeBrokerMPI::masterMain(AlpsTreeNode* root)
{
    //----------------------------------------------------------------
    // Start to count time.
    //----------------------------------------------------------------

    int i, j, k, t;
    int count;
    int preSysSendCount    = 0;
    int position           = 0;
    int masterCheckCount   = 0;
    int hubCheckCount      = 0;
    int reportIntCount     = 0;
 
    double startTime       = 0.0;
    double curTime         = 0.0;

    char reply;
    char* buf              = NULL; 

    bool allWorkerReported = false;   // All workers report load at least once
    bool allMsgReceived    = false;   // Indicate if no msg at that time
    bool terminate         = false;   // Normal terminate
    
    //------------------------------------------------------
    // Parameter.
    //------------------------------------------------------
    
    const bool interCB = 
	model_->AlpsPar()->entry(AlpsParams::interClusterBalance);
    const bool intraCB = 
	model_->AlpsPar()->entry(AlpsParams::intraClusterBalance);
    const int largeSize    = 
	model_->AlpsPar()->entry(AlpsParams::largeSize);
    const int medSize      = 
	model_->AlpsPar()->entry(AlpsParams::mediumSize);
    const int smallSize    = 
	model_->AlpsPar()->entry(AlpsParams::smallSize);
    const int nodeLimit    = 
	model_->AlpsPar()->entry(AlpsParams::nodeLimit);
    const double timeLimit = 
	model_->AlpsPar()->entry(AlpsParams::timeLimit);
    const double period    = 
	model_->AlpsPar()->entry(AlpsParams::masterBalancePeriod);
    const double zeroLoad  = 
	model_->AlpsPar()->entry(AlpsParams::zeroLoad);

    //------------------------------------------------------
    // Initialization and setup.
    //------------------------------------------------------

    hubNodeProcesseds_     = new int [hubNum_];     // Record node processed
    hubWorkQualities_      = new double [hubNum_];  // Record quality
    hubWorkQuantities_     = new double [hubNum_];  // Record quantity
    hubReported_           = new bool [hubNum_];
    workerNodeProcesseds_  = new int [clusterSize_+1];// FIXME: strange error
    workerWorkQualities_   = new double [clusterSize_+1];
    workerWorkQuantities_  = new double [clusterSize_+1];
    workerReported_        = new bool [clusterSize_+1];
  
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

    workerNodeProcesseds_[i] = nodeProcessedNum_;
    workerWorkQualities_[0] = workQuality_;
    workerWorkQuantities_[0] = workQuantity_;

    root->setKnowledgeBroker(this); 
    root->setQuality(ALPS_OBJ_MAX);
    root->setDepth(0);
    root->setIndex(0);
    root->setExplicit(1); // Always true for root.

    //------------------------------------------------------
    // Master's Ramp-up.
    //------------------------------------------------------

    setPhase(ALPS_PHASE_RAMPUP);

#ifdef NF_DEBUG
    std::cout << "MASTER: before rampup." << std::endl;
#endif

    //------------------------------------------------------
    // Estimate a tree node size and send it to other hubs.
    //------------------------------------------------------

    AlpsEncoded* encSize = root->encode();
    // TODO: Why 3.5?
    setNodeMemSize(static_cast<int>(encSize->size() * 3.5));

    for (i = 0; i < hubNum_; ++i) {
	if (hubRanks_[i] != globalRank_) {
	    MPI_Send(&nodeMemSize_, 1, MPI_INT, i, AlpsMsgNodeSize, hubComm_);
	}
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
    
    //------------------------------------------------------
    // Create required number of subtrees(nodes) for hubs.
    //------------------------------------------------------

    if (msgLevel_ > 0) {
	messageHandler()->message(ALPS_RAMPUP_MASTER_START, messages())
	    << globalRank_ << CoinMessageEol;
    }

    // Temporary store node for hub 0(master)
    AlpsNodePool* tempNodePool = new AlpsNodePool;

    // Best-first node selection duraing rampup
    AlpsNodeSelectionBest *rampupNodeSel = new AlpsNodeSelectionBest;    

    double rampUpStart = CoinCpuTime();
    AlpsSubTree* subTree = dynamic_cast<AlpsSubTree*>
	(const_cast<AlpsKnowledge *>(decoderObject("ALPS_SUBTREE")))
	->newSubTree();

    tempNodePool->setNodeSelection(*rampupNodeSel);	
    subTree->setKnowledgeBroker(this);
    subTree->setNodeSelection(rampupNodeSel);
    subTree->setNextIndex(1); // One more than root's index
    
    nodeProcessedNum_ += subTree->rampUp(treeDepth_, root);
    
    int numGenNodes = subTree->nodePool()->getNumKnowledges();

    //------------------------------------------------------
    // Spawn hub/worker processes (For dynamic process management only) 
    // Nothing now. FILL IN HERE IF NEEDED LATER.
    //------------------------------------------------------
 
    //------------------------------------------------------
    // Distribute subtrees(nodes) to hubs in Round-robin way.
    //------------------------------------------------------
    
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
		    sendSizeNode(i, subTree, hubComm_);	
		    // NOTE: master's rank is always 0 in hubComm_.
                    if (msgLevel_ > 100) {
                        std::cout << "master send a node to hub " << hubRanks_[i]
                                  << std::endl;
                    }
		}
		else {
		    AlpsTreeNode* nodeM = dynamic_cast<AlpsTreeNode* >
			(subTree->nodePool()->getKnowledge().first);
		    subTree->nodePool()->popKnowledge();
		    tempNodePool->addKnowledge(nodeM, nodeM->getQuality());
		}
		++numSent;
	    }
	}
    }

    //------------------------------------------------------
    // Move nodes from tempNodePool to subTree's node pool.
    // NOTE: These nodes do not necessary form a subtree. We want to use
    //       subtree's functions to generates more nodes for workers.
    //------------------------------------------------------

    while (tempNodePool->hasKnowledge()) {
	AlpsTreeNode* nodeT = dynamic_cast<AlpsTreeNode* > 
	    (tempNodePool->getKnowledge().first); 
	double valT = tempNodePool->getKnowledge().second;
	subTree->nodePool()->addKnowledge(nodeT, valT);
	tempNodePool->popKnowledge();
    }

    //------------------------------------------------------
    // Sent INITIALIZATION_COMPLETE tag to hubs.
    //------------------------------------------------------

    for (i = 0; i < hubNum_; ++i) {
	if (hubRanks_[i] != globalRank_) sendFinishInit(i, hubComm_);
    }
    
    double rampUpTimeMaster = AlpsCpuTime() - rampUpStart;

    //------------------------------------------------------
    // If have solution, broadcast its value and process id.
    //------------------------------------------------------

    if (hasKnowledge(ALPS_SOLUTION)) {
	double incVal = getBestKnowledge(ALPS_SOLUTION).second;
	if(incVal < incumbentValue_) {   // Assume Minimization
	    incumbentValue_ = incVal;
	    incumbentID_ = globalRank_;
	    //masterSendHubsIncumbent();
	    sendIncumbent();
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
            << globalRank_ << rampUpTimeMaster << nodeProcessedNum_ << numGenNodes
            << CoinMessageEol;
    }
 
    //------------------------------------------------------
    // Generate and send required number of nodes for master's workers.
    //------------------------------------------------------

    if (msgLevel_ > 0) {
        messageHandler()->message(ALPS_RAMPUP_HUB_START, messages())
            << globalRank_ << CoinMessageEol;
    }
    
    int treeSizeByHub = subTree->rampUp(treeDepth_);

    nodeProcessedNum_ += treeSizeByHub;
    const int numNode2 = subTree->nodePool()->getNumKnowledges();
 
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
		    sendSizeNode(i, subTree, clusterComm_);
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
 
    //------------------------------------------------------
    // Sent INITIALIZATION_COMPLETE tag to master's workers.
    //------------------------------------------------------

    for (i = 0; i < clusterSize_; ++i) {
	if (i != clusterRank_) {
	    sendFinishInit(i, clusterComm_);
	}
    }

    rampUpTime_ = CoinCpuTime() - rampUpStart;

    if (msgLevel_ > 0) {
        messageHandler()->message(ALPS_RAMPUP_HUB, messages())
            << globalRank_ << (rampUpTime_ - rampUpTimeMaster) 
            << treeSizeByHub << numNode2 << CoinMessageEol;
    }

    //------------------------------------------------------
    // If have better solution, broadcast its value and process id.
    //------------------------------------------------------

    if (hasKnowledge(ALPS_SOLUTION)) {
	double incVal = getBestKnowledge(ALPS_SOLUTION).second;
	if(incVal < incumbentValue_) {   // Assume Minimization
	    incumbentValue_ = incVal;
	    incumbentID_ = globalRank_;
	    // masterSendHubsIncumbent();
	    sendIncumbent();
            if (msgLevel_ > 0) {
                messageHandler()->message(ALPS_RAMPUP_HUB_SOL, messages())
                    << globalRank_ << incVal << CoinMessageEol;
            }
	} 
    }

    //------------------------------------------------------
    // End of Master's Ramp-up and start to search.
    //------------------------------------------------------

    setPhase(ALPS_PHASE_SEARCH);

    /* Reset to normal selection. */
    subTree->setNodeSelection(nodeSelection_);

#ifdef NF_DEBUG
    std::cout << "MASTER: after rampup." << std::endl;
#endif
    
    //======================================================
    // MASTER SCHEDULER: 
    // a. Listen and process messages periodically. 
    // b. Do termination check if the conditions are statisfied, otherwise,
    //    balances the workload of hubs.
    //======================================================

    MPI_Request request;
    MPI_Status status;
    int flag;
    
    char *buffer = new char [largeSize];
    MPI_Irecv(buffer, largeSize, MPI_PACKED, MPI_ANY_SOURCE, 
	      MPI_ANY_TAG, MPI_COMM_WORLD, &request);

    while (true) {
	startTime = CoinCpuTime();
	while ( CoinCpuTime() - startTime < period ) {
	    MPI_Test(&request, &flag, &status);
	    if (flag) { // Receive a msg
		processMessages(buffer, status, request);
	    }
	}
	
	//**------------------------------------------------
	// Check if all workers in this cluster have reported their status.
	//**------------------------------------------------
    
	if ( ! allWorkerReported ) {	
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
    
	if ( allHubReported_ != true ) {
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
            forceTerminate_ = true;
            if (timer_.reachWallLimit()) {
                setSolStatus(ALPS_TIME_LIMIT);
                masterForceHubTerm();
                hubForceWorkerTerm();
            }
            else if (systemNodeProcessed_ >= nodeLimit) {
                std::cout << "==== Master ask hubs terminate due to reaching node limit "
                          << nodeLimit << std::endl;
                setSolStatus(ALPS_NODE_LIMIT);
                masterForceHubTerm();
                hubForceWorkerTerm();
            }
        }
	
	//**------------------------------------------------
        // Print workload information to screen.
	//**------------------------------------------------

	const int reportInt = 
	    model_->AlpsPar()->entry(AlpsParams::nodeLogInterval);

	assert(reportInt > 0);

	if (reportIntCount % reportInt == 0) {
            if (msgLevel_ > 0) {
                messageHandler()->message(ALPS_LOADREPORT_MASTER, messages())
                    << globalRank_ << systemNodeProcessed_ << systemWorkQuantity_ 
                    << systemSendCount_ << systemRecvCount_ << incumbentValue_ 
                    << CoinMessageEol;
            }
            
#ifdef NF_DEBUG
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
	++reportIntCount;
        
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

#ifdef NF_DEBUG
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

	    // Ask other hubs to do termination check.
	    for (i = 0; i < hubNum_; ++i) {
		if (hubRanks_[i] != globalRank_) {
		    MPI_Send(buf, 0, MPI_PACKED, hubRanks_[i], 
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
		    MPI_Send(buf, 0, MPI_PACKED, i, 
			     AlpsMsgAskPause, MPI_COMM_WORLD);
		}
	    }
	    
	    if(buf != 0) {
		delete [] buf; 
		buf = 0;
	    }
	    buf = new char [smallSize];
	    MPI_Request termReq;
	    MPI_Status termSta;
	    
	    // Update the cluster status to which the Master belongs
	    for(i = 1; i < clusterSize_; ++i) {
		MPI_Irecv(buf, smallSize, MPI_PACKED, MPI_ANY_SOURCE,
			  AlpsMsgWorkerTermStatus, clusterComm_, &termReq);
		MPI_Wait(&termReq, &termSta);
		hubUpdateCluStatus(buf, &termSta, clusterComm_);
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
		MPI_Irecv(buf, smallSize, MPI_PACKED, MPI_ANY_SOURCE,
			  AlpsMsgHubTermStatus, hubComm_, &termReq);
		MPI_Wait(&termReq, &termSta);
		masterUpdateSysStatus(buf, &termSta, hubComm_);
	    }
	    
#ifdef NF_DEBUG
	    std::cout << "Master: TERM: finished updating hubs." <<std::endl;
#endif
	    // clusterWorkQuantity_ is always 0 in current design
	    // systemWorkQuality_ += clusterWorkQuality_;
	    
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
	    }
	    
#ifdef NF_DEBUG
	    std::cout << "Master: TERM: terminate=" << terminate <<std::endl;
#endif
	    // True idle, tell others to terminate.
	    if (terminate) {
                if (msgLevel_ > 0) {
                    messageHandler()->message(ALPS_TERM_MASTER_INFORM, messages())
                        << globalRank_ << "exit" << CoinMessageEol;
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
		break;  // Break *,  Master terminates
	    } 
	    else {  // Not true idle yet
                if (msgLevel_ > 0) {
                    messageHandler()->message(ALPS_TERM_MASTER_INFORM, messages())
                        << globalRank_ << "continue" << CoinMessageEol;
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
	}
      
	//**------------------------------------------------
	// If not terminate,  master balances work load of hubs.
	//**------------------------------------------------

	//std::cout << "masterDoBalance_ = " << masterDoBalance_ << std::endl;
	
	if ( ! terminate && allHubReported_ && masterDoBalance_ == 0 ) {
	    if (hubNum_ > 1 && interCB) {
		masterBalanceHubs();
		++masterCheckCount;
		if (masterCheckCount % 5 == 0) {
                    if (msgLevel_ > 0) {
                        messageHandler()->message(ALPS_LOADBAL_MASTER, messages())
                            << globalRank_ << masterCheckCount << CoinMessageEol;
                    }
		}
	    }
	}

	if ( ! terminate && allWorkerReported && hubDoBalance_ == 0 ) {
	    if (clusterSize_ > 2 && intraCB) {
		hubBalanceWorkers();
		++hubCheckCount;
		if (hubCheckCount % 5 == 0) {
                    if (msgLevel_ > 0) {
                        messageHandler()->message(ALPS_LOADBAL_HUB, messages())
                            << globalRank_ << hubCheckCount << CoinMessageEol;
                    }
		}
	    }
	}
    }

    //------------------------------------------------------
    // Cancel MPI_Irecv before leaving main().
    //------------------------------------------------------

    int cancelNum = 0;

    MPI_Cancel(&request);
    MPI_Wait(&request, &status);

    MPI_Test_cancelled(&status, &flag);
    if(flag) {  // Cancel succeeded
	++cancelNum;
    }
    
#if defined(NF_DEBUG_MORE)
    std::cout << "MASTER " << globalRank_ << " cancelled " << cancelNum 
	      << " MPI_Irecv" << std::endl;
#endif

    // Clean up before leaving
    if (buffer != 0) { 
	delete [] buffer; 
	buffer = 0; 
    }
    delete [] buf;

    delete rampupNodeSel;
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::hubMain()
{
    int i, k, t;
    int count;                        // Count msg size
    int hubCheckCount = 0;

    char reply;
    char* buf              = 0;
    double startTime       = 0.0;

    bool allMsgReceived    = false;
    bool allWorkerReported = false;   // Workers report load at least once
    bool terminate         = false;

    MPI_Status status;

    AlpsReturnCode rCode   = ALPS_OK;
    AlpsSolStatus solStatus = ALPS_INFEASIBLE;
    
    //------------------------------------------------------
    // Get parameters.
    //------------------------------------------------------

    const int largeSize    = 
	model_->AlpsPar()->entry(AlpsParams::largeSize);
    const int medSize      = 
	model_->AlpsPar()->entry(AlpsParams::mediumSize);
    const int smallSize    = 
	model_->AlpsPar()->entry(AlpsParams::smallSize);
    const bool intraCB     = 
	model_->AlpsPar()->entry(AlpsParams::intraClusterBalance);
    const double period    = 
	model_->AlpsPar()->entry(AlpsParams::hubReportPeriod);
    const double zeroLoad  = 
	model_->AlpsPar()->entry(AlpsParams::zeroLoad);
    const int unitWork     =
	model_->AlpsPar()->entry(AlpsParams::unitWorkNodes);
    const double unitTime  =
	model_->AlpsPar()->entry(AlpsParams::unitWorkTime);
    const double changeWorkThreshold = model_->AlpsPar()->
	entry(AlpsParams::changeWorkThreshold);
    const double rho = model_->AlpsPar()->entry(AlpsParams::rho);

    //------------------------------------------------------
    // Initialization and setup.
    //------------------------------------------------------
  
    workerNodeProcesseds_  = new int [clusterSize_];
    workerWorkQualities_   = new double [clusterSize_];
    workerWorkQuantities_  = new double [clusterSize_];
    workerReported_        = new bool [clusterSize_];

    for (i = 0; i < clusterSize_; ++i) {
	workerNodeProcesseds_[i] = 0;
	workerWorkQualities_[i] = ALPS_OBJ_MAX;
	workerWorkQuantities_[i] = 0.0;
    }
    
    workerNodeProcesseds_[0] = nodeProcessedNum_;
    workerWorkQualities_[0] = workQuality_;
    workerWorkQuantities_[0] = workQuantity_ = 0.0;

    double rampUpStart = CoinCpuTime();

    // Best-first node selection duraing rampup
    AlpsNodeSelectionBest *rampupNodeSel = new AlpsNodeSelectionBest;    
    
    AlpsSubTree* subTree = dynamic_cast<AlpsSubTree*>
        (const_cast<AlpsKnowledge *>(decoderObject("ALPS_SUBTREE")))->newSubTree();
    subTree->setKnowledgeBroker(this);
    subTree->setNodeSelection(rampupNodeSel);
    
    //------------------------------------------------------
    // Hub's Ramp-up.
    //------------------------------------------------------

    setPhase(ALPS_PHASE_RAMPUP);

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

    //------------------------------------------------------
    // Receive subtrees (nodes) sent by Master.
    // NOTE: These nodes are generally not from the same subtree. 
    // Just want to use subtree's functions to generates more nodes.
    //------------------------------------------------------

    while(true) {
	// NOTE: master's rank is always 0 in hubComm_.
	receiveSizeNode(0, subTree, hubComm_, &status);

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
    // Generate and send required number of nodes(subtree) for 
    // hub's workers.
    //------------------------------------------------------

    if (hubMsgLevel_ > 0) {
        messageHandler()->message(ALPS_RAMPUP_HUB_START, messages())
            << globalRank_ << CoinMessageEol;
    }

    nodeProcessedNum_ += subTree->rampUp(treeDepth_);
    
    const int numNode = subTree->nodePool()->getNumKnowledges();
    
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

		if ( (hubWork_ == true) && (i == masterRank_) ) {
		    // NOTE: masterRank_ is 0 or 1(debug).
		    // Hub need work, so Keep a node for self
		    
		    AlpsSubTree* myTree = dynamic_cast<AlpsSubTree*>
			(const_cast<AlpsKnowledge *>(
			    decoderObject("ALPS_SUBTREE")))->newSubTree();
		    myTree->setKnowledgeBroker(this);
		    myTree->setNodeSelection(nodeSelection_);
		    
		    AlpsTreeNode* node = static_cast<AlpsTreeNode* >
			(subTree->nodePool()->getKnowledge().first);
		    subTree->nodePool()->popKnowledge();

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
		    myTree->calculateQuality(incumbentValue_, rho);
		    addKnowledge(ALPS_SUBTREE, myTree, myTree->getQuality());
		    ++numSent;
		}
		if (i != clusterRank_) {
		    sendSizeNode(i, subTree, clusterComm_);
		    ++numSent;
		}
	    }
	}
    }
    
    //------------------------------------------------------
    // Sent finish initialization tag to workers so that they know to
    // stop receiving subtrees.
    //------------------------------------------------------

    for (i = 0; i < clusterSize_; ++i) {
	if (i != clusterRank_) {
	    sendFinishInit(i, clusterComm_);
	}
    }
    rampUpTime_ = CoinCpuTime() - rampUpStart;

    if (hubMsgLevel_ > 0) {
        messageHandler()->message(ALPS_RAMPUP_HUB, messages())
            << globalRank_ << rampUpTime_ << nodeProcessedNum_ << numNode
            << CoinMessageEol;
    }

    //------------------------------------------------------
    // If have better solution, broadcast its value and process id.
    //------------------------------------------------------

    if (hasKnowledge(ALPS_SOLUTION)) {
	double incVal = getBestKnowledge(ALPS_SOLUTION).second;
	if(incVal < incumbentValue_) {    // Assume Minimization
	    incumbentValue_ = incVal;
	    incumbentID_ = globalRank_;
	    //sendMasterIncumbent();      // Tell master
	    //hubSendWorkersIncumbent();  // Unpack with return false later
	    sendIncumbent();
            if (hubMsgLevel_ > 0) {
                messageHandler()->message(ALPS_RAMPUP_HUB_SOL, messages())
                    << globalRank_ << incVal << CoinMessageEol;
            }
	}
    }

    //------------------------------------------------------
    // End of Hub's Ramp-up and start to search.
    //------------------------------------------------------

    setPhase(ALPS_PHASE_SEARCH);

    /* Reset to normal selection. */
    subTree->setNodeSelection(nodeSelection_);
    
    //======================================================
    // HUB SCHEDULER:
    // (1) Listen and process messages periodically.
    // (2) If required, do one unit of work.
    // (3) Send work quality, quantity, and msg counts to Master.
    // (4) Balance workload quality of my workers.
    // (5) Do termination check if requred.
    //======================================================

    MPI_Request request;
    int flag = 1;
    //int reportCount = 0; // don't know what's the use, 11/27/06
    char* buffer = new char [largeSize];
    
    MPI_Irecv(buffer, largeSize, MPI_PACKED, MPI_ANY_SOURCE, 
	      MPI_ANY_TAG, MPI_COMM_WORLD, &request);

    while (true) {
	//++reportCount;
	flag = 1;
	startTime = CoinCpuTime();
	
	//**------------------------------------------------
	// Listen and process msg. If time is used up or no message,
        // exit loop.
	//**------------------------------------------------

	while ( CoinCpuTime() - startTime < period ) {
	    MPI_Test(&request, &flag, &status);
	    if (flag) { // Receive a msg
		processMessages(buffer, status, request);
	    }
	}
        
	//**------------------------------------------------
        // if forceTerminate_ == true;
	//**------------------------------------------------
        
        if (forceTerminate_) {
            //if (hubMsgLevel_ > 0) {
            std::cout << "HUB["<< globalRank_ << "] is asked to terminate by Master"
                      << std::endl;
            //}

            // Delete all subtrees if hub work.
            if (hubWork_) {   
                deleteSubTrees();
            }
            hubForceWorkerTerm();
        }
        
	//**------------------------------------------------
	// Check if all my workers have reported.
	//**------------------------------------------------

	if ( !allWorkerReported ) {
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

	if ( hubWork_ && (workingSubTree_ != 0 || hasKnowledge(ALPS_SUBTREE)) ) {

	    //reportCount = hubReportFreqency;

	    // NOTE: will stop when there is a better solution.
	    bool betterSolution = true;
            int thisNumNodes = 0;
            
	    rCode = doOneUnitWork(unitWork, 
                                  unitTime, 
                                  solStatus,
                                  thisNumNodes,
                                  treeDepth_, 
                                  betterSolution);
            nodeProcessedNum_ += thisNumNodes;

	    // Update work load quantity and quality info.
	    updateWorkloadInfo();
	    
	    // Adjust workingSubTree_ if it 'much' worse than the best one.
	    if (subTreePool_->hasKnowledge() && workingSubTree_ != 0) {
		double curQuality = workingSubTree_->getQuality();
		double topQuality = subTreePool_->getKnowledge().second;
		if (curQuality > topQuality) {
		    double perDiff = (curQuality - topQuality)/
			(curQuality + 1.0);
		    if (perDiff > changeWorkThreshold) {
#ifdef NP_DEBUG
			std::cout << "Change subtree " << curQuality
				  << " to " << topQuality << std::endl;
#endif			
			AlpsSubTree* tempST = workingSubTree_;
			workingSubTree_ = dynamic_cast<AlpsSubTree* >(
			    subTreePool_->getKnowledge().first);
			subTreePool_->popKnowledge();
			addKnowledge(ALPS_SUBTREE, tempST, curQuality);
		    }
		}
	    }

#ifdef NF_DEBUG_MORE
	    std::cout << "******** HUB [" << globalRank_ << "]: "
		      << " quality = " << workQuality_
		      << "; quantity = " << workQuantity_ << std::endl;
#endif

	    if (betterSolution) {
		// Double check if better. If yes, update and sent it to Master
		double incVal = getBestKnowledge(ALPS_SOLUTION).second;
		if(incVal < incumbentValue_) {  // Minimization
		    incumbentValue_ = incVal;
		    incumbentID_ = globalRank_;
		    // sendMasterIncumbent();
		    sendIncumbent();
                    if (hubMsgLevel_ > 0) {
                        messageHandler()->message(ALPS_SOLUTION_SEARCH, messages())
                            << globalRank_ << incVal << CoinMessageEol;
                    }
		} 
	    }
	}

	//**------------------------------------------------
	// Add in the status of hub itself. Need to check if report.
	//**------------------------------------------------

	refreshClusterStatus();
	
#ifdef NF_DEBUG_MORE
	std::cout << "HUB "<< globalRank_ 
		  << ": clusterWorkQuality_  = " << clusterWorkQuality_
		  << ": clusterWorkQuantity_  = " << clusterWorkQuantity_
		  << ", sendCount_ = " << sendCount_
		  << ", recvCount_ = " << recvCount_
		  << ", workReportNum = " << workReportNum
		  << ", allWorkerReported = " << allWorkerReported
		  << ", blockHubCheck = " << blockHubCheck
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
	    if(buf != 0) {
		delete [] buf; 
		buf = 0;
	    }
	    buf = new char [smallSize];
	    
	    // Ask hub's workers to check termination
	    for (i = 0; i < clusterSize_; ++i) {
		if (i != clusterRank_) {
		    int workRank = globalRank_ - masterRank_ + i;
		    MPI_Send(buf, 0, MPI_PACKED, workRank, 
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
		MPI_Irecv(buf, smallSize, MPI_PACKED, MPI_ANY_SOURCE,
			  AlpsMsgWorkerTermStatus, clusterComm_, &termReq);
		MPI_Wait(&termReq, &termSta);
		hubUpdateCluStatus(buf, &termSta, clusterComm_);
	    }
	    //workQuantity_ and workQuality doesn't exit for me (a hub)
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
	    MPI_Irecv(&reply, 1, MPI_CHAR, 0, AlpsMsgContOrTerm, hubComm_,&termReq);
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
	    }
	}

	//**------------------------------------------------
	// If not terminate,  hub balances work load of its workers.
	//**------------------------------------------------

	//std::cout << "hubDoBalance_ = " << hubDoBalance_ << std::endl;

	if ( ! terminate && allWorkerReported && hubDoBalance_ == 0 ) {
	    if (clusterSize_ > 2 && intraCB) {
		hubBalanceWorkers();
		++hubCheckCount;
		if (hubCheckCount%5 == 0) {
                    if (hubMsgLevel_ > 0) {
                        messageHandler()->message(ALPS_LOADBAL_HUB, messages())
                            << globalRank_ << hubCheckCount << CoinMessageEol;
                    }
		}
	    }
	}
	
    } // while

    //------------------------------------------------------
    // Cancel MPI_Irecv before leaving main().
    //------------------------------------------------------

    int cancelNum = 0;
    MPI_Cancel(&request);
    MPI_Wait(&request, &status);
    MPI_Test_cancelled(&status, &flag);
    if(flag) {  // Cancel succeeded
	++cancelNum;
    }

#if defined(NF_DEBUG_MORE)
    std::cout << "HUB " << globalRank_ << " cancelled " << cancelNum 
	      << " MPI_Irecv" << std::endl;
#endif

    // Free memory
    if (buffer != 0) { 
	delete [] buffer; 
	buffer = 0; 
    }
    delete rampupNodeSel;
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::workerMain()
{
    int i, t;
    int count              = 0;
    int firstIdle          = 0;
    int thisNumNodes       = 0;
    
    char reply;
    char* buf              = 0;

    double idleStart       = 0.0;
    double rampUpStart     = 0.0; 
    double rampDownStart   = 0.0;

    bool terminate         = false;
    bool allMsgReceived    = false;
    
    MPI_Status status;
    AlpsReturnCode rCode = ALPS_OK;
    AlpsSolStatus solStatus = ALPS_INFEASIBLE;
    
    //------------------------------------------------------
    // Parameter.
    //------------------------------------------------------
    
    const int unitNodes    =
	model_->AlpsPar()->entry(AlpsParams::unitWorkNodes);
    const int nodeLogInterval =
	model_->AlpsPar()->entry(AlpsParams::nodeLogInterval);
    
    const int largeSize    = 
	model_->AlpsPar()->entry(AlpsParams::largeSize);
    const int medSize      = 
	model_->AlpsPar()->entry(AlpsParams::mediumSize);
    const int smallSize    = 
	model_->AlpsPar()->entry(AlpsParams::smallSize);

    const int workerMsgLevel  = 
        model_->AlpsPar()->entry(AlpsParams::workerMsgLevel);
 
    const double zeroLoad  = 
	model_->AlpsPar()->entry(AlpsParams::zeroLoad); 

    const double changeWorkThreshold = 
	model_->AlpsPar()->entry(AlpsParams::changeWorkThreshold);

    const double needWorkThreshold = 
	model_->AlpsPar()->entry(AlpsParams::needWorkThreshold);

    const bool intraCB     = 
	model_->AlpsPar()->entry(AlpsParams::intraClusterBalance);

    const int unitWork     =
	model_->AlpsPar()->entry(AlpsParams::unitWorkNodes);
    const double unitTime  =
	model_->AlpsPar()->entry(AlpsParams::unitWorkTime);
    const double rho       = 
	model_->AlpsPar()->entry(AlpsParams::rho);

    rampUpStart = CoinCpuTime();

    //------------------------------------------------------
    // Recv tree node size from hub.
    //------------------------------------------------------

    MPI_Recv(&nodeMemSize_, 1, MPI_INT, masterRank_, /* masterRank_ is 0 or 1*/
	     AlpsMsgNodeSize, clusterComm_, &status);
    
    //------------------------------------------------------
    // Recv subtrees(nodes) sent by my hub.
    //------------------------------------------------------

#ifdef NF_DEBUG
    // DEBUG
    std::cout << "WORKER["<< globalRank_ << "]: before rampup." << std::endl;
#endif

    while(true) {
	AlpsSubTree* subTree = dynamic_cast<AlpsSubTree*>
	    (const_cast<AlpsKnowledge *>(decoderObject("ALPS_SUBTREE")))->
	    newSubTree();
	subTree->setKnowledgeBroker(this);
	subTree->setNodeSelection(nodeSelection_);

	// NOTE: hub rank is masterRank_ in clusterComm_.
	receiveSizeNode(masterRank_, subTree, clusterComm_, &status);

	if (status.MPI_TAG == AlpsMsgFinishInit) {
            if (workerMsgLevel > 0) {
                messageHandler()->message(ALPS_RAMPUP_WORKER_RECV, messages())
                    << globalRank_ << myHubRank_ << CoinMessageEol;
            }
	    break;
	}

	assert(subTree->getNumNodes() > 0);

	subTree->calculateQuality(incumbentValue_, rho);
	addKnowledge(ALPS_SUBTREE, subTree, subTree->getQuality()); 
    }

    rampUpTime_ = CoinCpuTime() - rampUpStart;

#ifdef NF_DEBUG
    // DEBUG
    std::cout << "WORKER["<< globalRank_ << "]: after rampup." << std::endl;
#endif

    if (workerMsgLevel > 0) {
        messageHandler()->message(ALPS_SEARCH_WORKER_START, messages())
            <<globalRank_ << CoinMessageEol;
    }
    
    //======================================================
    // WORKER SCHEDULER:
    // (1) Listen and process messages until no message is in msg queue.
    // (2) Do one unit of work.
    // (3) Report status or check termination.
    //======================================================

    MPI_Request request;
    
    int flag;
    
    char *buffer = new char [largeSize];

    MPI_Irecv(buffer, largeSize, MPI_PACKED, MPI_ANY_SOURCE, 
	      MPI_ANY_TAG, MPI_COMM_WORLD, &request);
  
    while (true) {
	
	blockTermCheck_ = true;
	
	//**------------------------------------------------
	// Listen and process until no message.
	//**------------------------------------------------
	
	while (true) {
	    MPI_Test(&request, &flag, &status);
	    if (flag) { // Receive a msg
		allMsgReceived = false;
		processMessages(buffer, status, request);
	    } 
	    else {
		allMsgReceived = true; 
		break;
	    }
	}

	//**------------------------------------------------
        // if forceTerminate_ == true;
	//**------------------------------------------------

        if (forceTerminate_) {
            // Do we want to clean up (msg, etc.)?
            //if (workerMsgLevel_ > 0) {
            std::cout << "Worker[" << globalRank_ 
                      << "] is asked to terminate by its hub" << std::endl;
            //}
            
            // Remove nodes from node pools
            deleteSubTrees();
        }
        
	idleStart = CoinCpuTime();
	
	//**------------------------------------------------
	// If don't check termination, do one unit of work.
	//**------------------------------------------------
	
	if (blockTermCheck_) {
	    
	    bool betterSolution = false;

	    if ( workingSubTree_ != 0 || hasKnowledge(ALPS_SUBTREE) ) {
		if(firstIdle != 0) {
		    idleTime_ +=  CoinCpuTime() - idleStart;
		    firstIdle = 0;
		}
		
		// Check whether need ask for node index before doing work
		if (getMaxNodeIndex() - getNextNodeIndex() < (unitWork + 5)) {
		    workerAskRecvIndices();
		}

		// Need check better solution.
		betterSolution = true;
		rCode = doOneUnitWork(unitWork, 
                                      unitTime,
                                      solStatus,
                                      thisNumNodes,
                                      treeDepth_, 
                                      betterSolution);
                nodeProcessedNum_ += thisNumNodes;
                
		// Adjust workingSubTree_ if it 'much' worse than the best one
		if (subTreePool_->hasKnowledge() && (workingSubTree_ != 0)) {

		    double curQuality = workingSubTree_->getQuality();
		    double topQuality = subTreePool_->getKnowledge().second;
		    
		    if (curQuality > topQuality) {
			double perDiff = (curQuality - topQuality)/
			    (curQuality + 1.0);
			if (perDiff > changeWorkThreshold) {
#ifdef NP_DEBUG
			    std::cout << "Change subtree " << curQuality
				      << " to " << topQuality << std::endl;
#endif	
			    AlpsSubTree* tempST = workingSubTree_;
			    workingSubTree_ = dynamic_cast<AlpsSubTree* >(
				subTreePool_->getKnowledge().first);
			    subTreePool_->popKnowledge();
			    addKnowledge(ALPS_SUBTREE, tempST, curQuality);
			}
		    }
		}
		
		// Print tree size    
		if (msgLevel_ == 1 && nodeProcessedNum_ % nodeLogInterval == 0) {
                    if (workerMsgLevel > 0) {
                        messageHandler()->message(ALPS_NODE_COUNT, messages())
                            << globalRank_ << nodeProcessedNum_<<updateNumNodesLeft()
                            << CoinMessageEol;
                    }
		}
		rampDownStart = CoinCpuTime();
	    } 
	    else {  // Worker is idle :-(
		idleStart = CoinCpuTime();
		firstIdle = 1;		
	    }
            
	    // If has better solution, check whether need to send it
	    if (betterSolution) {
		// Double check if better. If yes, update and sent it to Master
		double incVal = getBestKnowledge(ALPS_SOLUTION).second;
		if(incVal < incumbentValue_) {  // Minimization
		    incumbentValue_ = incVal;
		    incumbentID_ = globalRank_;
		    //sendMasterIncumbent();
#ifdef NF_DEBUG
		    std::cout << "\nWORKDER[" << globalRank_ <<
			"]: send a better solution. Quality = "
			      << incumbentValue_ << std::endl;
#endif 
		    sendIncumbent();
                    if (workerMsgLevel > 0) {
                        messageHandler()->message(ALPS_SOLUTION_SEARCH, messages())
                            << globalRank_ << incVal << CoinMessageEol;
                    }
		} 
	    }

	    // Report its status to hub periodically if msg counts are not 
	    // zero or not blocked. If no load, I will block report 
	    if (sendCount_ || recvCount_ || !blockWorkerReport_) {

		updateWorkloadInfo();

		incSendCount("workerMain() - workerReportStatus"); 

		workerReportStatus(AlpsMsgWorkerStatus, MPI_COMM_WORLD);

		if (workQuantity_ < zeroLoad) {
		    blockWorkerReport_ = true;
		}
#if 1
		if ( intraCB && 
		     (workQuantity_ < needWorkThreshold) && 
		     (blockAskForWork_ == false) ) {
		    MPI_Send(buf, 0, MPI_PACKED, myHubRank_, 
			     AlpsMsgWorkerNeedWork, MPI_COMM_WORLD);
		    incSendCount("workerAskForWork");
		    blockAskForWork_ = true;
		}
#endif
                
#ifdef NF_DEBUG
		std::cout << "WORKER: " << globalRank_ 
			  << " after updateWorkloadInfo() = "
		    	  << workQuantity_ << std::endl;
#endif
	    }
	} 
	else { 
#ifdef NF_DEBUG
	    std::cout << "WORKER[" << globalRank_ << "] check termination."
		      << std::endl;
#endif
	    // Do termination checking
	    rampDownTime_ = CoinCpuTime() - rampDownStart;
	    
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
		blockTermCheck_ = true;
		terminate = false;
	    }
	}
    }

    //------------------------------------------------------
    // How many node left? If complete search, left 0.
    //------------------------------------------------------

    updateNumNodesLeft();
    
    //------------------------------------------------------
    // Cancel MPI_Irecv before leaving main().
    //------------------------------------------------------

    int cancelNum = 0;
    MPI_Cancel(&request);
    MPI_Wait(&request, &status);
    MPI_Test_cancelled(&status, &flag);
    if(flag) {  // Cancel succeeded
	++cancelNum;
    }

#if defined(NF_DEBUG_MORE)
    std::cout << "HUB " << globalRank_ << " cancelled " << cancelNum 
	      << " MPI_Irecv" << std::endl;
#endif
    
    // Free memory
    if (buffer != 0) { 
	delete [] buffer; 
	buffer = 0; 
    }

}

//#############################################################################

// Process messages. 
void 
AlpsKnowledgeBrokerMPI::processMessages(char *&buf, 
                                        MPI_Status &status, 
                                        MPI_Request &request)
{
    int count;
    
    incRecvCount("Processing msg");
    switch (status.MPI_TAG) {
	
	//--------------------------------------------------
	// Following are master's msgs.
	//--------------------------------------------------

    case AlpsMsgHubPeriodReport:
	masterUpdateSysStatus(buf, &status, MPI_COMM_WORLD);
	break;
    case AlpsMsgTellMasterRecv:
	--masterDoBalance_;
	break;
    case AlpsMsgHubAskIndices:
	masterSendIndices(status.MPI_SOURCE);
	break;
	
	//-------------------------------------------------
	// Following are hub's msgs.
	//-------------------------------------------------

    case AlpsMsgWorkerNeedWork:
	hubSatisfyWorkerRequest(buf, &status);
	break;
    case AlpsMsgAskHubShare:
	hubsShareWork(buf, &status);
	break;
    case AlpsMsgAskHubPause: // Msg during term is NOT counted
	decRecvCount("hub periodical listening: AlpsMsgAskPause"); 
	blockTermCheck_ = false;
	break;
    case AlpsMsgTellHubRecv:
	--hubDoBalance_;
	break;
    case AlpsMsgWorkerStatus:
	hubUpdateCluStatus(buf, &status, MPI_COMM_WORLD);
	blockHubReport_ = false;
	break;
    case AlpsMsgWorkerAskIndices:
	hubSendIndices(status.MPI_SOURCE);
	break;
	
	//--------------------------------------------------
	// Following are worker's msg.
	//-------------------------------------------------

    case AlpsMsgAskDonate:
	donateWork(buf, AlpsMsgSubTree, &status);
	incSendCount("worker listening - AlpsMsgAskDonate");
	break;
    case AlpsMsgAskDonateToHub:
	donateWork(buf, AlpsMsgSubTreeByMaster, &status);
	incSendCount("worker listening - AlpsMsgAskDonateToHub");
	break;
    case AlpsMsgAskDonateToWorker:
	donateWork(buf, AlpsMsgSubTreeByWorker, &status);
	incSendCount("worker listening - AlpsMsgAskDonateToWorker");
	break;
    case AlpsMsgAskPause: //Msg during term is NOT counted
	decRecvCount("worker listening - AlpsMsgAskPause");
	blockTermCheck_ = false;
	break;
	//   case AlpsMsgHubPeriodCheck:
	//	incSendCount("worker listening - AlpsMsgHubPeriodCheck");  
	//workerReportStatus(AlpsMsgWorkerStatus, MPI_COMM_WORLD);
	//break;
    case AlpsMsgSubTree:
	receiveSubTree(buf, status.MPI_SOURCE, &status);
	tellHubRecv();
	MPI_Get_count(&status, MPI_PACKED, &count);
	if (count > 0){
	    blockWorkerReport_ = false;
	    //if(stati[t].MPI_SOURCE != myHubRank_)
	    blockAskForWork_  = false;
	}
	break;
    case AlpsMsgSubTreeByWorker:
	receiveSubTree(buf, status.MPI_SOURCE, &status);
	MPI_Get_count(&status, MPI_PACKED, &count);
	if (count > 0){
	    blockWorkerReport_ = false;
	    blockAskForWork_  = false;
	}
	break;

	//--------------------------------------------------
	// Following are common msgs.
	//--------------------------------------------------

    case AlpsMsgIncumbentTwo:
    {
	bool success = unpackSetIncumbent(buf, &status);
	if (success) {
	    sendIncumbent();
	    //updateIncumbent_ = false;
	}
	break;
    }
    case AlpsMsgSubTreeByMaster:
	if (globalRank_ == masterRank_) {
	    hubAllocateDonation(buf, &status);
	    MPI_Get_count(&status, MPI_PACKED, &count);
	    --masterDoBalance_;
	}
	else if (globalRank_ == myHubRank_) {
	    hubAllocateDonation(buf, &status);
	    tellMasterRecv();
	    MPI_Get_count(&status, MPI_PACKED, &count);
	    if (count > 0) {
		blockHubReport_ = false;
	    }
	}
	else {
	    receiveSubTree(buf, status.MPI_SOURCE, &status);
	    MPI_Get_count(&status, MPI_PACKED, &count);
	    if (count > 0) blockWorkerReport_ = false;
	    break;
	}
	
	break;
    case AlpsMsgForceTerm:
        forceTerminate_ = true;
        break;
    default:
	std::cout << "PROC " << globalRank_ 
		  << " : recved UNKNOWN message. tag = " 
		  << status.MPI_TAG <<  std::endl; 
	throw CoinError("Unknow message type", "workmain", 
			"AlpsKnowledgeBrokerMPI");
    }

    const int largeSize    = 
	model_->AlpsPar()->entry(AlpsParams::largeSize);
    MPI_Irecv(buf, largeSize, MPI_PACKED, MPI_ANY_SOURCE, 
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
    char* buf     = new char [smallSize];

    MPI_Pack(&receiverID, 1, MPI_INT, buf, smallSize, &pos, MPI_COMM_WORLD);
    MPI_Pack(&receiverWorkload, 1, MPI_DOUBLE, buf, smallSize, &pos,
             MPI_COMM_WORLD);

#ifdef NF_DEBUG_MORE
    std::cout << "masterAskHubDonate(): donor is " << donorID << std::endl;
#endif

    MPI_Send(buf,pos, MPI_PACKED, donorID, AlpsMsgAskHubShare, MPI_COMM_WORLD);
    incSendCount("masterAskHubDonate()");

    delete [] buf; 
    buf = NULL;
}

//#############################################################################

// Master ask the donor hub to send work to the receiver hub
void 
AlpsKnowledgeBrokerMPI::hubAskWorkerDonate(int donorID, 
					   int receiverID, 
					   double receiverWorkload)
{
    int smallSize = 
	model_->AlpsPar()->entry(AlpsParams::smallSize);
    int pos       = 0;
    char* buf     = new char [smallSize];
    
    MPI_Pack(&receiverID, 1, MPI_INT, buf, smallSize, &pos, MPI_COMM_WORLD);
    MPI_Pack(&receiverWorkload, 1, MPI_DOUBLE, buf, smallSize, &pos,
             MPI_COMM_WORLD);
    
#ifdef NF_DEBUG_MORE
    std::cout << "hubAskHubDonate() send to " << donorID << std::endl;
#endif

    MPI_Send(buf, pos, MPI_PACKED, donorID, AlpsMsgAskDonate, MPI_COMM_WORLD);
    incSendCount("hubAskWorkerDonate()");
    
    delete [] buf;
    buf = 0;
}

//#############################################################################

// Calculate the quality and quantity of workload on this process
void
AlpsKnowledgeBrokerMPI::updateWorkloadInfo()
{
    int count;
    
    const double rho = 
	model_->AlpsPar()->entry(AlpsParams::rho);
    workQuality_ = ALPS_OBJ_MAX;  // Worst ever possible
    workQuantity_ = 0.0;          // No work
    
    if (workingSubTree_ == NULL && subTreePool_->hasKnowledge() == false) {
	return;
    }
    
    std::vector<AlpsSubTree* > subTreeVec = 
	subTreePool_->getSubTreeList().getContainer();
    std::vector<AlpsSubTree* >::iterator pos1, pos2;
    pos1 = subTreeVec.begin();
    pos2 = subTreeVec.end();

    if (pos1 != pos2) {
        workQuality_ = (*pos1)->getQuality();//Best in pool
    }

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
	workingSubTree_->calculateQuality(incumbentValue_, rho);
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

// A worker donates a subtree to another worker(whose info is in buf)
void 
AlpsKnowledgeBrokerMPI::donateWork(char*& buf,
				   int tag,
				   MPI_Status* status,  
				   int recvID,
				   double recvWL)
{
    int size       = 
	model_->AlpsPar()->entry(AlpsParams::smallSize);
    int pos        = 0;
    int receiverID = 0;
    char* dummyBuf = 0;
    double receiverWorkload = 0.0;
    bool hasSentSubTree = false;
    AlpsSubTree* aSubTree = 0;    

#ifdef NF_DEBUG_MORE
    updateWorkloadInfo();
    messageHandler()->message(ALPS_DONATE_BEFORE, messages()) 
	<< globalRank_ << workQuantity_ << subTreePool_->getNumKnowledges() 
	<< CoinMessageEol;
#endif

    // Find out to which process I should donate
    if (recvID == -1) {
	MPI_Unpack(buf, size, &pos, &receiverID, 1, MPI_INT, MPI_COMM_WORLD);
	MPI_Unpack(buf, size, &pos, &receiverWorkload, 1, MPI_DOUBLE, 
                   MPI_COMM_WORLD);
    }
    else {
	receiverID = recvID;
	receiverWorkload = recvWL;
    }
    
    //------------------------------------------------------
    // Option 1: If subTreePool has subtrees, send the best one.
    // Option 2: If subTreePool has no subtrees AND workingSubTree_ does not 
    //           point to NULL, split it and send one part of it.
    // Option 3: Otherwise, sent a empty msg.
    //------------------------------------------------------

    if (subTreePool_->hasKnowledge()) {   // Option 1
	aSubTree = 
	    dynamic_cast<AlpsSubTree* >(subTreePool_->getKnowledge().first);
	subTreePool_->popKnowledge();
	sendSubTree(receiverID, aSubTree, tag);
	hasSentSubTree = true;

	// Since sent to other process, delete it.
	delete aSubTree; 
	
        if (msgLevel_ > 100) {
            messageHandler()->message(ALPS_DONATE_WHOLE, messages()) 
                << globalRank_ << receiverID << status->MPI_TAG << CoinMessageEol;
        }
    }
    else if (workingSubTree_ != 0) {     // Option 2
	int treeSize;
	aSubTree = workingSubTree_->splitSubTree(treeSize);
	if (treeSize > ALPS_GEN_TOL) {
	    sendSubTree(receiverID, aSubTree, tag);
	    hasSentSubTree = true;
	    
	    // Since sent to other process, delete it.
	    delete aSubTree;
	    
            if (msgLevel_ > 100) {
                messageHandler()->message(ALPS_DONATE_SPLIT, messages()) 
                    << globalRank_ << receiverID << status->MPI_TAG 
                    << CoinMessageEol;
            }
	}
    }   

    if (!hasSentSubTree) {               // Option 3
#ifdef NF_DEBUG
	std::cout << "donateWork(): " << globalRank_ << "has nothing send to " 
		  << receiverID << std::endl;
#endif
	MPI_Send(dummyBuf, 0, MPI_PACKED, receiverID, tag, MPI_COMM_WORLD);

        if (msgLevel_ > 100) {
            messageHandler()->message(ALPS_DONATE_FAIL, messages()) 
                << globalRank_ << receiverID << status->MPI_TAG << CoinMessageEol;
        }
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
AlpsKnowledgeBrokerMPI::hubAllocateDonation(char*& buf, MPI_Status* status)
{
    int i;
    int count = 0;

    int worstRank = -1;
    double worst = -ALPS_OBJ_MAX; // Minimization: best possible

    //------------------------------------------------------
    // Get the number of elements in buf.
    //------------------------------------------------------

    MPI_Get_count(status, MPI_PACKED, &count);

    //------------------------------------------------------
    // Find the worker who has worst work quality.
    //------------------------------------------------------

    const double zeroLoad = 
	model_->AlpsPar()->entry(AlpsParams::zeroLoad);
    
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
    // Forward the buf to this worst worker.
    //------------------------------------------------------

    if (worstRank != -1) {
	
#ifdef NF_DEBUG_MORE
	std::cout << "hubAllocateDonation() send to " 
		  << worstRank << std::endl;
#endif
	if (worstRank != globalRank_) {
	    MPI_Send(buf, count, MPI_PACKED, worstRank, 
		     AlpsMsgSubTreeByMaster, MPI_COMM_WORLD);
	    incSendCount("hubAllocateDonation");
	}
	else { // Hub self
	    std::cout << "Allocate to myself" << std::endl;
	    receiveSubTree(buf, status->MPI_SOURCE, status);
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
            messageHandler()->message(ALPS_LOADBAL_MASTER_NO, messages())
                << globalRank_ << systemWorkQuantity_ << CoinMessageEol;
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
	for (i = 0; i < clusterSize_; ++i) {
	    if ( i == clusterRank_ ) continue;

	    if (workerWorkQuantities_[i] > needWorkThreshold) {
		donors.insert(std::pair<double, int>(workerWorkQualities_[i], 
						     globalRank_-masterRank_ + i));
	    }
	}
    }
    else {   // Quality balance
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
		    workerWorkQualities_[i], globalRank_ - masterRank_ + i));
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
	    incSendCount("hubsShareWork");
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
AlpsKnowledgeBrokerMPI::hubSatisfyWorkerRequest(char*& buf, MPI_Status* status)
{
    int i;
    double bestQuality = ALPS_OBJ_MAX;  // Worst could be
    int size        =
	model_->AlpsPar()->entry(AlpsParams::smallSize);
    int pos         = 0;
    char* buffer    = new char [size];

    int donorRank      = -1;
    int donorGlobalRank = -1;      // Global rank
    int requestorRank;

    //------------------------------------------------------
    // Indentify the requestor's cluster rank.
    //------------------------------------------------------

    int requestor = status->MPI_SOURCE;
    requestorRank = requestor % cluSize_;

    //------------------------------------------------------
    // Find the worker having best quality in this cluster.
    //------------------------------------------------------

    for (i = 0; i < clusterSize_; ++i) {
	if (i == globalRank_ || i == requestorRank){
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
	MPI_Pack(&requestor, 1, MPI_INT, buffer, size, &pos, MPI_COMM_WORLD);
	MPI_Pack(&temp, 1, MPI_DOUBLE, buffer, size, &pos, MPI_COMM_WORLD);
	
	MPI_Send(buffer, pos, MPI_PACKED, 
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
	MPI_Send(buffer, 0, MPI_PACKED, 
		 requestor,
		 AlpsMsgSubTreeByWorker, 
		 MPI_COMM_WORLD);
        if (msgLevel_ > 100) {
            messageHandler()->message(ALPS_LOADBAL_HUB_FAIL, messages())
                << globalRank_ << requestor << CoinMessageEol;
        }
    }
    incSendCount("hubSatisfyWorkerRequest");

    delete [] buffer;
    buffer = 0;
} 

//#############################################################################

// Report cluster work quality, quantity, and msg counts.
void 
AlpsKnowledgeBrokerMPI::hubReportStatus(int tag, MPI_Comm comm)
{
    int size  = model_->AlpsPar()->entry(AlpsParams::smallSize);
    int pos   = 0;
    int receiver;
    char* buf = new char [size];

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

    MPI_Pack(&clusterNodeProcessed_, 1, MPI_INT, buf, size, &pos, comm);
    MPI_Pack(&clusterWorkQuality_, 1, MPI_DOUBLE, buf, size, &pos, comm);
    MPI_Pack(&clusterWorkQuantity_, 1, MPI_DOUBLE, buf, size, &pos, comm);
    MPI_Pack(&clusterSendCount_, 1, MPI_INT, buf, size, &pos, comm);
    MPI_Pack(&clusterRecvCount_, 1, MPI_INT, buf, size, &pos, comm);
    MPI_Send(buf, pos, MPI_PACKED, receiver, tag, comm);

    clusterSendCount_ = clusterRecvCount_ = 0;  // Only count new msg
  
    delete [] buf; 
    buf = 0;
}

//#############################################################################

// After receiving status (in buf) from a worker, a hub updates its 
// cluster's status
void 
AlpsKnowledgeBrokerMPI::hubUpdateCluStatus(char*& buf, 
					   MPI_Status* status, 
					   MPI_Comm comm)
{
    int msgSendNum, msgRecvNum;
    int position = 0;
    int sender;
    int size = model_->AlpsPar()->entry(AlpsParams::smallSize);

    if (comm == MPI_COMM_WORLD)
	sender = (status->MPI_SOURCE) % cluSize_;
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
    double preQuality = workerWorkQualities_[sender];
    double curQuality;
    double preQuantity = workerWorkQuantities_[sender];
    double curQuantity;

    MPI_Unpack(buf, size, &position, &curNodeProcessed, 1, MPI_INT, comm);
    MPI_Unpack(buf, size, &position, &curQuality, 1, MPI_DOUBLE, comm);
    MPI_Unpack(buf, size, &position, &curQuantity, 1, MPI_DOUBLE, comm);
    MPI_Unpack(buf, size, &position, &msgSendNum, 1, MPI_INT, comm);
    MPI_Unpack(buf, size, &position, &msgRecvNum, 1, MPI_INT, comm);
  
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
	    clusterWorkQuality_ *= cluSize_;
	}
	else {
	    clusterWorkQuality_ *= (cluSize_ - 1);
	}
	    
	clusterWorkQuality_ -= preQuality;
	clusterWorkQuality_ += curQuality;
	if (hubWork_) {
	    clusterWorkQuality_ /= cluSize_;
	}
	else {
	    clusterWorkQuality_ /= (cluSize_ - 1);
	}
#endif
	clusterWorkQuality_ = std::min(clusterWorkQuality_, curQuality);
	
    }
    
    clusterSendCount_ += msgSendNum;
    clusterRecvCount_ += msgRecvNum;

#ifdef NF_DEBUG
    std::cout << "HUB["<< globalRank_ <<"]: after hubUpdateCluStatus(): " 
	      << "curQuality = " << curQuality 
	      << ", preQuality = " << preQuality << ", sender = " << sender
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
// information (id and load) is in buf.
void 
AlpsKnowledgeBrokerMPI::hubsShareWork(char*& buf, 
				      MPI_Status* status)
{
    int i;
    int pos         = 0;
    int receiverID  = 0;
    int maxLoadRank = -1;            // Global rank
    double maxLoad  = ALPS_DBL_MAX;
    int size = model_->AlpsPar()->entry(AlpsParams::smallSize);

    int skipRank;
    
    double receiverWorkload = 0.0;
    char* buffer = new char [size];

    //------------------------------------------------------
    // Indentify the receiver and its current workload.
    //------------------------------------------------------

    MPI_Unpack(buf, size, &pos, &receiverID, 1, MPI_INT, MPI_COMM_WORLD);
    MPI_Unpack(buf, size, &pos, &receiverWorkload, 1, MPI_DOUBLE, MPI_COMM_WORLD);

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
	
#ifdef NF_DEBUG
	std::cout << "HUB[" << globalRank_ << "] : found process " << maxLoadRank
		  << " to share with " << receiverID << std::endl;
#endif
	
	if(maxLoadRank != globalRank_) { // Not hub 
	    // max loaded is NOT this hub.

	    pos = 0;                   // Reset position to pack
	    MPI_Pack(&receiverID, 1, MPI_INT, buffer, size, &pos, MPI_COMM_WORLD);
	    MPI_Pack(&receiverWorkload, 1, MPI_DOUBLE, buffer, size, &pos, 
		     MPI_COMM_WORLD);
	    MPI_Send(buffer, pos, MPI_PACKED, maxLoadRank, 
		     AlpsMsgAskDonateToHub, MPI_COMM_WORLD);
	    incSendCount("hubsShareWork");
	}
	else {
	    // Max loaded is this hub. 
	    // NOTE: buffer is NOT useful in this case.
	    donateWork(buffer, 
		       AlpsMsgSubTreeByMaster,
		       status, 
		       receiverID,
		       receiverWorkload);
	    incSendCount("hubsShareWork");
	}
    }
    else {
        if (msgLevel_ > 100) {
            messageHandler()->message(ALPS_LOADBAL_HUB_FAIL, messages())
                << globalRank_ << receiverID << CoinMessageEol;
        }
    }

    if (buffer != 0) {
	delete [] buffer;
	buffer = 0;
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
    char* buffer = new char [size];
    
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

    bool quantityBalance = false;
    for (i = 0; i < hubNum_; ++i) {
	if (hubWorkQuantities_[i] < ALPS_QUALITY_TOL) {    // Have not work
	    receivers.insert(std::pair<double, int>(hubWorkQualities_[i], 
						    hubRanks_[i]));
	    quantityBalance = true;
	}
    }
    
    if (quantityBalance) {  // Quantity balance
	for (i = 0; i < hubNum_; ++i) {
	    if (hubWorkQuantities_[i] > ALPS_QUALITY_TOL) {
		donors.insert(std::pair<double, int>(hubWorkQualities_[i], 
						     hubRanks_[i]));
	    }
	}
    }
    else {   // Quality balance
	double averQuality  = 0.0;
	for (i = 0; i < hubNum_; ++i) {
	    averQuality += hubWorkQualities_[i];
	}
	averQuality /= hubNum_;
	
	for (i = 0; i < hubNum_; ++i) {
	    double diff = hubWorkQualities_[i] - averQuality;
	    double diffRatio = fabs(diff / (averQuality + 1.0));
	    if (diff < 0 && diffRatio > donorSh) {  // Donor candidates
		donors.insert(std::pair<double, int>(hubWorkQualities_[i], 
						     hubRanks_[i]));
	    }
	    else if (diff > 0 && diffRatio > receiverSh){// Receiver candidates
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

#ifdef NF_DEBUG_MORE
    std::cout << "Master: donors size = " << numDonor << ", receiver size = "
	      << numReceiver << std::endl;
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
	    for (i = 0; i < clusterSize_; ++i) {
		if (i != clusterRank_) {
		    if (workerWorkQualities_[i] < maxLoad) {
			maxLoad = workerWorkQualities_[i];
			maxLoadRank = globalRank_ - masterRank_ + i;
		    }
		}
	    }
	    
	    // FIXME: how many workload to share? Righ now just 1 subtree. 
	    // Ask the donor worker to send a subtree to receiving hub)
	    if (maxLoadRank != -1) {
		if (buffer != 0) {
		    delete [] buffer; 
		    buffer = 0;
		}
		buffer = new char [size]; 

		MPI_Pack(&receiverID, 1, MPI_INT, buffer, size, &pos,
			 MPI_COMM_WORLD);
		MPI_Pack(&recvLoad, 1, MPI_DOUBLE, buffer, size, &pos, 
			 MPI_COMM_WORLD);

		incSendCount("masterBalanceHubs()");
		MPI_Send(buffer, pos, MPI_PACKED, maxLoadRank,
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

    if (buffer != 0) {
	delete [] buffer; 
	buffer = 0;
    }
}

//#############################################################################

// After recieve status (in buf) of a hub, update system status
void 
AlpsKnowledgeBrokerMPI::masterUpdateSysStatus(char*& buf, 
					      MPI_Status* status, 
					      MPI_Comm comm)
{
    int position = 0;
    int msgSendNum, msgRecvNum;
    int size = model_->AlpsPar()->entry(AlpsParams::smallSize);
    int sender;

    if (comm == MPI_COMM_WORLD) {
	sender = (int)(status->MPI_SOURCE) / cluSize_;//Be careful with / or %
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
    double preQuality = hubWorkQualities_[sender];
    double curQuality;
    double preQuantity = hubWorkQuantities_[sender];
    double curQuantity;

    MPI_Unpack(buf, size, &position, &curNodeProcessed, 1, MPI_INT, comm);
    MPI_Unpack(buf, size, &position, &curQuality, 1, MPI_DOUBLE, comm);
    MPI_Unpack(buf, size, &position, &curQuantity, 1, MPI_DOUBLE, comm);
    MPI_Unpack(buf, size, &position, &msgSendNum, 1, MPI_INT, comm);
    MPI_Unpack(buf, size, &position, &msgRecvNum, 1, MPI_INT, comm);

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

#ifdef NF_DEBUG
    std::cout << "MASTER[" << globalRank_ << "]: After updateSystem() : curQuality " 
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
   
#ifdef NF_DEBUG_MORE
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
    clusterWorkQuantity_ += workerWorkQuantities_[masterRank_];

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

// Unpack received incumbent value and process ID from buf. Update incumbent
// value and ID stored on this process.
bool
AlpsKnowledgeBrokerMPI::unpackSetIncumbent(char*& buf, MPI_Status* status)
{
    bool accept = false;
    int size     = 
	model_->AlpsPar()->entry(AlpsParams::smallSize);
    int position = 0;
    double incVal= 0.0;
    int incID    = 0;
 
    // Unpack incumbent value from buf
    MPI_Unpack(buf, size, &position, &incVal, 1, MPI_DOUBLE, MPI_COMM_WORLD);
    MPI_Unpack(buf, size, &position, &incID, 1, MPI_INT, MPI_COMM_WORLD);

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
	incumbentValue_ = incVal;
	incumbentID_ = incID;
	accept = true;
	
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
    int size  = model_->AlpsPar()->entry(AlpsParams::smallSize);
    int pos   = 0;
    char* buf = new char [size];
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

    MPI_Pack(&nodeProcessedNum_, 1, MPI_INT, buf, size, &pos, comm);
    MPI_Pack(&workQuality_, 1, MPI_DOUBLE, buf, size, &pos, comm);
    MPI_Pack(&workQuantity_, 1, MPI_DOUBLE, buf, size, &pos, comm);
    MPI_Pack(&sendCount_, 1, MPI_INT, buf, size, &pos, comm);
    MPI_Pack(&recvCount_, 1, MPI_INT, buf, size, &pos, comm);
    MPI_Send(buf, pos, MPI_PACKED, receiver, tag, comm);
   
#ifdef NF_DEBUG_MORE
    std::cout << "WORKER " << globalRank_ 
	      << " : report quality = " << workQuality_ 
	      << " : report quantity = " << workQuantity_ 
	      << " to hub " 
	      << myHubRank_ << "; Tag = " << tag << "; sendCount_ = "
	      << sendCount_ << "; recvCount_ = " << recvCount_ << std::endl;
#endif

    sendCount_ = recvCount_ = 0;      // Only count new message 
    delete [] buf; 
    buf = 0;
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::hubAskRecvIndices()
{
    int nextIndex;
    int maxIndex;
    
    char* dummyBuf = 0;
    MPI_Status status;

    MPI_Send(dummyBuf, 0, MPI_PACKED, masterRank_, AlpsMsgHubAskIndices, 
	     MPI_COMM_WORLD);
    MPI_Recv(&nextIndex, 1, MPI_INT, masterRank_, AlpsMsgIndicesFromMaster, 
	     MPI_COMM_WORLD, &status);
    MPI_Recv(&maxIndex, 1, MPI_INT, masterRank_, AlpsMsgIndicesFromMaster, 
	     MPI_COMM_WORLD, &status);
    incSendCount("hubAskRecvIndices()");
    incRecvCount("hubAskRecvIndices()", 2);

    if (nextIndex < 0 || maxIndex < 0 || nextIndex > maxIndex) {
	throw CoinError("value < 0 || maxIndex < 0 || nextIndex < maxIndex", 
			"hubAskRecvIndices", 
			"AlpsKnowledgeBrokerMPI");
    }
    else {
	setNextNodeIndex(nextIndex);
	setMaxNodeIndex(maxIndex);
    }
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::workerAskRecvIndices()
{
    int nextIndex;
    int maxIndex;
    
    char* dummyBuf = 0;
    MPI_Status status;

    MPI_Send(dummyBuf, 0, MPI_PACKED, myHubRank_, AlpsMsgWorkerAskIndices, 
	     MPI_COMM_WORLD);
    MPI_Recv(&nextIndex, 1, MPI_INT, myHubRank_, AlpsMsgIndicesFromHub, 
	     MPI_COMM_WORLD, &status);
    MPI_Recv(&maxIndex, 1, MPI_INT, myHubRank_, AlpsMsgIndicesFromHub, 
	     MPI_COMM_WORLD, &status);
    incSendCount("workerAskRecvIndices()");
    incRecvCount("workerAskRecvIndices()", 2);

    if (nextIndex < 0 || maxIndex < 0 || nextIndex > maxIndex) {
	throw CoinError("value < 0 || maxIndex < 0 || nextIndex < maxIndex", 
			"workerAskRecvIndices", 
			"AlpsKnowledgeBrokerMPI");
    }
    else {
	setNextNodeIndex(nextIndex);
	setMaxNodeIndex(maxIndex);
    }   
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::masterSendIndices(int recvHub)
{
    int nextIndex = getNextNodeIndex();
    int maxIndex = -1; 
    const int unitNodes = 
	model_->AlpsPar()->entry(AlpsParams::unitWorkNodes) + 10;

    if (ALPS_INT_MAX - nextIndex > unitNodes) {
	if ((ALPS_INT_MAX - nextIndex) <= masterIndexBatch_) {
	    maxIndex = (ALPS_INT_MAX - 1) ;
	    setNextNodeIndex(maxIndex);
	}
	else {
	    maxIndex = nextIndex + masterIndexBatch_;
	    setNextNodeIndex(maxIndex + 1);
	}
    }
    
    MPI_Send(&nextIndex, 1, MPI_INT, recvHub, AlpsMsgIndicesFromMaster,
	     MPI_COMM_WORLD);
    MPI_Send(&maxIndex, 1, MPI_INT, recvHub, AlpsMsgIndicesFromMaster,
	     MPI_COMM_WORLD);

    incSendCount("masterSendIndices", 2);
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::hubSendIndices(int recvWorker)
{
    int nextIndex = getNextNodeIndex();
    int maxIndex = -1; 
    const int unitNodes = 
	model_->AlpsPar()->entry(AlpsParams::unitWorkNodes) + 10;

    if (getMaxNodeIndex() - nextIndex > unitNodes) {
	if ((getMaxNodeIndex() - nextIndex) <= hubIndexBatch_) {
	    maxIndex = (getMaxNodeIndex() - 1) ;
	    setNextNodeIndex(maxIndex);
	}
	else {
	    maxIndex = nextIndex + hubIndexBatch_;
	    setNextNodeIndex(maxIndex + 1);
	}
    } 
    else {  // Assume masterIndexBatch_ > unitNodes, maybe reasonable.
	if (globalRank_ != masterRank_) {
	    hubAskRecvIndices();
	}
	else {
	    throw CoinError("MASTER does not have enough node indices", 
			    "HubSendIndices", 
			    "AlpsKnowledgeBrokerMPI");
	}
	
	if ((getMaxNodeIndex() - nextIndex) <= hubIndexBatch_) {
	    maxIndex = (getMaxNodeIndex() - 1) ;
	    setNextNodeIndex(maxIndex);
	}
	else {
	    maxIndex = nextIndex + hubIndexBatch_;
	    setNextNodeIndex(maxIndex + 1);
	}
    }

    MPI_Send(&nextIndex, 1, MPI_INT, recvWorker, AlpsMsgIndicesFromHub, 
	     MPI_COMM_WORLD);
    MPI_Send(&maxIndex, 1, MPI_INT, recvWorker, AlpsMsgIndicesFromHub,
	     MPI_COMM_WORLD);  
    incSendCount("hubSendIndices", 2);
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::broadcastModel(const int id, const int source)
{
    char* buf = 0;
    int size = -1;
    int position = -1;

    //------------------------------------------------------
    // Encode matrix and pack into buf.
    //------------------------------------------------------

    if (id == source) {
        // Pack model 
	packEncoded(model_->encode(), buf, size, position);

#ifdef NF_DEBUG
	std::cout << "MASTER: packed model. size = " 
		  << size << std::endl; 
#endif
    }

    //------------------------------------------------------
    // Broadcost the size of matrix.
    //------------------------------------------------------

    // Broadcast buf size first
    MPI_Bcast(&size, 1, MPI_INT, source, MPI_COMM_WORLD);  

    if (id != source) {
	// Process except master receive the size of model.
	if (size <= 0) {
	    throw CoinError("Msg size <= 0", 
			    "broadcastModel", 
			    "AlpsKnowledgeBrokerMPI");
	}
	if( buf != NULL ) {
	    delete [] buf;  
	    buf = NULL;
	}
	buf = new char[size + 100];
    }

    //------------------------------------------------------
    // Broadcost matrix.
    //------------------------------------------------------

    MPI_Bcast(buf, size, MPI_CHAR, source, MPI_COMM_WORLD); // Broadcast buf_
    
    if (id != source) {
	
	//--------------------------------------------------
	// Unpack to encoded model.
	//--------------------------------------------------

	AlpsEncoded* encodedModel = unpackEncoded(buf, size+100);
	
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
	
	if (encodedModel != NULL) {
	    delete encodedModel;
	    encodedModel = 0;
	}

	delete [] buf;
    }
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::sendIncumbent()
{
    int i         = -1;
    int position  = 0;
    int size      = 
	model_->AlpsPar()->entry(AlpsParams::smallSize);
    
    char* buf     = new char [size];

    int mySeq = rankToSequence(incumbentID_, globalRank_);
    int leftSeq = leftSequence(mySeq, processNum_);
    int rightSeq = rightSequence(mySeq, processNum_);

    if (mySeq >= processNum_ || leftSeq >= processNum_ 
	|| rightSeq >= processNum_) {
	std::cout << "sequence is Wrong !!!" << std::endl;
	abort();
    }

    MPI_Pack(&incumbentValue_, 1, MPI_DOUBLE, buf, size, &position, MPI_COMM_WORLD);
    MPI_Pack(&incumbentID_, 1, MPI_INT, buf, size, &position, MPI_COMM_WORLD);

    if (leftSeq != -1) {
	int leftRank = sequenceToRank(incumbentID_, leftSeq);
#if defined(NF_DEBUG_MORE)  
	std::cout << "PROC " <<  globalRank_
		  <<" : init send a solution - L,  value = " 
		  << incumbentValue_ << " to "<< leftRank << std::endl; 
#endif
	MPI_Send(buf, position, MPI_PACKED, leftRank, AlpsMsgIncumbentTwo, 
		 MPI_COMM_WORLD);
	incSendCount("sendIncumbent()");
    }
    
    if (rightSeq != -1) {
	int rightRank = sequenceToRank(incumbentID_, rightSeq);
#if defined(NF_DEBUG_MORE)  
	std::cout << "PROC " <<  globalRank_
		  <<" : init send a solution - R,  value = " 
		  << incumbentValue_ << " to "<< rightRank << std::endl; 
#endif
	MPI_Send(buf, position, MPI_PACKED, rightRank, AlpsMsgIncumbentTwo, 
		 MPI_COMM_WORLD);
	incSendCount("sendIncumbent()");
    }
    
    if(buf != 0) {
	delete [] buf;     buf = 0;
    }
}

//#############################################################################

void
AlpsKnowledgeBrokerMPI::collectBestSolution(int destination)
{
    MPI_Status status;
    int sender = incumbentID_;

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
	    int position = -1;

	    const AlpsSolution* solu = static_cast<const AlpsSolution* >
		(getBestKnowledge(ALPS_SOLUTION).first);
  
	    double value = getBestKnowledge(ALPS_SOLUTION).second;

	    AlpsEncoded* enc = solu->encode();
	    packEncoded(enc, senderBuf, size, position);
         
	    sendSizeBuf(senderBuf, size, position, destination, 
			AlpsMsgIncumbent, MPI_COMM_WORLD);
	    MPI_Send(&value, 1, MPI_DOUBLE, destination, AlpsMsgIncumbent, 
		     MPI_COMM_WORLD);      
	    if(senderBuf != NULL) {
		delete [] senderBuf; 
		senderBuf = 0;
	    }
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

	    AlpsEncoded* encodedSolu = unpackEncoded(destBuf);
	    AlpsSolution* bestSolu = static_cast<AlpsSolution* >
		( decoderObject(encodedSolu->type())->decode(*encodedSolu) );
	    
	    addKnowledge(ALPS_SOLUTION, bestSolu, value);
	    
	    if(destBuf != NULL) {
		delete [] destBuf;     
		destBuf = NULL;
	    }
	    if (encodedSolu != NULL) {
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
				    char*& buf,
				    int& size, 
				    int& position) 
{
    const int bufSpare = 
	model_->AlpsPar()->entry(AlpsParams::bufSpare);
    
    int typeSize       = strlen(enc->type());
    int repSize        = enc->size();

    size          = typeSize + repSize + 2*sizeof(int) + bufSpare;
    position      = 0;       // To record current position in buf.

    if(buf != 0) {
	delete [] buf;     buf = 0;
    }
    buf  = new char[size];

    // Pack typeSize, repSize, type_, representation_ of enc
    MPI_Pack(&typeSize, 1, MPI_INT, buf,  size, &position, MPI_COMM_WORLD);
    MPI_Pack(&repSize, 1, MPI_INT, buf,  size, &position, MPI_COMM_WORLD);
    MPI_Pack(const_cast<char*>(enc->type()), typeSize, MPI_CHAR, 
	     buf, size, &position, MPI_COMM_WORLD); 
    MPI_Pack(const_cast<char*>(enc->representation()), repSize, MPI_CHAR, 
	     buf, size, &position, MPI_COMM_WORLD); 

}

//#############################################################################

AlpsEncoded* 
AlpsKnowledgeBrokerMPI::unpackEncoded(char*& buf, int size) 
{
    int position = 0;
    int typeSize;
    int repSize;
    if (size <= 0) {
	size = model_->AlpsPar()->entry(AlpsParams::largeSize);
    }
    
    // Unpack typeSize, repSize, type and rep from buf_
    MPI_Unpack(buf, size, &position, &typeSize, 1, MPI_INT, MPI_COMM_WORLD);

#if defined(NF_DEBUG_MORE)
    std::cout << "PROC "<< globalRank_ <<" : typeSize is " 
	      << typeSize << ";\tsize = "<< size <<  std::endl;
#endif

    MPI_Unpack(buf, size, &position, &repSize, 1, MPI_INT, MPI_COMM_WORLD);
  
    char* type = new char [typeSize+1]; // At least larger than one
    char* rep = new char[repSize+1];

    MPI_Unpack(buf, size, &position, type, typeSize, MPI_CHAR, 
	       MPI_COMM_WORLD);

    *(type+typeSize) = '\0'; //MUST terminate cstring so that don't read trash 
  
    MPI_Unpack(buf, size, &position, rep, repSize, MPI_CHAR, MPI_COMM_WORLD);
    rep[repSize] = '\0';

#if defined(NF_DEBUG_MORE)
    std::cout << "PROC "<< globalRank_ << ": type = " << type 
	      << "; repSize = " << repSize << std::endl;
#endif

    // Return a enc of required knowledge type
    return new AlpsEncoded(type, repSize, rep );
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

void 
AlpsKnowledgeBrokerMPI::receiveSizeNode(int sender,
					AlpsSubTree*& st,
					MPI_Comm comm, 
					MPI_Status* status) 
{
    int i = 0;
    char* buf = 0;

    receiveSizeBuf(buf, sender, AlpsMsgNode, comm, status);
 
    if (status->MPI_TAG == AlpsMsgFinishInit ) {
	//std::cout << "PROC: " << globalRank_ 
	//	  <<" : rec AlpsMsgFinishInit... STOP INIT." << std::endl;
    } 
    else if (status->MPI_TAG == AlpsMsgNode) {
	AlpsEncoded* encodedNode = unpackEncoded(buf);
 
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

	st->nodePool()->addKnowledge(node, node->getQuality());
	assert(st->getNumNodes() > 0);
	if ( (st->nodePool()->getNumKnowledges() ) == 1) {
	    // Make the first node as root.
	    st->setRoot(node);
        }

	if (encodedNode != 0) {
	    delete encodedNode;
	    encodedNode = 0;
	}
    } 
    else {
	std::cout << "PROC " << globalRank_ 
		  << " : recved UNKNOWN message: tag = " << status->MPI_TAG 
		  << "; source = " << status->MPI_SOURCE <<std::endl;
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
AlpsKnowledgeBrokerMPI::receiveSubTree(char*& buf,
				       int sender,
				       MPI_Status* status)
{
#ifdef NF_DEBUG_MORE
    std::cout << "WORKER " << globalRank_ 
	      << " : start to receive a subtree from " << sender 
	      << "; num of subtrees = " 
	      << getNumKnowledges(ALPS_SUBTREE) << "; hasKnowlege() = "
	      << subTreePool_->hasKnowledge() <<std::endl;
#endif

    int count;
    int i = 0;

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
	
	AlpsEncoded* encodedST = unpackEncoded(buf);
	AlpsSubTree* tempST = dynamic_cast<AlpsSubTree*>
	    (const_cast<AlpsKnowledge *>(decoderObject("ALPS_SUBTREE")))->
	    newSubTree();
	tempST->setKnowledgeBroker(this);
	tempST->setNodeSelection(nodeSelection_);	
        
	AlpsSubTree* subTree = 
	    dynamic_cast<AlpsSubTree* >(tempST->decode(*encodedST) );
	const double rho = 
	    model_->AlpsPar()->entry(AlpsParams::rho);
	//subTree->approximateQuality(incumbentValue_, rho);
	assert(subTree->getNumNodes() > 0);
	subTree->calculateQuality(incumbentValue_, rho);
	
	addKnowledge(ALPS_SUBTREE, subTree, subTree->getQuality());


#ifdef NF_DEBUG_MORE
	std::cout << "WORKER " << globalRank_ << " : received a subtree from " 
		  << sender << "; num of subtrees = " 
		  << getNumKnowledges(ALPS_SUBTREE) <<std::endl; 
#endif

	if (encodedST != 0) {
	    delete encodedST;
	    encodedST = 0;
	}
	if (tempST != 0) {
	    delete tempST;
	    tempST = 0;
	}
	subTree = 0;
	
    } 
    else {
	std::cout << "PROC " << globalRank_ 
		  <<" : recved UNKNOWN message, tag = " << status->MPI_TAG 
		  << ", source = " << status->MPI_SOURCE <<std::endl; 
	throw CoinError("Unknow message type", 
			"receiveSubTree()", "AlpsKnowledgeBrokerMPI");
    }

}

//#############################################################################

// Only used in sendSubTree()

void 
AlpsKnowledgeBrokerMPI::sendBuf(char*& buf,
				int size,
				int position,
				const int target, 
				const int tag, 
				const MPI_Comm comm) 
{
    if (size > 0) {
	MPI_Send(buf, position, MPI_PACKED, target, tag, comm); 
    }
    else {
	throw CoinError("Msg size is <= 0", "send", "AlpsKnowledgeBrokerMPI");
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

// NOTE: comm can be hubComm_ or clusterComm_.
void 
AlpsKnowledgeBrokerMPI::sendSizeNode(const int target, 
				     AlpsSubTree*& st,
				     MPI_Comm comm) 
{
    char* buf = 0;
    int size = -1;
    int position = -1;

    AlpsTreeNode* node = dynamic_cast<AlpsTreeNode* >
	(st->nodePool()->getKnowledge().first);

    AlpsEncoded* enc = node->encode();
    
    st->nodePool()->popKnowledge();

    delete node;   // Since sending to other process
    
    packEncoded(enc, buf, size, position);
    sendSizeBuf(buf, size, position, target, AlpsMsgNode, comm);

    if (buf != 0) {
	delete [] buf; 
	buf = 0;
    }

    if (enc != 0) {
	delete enc;  
	enc = 0;                 // Allocated in encode()
    }
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::sendSubTree(const int target,
				    AlpsSubTree*& st,
				    int tag)
{
#ifdef NF_DEBUG
    std::cout << "WORKER["<< globalRank_ 
	      << "]: start to donor a subtree to PROC " << target << std::endl;
#endif

    char* buf = 0;
    int size = -1;
    int position = -1;

    AlpsEncoded* enc = st->encode();
    packEncoded(enc, buf, size, position);
    sendBuf(buf, size, position, target, tag, MPI_COMM_WORLD);

    if (buf != 0) {
	delete [] buf; 
	buf = 0;
    }
    if (enc != 0) {
	delete enc;  
	enc = 0;                 // Allocated in encode()
    }

#ifdef NF_DEBUG
    std::cout << "WORKER["<< globalRank_ 
	      << "]: donor a subtree to PROC " << target
	      << "; buf pos = " << position 
	      << "; buf size = " << size <<  std::endl; 
#endif
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::sendFinishInit(const int target, 
				       MPI_Comm comm) 
{
    char* dummyBuf = 0;
    MPI_Send(dummyBuf, 0, MPI_PACKED, target, AlpsMsgFinishInit, comm);
}

//#############################################################################

void AlpsKnowledgeBrokerMPI::deleteSubTrees()
{
    if (workingSubTree_) {
        delete workingSubTree_;
        workingSubTree_ = NULL;
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
    int count = 0;

    //------------------------------------------------------
    // Assign a pointer of data pool to model.
    //------------------------------------------------------
    
    //AlpsDataPool *tempDataPool = getDataPool();
    //model.setDataPool(tempDataPool);

    //------------------------------------------------------
    // Master read in parameters and model data.
    //------------------------------------------------------

    if (globalRank_ == masterRank_) {

	//--------------------------------------------------
	// Read in params.
	//--------------------------------------------------
	
	model.readParameters(argc, argv);
        
        msgLevel_ = model_->AlpsPar()->entry(AlpsParams::msgLevel);

        if (msgLevel_ > 0) {
            messageHandler()->message(ALPS_P_VERSION, messages())
                << CoinMessageEol;
        }

	//std::cout << "argc=" << argc << std::endl;
	
        if (msgLevel_ > 0) {
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

    while(true) {
	if (hubNum_ > 0) {
	    cluSize_ = 1;
	    while (cluSize_ * hubNum_ < processNum_) {
		++cluSize_;
	    }
	    color = globalRank_ / cluSize_; // [0,...,cluSize-1] in group 1
	} 
	else {
	    std::cout << "hubNum_ <= 0" << std::endl;
	    throw CoinError("hubNum_ <= 0", 
			    "initSearch",
			    "AlpsKnowledgeBrokerMPI");
	}

        // more than 1 proc at last P
	if (processNum_- cluSize_ * (hubNum_ - 1) > 1) {
	    break;
	}
	else {
	    --hubNum_;
	}
    }

    //------------------------------------------------------
    // Create clusterComm_.
    //------------------------------------------------------

    MPI_Comm_split(MPI_COMM_WORLD, color, key, &clusterComm_);
    MPI_Comm_rank(clusterComm_, &clusterRank_);
    MPI_Comm_size(clusterComm_, &clusterSize_);

    //------------------------------------------------------
    // Create hubGroup_ and hubComm_.
    //------------------------------------------------------

    hubRanks_ = new int [hubNum_];
    int k = 0;
    for (i = 0; i < processNum_; i += cluSize_) {
	hubRanks_[k++] = i + masterRank_;
    }

    MPI_Group worldGroup;
    MPI_Comm_group(MPI_COMM_WORLD, &worldGroup);
    MPI_Group_incl(worldGroup, hubNum_, hubRanks_, &hubGroup_);
    MPI_Comm_create(MPI_COMM_WORLD, hubGroup_, &hubComm_);
    
    //------------------------------------------------------
    // Allcoate index, classify process types, set up knowledge pools
    //   Master  :  [0, INT_MAX/4)]
    //   HUBs    :  [INT_MAX/4 + 1, INT_MAX]
    //------------------------------------------------------

    int masterIndexR = static_cast<int>(ALPS_INT_MAX / 4.0);
    int hubIndexR = static_cast<int>(ALPS_INT_MAX / (4.0 * hubNum_) * 3);
    int workerIndexR = static_cast<int>(hubIndexR / (2.0 * clusterSize_));
    int masterLow = 0;
    int masterUp = masterIndexR + hubIndexR / 2;
    int hubLow = masterIndexR + (globalRank_ / cluSize_) * hubIndexR + 1 ;
    int hubUp = hubLow + hubIndexR / 2;
    int workerLow = hubUp + workerIndexR * clusterRank_ + 1;
    int workerUp = workerLow + workerIndexR;
    masterIndexBatch_ = masterUp / (hubNum_ * 3);
    hubIndexBatch_ = hubIndexR / (2 * clusterSize_ * 3);

#ifdef NF_DEBUG_MORE
    std::cout << "masterIndexR = " << masterIndexR << "; hubIndexR = "
	      << hubIndexR << "; workerIndexR = " << workerIndexR << std::endl;
    std::cout << "masterUp = " << masterUp << "; hubLow = " << hubLow 
	      << ";  workerLow = " << workerLow << "workerUp = " 
	      << workerUp << std::endl;
    std::cout << "masterIndexBatch_ = " << masterIndexBatch_ 
	      <<  "hubIndexBatch_ = " <<  hubIndexBatch_ << std::endl;
#endif

    //------------------------------------------------------
    // Decide process type.
    //------------------------------------------------------

    if (globalRank_ == masterRank_) {
	processType_ = AlpsProcessTypeMaster;
	setNextNodeIndex(masterLow);
	setMaxNodeIndex(masterUp);
    } 
    else if ( (globalRank_ % cluSize_) == masterRank_ ) {
	processType_ = AlpsProcessTypeHub;
	setNextNodeIndex(hubLow);
	setMaxNodeIndex(hubUp);
	int maxHubWorkSize=model_->AlpsPar()->entry(AlpsParams::maxHubWorkSize);
	if (cluSize_ < maxHubWorkSize) { // If less, need work
	    hubWork_ = true;
	}
	else {
	    hubWork_ = false;
	}   
    }
    else {
	processType_ = AlpsProcessTypeWorker;
	setNextNodeIndex(workerLow);
	setMaxNodeIndex(workerUp);
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
	else if ( (i % cluSize_) == masterRank_) {
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
	
	myHubRank_ = (globalRank_ / cluSize_) * cluSize_ + masterRank_;
	
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
    // Register knowledge.
    //------------------------------------------------------

    model.registerKnowledge();

    //------------------------------------------------------
    // Set max number of solutions.
    //------------------------------------------------------

    const int mns = model_->AlpsPar()->entry(AlpsParams::solLimit);
    setMaxNumKnowledges(ALPS_SOLUTION, mns);
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
    double* rampUps = 0;
    double* rampDowns = 0;
    double *cpuTimes =  NULL;
    double *wallClocks = NULL;
    
    if (globalRank_ == masterRank_) {
	tSizes = new int [processNum_];
	tLefts = new int [processNum_];
	tDepths = new int [processNum_];
	idles = new double [processNum_];
	rampUps = new double [processNum_];
	rampDowns = new double [processNum_];
	cpuTimes = new double [processNum_];
	wallClocks = new double [processNum_];
    }  

    MPI_Gather(&nodeProcessedNum_, 1, MPI_INT, tSizes, 1, MPI_INT, 
	       masterRank_, MPI_COMM_WORLD);
    MPI_Gather(&nodeLeftNum_, 1, MPI_INT, tLefts, 1, MPI_INT, 
	       masterRank_, MPI_COMM_WORLD);
    MPI_Gather(&treeDepth_, 1, MPI_INT, tDepths, 1, MPI_INT, masterRank_, 
	       MPI_COMM_WORLD);
    MPI_Gather(&idleTime_, 1, MPI_DOUBLE, idles, 1, MPI_DOUBLE, masterRank_,
	       MPI_COMM_WORLD);
    MPI_Gather(&rampUpTime_, 1, MPI_DOUBLE, rampUps, 1, MPI_DOUBLE, 
	       masterRank_, MPI_COMM_WORLD);
    MPI_Gather(&rampDownTime_, 1, MPI_DOUBLE, rampDowns, 1, MPI_DOUBLE, 
	       masterRank_, MPI_COMM_WORLD);
    MPI_Gather(&(timer_.cpu_), 1, MPI_DOUBLE, cpuTimes, 1, MPI_DOUBLE, 
	       masterRank_, MPI_COMM_WORLD);
    MPI_Gather(&(timer_.wall_), 1, MPI_DOUBLE, wallClocks, 1, MPI_DOUBLE, 
	       masterRank_, MPI_COMM_WORLD);

    if (processType_ == AlpsProcessTypeMaster) {


	int numWorkers = 0;   // Number of process are processing nodes.
	
	int sumSize = 0, aveSize, maxSize = 0, minSize = ALPS_INT_MAX;
	int sumDep = 0, aveDep, maxDep = 0, minDep = ALPS_INT_MAX; 
	
	double sumIdle = 0.0, aveIdle = 0.0;
	double maxIdle = 0.0, minIdle = ALPS_DBL_MAX;
	
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
	double varRampUp = 0.0, stdRampUp = 0.0;
	double varRampDown = 0.0, stdRampDown = 0.0;
	
	int totalTreeSize = 0;
	int totalNodeLeft = 0;
	
	if(logFileLevel_ > 0 || msgLevel_ > 0) {
	    for(i = 0; i < processNum_; ++i) {
		totalTreeSize += tSizes[i];
		totalNodeLeft += tLefts[i];
		
		sumRampUp += rampUps[i];
		sumCpuTime += cpuTimes[i];
		sumWallClock += wallClocks[i];

		// Only valid for workers.
		if (processTypeList_[i] == AlpsProcessTypeWorker) {
		    ++numWorkers;
		    
		    sumSize += tSizes[i];
		    sumDep += tDepths[i];
		    sumIdle += idles[i];
		    sumRampDown += rampDowns[i];
		    
		    if (tSizes[i] > maxSize) maxSize = tSizes[i];
		    if (tSizes[i] < minSize) minSize = tSizes[i];
                    
		    if (tDepths[i] > maxDep) maxDep = tDepths[i];
		    if (tDepths[i] < minDep) minDep = tDepths[i];

		    if (idles[i] > maxIdle) maxIdle = idles[i];
		    if (idles[i] < minIdle) minIdle = idles[i];

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

	    for(i = 0; i < processNum_; ++i) { // Adjust idle and rampDown 
		if (tSizes[i] > 0) {           // due to periodically checking.
		    idles[i] -= minIdle;
		    rampDowns[i] -= minRampDown;
		}
	    }
            
	    sumIdle -= minIdle * numWorkers;
	    sumRampDown -= minRampDown * numWorkers;
	    maxIdle -= minIdle;
	    minIdle = 0.0;
	    maxRampDown -= minRampDown;
	    minRampDown = 0.0;	    
	    if( numWorkers != 0) {
		aveSize = sumSize / numWorkers; 
		aveDep = sumDep / numWorkers;
		aveIdle = sumIdle / numWorkers;
		aveRampDown = sumRampDown / numWorkers;
		aveRampUp = sumRampUp / numWorkers;
	    }
            
	    for (i = 0; i < processNum_; ++i) {
		if (processTypeList_[i] == AlpsProcessTypeWorker) {
		    varSize += pow(tSizes[i] - aveSize, 2);
		    varSize += pow(tDepths[i] - aveDep, 2);
		    varIdle = pow(idles[i] - aveIdle, 2);
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
		varRampDown /= numWorkers;
		stdRampDown = sqrt(varRampDown);
		varRampUp /= numWorkers;
	    }
	    stdRampUp = sqrt(varRampUp);
            
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
            logFout << "Number of nodes leftd = " 
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
		    << ", total CPU = " << sumCpuTime << std::endl;
	    logFout <<"Search wallclock  = "<<timer_.getWallClock()<<" seconds"
		    << ", max = " << maxWallClock
		    << ", min = "<< minWallClock 
		    <<", total Wall-clocks = " << sumWallClock << std::endl;

	    //----------------------------------------------
	    // Solution.
	    //----------------------------------------------

	    logFout << "Best solution quality = " << getBestQuality();
	    logFout << std::endl;
	    if (hasKnowledge(ALPS_SOLUTION) ) {
		dynamic_cast<AlpsSolution* >
		    (getBestKnowledge(ALPS_SOLUTION).first)->print(logFout);
	    }
            
	}  // Log if logFileLevel_ > 0

        if (msgLevel_ > 0) {
            if (getSolStatus() == ALPS_OPTIMAL) {
                messageHandler()->message(ALPS_T_OPTIMAL, messages())
                    << systemNodeProcessed_ << systemWorkQuantity_ 
                    << CoinMessageEol;
            }
            else if (getSolStatus() == ALPS_NODE_LIMIT) {
                messageHandler()->message(ALPS_T_NODE_LIMIT, messages())
                    << systemNodeProcessed_ << systemWorkQuantity_ 
                    << CoinMessageEol;
            }
            else if (getSolStatus() == ALPS_TIME_LIMIT) {
                messageHandler()->message(ALPS_T_TIME_LIMIT, messages())
                    << systemNodeProcessed_ << systemWorkQuantity_ 
                    << CoinMessageEol; 
            }
            else if (getSolStatus() == ALPS_FEASIBLE) {
                messageHandler()->message(ALPS_T_FEASIBLE, messages())
                    << systemNodeProcessed_ << systemWorkQuantity_ 
                    << CoinMessageEol;
            }
            else {
                messageHandler()->message(ALPS_T_INFEASIBLE, messages())
                    << systemNodeProcessed_ << systemWorkQuantity_ 
                    << CoinMessageEol;
            }
        
	    // Idle.
	    std::cout << "Average Idle = " << aveIdle << std::endl;
	    std::cout << "Max Idle = " << maxIdle << std::endl;
	    std::cout << "Min Idle = " 
                      << (minIdle != ALPS_DBL_MAX ? minIdle : 0.0) << std::endl;
	    std::cout << "Std Dev of Idle = " << stdIdle << std::endl;
	    
	    // Ramp down.
	    std::cout << "Average RampDown = " << aveRampDown << std::endl;
	    std::cout << "Max RampDown = " << maxRampDown << std::endl;
	    std::cout << "Min RampDown = " 
                      << (minRampDown != ALPS_DBL_MAX ? minRampDown : 0.0) 
                      << std::endl;
	    std::cout << "Std Dev of RampDown = " << stdRampDown << std::endl;
            
            messageHandler()->message(ALPS_LOADREPORT_MASTER, messages())
                << globalRank_ << systemNodeProcessed_ << systemWorkQuantity_ 
                << systemSendCount_ << systemRecvCount_ << incumbentValue_ 
                << CoinMessageEol;
            
	    
	    // Overall. 
	    std::cout << "Search CPU time = "<<timer_.getCpuTime() <<" seconds"
		      << ", max = " << maxCpuTime
		      << ", min = "<< minCpuTime 
		      << ", total CPU = " << sumCpuTime << std::endl;
	    std::cout << "Search wallclock  = "<<timer_.getWallClock() <<" seconds"
		      << ", max = " << maxWallClock
		      << ", min = "<< minWallClock 
		      <<", total Wall-clocks = " << sumWallClock << std::endl;
            
            std::cout << "Best solution quality = " << getBestQuality()
                      << std::endl;
        } // Print msg if msgLevel_ > 0

    }  // Only master log

    if (globalRank_ == masterRank_) {
	delete [] tSizes;     tSizes = 0;
	delete [] tDepths;    tDepths = 0;
	delete [] idles;      idles = 0;
	delete [] rampUps;    rampUps = 0;
	delete [] rampDowns;  rampDowns = 0;
    }
}

//#############################################################################

// Explore a subtree from subtree pool for certain units of work/time. 
// Return how many nodes have been processed. The same subtree will be
// explored next time if it still have unexplored nodes. 
AlpsReturnCode
AlpsKnowledgeBrokerMPI::doOneUnitWork(int unitWork,
                                      double unitTime, 
                                      AlpsSolStatus & solStatus,
                                      int & numNodesProcessed,
                                      int & depth,
                                      bool & betterSolution)
{
    
    AlpsReturnCode rCode = ALPS_OK;

    numNodesProcessed = 0; 
   
    if( (workingSubTree_ == NULL) && !(subTreePool_->hasKnowledge()) ) {
        return rCode;
    }
    
    if ( ! needWorkingSubTree_ )  {
        // Already has a subtree working on.

        rCode = workingSubTree_->exploreUnitWork(unitWork,
                                                 unitTime,
                                                 solStatus,
                                                 numNodesProcessed,
                                                 treeDepth_,
                                                 betterSolution);
        
        if ( !(workingSubTree_->nodePool()->hasKnowledge()) ) {
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
        
        rCode = workingSubTree_->exploreUnitWork(unitWork,
                                                 unitTime,
                                                 solStatus,
                                                 numNodesProcessed,
                                                 treeDepth_,
                                                 betterSolution);
        
        if ( !( workingSubTree_->nodePool()->hasKnowledge() ) ) {
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
    cluSize_ = 0;
    clusterRank_ = -1;
    hubRanks_ = 0;
    myHubRank_ = -1;
    masterRank_ = -1;
    processType_ = AlpsProcessTypeAny;
    processTypeList_ = NULL;
    hubWork_ = false;
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
    hubIndexBatch_ = 0;
    rampUpTime_ = 0.0;
    rampDownTime_ = 0.0;
    idleTime_ = 0.0;
    forceTerminate_ = false;
    blockTermCheck_ = true;
    blockHubReport_ = false;
    blockWorkerReport_ = false;
    blockAskForWork_   = false;    
}

//#############################################################################

/** Destructor. */
AlpsKnowledgeBrokerMPI::~AlpsKnowledgeBrokerMPI() 
{
    if (workerWorkQualities_ != 0) {
	delete [] workerWorkQualities_;
	workerWorkQualities_ = 0;
    }
    
//	std::cout << "Here1 -- " << globalRank_ << std::endl;
    if (hubRanks_ != 0) {
	delete [] hubRanks_; hubRanks_ = 0;
    }
//	std::cout << "Here2 -- " << globalRank_ << std::endl;
    if (processTypeList_ != NULL) {
	delete [] processTypeList_; processTypeList_ = NULL;
    }
//	std::cout << "Here3 -- " << globalRank_ << std::endl;
    if (hubWorkQualities_ != 0) {
	delete [] hubWorkQualities_; hubWorkQualities_ = 0;
    }
//	std::cout << "Here4 -- " << globalRank_ 
//		  << "c size = " << clusterSize_ << std::endl;

    if (hubWorkQuantities_ != 0) {
	delete [] hubWorkQuantities_; hubWorkQuantities_ = 0;
    }
    if (workerWorkQuantities_ != 0){
	delete [] workerWorkQuantities_;
	workerWorkQuantities_ = 0;
    }
//	std::cout << "Here5 -- " << globalRank_ << std::endl;
    
    if (workerReported_ != 0) {
	delete [] workerReported_; workerReported_ = 0;
    }
    if (hubReported_ != 0) {
	delete [] hubReported_; hubReported_ = 0;
    }
    if (workerNodeProcesseds_ != 0) {
	delete [] workerNodeProcesseds_; 
	workerNodeProcesseds_ = 0;
    }
    if (hubNodeProcesseds_ != 0) {
	delete [] hubNodeProcesseds_;
	hubNodeProcesseds_ = 0;
    }

    MPI_Finalize();
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::printBestSolution(char* outputFile) const 
{
    if (globalRank_ == masterRank_) {
	if (getNumKnowledges(ALPS_SOLUTION) <= 0) {
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
		(getBestKnowledge(ALPS_SOLUTION).first)->print(os);
	}
	else {             
	    // Write to std::cout
	    std::cout << "Quality = " << getBestQuality();
	    std::cout << "\nBest solution: " << std::endl;
	    dynamic_cast<AlpsSolution* >
		(getBestKnowledge(ALPS_SOLUTION).first)->print(std::cout);
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
AlpsKnowledgeBrokerMPI::sendKnowledge(AlpsKnowledgeType type, int receiver)
{
    // TODO
}

//#############################################################################

/** Receive knowlege from sender. */
void
AlpsKnowledgeBrokerMPI::receiveKnowlege(AlpsKnowledgeType type, int sender)
{
    // TODO
}

//#############################################################################
