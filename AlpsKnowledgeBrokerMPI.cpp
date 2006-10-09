#include "AlpsLicense.h"

#include <algorithm>
#include <cstring>

#include "CoinError.hpp"
#include "CoinHelperFunctions.hpp"

#include "AlpsData.h"
#include "AlpsHelperFunctions.h"
#include "AlpsKnowledgeBrokerMPI.h"
#include "AlpsMessageTag.h"

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::askShareWork(int donorID, int receiverID, 
				      double receiverWorkload, MPI_Comm comm)
{
  int smallSize = AlpsData::parameters()->entry(AlpsAllParam::smallSize);
  int pos       = 0;
  char* buf     = new char [smallSize];

  MPI_Pack(&receiverID, 1, MPI_INT, buf, smallSize, &pos, comm);
  MPI_Pack(&receiverWorkload, 1, MPI_DOUBLE, buf, smallSize, &pos, comm);
  //  MPI_Send(buf, pos, MPI_PACKED, donorID, AlpsMsgAskDonor, comm);
  MPI_Request req;
  MPI_Status status;
  MPI_Isend(buf, pos, MPI_PACKED, donorID, AlpsMsgAskDonor, comm, &req);
  MPI_Wait(&req, &status);
}

//#############################################################################

double 
AlpsKnowledgeBrokerMPI::calWorkload()
{
  std::vector<AlpsTreeNode* > allNodes = 
    dynamic_cast<AlpsNodePool*>(getKnowledgePool(ALPS_NODE))->
    getCandidateList().getContainer();

  double load = 
    std::for_each(allNodes.begin(), allNodes.end(), TotalWorkload()).result();

  return load; 
}

//#############################################################################

