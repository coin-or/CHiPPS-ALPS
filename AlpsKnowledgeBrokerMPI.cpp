#include "AlpsLicense.h"

#include <algorithm>
#include <cstring>

#include "CoinError.hpp"
#include "CoinHelperFunctions.hpp"

#include "AlpsDataPool.h"
#include "AlpsHelperFunctions.h"
#include "AlpsKnowledgeBrokerMPI.h"
#include "AlpsMessageTag.h"
#include "AlpsTreeNode.h"

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::masterMain(AlpsTreeNode* root)
{
  int i, j, k, t;
  int count;
  int hubCheckNum        = 0;
  int workReportNum      = 0;
  int msgSendNum         = 0;
  int preSysSendCount    = 0; 
  int position           = 0;

  double startTime       = 0.0;
  double curTime;

  char reply;
  char* buf              = 0; 

  bool allWorkerReported = false;          // All workers report load AL once
  bool blockTermCheck    = true;           // NO terminate check initially
  bool blockHubCheck     = false;          // HAS hubCheck initially
  bool allMsgReceived    = false;
  bool sentTermSignal    = false;
  bool terminate         = false;

  AlpsNodePool* tempNodePool = new AlpsNodePool;

  const double period    = 
    AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::masterBalancePeriod);
  const bool interCB     = 
    AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::interClusterBalance);

#if defined NF_DEBUG  
  std::cout << "MASTER: peroid = " << period << std::endl; 
#endif

  const int largeSize    = 
    AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::largeSize);
  const int medSize      = 
    AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::mediumSize);
  const int smallSize    = 
    AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::smallSize);

  const double zeroLoad  = 
    AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::zeroLoad);

  hubLoads_       = new double [hubNum_];       // Record workload
  hubReported_    = new bool [hubNum_];
  workerLoads_    = new double [clusterSize_];
  workerReported_ = new bool [clusterSize_];
  
  for (i = 0; i < hubNum_; ++i) {
    hubReported_[i] = false;
    hubLoads_[i] = 0.0;
  }
  for (i = 0; i < clusterSize_; ++i) {
    workerLoads_[i] = 0.0;
    workerReported_[i] = false;
  }
  workerLoads_[0] = workload_;

  root->setKnowledgeBroker(this); 
  root->setPriority(0);
  root->setLevel(0);
  root->setIndex(0);
  subtree_->setNextIndex(1);                   // One more than root's index

  //---------------------------------------------------------------------------
  // 1. Create required number of nodes 
 
#if defined(NF_DEBUG)
  std::cout << "MASTER: start to create nodes..." << std::endl; 
#endif

  dynamic_cast<AlpsSubTreeMaster* >(subtree_)->rampUp(root);

#if defined(NF_DEBUG)
  std::cout << "MASTER: finish creating nodes." << std::endl; 
  std::cout << "MASTER: the number of nodes Master created is " 
	    <<  getNumKnowledges(ALPS_NODE) << std::endl; 
#endif
 
  //---------------------------------------------------------------------------
  // Spawn worker processes (Not needed for static process management) 
  // FILL IN HERE IF NEEDED 
  //---------------------------------------------------------------------------
 
  //---------------------------------------------------------------------------
  // 2. Dispatch nodes to hubs

  if (getNumKnowledges(ALPS_NODE) == 0) {
    std::cout << "MASTER: Failed to generate enought nodes and finish search "
	      << "by itself." << std::endl;
  }
  else {   
    const int numNode = getNumKnowledges(ALPS_NODE);
    int       numSent = 0;
  
    while ( numSent < numNode) {
      for (i = 0; i < hubNum_; ++i) {
	if(numSent >= numNode) 
	  break;                      // Break in the sending round
	++numSent;
	if (hubRanks_[i] != globalRank_) { 
	  sendSizeNode(i, hubComm_);	
#if defined NF_DEBUG
	  std::cout << "MASTER: finish sending a node to HUB "  
		    << hubRanks_[i] << std::endl; 
#endif
	}
	else {
	  // const AlpsTreeNode* nodeM = static_cast<const AlpsTreeNode* >
	  //  (getKnowledge(ALPS_NODE).first);
	  // Cast from AlpsKnowledge* to AlpsTreeNode
	  const AlpsTreeNode* nodeM = dynamic_cast<const AlpsTreeNode*> 
	    ( getKnowledge(ALPS_NODE).first);
	  double val = getKnowledge(ALPS_NODE).second;
	  popKnowledge(ALPS_NODE);
	  tempNodePool->addKnowledge(nodeM, val);
	}
      }
    }
  }

  int tem = tempNodePool->getNumKnowledges();

  // Move nodes in tempNodePool to Master's node pool
  while (tempNodePool->hasKnowledge()) {
    const AlpsTreeNode* nodeT = dynamic_cast<const AlpsTreeNode*> 
      ( tempNodePool->getKnowledge().first ); 
    double valT = tempNodePool->getKnowledge().second;
    addKnowledge(ALPS_NODE, nodeT, valT);
    tempNodePool->popKnowledge();  
  }

  //---------------------------------------------------------------------------
  // 3. Sent finish initialization tag

  for (i = 0; i < hubNum_; ++i) {
    if (hubRanks_[i] != globalRank_)
      sendFinishInit(i, hubComm_);
  }

  //---------------------------------------------------------------------------
  // 4. If have solution, broadcast its value and process id

  if (hasKnowledge(ALPS_SOLUTION)) {
    double incVal = getBestKnowledge(ALPS_SOLUTION).second;
    if(incVal <= incumbentValue_ - TOLERANCE_) {   // Assume Minimization
      incumbentValue_ = incVal;
      incumbentID_ = globalRank_;
      broadcastIncumbent(MPI_COMM_WORLD);
    } 
  }
 
  //---------------------------------------------------------------------------
  // 5. Generate and send required number of nodes for my workers

#if defined NF_DEBUG
  std::cout << "MASTER/HUB " << globalRank_ << " : start to create nodes..." 
	    << std::endl; 
#endif

  dynamic_cast<AlpsSubTreeHub* >(subtree_)->rampUp();

#if defined NF_DEBUG
  std::cout << "MASTER/HUB " << globalRank_ <<" : finish creating nodes." 
	    << std::endl; 
  std::cout << "MASTER/HUB " << globalRank_ 
	    <<" : the number of nodes Master/Hub created is " 
	    <<  getNumKnowledges(ALPS_NODE) << std::endl; 
#endif
 
  if (getNumKnowledges(ALPS_NODE) == 0) {
    std::cout << "MASTER/HUB " << globalRank_ 
	      << " : Failed to generate enought nodes and finish search "
	      << "by itself (without help from workers)" << std::endl;
  }
  else {    // Send nodes to workers
    const int numNode2 = getNumKnowledges(ALPS_NODE);
    int       numSent2 = 0;
    
    while ( numSent2 < numNode2) {
      for (i = 0; i < clusterSize_; ++i) {
	if(numSent2 >= numNode2 ) 
	  break;
	if (i != clusterRank_) {
	  std::cout << "Master: Here" << std::endl; 
	  sendSizeNode(i, clusterComm_);
	  std::cout << "Master: after" << std::endl;
	  ++numSent2;
#if defined NF_DEBUG
	  std::cout << "MASTER/HUB " << globalRank_ 
		    <<" : finish sending nodes to Worker " 
		    << i + globalRank_ << std::endl; 
#endif
	}
      }
    }
  }
 
  //---------------------------------------------------------------------------
  // 6. Sent finish initialization tag

  for (i = 0; i < clusterSize_; ++i) {
    if (i != clusterRank_)
      sendFinishInit(i, clusterComm_);
  }

  //---------------------------------------------------------------------------
  // 7. SCHEDULER: 
  // a. Listening messages periodically. If Master found system is ready
  //    for termination check, it will activate termination checking thread.
  // b. Do termination check if termination checking thread is activated. 
  //    Do load balance if system is not tree idle. 
  //   
  //*--------------------------------------------------------------------------
  // 7.1. Listen msg
  int reqNum = 6;
  int outCount = 0;
  int* indices = new int [reqNum];
  MPI_Request* requests = new MPI_Request [reqNum];
  MPI_Status* stati = new MPI_Status [reqNum];

  char** bufs = new char* [reqNum];
  bufs[0] = new char [smallSize];
  bufs[1] = new char [smallSize];
  bufs[2] = new char [smallSize];
  bufs[3] = new char [largeSize];
  bufs[4] = new char [smallSize];
  bufs[5] = new char [smallSize];

  MPI_Irecv(bufs[0], smallSize, MPI_PACKED, MPI_ANY_SOURCE, 
	    AlpsMsgAskHubShare, MPI_COMM_WORLD, &requests[0]);
  MPI_Irecv(bufs[1], smallSize, MPI_PACKED, MPI_ANY_SOURCE, 
	    AlpsMsgIncumbent, MPI_COMM_WORLD, &requests[1]);
  MPI_Irecv(bufs[2], smallSize, MPI_PACKED, MPI_ANY_SOURCE, 
	    AlpsMsgHubPeriodReport, MPI_COMM_WORLD, &requests[2]);
  MPI_Irecv(bufs[3], largeSize, MPI_PACKED, MPI_ANY_SOURCE, 
	    AlpsMsgNode, MPI_COMM_WORLD, &requests[3]);
  MPI_Irecv(bufs[4], smallSize, MPI_PACKED, MPI_ANY_SOURCE, 
	    AlpsMsgWorkerNeedWork, MPI_COMM_WORLD, &requests[4]);
  MPI_Irecv(bufs[5], smallSize, MPI_PACKED, MPI_ANY_SOURCE, 
	    AlpsMsgWorkerStatus, MPI_COMM_WORLD, &requests[5]);

  //Testing
  //systemSendCount_ += 2; 


  while (true) {                        // Keep doing until terminate

    hubCheckNum = 0;
    workReportNum = 0;
    
    if ( ! blockHubCheck ) {
      //std::cout << "MASTER: hubCheckOwnCluster(...)" << std::endl; 
      hubCheckOwnCluster(AlpsMsgHubPeriodCheck, MPI_COMM_WORLD);  
      hubCheckNum += (clusterSize_ - 1);
      sendCount_ += (clusterSize_ - 1);
    }

    startTime = AlpsCpuTime();
    // blockTermCheck   = true; 
     
    curTime = AlpsCpuTime();
#if defined NF_DEBUG_MORE
    std::cout << "MASTER " << globalRank_ << " : curTime =  " << curTime 
	      << std::endl; 
#endif

    //**-----------------------------------------------------------------------
    // listen one period 
    // while ((AlpsCpuTime() - startTime) < period || startTermCheck  == true) {

    while ( AlpsCpuTime() + 0.01 - startTime < period ) {
  
      //std::cout << "Master: inside time period" << std::endl;
      MPI_Testsome(reqNum, requests, &outCount, indices, stati);
 
      //***--------------------------------------------------------------------
      for (t = 0; t < outCount; ++t) {            
	
	++recvCount_;
	k = indices[t];

	switch(stati[t].MPI_TAG) {

	case AlpsMsgAskHubShare:
	  hubsShareWork(bufs[0], &stati[t], MPI_COMM_WORLD);
	  MPI_Irecv(bufs[0], smallSize, MPI_PACKED, MPI_ANY_SOURCE, 
		    AlpsMsgAskHubShare, MPI_COMM_WORLD, &requests[0]);
	  break;
	case AlpsMsgIncumbent:
	  unpackSetIncumbent(bufs[1], &stati[t], MPI_COMM_WORLD);
	  MPI_Irecv(bufs[1], smallSize, MPI_PACKED, MPI_ANY_SOURCE, 
		    AlpsMsgIncumbent, MPI_COMM_WORLD, &requests[1]);
	  break;
	case AlpsMsgHubPeriodReport:
	  masterUpdateSysStatus(bufs[2], &stati[t], MPI_COMM_WORLD);
	  MPI_Irecv(bufs[2], smallSize, MPI_PACKED, MPI_ANY_SOURCE, 
		    AlpsMsgHubPeriodReport, MPI_COMM_WORLD, &requests[2]);
	  break;
	case AlpsMsgNode:
	  ++sendCount_;
	  hubAllocateDonation(bufs[3], &stati[t], MPI_COMM_WORLD);
	  MPI_Get_count(&stati[t], MPI_PACKED, &count);
	  if (count > 0) 
	    blockHubCheck  = false;
	  MPI_Irecv(bufs[3], largeSize, MPI_PACKED, MPI_ANY_SOURCE, 
		    AlpsMsgNode, MPI_COMM_WORLD, &requests[3]);
	  break;
	case AlpsMsgWorkerNeedWork:
	  ++sendCount_;
	  hubBalanceWorkers(bufs[4], &stati[t], MPI_COMM_WORLD);
	  MPI_Irecv(bufs[4], smallSize, MPI_PACKED, MPI_ANY_SOURCE, 
		    AlpsMsgWorkerNeedWork, MPI_COMM_WORLD, &requests[4]);
	  break;
	case AlpsMsgWorkerStatus:
	  ++workReportNum;
	  hubUpdateCluStatus(bufs[5], &stati[t], MPI_COMM_WORLD);
	  MPI_Irecv(bufs[5], smallSize, MPI_PACKED, MPI_ANY_SOURCE, 
		    AlpsMsgWorkerStatus, MPI_COMM_WORLD, &requests[5]);
	  break;
	default: 
	  std::cout << "Master: recved UNKNOWN message. tag = " 
		    << stati[t].MPI_TAG <<  std::endl; 
	  throw CoinError("Unknow message type", 
			  "workmain", "AlpsKnowledgeBrokerMPI");
	}

      }
      //***--------------------------------------------------------------------
    
    }
    //**-----------------------------------------------------------------------
    
    if (outCount > 0)
      allMsgReceived = false;
    else
      allMsgReceived = true; 
      
    if ( ! allWorkerReported ) {	
      workerReported_[0] = true;
      allWorkerReported = true;
      for (i = 0; i < clusterSize_; ++i) {
	if (workerReported_[i] == false) {
	  allWorkerReported = false;
	  break;
	}
      }
    }
      

    // MUST do block hub check! When (1) all workers have reported workloads
    // at least once, (2) the total workload of the cluster is zero, and 
    // (3) all the msgs are hub check and worker report, it is ready to
    // block hub periodically checking.
    
    clusterWorkload_ += workload_;         // Include the status of hub
    clusterSendCount_ += sendCount_;
    clusterRecvCount_ += recvCount_;
    
#if defined NF_DEBUG_MORE
  std::cout << "HUB "<< globalRank_ 
	    << ": clusterWorkload_  = " << clusterWorkload_
	    << ", sendCount_ = " << sendCount_
	    << ", recvCount_ = " << recvCount_
	    << ", hubCheckNum = " << hubCheckNum
	    << ", workReportNum = " << workReportNum
	    << ", allWorkerReported = " << allWorkerReported
	    << ", blockHubCheck = " << blockHubCheck
	    << std::endl; 
#endif

    if ( ! blockHubCheck ) {                  // NOT block
      if (allWorkerReported && 
	  clusterWorkload_ < zeroLoad && 
	  hubCheckNum == sendCount_ &&
	  workReportNum == recvCount_) {
	
	blockHubCheck  = true;                // Stop checking hub status  
#if defined NF_DEBUG_MORE
	std::cout << "Master: blockHubCheck." << std::endl;
#endif
      }
    }
    else {                                    // Already blocked
      if ( ! allWorkerReported ||
	   clusterWorkload_ > zeroLoad ||
	   hubCheckNum != sendCount_ ||
	   workReportNum != recvCount_) {
	
	blockHubCheck = false;
#if defined NF_DEBUG_MORE
	std::cout << "Master: unblockHubCheck." << std::endl;
#endif
      }
    }
    
    sendCount_ = recvCount_ = 0;       // IMPORTANT!

    // Master cal the status of its cluster and the status of the system
    refreshSysStatus();

#if defined NF_DEBUG_MORE
    std::cout << "Master: systemWorkload_ = " << systemWorkload_
	      << ", systemSendCount_ = " << systemSendCount_
	      << ", systemRecvCount_ = " << systemRecvCount_ 
	      << ", blockTermCheck = " << blockTermCheck 
	      << ", allHubReported_ = " << allHubReported_
	      << std::endl;
#endif

    if (allMsgReceived  && 
	allWorkerReported && 
	allHubReported_ &&
	systemWorkload_ < zeroLoad && 
	systemSendCount_ == systemRecvCount_) 
   
      blockTermCheck = false;                  // Activate termination check


    // 7.2.1 Do termination checking if activated
    //       All msgs during termination checking are not counted.
    if ( ! blockTermCheck ) {
      
      // Inform all other processes to do termination checking
      for (i = 0; i < processNum_; ++i) {
	if (i != globalRank_ )                 // i is not Master 
	  MPI_Send(buf, 0, MPI_PACKED, i, AlpsMsgAskPause,
		   MPI_COMM_WORLD);
      }

      preSysSendCount = systemSendCount_;

      if(buf != 0) {
	delete [] buf; 
	buf = 0;
      }
      buf = new char [smallSize];

      MPI_Request termReq;
      MPI_Status termSta;

      // Update the cluster status to which Master belongs
      for(i = 1; i < clusterSize_; ++i) {
	MPI_Irecv(buf, smallSize, MPI_PACKED, MPI_ANY_SOURCE,
		  AlpsMsgWorkerTermStaus, clusterComm_, &termReq);
	MPI_Wait(&termReq, &termSta);
	hubUpdateCluStatus(buf, &termSta, clusterComm_);
      }
      clusterWorkload_ += workload_;      // Include the status of hub(Master)
      clusterSendCount_ += sendCount_;
      clusterRecvCount_ += recvCount_;
      sendCount_ = recvCount_ = 0;

      // Update total system's status
      for(i = 1; i < hubNum_; ++i) {
	MPI_Irecv(buf, smallSize, MPI_PACKED, MPI_ANY_SOURCE,
		  AlpsMsgHubTermStaus, hubComm_, &termReq);
	MPI_Wait(&termReq, &termSta);
	masterUpdateSysStatus(buf, &termSta, hubComm_);
      }
      systemWorkload_ += clusterWorkload_;    // Include the cluster Master in
      systemSendCount_ += clusterSendCount_;
      systemRecvCount_ += clusterRecvCount_;
      clusterSendCount_ = clusterRecvCount_ = 0;

#if defined NF_DEBUG_MORE
	std::cout << "Master: TERM: systemWorkload_ = " << systemWorkload_
		  << ", systemSendCount_ = " << systemSendCount_
		  << ", systemRecvCount_ = " << systemRecvCount_ 
		  << std::endl;
#endif


      if (systemWorkload_ < zeroLoad && preSysSendCount == systemSendCount_)
      //if (systemWorkload_ < zeroLoad)
	terminate = true;
      else 
	terminate = false;

      if (terminate) {                               // True idle, terminate
	for (i = 0; i < processNum_; ++i) {
	  if (i != globalRank_) {                    // i is not Master 
	    reply = 'T';
	    MPI_Send(&reply, 1, MPI_CHAR, i, AlpsMsgContOrTerm,
		     MPI_COMM_WORLD);
	  }
	}

	break;                             // Break *,  Master terminates
      }
      else {                                         // Not idle yet
	blockTermCheck = true;
	for (i = 0; i < processNum_; ++i) {
	  if (i != globalRank_) {                    // i is not Master 
	    reply = 'C';
	    MPI_Send(&reply, 1, MPI_CHAR, i, AlpsMsgContOrTerm,
		     MPI_COMM_WORLD);
	  }
	}    
      }
    }
      
    // Do not terminate
    if ( ! terminate ) {
      if (hubNum_ > 1 && interCB) {
	// Master balance 
	masterBalanceHubs(MPI_COMM_WORLD);
      }
    }

  }

  //*--------------------------------------------------------------------------
  // Cancel MPI_Irecv
  int flag;
  int cancelNum = 0;

  for (i = 0; i < reqNum; ++i) {
    MPI_Cancel(&requests[i]);
    MPI_Wait(&requests[i], &stati[i]);

    MPI_Test_cancelled(&stati[i], &flag);
    if(flag)   // Cancel succeeded
      ++cancelNum;
  }

#if defined NF_DEBUG_MORE
  std::cout << "MASTER " << globalRank_ << " cancelled " << cancelNum 
	    << " MPI_Irecv" << std::endl;
#endif


  // Free memory
  if (buf != 0) { 
    delete [] buf; 
    buf = 0; 
  }
  if (bufs != 0) {
    for (i = 0; i < reqNum; ++i) 
      delete [] bufs[i];
    delete [] bufs;
    bufs = 0;
  }
  if (indices != 0) {
    delete [] indices;
    indices = 0;
  }
  if (requests != 0) {
    delete [] requests;
    requests = 0;
  }
  if (stati != 0) {
    delete [] stati;
    stati = 0;
  }

}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::hubMain()
{
  int i, k, t;
  int count;
  int hubCheckNum        = 0;
  int hubReportNum       = 0;
  int workReportNum      = 0;

  char reply;
  char* buf              = 0;
  double startTime       = 0.0;

  bool allMsgReceived    = false;
  bool allWorkerReported = false;          // All workers report load AL once
  bool blockTermCheck    = true;
  bool terminate         = false;
  bool blockHubReport    = false;
  bool blockHubCheck     = false;

  const int largeSize    = 
    AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::largeSize);
  const int medSize      = 
    AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::mediumSize);
  const int smallSize    = 
    AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::smallSize);
  const double period    = 
    AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::hubReportPeriod);
  const double zeroLoad  = 
    AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::zeroLoad); 

  MPI_Status status;
  
  workerLoads_    = new double [clusterSize_];
  workerReported_ = new bool [clusterSize_];

  for (i = 0; i < clusterSize_; ++i)
    workerLoads_[i] = 0.0;
  workerLoads_[0] = workload_ = 0.0;

  //---------------------------------------------------------------------------
  // 1. Recv nodes sent by Master

#if defined NF_DEBUG
  std::cout << "HUB " << globalRank_ << " : start to recv nodes."<< std::endl; 
#endif
  while(true) {
    receiveSizeNode(0, hubComm_, &status);   // From rank 0 (master)
    if (status.MPI_TAG == AlpsMsgFinishInit) {
      std::cout << "HUB " << globalRank_ << " rec AlpsMsgFinishInit." 
		<< std::endl; 
      break;
    }
  }
 
  //---------------------------------------------------------------------------
  // 2. Generate and send required number of nodes for my workers

#if defined NF_DEBUG
  std::cout << "HUB " << globalRank_ << " : start to create nodes..." 
	    << std::endl; 
#endif

  dynamic_cast<AlpsSubTreeHub*>(subtree_)->rampUp();

#if defined NF_DEBUG
  std::cout << "HUB " << globalRank_ << " : finish creating nodes." 
	    << std::endl; 
  std::cout << "HUB " << globalRank_ 
	    << " : the number of nodes Hub created is " 
	    <<  getNumKnowledges(ALPS_NODE) << std::endl; 