void  
AlpsKnowledgeBrokerMPI::hubBalanceLoad(double averLoad, 
	                  std::vector<std::pair<double, int> >& loadIDVector)
{
  std::map<double, int, std::greater<double> > donors;         
  std::map<double, int> receivers; 

  const double donorSh = AlpsData::parameters()->
    entry(AlpsAllParam::donorThreshold);
  const double receiverSh = AlpsData::parameters()->
    entry(AlpsAllParam::receiverThreshold);

  std::vector<std::pair<double, int> >::iterator pos = loadIDVector.begin();
  
  // Indentify donors and receivers
  for ( ; pos != loadIDVector.end(); ++pos) {
    if (pos->first > donorSh)
      donors.insert(*pos);
    else if (pos->first < receiverSh)  
      receivers.insert(*pos);
    else { /* DO NOTHING */}
  }

  const int numDonor    = donors.size();
  const int numReceiver = receivers.size();
  const int numPair     = CoinMin(numDonor, numReceiver);
  int i;
  int donorID;
  int receiverID;
 
  std::map<double, int, std::greater<double> >::iterator posD = donors.begin();
  std::map<double, int>::iterator posR = receivers.begin();

  // Instruct donors to send nodes to receivers 
  for(i = 0; i < numPair; ++i) {
    donorID = posD->second;
    receiverID = posR->second;
    askShareWork(donorID, receiverID, posR->first, MPI_COMM_WORLD);
  }

}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::sendIncumbentToHub(double incVal, int incID, 
					   MPI_Comm comm) 
{
  int size   = AlpsData::parameters()->entry(AlpsAllParam::smallSize);
  int pos    = 0;
  char * buf = new char [size];

#if defined NF_DEBUG
  std::cout << "PROCESS : " << AlpsData::getProcessID() 
	    << " : send incVal = " << incVal << " to PROC " << incID
	    << std::endl;
#endif

  MPI_Pack(&incVal, 1, MPI_DOUBLE, buf, size, &pos, comm);
  MPI_Pack(&incID, 1, MPI_INT, buf, size, &pos, comm);
  MPI_Send(buf, pos, MPI_PACKED, AlpsProcessTypeHub, 
	   AlpsMsgIncumbent, comm);

  if(buf != 0) {
    delete [] buf;     buf = 0;
  }
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::shareWork(char* buf, MPI_Status*,  MPI_Comm comm)
{
  int size       = AlpsData::parameters()->entry(AlpsAllParam::smallSize);
  int pos        = 0;
  int receiverID = 0;
  double receiverWorkload = 0.0;

  MPI_Unpack(buf, size, &pos, &receiverID, 1, MPI_INT, comm);
  MPI_Unpack(buf, size, &pos, &receiverWorkload, 1, MPI_DOUBLE, comm);

  // Send node(s) to receiverID
#if defined NF_DEBUG
  std::cout << "PROCESS : " << AlpsData::getProcessID() 
	    << " : will share workload with PROC " <<  receiverID
	    << ", whose load = " << receiverWorkload
	    << std::endl;
#endif

  const int minNodeNum = 
    AlpsData::parameters()->entry(AlpsAllParam::minNodeNum);

  if (getNumKnowledges(ALPS_NODE) > minNodeNum) // Otherwise don't bother to share
    sendNode(receiverID, comm);

}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::unpackSetIncumbent(char* buf, 
					   MPI_Status* status, MPI_Comm comm)
{

  int size     = AlpsData::parameters()->entry(AlpsAllParam::smallSize);
  int position = 0;
  double incVal= 0.0;
  int incID    = 0;
 
 // Unpack incumbent value from buf
  MPI_Unpack(buf, size, &position, &incVal, 1, MPI_DOUBLE, comm);
  MPI_Unpack(buf, size, &position, &incID, 1, MPI_INT, comm);

  if (incID == AlpsData::getProcessID()) { 

#if defined NF_DEBUG
    std::cout << "PROCESS : " << AlpsData::getProcessID() 
	      << " : unpack and ingore incVal = " << incVal << " at PROC" 
	      << incID << ", since I am " << incID
	      << std::endl;
#endif

    return;  // Do nothing
  }

#if defined NF_DEBUG
  std::cout << "PROCESS : " << AlpsData::getProcessID() 
	    << " : unpack incVal = " << incVal << " at PROC " << incID 
	    << std::endl;
#endif

  // Assume minimization
  if (incVal < AlpsData::getIncumbentValue()) {
    AlpsData::setIncumbentValue(incVal);
    AlpsData::setIncumbentID(incID);

    AlpsData::setUpdateIncumbent(true);    // The incumbent value is updated. 
  }
  else { 
#if defined NF_DEBUG
  std::cout << "PROCESS : " << AlpsData::getProcessID() 
	    << " : unpacked but discard incVal = " << incVal  
	    << " at PROC " << incID << std::endl;
#endif  
  } // FIXME: send back a msg to stop?
  
}

//#############################################################################

int
AlpsKnowledgeBrokerMPI::takeSurvey(MPI_Comm comm)
{
  char* dummyBuf      = 0;
  const int medSize   = 
    AlpsData::parameters()->entry(AlpsAllParam::mediumSize);
  const int smallSize = 
    AlpsData::parameters()->entry(AlpsAllParam::smallSize);

  MPI_Status status;
  MPI_Send(dummyBuf, 0, MPI_PACKED, AlpsProcessTypeHub, AlpsMsgIdle, comm);

  //---------------------------------------------------------------------------
  //Calculate its workload
  double workload = calWorkload();
  AlpsData::setWorkload(workload);

#if defined NF_DEBUG  
  std::cout << "WORKER : "<< AlpsData::getProcessID() 
	    <<" : workload = " << AlpsData::getWorkload() <<  std::endl; 
#endif

  MPI_Send(&workload, 1, MPI_DOUBLE, AlpsProcessTypeHub, 
	   AlpsMsgLoadInfo, comm);

#if defined NF_DEBUG  
  std::cout << "WORKER : "<< AlpsData::getProcessID() 
    << " : sent LoadInfo to HUB" <<  std::endl; 
#endif

  //---------------------------------------------------------------------------
  // Ask for instruction from hub about continue or not
  MPI_Recv(dummyBuf, 0, MPI_PACKED, AlpsProcessTypeHub, MPI_ANY_TAG,
	   comm, &status);

#if defined NF_DEBUG  
  std::cout << "WORKER: recved msg tagged " << status.MPI_TAG
	    << " from HUB" <<  std::endl; 
#endif


  if (dummyBuf != 0) {
    delete [] dummyBuf;   dummyBuf = 0;
  }
  if (status.MPI_TAG != AlpsMsgAskCont) 
    return 0;        // WORKER exit
  return 1;
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::hubMain(AlpsTreeNode* root, MPI_Comm comm)
{
  int i = 0;
  char * dummyBuf = new char[8];

  root->setKnowledgeBroker(this); 

  root->setPriority(0);
  root->setLevel(0);
  root->setIndex(0);
  subtree_->setNextIndex(1); // One more than root's index
  int numWorker = 
    AlpsData::parameters()->entry(AlpsAllParam::workerProcessNum);
  int numProcess = 
    AlpsData::parameters()->entry(AlpsAllParam::processNum);
  int myID = AlpsData::getProcessID();

  //===========================================================================
  // Create required number of nodes (or more).
  //===========================================================================

#if defined(NF_DEBUG)
  std::cout << "HUB: start to create nodes..." << std::endl; 
#endif

  dynamic_cast<AlpsSubTreeHub*>(subtree_)->initializeSearch(root);

#if defined(NF_DEBUG)
  std::cout << "HUB: finish creating nodes." << std::endl; 
  std::cout << "HUB: the number of nodes Hub created is " 
	    <<  getNumKnowledges(ALPS_NODE) << std::endl; 
#endif
 
  //===========================================================================
  // Spawn worker processes (Not needed for static process management) 
  // FILL IN HERE IF NEEDED 
  //===========================================================================
 
  if (getNumKnowledges(ALPS_NODE) == 0) {
    std::cout << "HUB: Failed to generate enought nodes and finish search "
	      << "by itself (without help from workers)" << std::endl;
  }
  else {    // Send nodes to workers
    const int numNode = getNumKnowledges(ALPS_NODE);
    int       numSent = 0;
    
    while ( numSent < numNode) {
      for (i = 0; i < numProcess; ++i) {
	if(numSent >= numNode ) break;
	if (i != myID) { 
	  sendSizeNode(i, comm);
	
#if defined NF_DEBUG
	  std::cout << "HUB: finish sending nodes to " << i+1 << std::endl; 
#endif
	  ++numSent;
	}
      }
    }
  }
 
  // Sent finish initialization tag
  for (i = 0; i < numProcess; ++i) {
    if (i != myID)
      sendFinishInit(i, comm);
  }

  // If have solution, broadcast its value and process id
  if (hasKnowledge(ALPS_SOLUTION)) 
    broadcastIncumbent(MPI_COMM_WORLD);

  //===========================================================================
  // HUB: SCHEDULER 
  //===========================================================================
 
  // Start to do peroidical checking
  const double period = 
    AlpsData::parameters()->entry(AlpsAllParam::balancePeriod);
#if defined NF_DEBUG  
  std::cout << "HUB: peroid = " << period << std::endl; 
#endif

  int flag            = 0;
  int countStop       = 0;
  double startTime    = 0.0;
  MPI_Status status;

  const int medSize   = 
    AlpsData::parameters()->entry(AlpsAllParam::mediumSize);
  const int smallSize = 
    AlpsData::parameters()->entry(AlpsAllParam::smallSize);
  const double zeroLoad = 
    AlpsData::parameters()->entry(AlpsAllParam::zeroLoad);

  //===========================================================================

  while (true) {
    // Message triggered thread: hub thread
    startTime = AlpsCpuTime();

    if (buf_ != 0) {
      delete buf_; buf_ = 0;
    }
    buf_ = new char [medSize];
   
    //-------------------------------------------------------------------------
    while ((AlpsCpuTime() - startTime) < period) {  // Listen one period 
      flag = 0;
      MPI_Request request;
      MPI_Status status;

      MPI_Irecv(buf_, medSize, MPI_PACKED, MPI_ANY_SOURCE, 
		MPI_ANY_TAG, comm, &request);
      //-----------------------------------------------------------------------
      while ( true ) {
	
	MPI_Test(&request, &flag, &status);
	
	if (flag) {   // There is a msg

	  switch(status.MPI_TAG) {
	  case AlpsMsgIncumbent:
	    unpackSetIncumbent(buf_, &status, MPI_COMM_WORLD);
	    break;
	  
	  default: 
	    std::cout << "HUB: recved UNKNOWN message. tag = " 
		      << status.MPI_TAG <<  std::endl; 
	    throw CoinError("Unknow message type", 
			    "workmain", "AlpsKnowledgeBrokerMPI");
	  }
	  
	  if (buf_ != 0) {
	    delete buf_; buf_ = 0;
	  }
	  buf_ = new char [medSize];
	  
	  break;      // Break while(true) to post another MPI_Irecv
	}

	//---------------------------------------------------------------------
	if ((AlpsCpuTime() - startTime) > period) {
	  if (! flag)  { // If Irecv is unsatisfied, MUST cancel it
	    MPI_Cancel(&request);
	    MPI_Wait(&request, &status);

#if defined NF_DEBUG
	    int flag2;
	    MPI_Test_cancelled(&status, &flag2);
	    if (flag2)
	      std::cout << "PROCESS: " << AlpsData::getProcessID() 
			<< " : Cancel a Irecv" << std::endl;
#endif
	  }
	  break;
	}
	//---------------------------------------------------------------------
      }
    }

    //-------------------------------------------------------------------------
    // If have better solution, broadcast its value and process id
    if (AlpsData::getUpdateIncumbent() == true) { 
      broadcastIncumbent(MPI_COMM_WORLD);
      AlpsData::setUpdateIncumbent(false);
    }

    //=========================================================================
    // Periodically do load balancing and termination check 
    // 1. Synchronize  2. Survey  3. Balance or Terminate
    //=========================================================================

#if defined NF_DEBUG  
    std::cout << "HUB: start to balance. " << std::endl; 
#endif

    for (i = 0; i < numProcess; ++i) {
      if (i != myID)
	MPI_Send(dummyBuf, 0, MPI_PACKED, i, AlpsMsgAskPause, comm);
    }

    for (i = 0; i < numWorker; ++i) {   // make sure all workers idle
      MPI_Recv(dummyBuf, 0, MPI_PACKED, MPI_ANY_SOURCE, AlpsMsgIdle, 
		 comm, &status);
    }

    //-------------------------------------------------------------------------
    // 2. Survey workers
    countStop        = 0;
    double load      = 0.0;
    double totalLoad = 0.0;
    double averLoad  = 0.0;
    
    std::vector<std::pair<double, int> > loadIDVector;
    
    loadIDVector.reserve(numProcess);
    
    for (i = 0; i < numWorker; ++i) {   
      MPI_Recv(&load, 1, MPI_DOUBLE, MPI_ANY_SOURCE, AlpsMsgLoadInfo, 
	       comm, &status);
      
#if defined NF_DEBUG
      std::cout << "HUB : recved workload info = " << load << " from PROC "
		<< status.MPI_SOURCE << std::endl;
#endif
      
      totalLoad += load;
      if (load < zeroLoad)
	++countStop;

      loadIDVector.push_back(std::pair<double, int>(load, status.MPI_SOURCE));
    }

    if (numWorker != 0)
      averLoad = totalLoad / numWorker;
    else
      throw CoinError("numberer == 0", 
		      "takeSurvey()", "AlpsKnowledgeBrokerMPI"); 

    //-------------------------------------------------------------------------
    // 3. Balance or Terminate

#if defined NF_DEBUG  
    std::cout << "HUB: countStop = " << countStop << std::endl; 
#endif

    if (countStop != numWorker) {          // Continue working
      for (i = 0; i < numProcess; ++i) {
	if (i != myID) 
	  MPI_Send(dummyBuf, 0, MPI_PACKED, i, AlpsMsgAskCont, comm);
      }
      // Do load balancing
      hubBalanceLoad(totalLoad, loadIDVector);
    }
    else {                                 // All process should exit
      for (i = 0; i < numProcess; ++i) {
	if (i != myID) 
	  MPI_Send(dummyBuf, 0, MPI_PACKED, i, AlpsMsgAskStop, comm);
      }      
      return;  
    }
    //-------------------------------------------------------------------------
  }     //END_OF_WHILE
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::workerMain(MPI_Comm comm)
{
  int i, iter = 0;
  char* buf = 0;
  
  MPI_Status status;
  const int largeSize = 
    AlpsData::parameters()->entry(AlpsAllParam::largeSize);
  const int medSize = 
    AlpsData::parameters()->entry(AlpsAllParam::mediumSize);
  const int smallSize = 
    AlpsData::parameters()->entry(AlpsAllParam::smallSize);

  //---------------------------------------------------------------------------
  // Receive node sending by HUB
  //---------------------------------------------------------------------------
  
  while(true) {
    receiveSizeNode(AlpsProcessTypeHub, comm, &status);
    if (status.MPI_TAG == AlpsMsgFinishInit) {
      std::cout << "WORKER: rec AlpsMsgFinishInit." << std::endl; 
      break;
    }
  }
#if defined NF_DEBUG  
  std::cout << "WORKER: start to solve(node)." << std::endl; 
#endif

  //===========================================================================
  // WORKER: SCHEDULER
  //===========================================================================

  while (true) {

    //-------------------------------------------------------------------------
    // Communication threads. First recv msgs, process them and recv again
    // until there is no msg.
    //-------------------------------------------------------------------------

    int outCount = 0;
    MPI_Status status;
    MPI_Request req;
    int flag = 0;

    if(buf != 0) {
      delete [] buf;     buf = 0;
    }
    buf  = new char[largeSize];

    //-------------------------------------------------------------------------
    MPI_Irecv(buf, largeSize, MPI_PACKED, MPI_ANY_SOURCE, MPI_ANY_TAG, 
	      comm, &req);

    while (true) {
      flag = 0;

      MPI_Test(&req, &flag, &status);

      //-----------------------------------------------------------------------
      // Process msg if there is one
      if ( flag ) {
	switch (status.MPI_TAG) {
	  
	case AlpsMsgAskPause:
	  if (takeSurvey(comm) == 0) 
	    return;           // Return from Workmain() and terminates
	  break;
	  
	case AlpsMsgIncumbent:
	  unpackSetIncumbent(buf, &status, MPI_COMM_WORLD);
	  break;
	  
	case AlpsMsgAskDonor:
	  shareWork(buf, &status, MPI_COMM_WORLD);
	  break;

	case AlpsMsgNode:
#if defined NF_DEBUG
	  std::cout << "PROCESS : "<< AlpsData::getProcessID() 
  	    << ": start to recv node from  "<< status.MPI_SOURCE <<  std::endl;
#endif
	  receiveNode(buf, status.MPI_SOURCE, MPI_COMM_WORLD, &status);
	  break;

	default: 
	  std::cout << "WORKER: recved UNKNOWN message. tag = " 
		    << status.MPI_TAG <<  std::endl; 
	  throw CoinError("Unknow message type", 
			  "workmain", "AlpsKnowledgeBrokerMPI"); 
	} 
   
	break;      // break while(true) to post a non-block recv again
      }
    
      //-----------------------------------------------------------------------
      // Since no msg, do Computation threads
      //-----------------------------------------------------------------------
      if ( hasKnowledge(ALPS_NODE) ) {
	doOneUnitWork();
	++iter;
      }

#if defined NF_DEBUG
      if (iter % 10000 == 0)  
	std::cout << "WORKER: iter = " << iter << std::endl;
#endif

      // If found a solution, check whether need to send to HUB
      if (hasKnowledge(ALPS_SOLUTION)) {
	double incVal = getBestKnowledge(ALPS_SOLUTION).second;
	if(incVal < (AlpsData::getIncumbentValue()) ){ // Assume Minimization
	  AlpsData::setIncumbentValue(incVal);
	  int incID = AlpsData::getProcessID();
	  AlpsData::setIncumbentID(incID);
	  sendIncumbentToHub(incVal, incID, MPI_COMM_WORLD);
	} 
      }
      //-----------------------------------------------------------------------
    }   
  }  

  if(buf != 0) {
    delete [] buf;     buf = 0;
  }
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::solve(AlpsTreeNode* root, 
			      MPI_Comm comm)
{
  const int id = AlpsData::getProcessID(); 
  // Broadcast model and parameters.
  broadcastParams(id, AlpsProcessTypeHub, comm);
  MPI_Barrier(comm);
  broadcastModel(id, AlpsProcessTypeHub, comm); 
  MPI_Barrier(comm);

  if(id == AlpsProcessTypeHub) {
    AlpsSubTreeHub* st = new AlpsSubTreeHub;  // Broker will delete the momery
    setSubTree(st);
    setupKnowledgePools();
    hubMain(root, comm);
  }
  else {
    AlpsSubTreeWorker* st = new AlpsSubTreeWorker;
    setSubTree(st);
    setupKnowledgePools();
    const int mns=AlpsData::parameters()->entry(AlpsAllParam::maxNumSolustion);
    setMaxNumKnowledges(ALPS_SOLUTION, mns);
    workerMain(comm);
  }

  gatherBestSolution(AlpsProcessTypeHub, comm);
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::packEncoded(AlpsEncoded* enc) 
{
  position_          = 0;       // To record current position in buf_.
  int typeSize = strlen(enc->type());
  int repSize  = enc->size();
  const int bufSpare = AlpsData::parameters()->entry(AlpsAllParam::bufSpare);
  size_              = typeSize + repSize + 2*sizeof(int) + bufSpare;
 
  if(buf_ != 0) {
    delete [] buf_;     buf_ = 0;
  }
  buf_  = new char[size_];

  // Pack typeSize, repSize, type_, representation_ of enc
  MPI_Pack(&typeSize, 1, MPI_INT, buf_,  size_, &position_, MPI_COMM_WORLD);
  MPI_Pack(&repSize, 1, MPI_INT, buf_,  size_, &position_, MPI_COMM_WORLD);
  MPI_Pack(const_cast<char*>(enc->type()), typeSize, MPI_CHAR, 
	   buf_, size_, &position_, MPI_COMM_WORLD); 
  MPI_Pack(const_cast<char*>(enc->representation()), repSize, MPI_CHAR, 
	   buf_, size_, &position_, MPI_COMM_WORLD); 
}

//#############################################################################
AlpsEncoded* 
AlpsKnowledgeBrokerMPI::unpackEncoded() 
{
  position_ = 0;

  int typeSize;
  int repSize; 

  // Unpack typeSize, repSize, type and rep from buf_
  MPI_Unpack(buf_, size_, &position_, &typeSize, 1, MPI_INT, MPI_COMM_WORLD);

#if defined NF_DEBUG
  std::cout << "PROCESS : "<< AlpsData::getProcessID() <<" : typeSize is " 
	    << typeSize << ";\tsize_ = "<< size_ <<  std::endl;
#endif

  MPI_Unpack(buf_, size_, &position_, &repSize, 1, MPI_INT, MPI_COMM_WORLD);
  
  char* type = new char [typeSize+1]; // MUST larger
  char* rep = new char[repSize+1];

  MPI_Unpack(buf_, size_, &position_, type, typeSize, MPI_CHAR, 
	     MPI_COMM_WORLD);
  // MUST terminate cstring so that don't read trash
  *(type+typeSize) = '\0';        
  
  MPI_Unpack(buf_, size_, &position_, rep, repSize, MPI_CHAR, MPI_COMM_WORLD);
  rep[repSize] = '\0';

#if defined NF_DEBUG
  std::cout << "PROCESS : "<< AlpsData::getProcessID() << ": type = " << type 
	    << "; repSize = " << repSize << std::endl;
#endif

  // Return a enc of required knowledge type
  return new AlpsEncoded(type, repSize, rep );
}

//#############################################################################
AlpsEncoded* 
AlpsKnowledgeBrokerMPI::unpackEncoded(char* buf) 
{
  int position = 0;

  int typeSize;
  int repSize; 
  int size = AlpsData::parameters()->entry(AlpsAllParam::largeSize);

  // Unpack typeSize, repSize, type and rep from buf_
  MPI_Unpack(buf, size, &position, &typeSize, 1, MPI_INT, MPI_COMM_WORLD);

#if defined NF_DEBUG
  std::cout << "PROCESS : "<< AlpsData::getProcessID() <<" : typeSize is " 
	    << typeSize << ";\tsize = "<< size <<  std::endl;
#endif

  MPI_Unpack(buf, size, &position, &repSize, 1, MPI_INT, MPI_COMM_WORLD);
  
  char* type = new char [typeSize+1]; // MUST larger
  char* rep = new char[repSize+1];

  MPI_Unpack(buf, size, &position, type, typeSize, MPI_CHAR, 
	     MPI_COMM_WORLD);
  // MUST terminate cstring so that don't read trash
  *(type+typeSize) = '\0';        
  
  MPI_Unpack(buf, size, &position, rep, repSize, MPI_CHAR, MPI_COMM_WORLD);
  rep[repSize] = '\0';

#if defined NF_DEBUG
  std::cout << "PROCESS : "<< AlpsData::getProcessID() << ": type = " << type 
	    << "; repSize = " << repSize << std::endl;
#endif

  // Return a enc of required knowledge type
  return new AlpsEncoded(type, repSize, rep );
}

//#############################################################################
void
AlpsKnowledgeBrokerMPI::receiveBuf(int sender, int tag, 
				   MPI_Comm comm, MPI_Status* status) 
{
  const int size = AlpsData::parameters()->entry(AlpsAllParam::largeSize);
  if (buf_ != NULL) {
    delete [] buf_;  buf_ = 0;
  }
  buf_ = new char [size];

  MPI_Recv(buf_, size, MPI_PACKED, sender, tag, comm, status);
}

//#############################################################################
void
AlpsKnowledgeBrokerMPI::receiveSizeBuf(int sender, int tag, 
				       MPI_Comm comm, MPI_Status* status) 
{
  size_ = 0;
  // First recv the buf size
  MPI_Recv(&size_, 1, MPI_INT, sender, tag, comm, status);
  if (status->MPI_TAG == AlpsMsgFinishInit)
    return;

  // Secondly, allocate memory for buf_
  if (buf_ != NULL) {
    delete [] buf_;  buf_ = 0;
  }
  buf_ = new char [size_];

  // Third, receive MPI packed data and put in buf_
  MPI_Recv(buf_, size_, MPI_PACKED, sender, tag, comm, status);
}

//#############################################################################
void 
AlpsKnowledgeBrokerMPI::receiveNode(char* buf, int sender,
				    MPI_Comm comm, MPI_Status* status) 
{
#if defined NF_DEBUG
  std::cout << "WORKER: start to recv node send by " << sender << std::endl; 
#endif
  //  receiveBuf(sender, MPI_ANY_TAG, comm, status); // Alread received
  int i = 0;

  if (status->MPI_TAG == AlpsMsgFinishInit ) 
    std::cout << "WORKER: rec AlpsMsgFinishInit... STOP INIT." << std::endl; 
  else if (status->MPI_TAG == AlpsMsgNode) {
    AlpsEncoded* encodedNode = unpackEncoded(buf);
    AlpsTreeNode* node = static_cast<AlpsTreeNode* >( AlpsKnowledge::
		decoderObject(encodedNode->type())->decode(*encodedNode) );

#if defined NF_DEBUG  
    std::cout << "WORKER: finish docoding node." << std::endl; 
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
    std::cout << "PROCESS : " << AlpsData::getProcessID() 
	      <<" : recved UNKNOWN message. tag = " 
	      << status->MPI_TAG <<  std::endl; 
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
#if defined NF_DEBUG
  std::cout << "WORKER: start to recv node." << std::endl; 
#endif
  int i = 0;
  receiveSizeBuf(sender, MPI_ANY_TAG, comm, status);
  //receive(sender, AlpsMsgNode, comm, status);  // MUST RECV ANY TAG TO STOP

  if (status->MPI_TAG == AlpsMsgFinishInit ) 
    std::cout << "WORKER: rec AlpsMsgFinishInit... STOP INIT." << std::endl; 
  else if (status->MPI_TAG == AlpsMsgNode) {
    AlpsEncoded* encodedNode = unpackEncoded();
    std::cout << "WORKER: received and unpacked a node." << std::endl; 

#if defined NF_DEBUG_MORE  
    std::cout << "WORKER: type() is " << encodedNode->type() << std::endl;
    std::cout << "WORKER: finish unpacking node." << std::endl; 
    std::cout << "WORKER: start to decode node." << std::endl; 
    const AlpsTreeNode* bugNode = dynamic_cast<const AlpsTreeNode* >(
		  AlpsKnowledge::decoderObject(encodedNode->type()));
    std::cout <<"WORKER: bugNode'a Priority = " 
	      << bugNode->getPriority() << std::endl;
    std::cout << "WORKER: finish just decoding node." << std::endl; 
#endif

    AlpsTreeNode* node = dynamic_cast<AlpsTreeNode* >( AlpsKnowledge::
		decoderObject(encodedNode->type())->decode(*encodedNode) );

#if defined NF_DEBUG  
    std::cout << "WORKER: finish docoding node." << std::endl; 
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
    std::cout << "PROCESS : " << AlpsData::getProcessID() 
	      <<" : recved UNKNOWN message. tag = " 
	      << status->MPI_TAG <<  std::endl; 
    throw CoinError("Unknow message type", 
		    "workmain", "AlpsKnowledgeBrokerMPI");
  }
}


//#############################################################################
void 
AlpsKnowledgeBrokerMPI::sendBuf(const int target, const int tag, 
				const MPI_Comm comm) 
{
  // If packing successfully, send it to target
  if (size_ > 0)
    MPI_Send(buf_, position_, MPI_PACKED, target, tag, comm); 
  else
    throw CoinError("Msg size is <= 0", "send", "AlpsKnowledgeBrokerMPI");
}

//#############################################################################
void 
AlpsKnowledgeBrokerMPI::sendSizeBuf(const int target, const int tag, 
				    const MPI_Comm comm) 
{
  // If packing successfully, send it to target
  if (size_ > 0) {
    MPI_Send(&size_, 1, MPI_INT, target, tag, comm);
    MPI_Send(buf_, position_, MPI_PACKED, target, tag, comm); 
  }
  else
    throw CoinError("Msg size is <= 0", "send", "AlpsKnowledgeBrokerMPI");
}

//#############################################################################
void 
AlpsKnowledgeBrokerMPI::sendNode(const int target, MPI_Comm comm) 
{
  const AlpsTreeNode* node = static_cast<const AlpsTreeNode* >
    (getKnowledge(ALPS_NODE).first);
  AlpsEncoded* enc = node->encode();
  popKnowledge(ALPS_NODE);
  packEncoded(enc);
  sendBuf(target, AlpsMsgNode, comm);

#if defined NF_DEBUG  
  std::cout << "WORKER : "<< AlpsData::getProcessID() 
	    << " : donor node to PROC " << target
	    << "; buf_ pos = " << position_ 
	    << "; buf_ size = " << size_ <<  std::endl; 
#endif
}

//#############################################################################
void 
AlpsKnowledgeBrokerMPI::sendSizeNode(const int target, MPI_Comm comm) 
{
  const AlpsTreeNode* node = static_cast<const AlpsTreeNode* >
    (getKnowledge(ALPS_NODE).first);
  AlpsEncoded* enc = node->encode();
  popKnowledge(ALPS_NODE);
  packEncoded(enc);
  sendSizeBuf(target, AlpsMsgNode, comm);
}

//#############################################################################
void 
AlpsKnowledgeBrokerMPI::broadcastModel(const int id, 
				       const int source, 
				       MPI_Comm comm)
{
  if (id == source) {
    // Pack model 
    // pack( ( (AlpsData::ADPool())->getKnowledge(ALPS_MODEL))->encode() );
    packEncoded( (AlpsData::getModel())->encode() );
  }

  MPI_Bcast(&size_, 1, MPI_INT, source, comm);  // Broadcast buf size first

  if (id != source) {
    if (size_ <= 0)
      throw CoinError("Msg size <= 0", "broadcastModel", 
		      "AlpsKnowledgeBrokerMPI");
    if( buf_ != 0 ) {
      delete [] buf_;  buf_ = 0;
    }
    buf_ = new char[size_];
  }

  MPI_Bcast(buf_, size_, MPI_CHAR, source, comm); // Broadcast buf_

  if (id != source) {
    
    AlpsEncoded* encodedModel = unpackEncoded();

#if defined NF_DEBUG
    std::cout << "WORKER: start to decode model." << std::endl; 
    std::cout << encodedModel->type() << std::endl;
#endif

    AlpsModel* model = dynamic_cast<AlpsModel* >( AlpsKnowledge::
	    decoderObject(encodedModel->type())->decode(*encodedModel) );

#if defined NF_DEBUG
    std::cout << "WORDER: finish decoding model." << std::endl;
#endif

    //AlpsData::ADPool()->updateKnowledgeValue(ALPS_MODEL, model);
    AlpsData::setModel(model);
#if defined NF_DEBUG
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
  // Broadcast Alps parameters
  if (id == source) {
    AlpsEncoded* enc   =  new AlpsEncoded;
    AlpsData::parameters()->pack(*enc); // enc's rep is filled.

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

#if defined NF_DEBUG
   std::cout << "PROC: " << id <<" : Alps buf = " << buf << std::endl;
#endif

  if (id != source) {
    AlpsEncoded* enc2   = new AlpsEncoded(buf);

    AlpsData::parameters()->unpack(*enc2);        // Fill in parameters

#if defined NF_DEBUG  
    std::cout << "PROC: " << id <<" : Received Alps params."<< std::endl; 
#endif

  }

  //---------------------------------------------------------------------------
  // Broadcast user parameters
  if (id == source) {
    AlpsEncoded* enc1   =  new AlpsEncoded;
    AlpsData::userPars()->pack(*enc1); // enc's rep is filled.

    repSize  = enc1->size();
    size     = repSize + 1;              // 1 for '\0'

    if( buf != 0 ) {
      delete [] buf;  buf = 0;
    } 
    buf = new char[size];

    memcpy(buf, enc1->representation(), repSize);  
  }

  MPI_Bcast(&size, 1, MPI_INT, source, comm);

#if defined NF_DEBUG  
  std::cout << "PROC: " << id <<" : size = "<< size << std::endl; 
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
#if defined NF_DEBUG
   std::cout << "PROC: " << id <<" : buf = " << buf << std::endl;
#endif

  if (id != source) {

    AlpsEncoded* enc3   = new AlpsEncoded(buf);

    (AlpsData::userPars())->unpack(*enc3);        // Fill in parameters
#if 0
#endif
#if defined NF_DEBUG  
    std::cout << "PROC: " << id <<" : Received user params."<< std::endl;
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
  int myID      = AlpsData::getProcessID();
  int incID     = AlpsData::getIncumbentID();
  const int pn  = AlpsData::parameters()->entry(AlpsAllParam::processNum);
  double incVal = AlpsData::getIncumbentValue();
  int size      = AlpsData::parameters()->entry(AlpsAllParam::smallSize);
  int position  = 0;
  char* buf     = new char [size];

  MPI_Pack(&incVal, 1, MPI_DOUBLE, buf, size, &position, comm);
  MPI_Pack(&incID, 1, MPI_INT, buf, size, &position, comm);

  for (i = 0; i != pn; ++i) {          // FIXME: loop --> binary tree
//      if (i == myID) {            // Set itself
//        AlpsData::setIncumbentValue(incVal); // Since HUB knows
//        AlpsData::setIncumbentID(myID);  
//      }

    if (i != myID) {                       // Send to other processes
#if defined NF_DEBUG  
      std::cout << "PROCESS: " <<  AlpsData::getProcessID()
		<<" : broadcast a solution,  value = " 
		<< AlpsData::getIncumbentValue() << " to "<< i << std::endl; 
#endif
      MPI_Send(buf, position, MPI_PACKED, i, AlpsMsgIncumbent, comm);
    }

  }
  
  if(buf != 0) {
    delete [] buf;     buf = 0;
  }
}

//#############################################################################
void
AlpsKnowledgeBrokerMPI::gatherBestSolution(int destination, MPI_Comm comm)
{
  MPI_Status status;
  int sender = AlpsData::getIncumbentID();

  if ( sender == destination ) {
    // DO NOTHING since the best solution is in the solution of its pool
  }
  else {
    if (AlpsData::getProcessID() == sender) {                 // Send solu
      const AlpsSolution* solu = static_cast<const AlpsSolution* >
	(getBestKnowledge(ALPS_SOLUTION).first);
      double value = getBestKnowledge(ALPS_SOLUTION).second;
      AlpsEncoded* enc = solu->encode();
      packEncoded(enc);
      sendSizeBuf(destination, AlpsMsgIncumbent, comm);
      MPI_Send(&value, 1, MPI_DOUBLE, destination, AlpsMsgIncumbent, comm);
    }
    if (AlpsData::getProcessID() == destination) {            // Recv solu
      double value;
      receiveSizeBuf(sender, AlpsMsgIncumbent, comm, &status);
      MPI_Recv(&value, 1, MPI_DOUBLE, sender, AlpsMsgIncumbent,
	   comm, &status);
      AlpsEncoded* encodedSolu = unpackEncoded();

      AlpsSolution* bestSolu = dynamic_cast<AlpsSolution* >( AlpsKnowledge::
		 decoderObject(encodedSolu->type())->decode(*encodedSolu) );

      addKnowledge(ALPS_SOLUTION,  bestSolu, value);
    }
  }
}

//#############################################################################
void 
AlpsKnowledgeBrokerMPI::sendFinishInit(const int target, MPI_Comm comm) 
{
  char* dummyBuf = 0;
  MPI_Send(dummyBuf, 0, MPI_PACKED, target, AlpsMsgFinishInit, comm);
}

//#############################################################################
void 
AlpsKnowledgeBrokerMPI::initMsgEnv(int argc, char* argv[], 
				   int& myRank, int& procSize)
{
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
  MPI_Comm_size(MPI_COMM_WORLD, &procSize);

  AlpsData::setProcessID(myRank);     // Set process id
}

//#############################################################################
void
AlpsKnowledgeBrokerMPI::finializeMsgEnv() 
{
  MPI_Finalize();
}

//#############################################################################