#endif
 
  if (getNumKnowledges(ALPS_NODE) == 0) {
    std::cout << "HUB " << globalRank_ 
	      << " : Failed to generate enought nodes and finish search "
	      << "by itself (without help from workers)" << std::endl;
  }
  else {    // Send nodes to workers
    const int numNode = getNumKnowledges(ALPS_NODE);
    int       numSent = 0;
    
    while ( numSent < numNode) {
      for (i = 0; i < clusterSize_; ++i) {
	if(numSent >= numNode ) break;
	if (i != clusterRank_) { 
	  sendSizeNode(i, clusterComm_);
	  ++numSent;
#if defined NF_DEBUG
	  std::cout << "HUB " << globalRank_ 
		    << " : finish sending nodes to Worker " 
		    << i + globalRank_ << std::endl; 
#endif
	}
      }
    }
  }
 
  //---------------------------------------------------------------------------
  // 3. Sent finish initialization tag

  for (i = 0; i < clusterSize_; ++i) {
    if (i != clusterRank_)
      sendFinishInit(i, clusterComm_);
  }

  //---------------------------------------------------------------------------
  // 4. If have solution, broadcast its value and process id

  if (hasKnowledge(ALPS_SOLUTION)) {
    double incVal = getBestKnowledge(ALPS_SOLUTION).second;
    if(incVal <= incumbentValue_ -  TOLERANCE_) {   // Assume Minimization
      incumbentValue_ = incVal;
      incumbentID_ = globalRank_;
      broadcastIncumbent(MPI_COMM_WORLD);
    } 
  }

  //---------------------------------------------------------------------------
  // 5. SCHEDULER:
  // (1) Listening messages
  // (2) Send load info, msg counts to Master

  //*--------------------------------------------------------------------------

  int reqNum = 6;
  int outCount = 0;
  int* indices = new int [reqNum];
  MPI_Request* requests = new MPI_Request [reqNum];
  MPI_Status* stati = new MPI_Status [reqNum];

  char** bufs = new char* [reqNum];
  bufs[0] = new char [smallSize];
  bufs[1] = new char [smallSize];
  bufs[2] = new char [smallSize];
  bufs[3] = new char [smallSize];
  bufs[4] = new char [largeSize];
  bufs[5] = new char [smallSize];

  MPI_Irecv(bufs[0], smallSize, MPI_PACKED, MPI_ANY_SOURCE, 
	    AlpsMsgAskHubShare, MPI_COMM_WORLD, &requests[0]);
  MPI_Irecv(bufs[1], smallSize, MPI_PACKED, MPI_ANY_SOURCE, 
	    AlpsMsgAskPause, MPI_COMM_WORLD, &requests[1]);
  MPI_Irecv(bufs[2], smallSize, MPI_PACKED, MPI_ANY_SOURCE, 
	    AlpsMsgIncumbent, MPI_COMM_WORLD, &requests[2]);
  MPI_Irecv(bufs[3], smallSize, MPI_PACKED, MPI_ANY_SOURCE, 
	    AlpsMsgWorkerNeedWork, MPI_COMM_WORLD, &requests[3]);
  MPI_Irecv(bufs[4], largeSize, MPI_PACKED, MPI_ANY_SOURCE, 
	    AlpsMsgNode, MPI_COMM_WORLD, &requests[4]);
  MPI_Irecv(bufs[5], smallSize, MPI_PACKED, MPI_ANY_SOURCE, 
	    AlpsMsgWorkerStatus, MPI_COMM_WORLD, &requests[5]);

  while (true) {
 
    startTime = AlpsCpuTime();
    hubCheckNum = 0;
    hubReportNum = 0;
    workReportNum = 0; 

    // Periodically check cluster status
    if ( ! blockHubCheck ) {
      //std::cout << "HUB " << globalRank_ << ": ! blockHubCheck" << std::endl;
      hubCheckOwnCluster(AlpsMsgHubPeriodCheck, MPI_COMM_WORLD);  
      hubCheckNum += (clusterSize_ - 1);
      sendCount_ += (clusterSize_ - 1);
    }  
 
 
    //**-----------------------------------------------------------------------
    // 5.1. Listen msg for one period
 
    while ( AlpsCpuTime() + TOLERANCE_ - startTime < period ) { 

      MPI_Testsome(reqNum, requests, &outCount, indices, stati);

      for (t = 0; t < outCount; ++t) {            
	++recvCount_;
	k = indices[t];   
	
	switch(stati[t].MPI_TAG) {
	case AlpsMsgAskHubShare:
	  hubsShareWork(bufs[0], &stati[t], MPI_COMM_WORLD);
	  MPI_Irecv(bufs[0], smallSize, MPI_PACKED, MPI_ANY_SOURCE, 
		    AlpsMsgAskHubShare, MPI_COMM_WORLD, &requests[0]);
	  break;
	case AlpsMsgAskPause:
	  --recvCount_;                    // Msg during term is NOT counted
	  blockTermCheck = false;
	  std::cout << "HUB " << globalRank_ << " : startTermCheck"
		    << std::endl; 
	  MPI_Irecv(bufs[1], smallSize, MPI_PACKED, MPI_ANY_SOURCE, 
		    AlpsMsgAskPause, MPI_COMM_WORLD, &requests[1]);
	  break;
	case AlpsMsgIncumbent:
	  unpackSetIncumbent(bufs[2], &stati[t], MPI_COMM_WORLD);
	  MPI_Irecv(bufs[2], smallSize, MPI_PACKED, MPI_ANY_SOURCE, 
		    AlpsMsgIncumbent, MPI_COMM_WORLD, &requests[2]);
	  break;
	case AlpsMsgWorkerNeedWork:
	  ++sendCount_;
	  hubBalanceWorkers(bufs[3], &stati[t], MPI_COMM_WORLD);
	  MPI_Irecv(bufs[3], smallSize, MPI_PACKED, MPI_ANY_SOURCE, 
		    AlpsMsgWorkerNeedWork, MPI_COMM_WORLD, &requests[3]);
	  break;
	case AlpsMsgNode:
	  ++sendCount_;
	  hubAllocateDonation(bufs[4], &stati[t], MPI_COMM_WORLD);
	  MPI_Get_count(&stati[t], MPI_PACKED, &count);
	  if (count > 0) {
	    blockHubReport = false;
	    blockHubCheck  = false;
	  }
	  MPI_Irecv(bufs[4], largeSize, MPI_PACKED, MPI_ANY_SOURCE, 
		    AlpsMsgNode, MPI_COMM_WORLD, &requests[4]);
	  break;
	case AlpsMsgWorkerStatus:
	  ++workReportNum;
	  hubUpdateCluStatus(bufs[5], &stati[t], MPI_COMM_WORLD);
//  	  if (clusterWorkload_ < zeroLoad)
//  	    blockHubCheck = true;
	  MPI_Irecv(bufs[5], smallSize, MPI_PACKED, MPI_ANY_SOURCE, 
		    AlpsMsgWorkerStatus, MPI_COMM_WORLD, &requests[5]);
	  break;
	default: 
	  std::cout << "HUB " << globalRank_ 
		    << " : recved UNKNOWN message. tag = " 
		    << status.MPI_TAG <<  std::endl; 
	  throw CoinError("Unknow message type", 
			  "workmain", "AlpsKnowledgeBrokerMPI");
	}	
   
      }

    }
    //**-----------------------------------------------------------------------

    // 5.2 Periodically report cluster status (workload info and msg counts)
    //     to master if needed.


    if (outCount > 0)
      allMsgReceived = false;
    else
      allMsgReceived = true; 

    if ( ! allWorkerReported ) {
      workerLoads_[0] = workload_;
      workerReported_[0] = true;
      allWorkerReported = true;
      for (i = 0; i < clusterSize_; ++i) {
	if (workerReported_[i] == false) {
	  allWorkerReported = false;
	  break;
	}
      }
    }

    // MUST do block hub check! When (1) all workers have reported workloads
    // at least once, (2) the total workload of the cluster is zero, and 
    // (3) all the msgs are hub check and worker report, it is ready to
    // block hub periodically checking.

    clusterWorkload_ += workload_;         // Include the status of hub

#if defined NF_DEBUG_MORE
  std::cout << "HUB "<< globalRank_ 
	    << ": clusterWorkload_  = " << clusterWorkload_
	    << ", sendCount_ = " << sendCount_
	    << ", recvCount_ = " << recvCount_
	    << ", hubCheckNum = " << hubCheckNum
	    << ", workReportNum = " << workReportNum
	    << ", allWorkerReported = " << allWorkerReported
	    << ", blockHubCheck = " << blockHubCheck
	    << ", blockTermCheck = " << blockTermCheck
	    << std::endl; 
#endif

    if ( ! blockHubCheck ) {                  // NOT block
      if (allWorkerReported && 
	  clusterWorkload_ < zeroLoad && 
	  hubCheckNum == sendCount_ &&
	  workReportNum == recvCount_) {
	
	blockHubCheck  = true;                // Stop checking hub status  
#if defined NF_DEBUG_MORE
	std::cout << "HUB " << globalRank_ << ": blockHubCheck." << std::endl;
#endif
      }
    }
    else {                                    // Already blocked
      if ( ! allWorkerReported ||
	   clusterWorkload_ > zeroLoad ||
	   hubCheckNum != sendCount_ ||
	   workReportNum != recvCount_) {
	
	blockHubCheck = false;
#if defined NF_DEBUG_MORE
	std::cout << "HUB " << globalRank_<<" : unblockHubCheck." << std::endl;
#endif
      }
    }

    // First check whether need report, then check whether need block
    if ( ! blockHubReport || ! blockHubCheck ) {
      ++sendCount_;
      clusterSendCount_ += sendCount_;
      clusterRecvCount_ += recvCount_;
      sendCount_ = recvCount_ = 0;                           // IMPORTANT!
      hubReportStatus(AlpsMsgHubPeriodReport, MPI_COMM_WORLD);
    }


    // If hub check is blocked, then there is no need to report. 
    // If hub check is activated, then hub needs report.
    blockHubReport = blockHubCheck;



//      if (blockHubReport == true) {                     // Check if need unblock
//        if (clusterWorkload_ < zeroLoad || clusterSendCount_ == 0
//  	  || clusterRecvCount_ == 0)
//  	blockHubReport = false;
//      }


//      if (clusterWorkload_ < zeroLoad && clusterSendCount_ == 0
//  	&& clusterRecvCount_ == 0 && allWorkerReported) // Check if need block
//        blockHubReport = true;


    // 5.3 Termination check
    //    if (startTermCheck && blockHubReport && allMsgReceived) {

    if ( ! blockTermCheck && blockHubCheck ) {
      
      if(buf != 0) {
	delete [] buf; 
	buf = 0;
      }
      buf = new char [smallSize];
  
      // a. Recv workers' stati
      MPI_Request termReq;
      MPI_Status termSta;
      for(i = 1; i < clusterSize_; ++i) {
	MPI_Irecv(buf, smallSize, MPI_PACKED, MPI_ANY_SOURCE,
		  AlpsMsgWorkerTermStaus, clusterComm_, &termReq);
	MPI_Wait(&termReq, &termSta);
	hubUpdateCluStatus(buf, &termSta, clusterComm_);
      }
      clusterWorkload_ += workload_;         // Include the status of hub
      clusterSendCount_ += sendCount_;
      clusterRecvCount_ += recvCount_;
      sendCount_ = recvCount_ = 0;

      // b. Report hub's status
      hubReportStatus(AlpsMsgHubTermStaus, hubComm_);

      // c. Get instruction from Master
      MPI_Irecv(&reply, 1, MPI_CHAR, masterRank_,
		AlpsMsgContOrTerm, MPI_COMM_WORLD, &termReq);
      MPI_Wait(&termReq, &termSta);

      if(reply == 'T') {
	std::cout << "HUB " << globalRank_ << ": is exiting..." << std::endl; 
	terminate = true;
	break;                               // Break * and terminate
      }
      else {
	std::cout << "HUB " << globalRank_ << ": will continue." << std::endl; 
	terminate = false;
	blockTermCheck = true;
      }
    
    }
  }
  //*--------------------------------------------------------------------------

  // Cancel MPI_Irecv
  int flag;
  int cancelNum = 0;

  for (i = 0; i < reqNum; ++i) {
    MPI_Cancel(&requests[i]);
    MPI_Wait(&requests[i], &stati[i]);

    MPI_Test_cancelled(&stati[i], &flag);
    if(flag)   // Cancel succeeded
      ++cancelNum;
  }

#if defined NF_DEBUG_MORE
  std::cout << "HUB " << globalRank_ << " cancelled " << cancelNum 
	    << " MPI_Irecv" << std::endl;
#endif

  // Free memory
  if (buf != 0) { 
    delete [] buf; 
    buf = 0; 
  }
  if (bufs != 0) {
    for (i = 0; i < reqNum; ++i) 
      delete [] bufs[i];
    delete [] bufs;
    bufs = 0;
  }
  if (indices != 0) {
    delete [] indices;
    indices = 0;
  }
  if (requests != 0) {
    delete [] requests;
    requests = 0;
  }
  if (stati != 0) {
    delete [] stati;
    stati = 0;
  }

}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::workerMain()
{
  int i, t;
  int iter            = 0;
  int count           = 0;
  int hubCheckNum     = 0;
 
  char reply;
  char* buf           = 0;
  double startTime    = 0.0;

  bool terminate          = false;
  bool allMsgReceived     = false;

  bool blockTermCheck     = true;
  bool blockAskForWork    = false;
  //bool blockWorkerReport  = false;

  const int unitNodes     =
    AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::unitWorkNodes);
  const int display      =
    AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::display);
  const int nodeInterval =
    AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::nodeInterval);
    
  const int largeSize    = 
    AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::largeSize);
  const int medSize      = 
    AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::mediumSize);
  const int smallSize    = 
    AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::smallSize);
 
  const double workerAP  = 
    AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::workerAskPeriod); 
  const double zeroLoad  = 
    AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::zeroLoad); 
  const double needLoadThreshold  = 
    AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::needLoadThreshold); 

  const bool intraCB     = 
    AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::intraClusterBalance);

  MPI_Status status;

  //---------------------------------------------------------------------------
  // 1. Recv nodes sent by my hub
#if defined NF_DEBUG
  std::cout << "WORKER " << globalRank_ << " : start to recv nodes."
	    << std::endl; 
#endif
  while(true) {
    receiveSizeNode(0, clusterComm_, &status); // From rank 0 (hub)
    if (status.MPI_TAG == AlpsMsgFinishInit) {
      std::cout << "WORKER " << globalRank_ << " : rec AlpsMsgFinishInit." 
		<< std::endl; 
      break;
    }
  }

#if defined NF_DEBUG  
  std::cout << "WORKER " << globalRank_ <<" : start to solve(node)." 
	    << std::endl; 
#endif

  //---------------------------------------------------------------------------
  // 2. SCHEDULER:
  // (1) Listening messages
  // (2) Do one unit of work


  int reqNum = 6;
  int outCount = 0;
  int* indices = new int [reqNum];
  MPI_Request* requests = new MPI_Request [reqNum];
  MPI_Status* stati = new MPI_Status [reqNum];

  char** bufs = new char* [reqNum];
  bufs[0] = new char [smallSize];
  bufs[1] = new char [smallSize];
  bufs[2] = new char [smallSize];
  bufs[3] = new char [smallSize];
  bufs[4] = new char [smallSize];
  bufs[5] = new char [largeSize];

  MPI_Irecv(bufs[0], smallSize, MPI_PACKED, MPI_ANY_SOURCE, 
	    AlpsMsgAskDonate, MPI_COMM_WORLD, &requests[0]);
  MPI_Irecv(bufs[1], smallSize, MPI_PACKED, MPI_ANY_SOURCE, 
	    AlpsMsgAskDonateToHub, MPI_COMM_WORLD, &requests[1]);
  MPI_Irecv(bufs[2], smallSize, MPI_PACKED, MPI_ANY_SOURCE, 
	    AlpsMsgAskPause, MPI_COMM_WORLD, &requests[2]);
  MPI_Irecv(bufs[3], smallSize, MPI_PACKED, MPI_ANY_SOURCE, 
	    AlpsMsgHubPeriodCheck, MPI_COMM_WORLD, &requests[3]);
  MPI_Irecv(bufs[4], smallSize, MPI_PACKED, MPI_ANY_SOURCE, 
	    AlpsMsgIncumbent, MPI_COMM_WORLD, &requests[4]);
  MPI_Irecv(bufs[5], largeSize, MPI_PACKED, MPI_ANY_SOURCE, 
	    AlpsMsgNode, MPI_COMM_WORLD, &requests[5]);

  
  startTime = AlpsCpuTime();
  //*--------------------------------------------------------------------------
  while (true) {
    
    blockTermCheck = true;

    //**-----------------------------------------------------------------------
    // 2.1 listen and process until no message
    while (true) {
      
      hubCheckNum = 0;

      MPI_Testsome(reqNum, requests, &outCount, indices, stati);

      if (outCount > 0)
	allMsgReceived = false;
      else
	allMsgReceived = true; 

      for (t = 0; t < outCount; ++t) {        

	++recvCount_;

	switch (stati[t].MPI_TAG) {

	case AlpsMsgAskDonate:
	  ++sendCount_;
	  donateWork(bufs[0], &stati[t], MPI_COMM_WORLD);
	  MPI_Irecv(bufs[0], smallSize, MPI_PACKED, MPI_ANY_SOURCE, 
		    AlpsMsgAskDonate, MPI_COMM_WORLD, &requests[0]);
	  break;
	case AlpsMsgAskDonateToHub:
	  ++sendCount_;
	  donateWork(bufs[1], &stati[t], MPI_COMM_WORLD);
	  MPI_Irecv(bufs[1], smallSize, MPI_PACKED, MPI_ANY_SOURCE, 
		    AlpsMsgAskDonateToHub, MPI_COMM_WORLD, &requests[1]);
	  break;
	case AlpsMsgAskPause:
	  --recvCount_;                    // Msg during term is NOT counted
	  // startTermCheck = true;
	  blockTermCheck = false;
	  MPI_Irecv(bufs[2], smallSize, MPI_PACKED, MPI_ANY_SOURCE, 
		    AlpsMsgAskPause, MPI_COMM_WORLD, &requests[2]);
	  break;
	case AlpsMsgHubPeriodCheck:
	  ++hubCheckNum;
//  	  if (blockWorkerReport == true) {   // Whether need unblock report 
//  	    if (workload_ > zeroLoad || sendCount_ > 0)
//  	      blockWorkerReport = false;
//  	  }
//  	  if (workload_ < zeroLoad && sendCount_ == 0)
//  	    blockWorkerReport = true;        // Block report 
//  	  if ( ! blockWorkerReport ) {
  	    ++sendCount_;
	    workerReportStatus(AlpsMsgWorkerStatus, MPI_COMM_WORLD);
//  	    if (workload_ < zeroLoad)
//  	      blockWorkerReport = true;

#if defined NF_DEBUG_MORE
	  std::cout << "WORKER "<< globalRank_
  	    << ": WHILEreport status, tag = "<< stati[t].MPI_TAG <<  std::endl;
#endif
//  	  }
	  MPI_Irecv(bufs[3], smallSize, MPI_PACKED, MPI_ANY_SOURCE, 
		    AlpsMsgHubPeriodCheck, MPI_COMM_WORLD, &requests[3]);
	  break;
	case AlpsMsgIncumbent:
	  unpackSetIncumbent(bufs[4], &stati[t], MPI_COMM_WORLD);
	  MPI_Irecv(bufs[4], smallSize, MPI_PACKED, MPI_ANY_SOURCE, 
		    AlpsMsgIncumbent, MPI_COMM_WORLD, &requests[4]);
	  break;
	case AlpsMsgNode:
	  receiveNode(bufs[5], stati[t].MPI_SOURCE, MPI_COMM_WORLD, &stati[t]);
	  MPI_Get_count(&stati[t], MPI_PACKED, &count);
	  if (count <= 0) {
	    blockAskForWork  = true;
	    std::cout << "WORKER " << globalRank_<<": block asking for work. "
		      << std::endl;
	  }
	  else {
	    blockAskForWork  = false;
	    //  blockWorkerReport   = false;
	  }
	  MPI_Irecv(bufs[5], largeSize, MPI_PACKED, MPI_ANY_SOURCE, 
		    AlpsMsgNode, MPI_COMM_WORLD, &requests[5]);
	  break;
	default: 
	  std::cout << "WORKER " << globalRank_ 
		    << " : recved UNKNOWN message. tag = " 
		    << stati[t].MPI_TAG <<  std::endl; 
	  throw CoinError("Unknow message type", 
			  "workmain", "AlpsKnowledgeBrokerMPI"); 
	} 
      }

      if (allMsgReceived)
	break;
    }
    //**-----------------------------------------------------------------------

//      if (hubCheckNum == recvCount_)
//        blockWorkerReport = true;

    if (blockTermCheck) {                   // Don't do termination checking
    
      // a. Do one unit work 
      if ( hasKnowledge(ALPS_NODE) ) {
	doOneUnitWork();
	iter += unitNodes;
	if (display == 1 && iter % nodeInterval == 0) {
	  std::cout << "WORKER " << globalRank_ << " : Iteration: " << iter 
		    << "\tNum of Nodes in Tree: " 
		    << getNumKnowledges(ALPS_NODE) << std::endl;
	}
      }
    
      // b. If found a solution, check whether need to broadcast it
      if ( hasKnowledge(ALPS_SOLUTION) ) {
	double incVal = getBestKnowledge(ALPS_SOLUTION).second;
	if(incVal <= incumbentValue_ -  TOLERANCE_) {  // Assume Minimization
	  incumbentValue_ = incVal;
	  incumbentID_ = globalRank_;
	  broadcastIncumbent(MPI_COMM_WORLD);
	} 
      }
      
      // c. Periodically cal load and ask for load if needed. 
      if (intraCB && ! blockAskForWork && 
	  (AlpsCpuTime() - startTime > workerAP )) {
	workload_ = calWorkload();
	if (workload_ < needLoadThreshold)  {
	
	  MPI_Send(buf, 0, MPI_PACKED, myHubRank_, AlpsMsgWorkerNeedWork, 
		   MPI_COMM_WORLD);
	  ++sendCount_;
	  blockAskForWork = true;                 // Block asking
	  
#if defined NF_DEBUG
	  std::cout << "WORKER " << globalRank_ << " : INTRA: ask for work"
		    << std::endl;
#endif
	  startTime = AlpsCpuTime();              // Reset time
	}
      }
    }
    else {                                        // Do termination checking
      // a. Report its status
      workerReportStatus(AlpsMsgWorkerTermStaus, clusterComm_);

      // b. Get instruction from Master
      MPI_Request termReq;
      MPI_Status termSta;
      MPI_Irecv(&reply, 1, MPI_CHAR, masterRank_,
		AlpsMsgContOrTerm, MPI_COMM_WORLD, &termReq);
      MPI_Wait(&termReq, &termSta);
	
#if defined NF_DEBUG_MORE
      std::cout << "WORKER " << globalRank_ << " : TERM : "
		<< "report." << std::endl;
#endif

      if(reply == 'T') {
	terminate = true;
	
#if defined NF_DEBUG
	std::cout << "WORKER " << globalRank_ << " : get instruction to "
		  << "terminate. " << std::endl;
#endif

	break;                   // Break * and terminate
      }
      else {
	blockTermCheck = true;
	terminate = false;	
#if defined NF_DEBUG
	std::cout << "WORKER " << globalRank_ << " : get instruction to "
		  << "continue. " << std::endl;
#endif
      }
    }
  }
  //*--------------------------------------------------------------------------

  // Cancel MPI_Irecv
  int flag;
  int cancelNum = 0;

  for (i = 0; i < reqNum; ++i) {
    MPI_Cancel(&requests[i]);
    MPI_Wait(&requests[i], &stati[i]);

    MPI_Test_cancelled(&stati[i], &flag);
    if(flag)   // Cancel succeeded
      ++cancelNum;
  }


#if defined NF_DEBUG_MORE
  std::cout << "WORKER " << globalRank_ << " cancelled " << cancelNum 
	    << " MPI_Irecv" << std::endl;
#endif

  // Free memory
  if (buf != 0) { 
    delete [] buf; 
    buf = 0; 
  }
  if (bufs != 0) {
    for (i = 0; i < reqNum; ++i) 
      delete [] bufs[i];
    delete [] bufs;
    bufs = 0;
  }
  if (indices != 0) {
    delete [] indices;
    indices = 0;
  }
  if (requests != 0) {
    delete [] requests;
    requests = 0;
  }
  if (stati != 0) {
    delete [] stati;
    stati = 0;
  }

}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::askHubsShareWork(int donorID, 
					 int receiverID, 
					 double receiverWorkload, 
					 MPI_Comm comm)
{
  int smallSize = AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::smallSize);
  int pos       = 0;
  char* buf     = new char [smallSize];

  MPI_Pack(&receiverID, 1, MPI_INT, buf, smallSize, &pos, comm);
  MPI_Pack(&receiverWorkload, 1, MPI_DOUBLE, buf, smallSize, &pos, comm);
  //  MPI_Send(buf, pos, MPI_PACKED, donorID, AlpsMsgAskDonor, comm);
  //  MPI_Request req;
  // MPI_Status status;
  MPI_Send(buf, pos, MPI_PACKED, donorID, AlpsMsgAskHubShare, comm);

  //  MPI_Isend(buf, pos, MPI_PACKED, donorID, AlpsMsgAskHubShare, comm, &req);
  // MPI_Wait(&req, &status);
  delete [] buf; 
  buf = 0;
}

//#############################################################################

double 
AlpsKnowledgeBrokerMPI::calWorkload()
{
  const int size = AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::keyNodeNum);
  std::vector<AlpsTreeNode* > allNodes = 
    dynamic_cast<AlpsNodePool*>(getKnowledgePool(ALPS_NODE))->
    getCandidateList().getContainer();

  double load = 0.0;
  std::vector<AlpsTreeNode* >::iterator pos;

  if (size >= allNodes.size())
    load = std::for_each(allNodes.begin(), allNodes.end(), 
			 TotalWorkload(incumbentValue_)).result();
  else { 
    pos = allNodes.begin() + size;
    load = std::for_each(allNodes.begin(), pos, 
			 TotalWorkload(incumbentValue_)).result(); 
  }
  return load; 
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::donateWork(char* buf, 
				   MPI_Status* status,  
				   MPI_Comm comm)
{
  int size       = AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::smallSize);
  int pos        = 0;
  int receiverID = 0;
  char* dummyBuf = 0;
  double receiverWorkload = 0.0;

  MPI_Unpack(buf, size, &pos, &receiverID, 1, MPI_INT, comm);
  MPI_Unpack(buf, size, &pos, &receiverWorkload, 1, MPI_DOUBLE, comm);

  // Send node(s) to receiverID

  const int minNodeNum = 
    AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::minNodeNum);

  if (getNumKnowledges(ALPS_NODE) > minNodeNum) {// Avoid the case that it {
    sendNode(receiverID, comm);                 // only one node.
#if defined NF_DEBUG_MORE
    std::cout << "PROC " << globalRank_ 
	      << " : donate a node to PROC " <<  receiverID
	      << ", whose load = " << receiverWorkload 
	      << "tag = "<< status->MPI_TAG <<std::endl;
#endif
  }
  else {
    MPI_Send(dummyBuf, 0, MPI_PACKED, receiverID, AlpsMsgNode, comm);
#if defined NF_DEBUG
    std::cout << "PROC " << globalRank_ 
	      << " : don't have enought work to share with PROC "<<  receiverID
	      << ", whose load = " << receiverWorkload 
	      << "tag = "<< status->MPI_TAG <<std::endl;
#endif 
  }
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::hubAllocateDonation(char* buf, 
					    MPI_Status* status, 
					    MPI_Comm comm)
{
  int i;
  int minLoadRank = -1;
  int count       = 0;
  double minLoad  = DBL_MAX;

  const double zeroLoad = 
    AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::zeroLoad);

  // 1. Find the worker (exclude hub) with zero or smallest workload
  for (i = 1; i < clusterSize_; ++i) {
    if (workerLoads_[i] < minLoad) {
      minLoad = workerLoads_[i];
      minLoadRank = globalRank_ + i;
      if (minLoad < zeroLoad)
	break;
    }
  }

  // 2. Get the number of elements in buf
  MPI_Get_count(status, MPI_PACKED, &count);

//    if (count <= 0) {
//      std::cout << "PROC " << status->MPI_SOURCE << " : hub as me to donate,"
//  	      << " but I don't have enought load " << std::endl; 
//    }

  // 3. Forward the buf to the worker
  if (minLoadRank != -1) {
    MPI_Send(buf, count, MPI_PACKED, minLoadRank, AlpsMsgNode, comm);
  }
  else {
    std::cout << "minLoadRank == -1" << std::endl; 
    throw CoinError("minLoadRank == -1", 
		    "hubAllocateDonation", "AlpsKnowledgeBrokerMPI");
  }
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::hubBalanceWorkers(char* buf, 
					  MPI_Status* status, 
					  MPI_Comm comm)
{
  int i;
  int maxLoadRank = -1;            // Global rank
  double  maxLoad = AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::zeroLoad);

  int size        = AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::smallSize);
  int pos         = 0;
  char* buffer    = new char [size];
  int tempID      = -1;
  int tempSender;

  // 1. Indentify sender
  int sender = status->MPI_SOURCE;
  if (comm == MPI_COMM_WORLD)
    tempSender = sender % cluSize_;
  else if (comm == hubComm_)
    tempSender = sender;
  else {
    std::cout << "HUB " << globalRank_ 
	      <<" : Unkown Comm in hubBalanceWorkers" << std::endl;
    throw CoinError("Unkown Comm", "hubBalanceWorkers", 
		    "AlpsKnowledgeBrokerMPI");

  }

  // 2. Find the most loaded worker in this cluster
  for (i = 1; i < clusterSize_; ++i) {
    if (workerLoads_[i] > maxLoad) {
      maxLoad = workerLoads_[i];
      tempID = i;
      if (comm == MPI_COMM_WORLD)
	maxLoadRank = globalRank_ + i;
      else if (comm == hubComm_)
	maxLoadRank = i;
      else {
	std::cout << "HUB " << globalRank_ 
		  <<" : Unkown Comm in hubBalanceWorkers" << std::endl;
	throw CoinError("Unkown Comm", "hubBalanceWorkers", 
			"AlpsKnowledgeBrokerMPI");

      }
    }
  }

  // 3. Ask the most loaded worker share work with sender
  double temp = 0.0;
  if (maxLoadRank != -1 && tempID != tempSender) {
    MPI_Pack(&sender, 1, MPI_INT, buffer, size, &pos, comm);
    MPI_Pack(&temp, 1, MPI_DOUBLE, buffer, size, &pos, comm);
    MPI_Send(buffer, pos, MPI_PACKED, maxLoadRank, AlpsMsgAskDonate, comm);
  }
  else {
    std::cout << "HUB " << globalRank_ << " : I can't find a worker to share"
	      << " work with worker " << sender << std::endl;
    MPI_Send(buffer, 0, MPI_PACKED, sender, AlpsMsgNode, comm);
  }
 
  delete [] buffer;
  buffer = 0;
} 

//#############################################################################

void  
AlpsKnowledgeBrokerMPI::hubCheckOwnCluster(int tag, 
					   MPI_Comm comm)
{
  int i, receiver;
  char* buf         = 0;
   
  // Send status check signal to workers
  for(i = 0; i < clusterSize_; ++i) {

    if (comm = MPI_COMM_WORLD)
      receiver = globalRank_ + i;
    else if (comm = clusterComm_)
      receiver = i;
    else {
      std::cout << "HUB " << globalRank_ 
		<<" : Unkown Comm in hubCheckOwnCluster" << std::endl;
      throw CoinError("Unkown Comm", "hubCheckOwnCluster", 
		      "AlpsKnowledgeBrokerMPI");
      
    }
    
    if (i != clusterRank_)                // NOT the hub
      MPI_Send(buf, 0, MPI_PACKED, receiver, tag, comm);
  }

}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::hubReportStatus(int tag, 
					MPI_Comm comm)
{
  int size   = AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::smallSize);
  int pos    = 0;
  char* buf  = new char [size];

  int receiver;

  if (comm == MPI_COMM_WORLD)
    receiver = masterRank_;
  else if (comm == hubComm_)
    receiver = 0;
  else {
    std::cout << "HUB " << globalRank_ 
	      <<" : Unkown Comm in hubReportStatus" << std::endl;
    throw CoinError("Unkown Comm", "hubReportStatus", 
		    "AlpsKnowledgeBrokerMPI");
  }

  clusterWorkload_ = 0.0;
  workerLoads_[0] = workload_;
  for (int i = 0; i < clusterSize_; ++i)
    clusterWorkload_ += workerLoads_[i]; 

#if defined NF_DEBUG_MORE
  std::cout << "HUB " << globalRank_ 
	    << " : report load = " << clusterWorkload_ << " to Master."
	    << " Tag = " << tag  << std::endl;
#endif

  MPI_Pack(&clusterWorkload_, 1, MPI_DOUBLE, buf, size, &pos, comm);
  MPI_Pack(&clusterSendCount_, 1, MPI_INT, buf, size, &pos, comm);
  MPI_Pack(&clusterRecvCount_, 1, MPI_INT, buf, size, &pos, comm);
  MPI_Send(buf, pos, MPI_PACKED, masterRank_, tag, comm);

  clusterSendCount_ = clusterRecvCount_ = 0;  // Only count new msg
  
  delete [] buf; 
  buf = 0;
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::hubUpdateCluStatus(char* buf, 
					   MPI_Status* status, 
					   MPI_Comm comm)
{
  int msgSendNum, msgRecvNum;
  int position = 0;
  int size     = AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::smallSize);

  int sender;

  if (comm == MPI_COMM_WORLD)
    sender   = (status->MPI_SOURCE) % cluSize_;
  else if (comm == clusterComm_)
    sender   = status->MPI_SOURCE;
  else {
    std::cout << "unknown sender in hubUpdateCluStatus()" 
	      << std::endl;
    throw CoinError("unknown sender", 
		    "hubUpdateCluStatus()", "AlpsKnowledgeBrokerMPI");
  }

  if (workerReported_[sender] == false)
    workerReported_[sender] = true;

  double preLoad = workerLoads_[sender];
  double curLoad;

  MPI_Unpack(buf, size, &position, &curLoad, 1, MPI_DOUBLE, comm);
  MPI_Unpack(buf, size, &position, &msgSendNum, 1, MPI_INT, comm);
  MPI_Unpack(buf, size, &position, &msgRecvNum, 1, MPI_INT, comm);
  
  workerLoads_[sender] = curLoad;

  clusterWorkload_  -= preLoad;
  clusterWorkload_  += curLoad;
  clusterSendCount_ += msgSendNum;
  clusterRecvCount_ += msgRecvNum;

#if defined NF_DEBUG_MORE
  std::cout << "HUB "<< globalRank_ <<" : curLoad = " << curLoad 
	    << " preLoad = " << preLoad << ", sender = " << sender
	    << ", clusterWorkload_  = " << clusterWorkload_
	    << ", clusterSendCount_ = " << clusterSendCount_
	    << ", clusterRecvCount_ = " << clusterRecvCount_
	    << ",  cluSize_ = " << cluSize_
	    << ", status->MPI_SOURCE = " << status->MPI_SOURCE
	    << std::endl; 
#endif

}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::hubsShareWork(char* buf, 
				      MPI_Status* status, 
				      MPI_Comm comm)
{
  int i;
  int size        = AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::smallSize);
  int pos         = 0;
  int receiverID  = 0;
  int maxLoadRank = -1;            // Global rank
  double maxLoad  = 0.0;

  double receiverWorkload = 0.0;
  char* buffer            = new char [size];

  // 1. Indentify the receiver and its current workload
  MPI_Unpack(buf, size, &pos, &receiverID, 1, MPI_INT, comm);
  MPI_Unpack(buf, size, &pos, &receiverWorkload, 1, MPI_DOUBLE, comm);

  // 2. Find the donor worker in hub's cluster
  for (i = 1; i < clusterSize_; ++i) {
    if (workerLoads_[i] > maxLoad) {
      maxLoad = workerLoads_[i];
      maxLoadRank = globalRank_ + i;
    }
  }

  // FIXME: how many nodes? Righ now just 1. 
  // 3. Ask the donor worker to share work (send node to receiving hub)
  if (maxLoadRank != -1) {
    pos = 0;                   // Reset position to pack
    MPI_Pack(&receiverID, 1, MPI_INT, buffer, size, &pos, comm);
    MPI_Pack(&receiverWorkload, 1, MPI_DOUBLE, buffer, size, &pos, comm);
    ++sendCount_;
    MPI_Send(buffer, pos, MPI_PACKED, maxLoadRank, AlpsMsgAskDonateToHub, 
	     comm);
  }
  else {
    std::cout << "Hub " << globalRank_ << " : sorry, I can not find "
	      << "a worker to donate node to PROC " << receiverID << std::endl;

  }
}

//#############################################################################

void
AlpsKnowledgeBrokerMPI::masterBalanceHubs(MPI_Comm comm)
{
  int i;
  int size     = AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::smallSize);
  char* buffer = new char [size];

  std::vector<std::pair<double, int> > loadIDVector;
  loadIDVector.reserve(hubNum_);
  std::map<double, int, std::greater<double> > donors;         
  std::map<double, int> receivers; 

  const double donorSh      = AlpsDataPool::getOwnParams()->
    entry(AlpsOwnParams::donorThreshold);
  const double receiverSh   = AlpsDataPool::getOwnParams()->
    entry(AlpsOwnParams::receiverThreshold);
  const double zeroLoad     = 
    AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::zeroLoad);

  // Indentify donors and receivers
  const double averLoad     = systemWorkload_ / hubNum_;
  if (averLoad < zeroLoad) {
    std::cout << "averLoad = 0, masterBalanceHub() do nothing." 
	      << std::endl;
    return;
//      throw CoinError("averLoad = 0", 
//  		    "masterBalanceHub()", "AlpsKnowledgeBrokerMPI");
  } 
  for (i = 0; i < hubNum_; ++i) {
    if (hubLoads_[i]/averLoad > donorSh)
      donors.insert(std::pair<double, int>(hubLoads_[i], hubRanks_[i]));
    else if (hubLoads_[i]/averLoad < receiverSh)
      receivers.insert(std::pair<double, int>(hubLoads_[i], hubRanks_[i]));
  }
  
  // Instruct donor hubs to send nodes to receiver hubs 
  const int numDonor    = donors.size();
  const int numReceiver = receivers.size();
  const int numPair     = CoinMin(numDonor, numReceiver);
  int donorID;
  int receiverID;
 
  std::map<double, int, std::greater<double> >::iterator posD = donors.begin();
  std::map<double, int>::iterator posR = receivers.begin();

  for(i = 0; i < numPair; ++i) {
    donorID = posD->second;
    receiverID = posR->second;
   
#if defined NF_DEBUG_MORE
    std::cout << "Master : receiverID = " << receiverID << std::endl;
    std::cout << "Master : donorID = " << donorID << std::endl;
#endif
    
    if (CoinAbs(receiverID) >= processNum_ || 
	CoinAbs(donorID) >= processNum_) {
      std::cout << "Master : receiverID = " << receiverID << std::endl;
      std::cout << "Master : donorID = " << donorID << std::endl;
      throw CoinError("receiverID >= processNum_ || donorID >= processNum_", 
		      "masterBalanceHubs", "AlpsKnowledgeBrokerMPI");
    }

    if (donorID != masterRank_) {
      ++sendCount_;
      askHubsShareWork(donorID, receiverID, posR->first, comm);
    }
    else {
      double maxLoad = 0.0;
      int maxLoadRank = -1;
      int pos = 0;
      double recvLoad = posR->first;

      // Find the donor worker in hub's cluster
      for (i = 1; i < clusterSize_; ++i) {
	if (workerLoads_[i] > maxLoad) {
	  maxLoad = workerLoads_[i];
	  maxLoadRank = globalRank_ + i;
	}
      }
      
      // FIXME: how many nodes? Righ now just 1. 
      // Ask the donor worker to share work (send node to receiving hub)
      if (maxLoadRank != -1) {
	if (buffer != 0) {
	  delete [] buffer; buffer = 0;
	}
	buffer = new char [size]; 

	MPI_Pack(&receiverID, 1, MPI_INT, buffer, size, &pos, comm);
	MPI_Pack(&recvLoad, 1, MPI_DOUBLE, buffer, size, &pos, comm);
	++sendCount_;
	MPI_Send(buffer, pos, MPI_PACKED, maxLoadRank, AlpsMsgAskDonateToHub, 
		 comm);
      }      
    }
    ++posD;
    ++posR;
  }

  if (buffer != 0) {
    delete [] buffer; buffer = 0;
  }
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::masterUpdateSysStatus(char* buf, 
					      MPI_Status* status, 
					      MPI_Comm comm)
{
  int position = 0;
  int msgSendNum, msgRecvNum;
  int size   = AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::smallSize);

  int sender;
  if (comm == MPI_COMM_WORLD)
    sender   = (status->MPI_SOURCE) / cluSize_;     // Be careful with / or %
  else if (comm == hubComm_)
    sender   = status->MPI_SOURCE;
  else {
    std::cout << "unknown COMM in masterUpdateSysStatus" 
	      << std::endl;
    throw CoinError("unknown sender", 
		    "masterUpdateSysStatus", "AlpsKnowledgeBrokerMPI");
  }

  double preLoad = hubLoads_[sender];
  double curLoad;

  MPI_Unpack(buf, size, &position, &curLoad, 1, MPI_DOUBLE, comm);
  MPI_Unpack(buf, size, &position, &msgSendNum, 1, MPI_INT, comm);
  MPI_Unpack(buf, size, &position, &msgRecvNum, 1, MPI_INT, comm);
  
  // Update the hub's status
  hubLoads_[sender] = curLoad;

  // Update system status
  systemSendCount_ += msgSendNum;
  systemRecvCount_ += msgRecvNum;
  systemWorkload_ -= preLoad;
  systemWorkload_ += curLoad;

  if ( hubReported_[sender] != true )
    hubReported_[sender] = true;

#if defined NF_DEBUG_MORE
  std::cout << "MASTER "<< globalRank_ <<" UpdateSystem : curLoad " << curLoad 
	    << " preLoad = " << preLoad 
	    << ", hubReported_[" << sender << "] = "
	    << hubReported_[sender]
	    << ", systemSendCount_ = " << systemSendCount_
	    << ", systemRecvCount_ = " << systemRecvCount_
	    << ", systemWorkload_ = " << systemWorkload_
	    << std::endl; 
#endif

}

//#############################################################################

void
AlpsKnowledgeBrokerMPI::refreshSysStatus()
{
  // Get the load of master's cluster
  int i;
  clusterWorkload_ = 0.0;
  workerLoads_[0] = workload_;
  for (i = 0; i < clusterSize_; ++i)
    clusterWorkload_ += workerLoads_[i]; 
 
  hubLoads_[0] = clusterWorkload_;
  
  // Cal total system load
  systemWorkload_ = 0.0;
  for (i = 0; i < hubNum_; ++i) 
    systemWorkload_ += hubLoads_[i];

  // In status of Master/hub cluster into system msg send and recv
  systemSendCount_ += sendCount_; 
  systemRecvCount_ += recvCount_; 
  sendCount_ = recvCount_ = 0;

  systemSendCount_ += clusterSendCount_;
  systemRecvCount_ += clusterRecvCount_;
  clusterSendCount_ = clusterRecvCount_ = 0;

  if ( allHubReported_ != true) {
    allHubReported_ = true;
    hubReported_[0] = true;
    for (i = 0; i < hubNum_; ++i) {
      if ( hubReported_[i] != true ) {
	allHubReported_ =  false;
	break;
      }
    }
  }

}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::unpackSetIncumbent(char* buf, 
					   MPI_Status* status, 
					   MPI_Comm comm)
{

  int size     = AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::smallSize);
  int position = 0;
  double incVal= 0.0;
  int incID    = 0;
 
  // Unpack incumbent value from buf
  MPI_Unpack(buf, size, &position, &incVal, 1, MPI_DOUBLE, comm);
  MPI_Unpack(buf, size, &position, &incID, 1, MPI_INT, comm);

  if (incID == globalRank_) { 

#if defined NF_DEBUG_MORE
    std::cout << "PROC " << globalRank_ 
	      << " : unpack and ingore incVal = " << incVal << " at PROC" 
	      << incID << ", since I am " << incID
	      << std::endl;
#endif

    return;  // Do nothing
  }

#if defined NF_DEBUG
  std::cout << "PROC " << globalRank_ 
	    << " : unpack incVal = " << incVal << " at PROC " << incID 
	    << std::endl;
#endif

  // Assume minimization
  if ( incVal <= (incumbentValue_ - TOLERANCE_) ) {                     // <
      incumbentValue_ = incVal;
      incumbentID_ = incID;
      updateIncumbent_ = true;      // The incumbent value is updated. 
  }
  else if( incVal > (incumbentValue_ - TOLERANCE_) &&  
	   incVal < (incumbentValue_ + TOLERANCE_) ) {    // ==
	   
    if (incID < incumbentID_) {     // So that all process consistant 
      incumbentValue_ = incVal;
      incumbentID_ = incID;
      updateIncumbent_ = true; 
    }
    else{
#if defined NF_DEBUG_MORE
      std::cout << "PROC " << globalRank_ 
		<< " : unpacked but discard incVal = " << incVal  
		<< " at PROC " << incID << std::endl;
#endif  
    } 
  }
  else{                                               // >
#if defined NF_DEBUG_MORE
    std::cout << "PROC " << globalRank_ 
	      << " : unpacked but discard incVal = " << incVal  
	      << " at PROC " << incID << std::endl;
#endif  
  } 
  
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::workerReportStatus(int tag, 
					   MPI_Comm comm)
{
  int size   = AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::smallSize);
  int pos    = 0;
  char* buf  = new char [size];
  int receiver;

  if (comm == MPI_COMM_WORLD)
    receiver = myHubRank_;
  else if (comm == clusterComm_)
    receiver = 0;
  else {
    std::cout << "WORKER " << globalRank_ 
	      <<" : Unkown Comm in workerReportStatus" << std::endl;
    throw CoinError("Unkown Comm", "workerReportStatus", 
		    "AlpsKnowledgeBrokerMPI");
  }

  workload_ = calWorkload();
  //workload_ = getNumKnowledges(ALPS_NODE);
  MPI_Pack(&workload_, 1, MPI_DOUBLE, buf, size, &pos, comm);
  MPI_Pack(&sendCount_, 1, MPI_INT, buf, size, &pos, comm);
  MPI_Pack(&recvCount_, 1, MPI_INT, buf, size, &pos, comm);
  sendCount_ = recvCount_ = 0;      // Only count new message

  MPI_Send(buf, pos, MPI_PACKED, receiver, tag, comm);
  
#if defined NF_DEBUG_MORE
  std::cout << "WORKER " << globalRank_ 
	    << " : report load = " << workload_ << " to its hub " 
	    << myHubRank_ << "; Tag = " << tag << std::endl;
#endif
  
  delete [] buf; 
  buf = 0;
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::broadcastModel(const int id, 
				       const int source, 
				       MPI_Comm comm)
{
  char* buf = 0;
  int size = -1;
  int position = -1;

  if (id == source) {
    // Pack model 
    // pack( ( (AlpsDataPool::ADPool())->getKnowledge(ALPS_MODEL))->encode() );
    packEncoded((AlpsDataPool::getModel())->encode(), buf, size, position);
  }

  MPI_Bcast(&size, 1, MPI_INT, source, comm);  // Broadcast buf size first

  if (id != source) {
    if (size <= 0)
      throw CoinError("Msg size <= 0", "broadcastModel", 
		      "AlpsKnowledgeBrokerMPI");
    if( buf != 0 ) {
      delete [] buf;  buf = 0;
    }
    buf = new char[size];
  }
  std::cout << "WORKER: here" << std::endl;
  MPI_Bcast(buf, size, MPI_CHAR, source, comm); // Broadcast buf_
  
  if (id != source) {
    
    std::cout << "WORKER: start to unpack model." << std::endl; 
    AlpsEncoded* encodedModel = unpackEncoded(buf);

#if defined NF_DEBUG
    std::cout << "WORKER: start to decode model." << std::endl; 
    std::cout << encodedModel->type() << std::endl;
#endif

    //    AlpsModel* model = dynamic_cast<AlpsModel* >( AlpsKnowledge::
    //	    decoderObject(encodedModel->type())->decode(*encodedModel) );

    AlpsModel* model = dynamic_cast<AlpsModel* >
      ( decoderObject(encodedModel->type())->decode(*encodedModel) );

#if defined NF_DEBUG
    std::cout << "WORDER: finish decoding model." << std::endl;
#endif

    //AlpsDataPool::ADPool()->updateKnowledgeValue(ALPS_MODEL, model);
    AlpsDataPool::setModel(model);
#if defined NF_DEBUG_MORE
    std::cout << "WORKER: finish adding model." << std::endl;
#endif
  }
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::broadcastParams(const int id, 
					const int source, 
					MPI_Comm comm)
{
  char* buf   = 0;
  int size    = 0;
  int repSize = 0;

  //---------------------------------------------------------------------------
  // 1. Broadcast Alps parameters
  if (id == source) {
    AlpsEncoded* enc   =  new AlpsEncoded;
    AlpsDataPool::getOwnParams()->pack(*enc); // enc's rep is filled.

    repSize  = enc->size();
    size     = repSize + 1;             // 1 for '\0'

    if( buf != 0 ) {
      delete [] buf;  buf = 0;
    } 
    buf = new char[size];

    memcpy(buf, enc->representation(), repSize);  
  }

  MPI_Bcast(&size, 1, MPI_INT, source, comm);

  if (id != source) {
    if (size <= 0)
      throw CoinError("Msg size <= 0", "broadcastParams", 
		      "AlpsKnowledgeBrokerMPI");
    if( buf != 0 ) {
      delete [] buf;  buf = 0;
    }
    buf = new char[size];
  }
  
  MPI_Bcast(buf, size, MPI_CHAR, source, comm);   // Broadcast buf

#if defined NF_DEBUG_MORE
   std::cout << "PROC " << id <<" : Alps param buf size = " << size 
	     << std::endl;
#endif

  if (id != source) {
    //    AlpsEncoded* enc2   = new AlpsEncoded(buf);
    AlpsEncoded* enc2   = new AlpsEncoded;
    enc2->setRepresentation(buf);

    AlpsDataPool::getOwnParams()->unpack(*enc2);        // Fill in parameters

#if defined NF_DEBUG_MORE  
    std::cout << "PROC " << id <<" : Received Alps params."<< std::endl; 
#endif
  }

  //---------------------------------------------------------------------------
  // 2. Broadcast user parameters
  if (id == source) {
    AlpsEncoded* enc1   =  new AlpsEncoded;
    AlpsDataPool::getAppParams()->pack(*enc1); // enc's rep is filled.

    repSize  = enc1->size();
    size     = repSize + 1;              // 1 for '\0'

    if( buf != 0 ) {
      delete [] buf;  buf = 0;
    } 
    buf = new char[size];

    memcpy(buf, enc1->representation(), repSize);  
  }

  MPI_Bcast(&size, 1, MPI_INT, source, comm);

#if defined NF_DEBUG_MORE  
  std::cout << "PROC " << id <<" : size = "<< size << std::endl; 
#endif

  if (id != source) {
    if (size < 0)
      throw CoinError("Msg size < 0", "broadcastParams", 
		      "AlpsKnowledgeBrokerMPI");
    if( buf != 0 ) {
      delete [] buf;  buf = 0;
    }
    buf = new char[size];
  }

  MPI_Bcast(buf, size, MPI_CHAR, source, comm);   // Broadcast buf
#if defined NF_DEBUG_MORE
   std::cout << "PROC " << id <<" : buf = " << buf << std::endl;
#endif

  if (id != source) {

    //   AlpsEncoded* enc3   = new AlpsEncoded(buf);
    AlpsEncoded* enc3   = new AlpsEncoded;
    enc3->setRepresentation(buf);

    (AlpsDataPool::getAppParams())->unpack(*enc3);    // Fill in parameters
  

#if defined NF_DEBUG_MORE  
    std::cout << "PROC " << id <<" : Received user params."<< std::endl;
#endif

  }

  if( buf != 0 ) {
    delete [] buf;  buf = 0;
  }

}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::broadcastIncumbent(MPI_Comm comm)
{
  int i         = -1;
  //  const int pn  = AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::processNum);
  int size      = AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::smallSize);
  int position  = 0;
  char* buf     = new char [size];

  MPI_Pack(&incumbentValue_, 1, MPI_DOUBLE, buf, size, &position, comm);
  MPI_Pack(&incumbentID_, 1, MPI_INT, buf, size, &position, comm);

  for (i = 0; i < processNum_; ++i) {     

    if (i != globalRank_) {               // Send to other processes
#if defined NF_DEBUG_MORE  
      std::cout << "PROC " <<  globalRank_
		<<" : broadcast a solution,  value = " 
		<< incumbentValue_ << " to "<< i << std::endl; 
#endif
      MPI_Send(buf, position, MPI_PACKED, i, AlpsMsgIncumbent, comm);
        ++sendCount_;
//        std::cout << "++send" << std::endl;
    }
  }
  

  if(buf != 0) {
    delete [] buf;     buf = 0;
  }
}

//#############################################################################

void
AlpsKnowledgeBrokerMPI::collectBestSolution(int destination, 
					    MPI_Comm comm)
{
  MPI_Status status;
  int sender = incumbentID_;

  if ( sender == destination ) {
    // DO NOTHING since the best solution is in the solution of its pool
  }
  else {
    if (globalRank_ == sender) {                 // Send solu
      char* senderBuf = 0;  // If not intialize to NULL, cause packEncoded(..)
      int size = -1;        // crash. 
      int position = -1;

      const AlpsSolution* solu = static_cast<const AlpsSolution* >
	(getBestKnowledge(ALPS_SOLUTION).first);
  
      double value = getBestKnowledge(ALPS_SOLUTION).second;
      AlpsEncoded* enc = solu->encode();
      packEncoded(enc, senderBuf, size, position);
         
      std::cout << "Process "<< sender <<" start to send incumbent to " 
		<< "Process "<< destination << std::endl;
      sendSizeBuf(senderBuf, size, position, destination, 
		  AlpsMsgIncumbent, comm);
      //      sendBuf(senderBuf, size, position, destination, 
      //	  AlpsMsgIncumbent, comm);
      MPI_Send(&value, 1, MPI_DOUBLE, destination, AlpsMsgIncumbent, comm);
      
      if(senderBuf != 0) {
	delete [] senderBuf;     senderBuf = 0;
      }
    }
    else if (globalRank_ == destination) {            // Recv solu
      double value = 0.0;
      // int ls = AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::largeSize);
      char* destBuf = 0;  //new char [ls];

      std::cout << "Process "<< destination <<" start to recv incumbent from" 
		<< "Process "<< sender << std::endl;
      receiveSizeBuf(destBuf, sender, AlpsMsgIncumbent, comm, &status);
      // receiveBuf(destBuf, sender, AlpsMsgIncumbent, comm, &status);
      

      MPI_Recv(&value, 1, MPI_DOUBLE, sender, AlpsMsgIncumbent, comm, &status);
      AlpsEncoded* encodedSolu = unpackEncoded(destBuf);

      //AlpsSolution* bestSolu = dynamic_cast<AlpsSolution* >( AlpsKnowledge::
      //	 decoderObject(encodedSolu->type())->decode(*encodedSolu) );
      AlpsSolution* bestSolu = dynamic_cast<AlpsSolution* >
	( decoderObject(encodedSolu->type())->decode(*encodedSolu) );

      addKnowledge(ALPS_SOLUTION, bestSolu, value);
      if(destBuf != 0) {
	delete [] destBuf;     destBuf = 0;
      }
    }
  }
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::packEncoded(AlpsEncoded* enc, 
				    char*& buf,
				    int& size, 
				    int& position) 
{
  const int bufSpare = AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::bufSpare);
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
AlpsKnowledgeBrokerMPI::unpackEncoded(char*& buf) 
{
  int position = 0;

  int typeSize;
  int repSize; 
  int size = AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::largeSize);

  // Unpack typeSize, repSize, type and rep from buf_
  MPI_Unpack(buf, size, &position, &typeSize, 1, MPI_INT, MPI_COMM_WORLD);

#if defined NF_DEBUG
  std::cout << "PROC "<< globalRank_ <<" : typeSize is " 
	    << typeSize << ";\tsize = "<< size <<  std::endl;
#endif

  MPI_Unpack(buf, size, &position, &repSize, 1, MPI_INT, MPI_COMM_WORLD);
  
  char* type = new char [typeSize+1]; // At least larger than one
  char* rep = new char[repSize+1];

  MPI_Unpack(buf, size, &position, type, typeSize, MPI_CHAR, 
	     MPI_COMM_WORLD);
  // MUST terminate cstring so that don't read trash
  *(type+typeSize) = '\0';        
  
  MPI_Unpack(buf, size, &position, rep, repSize, MPI_CHAR, MPI_COMM_WORLD);
  rep[repSize] = '\0';

#if defined NF_DEBUG_MORE
  std::cout << "PROC "<< globalRank_ << ": type = " << type 
	    << "; repSize = " << repSize << std::endl;
#endif

  // Return a enc of required knowledge type
  return new AlpsEncoded(type, repSize, rep );
}

//#############################################################################

void
AlpsKnowledgeBrokerMPI::receiveBuf(char*& buf, 
				   int sender, 
				   int tag, 
				   MPI_Comm comm, 
				   MPI_Status* status) 
{
  const int size = AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::largeSize);
  if (buf != 0) {
    delete [] buf;  buf = 0;
  }
  buf = new char [size];

  MPI_Recv(buf, size, MPI_PACKED, sender, tag, comm, status);
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
  // First recv the buf size
  MPI_Recv(&size, 1, MPI_INT, sender, MPI_ANY_TAG, comm, status);
  if (status->MPI_TAG == AlpsMsgFinishInit)
    return;
  
  if (size < 0)
    throw CoinError("size < 0", "receiveSizeBuf", "AlpsKnowledgeBrokerMPI");

  // Secondly, allocate memory for buf_
  if (buf != 0) {
    delete [] buf;  buf = 0;
  }
  buf = new char [size];

  // Third, receive MPI packed data and put in buf_
  MPI_Recv(buf, size, MPI_PACKED, sender, tag, comm, status);
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::receiveNode(char*& buf, 
				    int sender,
				    MPI_Comm comm, 
				    MPI_Status* status) 
{
  int count;
  int i = 0;

  MPI_Get_count(status, MPI_PACKED, &count);

  if (count <= 0) {
    std::cout << "PROC " << globalRank_ 
	      << " : ask for a node but receive nothing from "
	      << status->MPI_SOURCE << std::endl;
    return;
  }

  if (status->MPI_TAG == AlpsMsgFinishInit ) {
    // DO NOTHING
  }
  else if (status->MPI_TAG == AlpsMsgNode) {
    AlpsEncoded* encodedNode = unpackEncoded(buf);
    //    AlpsTreeNode* node = static_cast<AlpsTreeNode* >( AlpsKnowledge::
    //	decoderObject(encodedNode->type())->decode(*encodedNode) );
    AlpsTreeNode* node = static_cast<AlpsTreeNode* >
      ( decoderObject(encodedNode->type())->decode(*encodedNode) );

#if defined NF_DEBUG_MORE  
    std::cout << "WORKER " << globalRank_ << " : received a node from " 
	      << sender << std::endl; 
#endif

    // Add node to the local node pool
    node->setKnowledgeBroker(this); 
    node->setPriority(0);             // FIXME
    node->setLevel(0);                // FIXME
    i = subtree_->getNextIndex(); // FIXME
    node->setIndex(i);
    subtree_->setNextIndex(i+1);      // One more than root's index
    addKnowledge(ALPS_NODE, node);
  }  
  else {
    std::cout << "PROC " << globalRank_ 
	      <<" : recved UNKNOWN message, tag = " << status->MPI_TAG 
	      << ", source = " << status->MPI_SOURCE <<std::endl; 
    throw CoinError("Unknow message type", 
		    "workmain", "AlpsKnowledgeBrokerMPI");
  }
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::receiveSizeNode(int sender,
					MPI_Comm comm, 
					MPI_Status* status) 
{
  int i = 0;
  char* buf = 0;

  receiveSizeBuf(buf, sender, AlpsMsgNode, comm, status);
 
  if (status->MPI_TAG == AlpsMsgFinishInit ) {
    //std::cout << "WORKER: rec AlpsMsgFinishInit... STOP INIT." << std::endl;
  } 
  else if (status->MPI_TAG == AlpsMsgNode) {
    AlpsEncoded* encodedNode = unpackEncoded(buf);
 
#if defined NF_DEBUG_MORE 
    std::cout << "WORKER: received and unpacked a node." << std::endl; 
    std::cout << "WORKER: type() is " << encodedNode->type() << std::endl;
    std::cout << "WORKER: finish unpacking node." << std::endl; 
    std::cout << "WORKER: start to decode node." << std::endl; 

    //    const AlpsTreeNode* bugNode = dynamic_cast<const AlpsTreeNode* >(
    //	  AlpsKnowledge::decoderObject(encodedNode->type()));
    const AlpsTreeNode* bugNode = dynamic_cast<const AlpsTreeNode* >
      ( decoderObject(encodedNode->type()) );

    std::cout <<"WORKER: bugNode's Priority = " 
	      << bugNode->getPriority() << std::endl;
    std::cout << "WORKER: finish just decoding node." << std::endl; 
#endif

    //    AlpsTreeNode* node = dynamic_cast<AlpsTreeNode* >( AlpsKnowledge::
    //	decoderObject(encodedNode->type())->decode(*encodedNode) );
    AlpsTreeNode* node = dynamic_cast<AlpsTreeNode* >
      ( decoderObject(encodedNode->type())->decode(*encodedNode) );

#if defined NF_DEBUG 
    std::cout << "WORKER " << globalRank_ << " : received a node from " 
	      << sender << std::endl; 
#endif
    // Add node to the local node pool
    node->setKnowledgeBroker(this); 
    node->setPriority(0);             // FIXME
    node->setLevel(0);                // FIXME
    i = subtree_->getNextIndex();     // FIXME
    node->setIndex(i);
    subtree_->setNextIndex(i+1);      // One more than root's index
    addKnowledge(ALPS_NODE, node);
  }  
  else {
    std::cout << "PROC " << globalRank_ 
	      <<" : recved UNKNOWN message: tag = " << status->MPI_TAG 
	      << "; source = " << status->MPI_SOURCE <<std::endl;
    throw CoinError("Unknow message type", 
		    "workmain", "AlpsKnowledgeBrokerMPI");
  }

  if (buf != 0) {
    delete [] buf; buf = 0;
  }

}


//#############################################################################

void 
AlpsKnowledgeBrokerMPI::sendBuf(char* buf,
				int size,
				int position,
				const int target, 
				const int tag, 
				const MPI_Comm comm) 
{
  // If packing successfully, send buf to target
  if (size > 0)
    MPI_Send(buf, position, MPI_PACKED, target, tag, comm); 
  else
    throw CoinError("Msg size is <= 0", "send", "AlpsKnowledgeBrokerMPI");
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::sendSizeBuf(char* buf, 
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

void 
AlpsKnowledgeBrokerMPI::sendNode(const int target, 
				 MPI_Comm comm) 
{
  char* buf = 0;
  int size = -1;
  int position = -1;

  const AlpsTreeNode* node = static_cast<const AlpsTreeNode* >
    (getKnowledge(ALPS_NODE).first);
  AlpsEncoded* enc = node->encode();
  popKnowledge(ALPS_NODE);
  packEncoded(enc, buf, size, position);
  sendBuf(buf, size, position, target, AlpsMsgNode, comm);

  if (buf != 0) {
    delete [] buf; buf = 0;
  }

#if defined NF_DEBUG_MORE  
  std::cout << "WORKER : "<< globalRank_ 
	    << " : donor node to PROC " << target
	    << "; buf_ pos = " << position_ 
	    << "; buf_ size = " << size_ <<  std::endl; 
#endif
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::sendSizeNode(const int target, 
				     MPI_Comm comm) 
{
  char* buf = 0;
  int size = -1;
  int position = -1;

  const AlpsTreeNode* node = static_cast<const AlpsTreeNode* >
    (getKnowledge(ALPS_NODE).first);
  AlpsEncoded* enc = node->encode();
  popKnowledge(ALPS_NODE);                // Remove the sent node
  delete node;                            // Send to other process
  packEncoded(enc, buf, size, position);
  sendSizeBuf(buf, size, position, target, AlpsMsgNode, comm);

  if (buf != 0){
    delete [] buf; buf = 0;
  }
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

void 
AlpsKnowledgeBrokerMPI::initializeSolver(int argc, 
					 char* argv[], 
					 AlpsModel& model, 
					 AlpsParameterSet& userParams) 
{
 
  // 1. init msg env
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &globalRank_);
  MPI_Comm_size(MPI_COMM_WORLD, &processNum_);

  masterRank_ = 0;                               // MASTER RANK PREDEFINED
 
  int color, i;
  int key   = globalRank_;
  int count = 0;
    
  // 2. Read in parameters and model data
  if (globalRank_ == masterRank_) {
    std::cout << processNum_ << " processes have been launched. " 
	      << std::endl; 
    std::cout << "\nALPS Version 0.71 \n\n";
    
    // Read in params
    AlpsDataPool::getOwnParams()->readFromFile(argv[1]);
    userParams.readFromFile(argv[1]);
    
    // Read in model data if NEEDED
    if (AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::inputFromFile) == true) {
      const char* dataFile = 
	AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::dataFile).c_str();
      std::cout << "DATA FILE = " << dataFile << std::endl;
      model.readData(dataFile);
    }

#if defined NF_DEBUG_MORE
    std::cout << "MASTER: num of process = " 
	      << AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::processNum)
	      << std::endl;
#endif

  }
  
  // 3. Set up user params and model
  MPI_Barrier(MPI_COMM_WORLD);
  AlpsDataPool::setAppParams(&userParams);

  // Put model* in AlpsDataPool. MUST do it before registering user node class
  // otherwise will seg
  AlpsDataPool::setModel(&model);  
                                 
  
  // 4. Broadcast parameters.
  broadcastParams(globalRank_, masterRank_, MPI_COMM_WORLD);
  MPI_Barrier(MPI_COMM_WORLD);

#if defined NF_DEBUG_MORE
  std::cout << "PROC " << globalRank_ << " : num of process = " 
	    << AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::processNum)
	    << std::endl;
#endif
  
  hubNum_ = AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::hubNum);
  
#if defined NF_DEBUG_MORE  
  std::cout << "PROC " << globalRank_ << " : hubNum = " << hubNum_ 
	    << std::endl; 
#endif
  

  // 5. Create MPI communicators and groups
  // Deside the cluster size and actual hubNum_
  
  while(true) {
    if (hubNum_ > 0) {
      cluSize_ = 1;
      while (cluSize_ * hubNum_ < processNum_)
	++cluSize_;
      color = globalRank_ / cluSize_;       // [0,...,cluSize-1] in group 1
    }
    else {
      std::cout << "hubNum_ <= 0" << std::endl;
      throw CoinError("hubNum_ <= 0", "initSolver", "AlpsKnowledgeBrokerMPI");
    }

    if (processNum_- cluSize_ * (hubNum_ - 1) > 1) //more than 1 proc at last P
      break;
    else 
      --hubNum_;
  }

  // Create clusterComm_
  MPI_Comm_split(MPI_COMM_WORLD, color, key, &clusterComm_);
  MPI_Comm_rank(clusterComm_, &clusterRank_);
  MPI_Comm_size(clusterComm_, &clusterSize_);
  
  // Create hubGroup_ and hubComm_
  hubRanks_ = new int [hubNum_];
  int k = 0;
  for (i = 0; i < processNum_; i += cluSize_)
    hubRanks_[k++] = i;
  int* t1 = hubRanks_;
  
  
  MPI_Group worldGroup;
  MPI_Comm_group(MPI_COMM_WORLD, &worldGroup);
  MPI_Group_incl(worldGroup, hubNum_, hubRanks_, &hubGroup_);
  MPI_Comm_create(MPI_COMM_WORLD, hubGroup_, &hubComm_);
    
  // 6. Classify process types, set up subtree and pools
  AlpSubTree* st = 0;
  if (globalRank_ == 0) {
    processType_ = AlpsProcessTypeMaster;
    st = new AlpsSubTreeMaster; // Broker will delete momery
  }
  else if (globalRank_ % cluSize_ == 0) {
    processType_ = AlpsProcessTypeHub;
    st = new AlpsSubTreeHub;  
  }
  else {
    processType_ = AlpsProcessTypeWorker;
    st = new AlpSubTree;
  }
  setSubTree(st);
  setupKnowledgePools();


  // 7. Set hub's global rank for workers
  if (processType_ == AlpsProcessTypeWorker) 
    myHubRank_ = (globalRank_ / cluSize_) * cluSize_;
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::solve(AlpsTreeNode* root)
{
    
  broadcastModel(globalRank_, masterRank_, MPI_COMM_WORLD); 
  MPI_Barrier(MPI_COMM_WORLD);

  // Call related main functions
  if (processType_ == AlpsProcessTypeMaster) {
    masterMain(root);
  }
  else if(processType_ == AlpsProcessTypeHub) {
    hubMain();
  }
  else {
    const int mns = 
      AlpsDataPool::getOwnParams()->entry(AlpsOwnParams::maxNumSolustion);
    setMaxNumKnowledges(ALPS_SOLUTION, mns);
    workerMain();
  }

  // 3. Collect best solution
  MPI_Barrier(MPI_COMM_WORLD);
  collectBestSolution(masterRank_, MPI_COMM_WORLD);
  MPI_Barrier(MPI_COMM_WORLD);
}

//#############################################################################
