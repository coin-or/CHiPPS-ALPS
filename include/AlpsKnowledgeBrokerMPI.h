#include "AlpsLicense.h"

#ifndef AlpsKnowledgeBrokerMPI_h_
#define AlpsKnowledgeBrokerMPI_h_

#include <cfloat>     // For DBL_MAX
#include <cmath>
#include <iosfwd>

#include "mpi.h"

#include "AlpsEnumProcessT.h"
#include "AlpsKnowledge.h"
#include "AlpsKnowledgeBroker.h"
#include "AlpsParameter.h"


//#############################################################################

class AlpsKnowledgeBrokerMPI : public AlpsKnowledgeBroker {
 private:
  AlpsKnowledgeBrokerMPI(const AlpsKnowledgeBrokerMPI&);
  AlpsKnowledgeBrokerMPI& operator=(const AlpsKnowledgeBrokerMPI&);

 private:
  /** @name Numerical tolerance
   *
   */
  //@{
  double TOLERANCE_;
  //@}

  /** @name Process information
   *
   */
  //@{
  /** The Number of processes launched. */
  int processNum_;
  /** The Number of hubs. */
  int hubNum_;
  /** The rank of the process in MPI_COMM_WORLD. */
  int globalRank_;
  /** Communicator of the cluster to which the process belongs. */
  MPI_Comm clusterComm_;
  /** Communicator consists of all hubs. */
  MPI_Comm hubComm_;
  /** MPI_Group consists of all hubs. */
  MPI_Group hubGroup_;
  /** The actual size of the cluster to which the process belongs. */
  int clusterSize_;
  /** The designed size of a cluster. */
  int cluSize_;
  /** The rank of the process in clusterComm_. */
  int clusterRank_;
  /** The global ranks of hubs in hubComm_. */
  int* hubRanks_;
  /** The global rank of its hub for a worker. */
  int myHubRank_;
  /** The global rank of the master. */
  int masterRank_;
  /** The AlpsProcessType of the process. */
  AlpsProcessType processType_;
  //@}

  /** @name Incumbent data
   *
   */
  //@{
  /** Incumbent value. */
  double incumbentValue_;
  /** The process id that store the incumbent. */
  int incumbentID_;
  /** Indicate whether the incumbent value is updated between two 
      checking point. */
  bool updateIncumbent_;
  //@}

  /** @name Workload information
   *
   */
  //@{
  /** The workload of the process. */
  double workload_;
  /** The workload of the cluster to which the process belong. */
  double clusterWorkload_;
  /** The workload of the whole system. */
  double systemWorkload_;
  /** The workloads of hubs. */
  double* hubLoads_;
  /** The workloads of workers. */
  double* workerLoads_;
  /** Indicate which worker has been reported its work. */
  bool* workerReported_;
  /** Indicate which hub has been reported its work. */
  bool* hubReported_;
  bool allHubReported_;
  //@}

  /** @name Message counts
   *
   */
  //@{
  /** The number of new messages sent by the process after last survey. */
  int sendCount_;
  /** The number of new messages received by the process after last survey. */
  int recvCount_;
  /** The number of new messages sent by the processes in clusterComm_ 
      after last survey.*/
  int clusterSendCount_;
  /** The number of new messages received by the processes in clusterComm_ 
      after last survey.*/
  int clusterRecvCount_;
  /** The total number of messages sent by the all processes. */
  int systemSendCount_;
   /** The total number of messages sent by the all processes. */ 
  int systemRecvCount_;
  //@}

  //---------------------------------------------------------------------------

 private:

  /** Initialize member data. */
  void init() {
    TOLERANCE_ = 1.0e-6;
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
    incumbentValue_ = DBL_MAX;
    incumbentID_ = 0;
    updateIncumbent_ = false;
    workload_ = 0.0;
    clusterWorkload_ = 0.0;
    systemWorkload_ = 0.0;
    hubLoads_ = 0;
    workerLoads_ = 0;
    workerReported_ = 0;
    hubReported_ = 0;
    allHubReported_  = false; 
    sendCount_ = 0;
    recvCount_ = 0;
    clusterSendCount_ = 0;
    clusterRecvCount_ = 0;
    systemSendCount_ = 0;
    systemRecvCount_ = 0;
  }

  //---------------------------------------------------------------------------



  /** Explore the subtree for certain units of work/time. */
  void doOneUnitWork() {
    subtree_->doOneUnitWork();
  }

  /** @name Scheduler member functions
   *
   */
  //@{
  /** Master generates nodes and sends them to hubs in Round-Robin way.
      Master periodically do inter-cluster load balancing, 
      and termination check. */
  void masterMain(AlpsTreeNode* root);

  /** Hub generates nodes and sends them to workers in Round-Robin way.
      Hub do intra-cluster load balancing. */
  void hubMain();

  /** Worker first receive nodes, then start to explore them. Worker also
      peroidically check message and process message. */
  void workerMain();
  //@}

  //---------------------------------------------------------------------------

  /** @name Load balancing member functions
   *
   */
  //@{
  /** Ask a worker/hub to share it workload with the specified receiver. */
  void askHubsShareWork(int donorID, 
			int receiverID, 
			double receiverWorkload, 
			MPI_Comm comm);

  /** Calculate the workload in this process. */
  double calWorkload();

  /** A worker donate it workload with the specified worker. */
  void donateWork(char* buf, 
		  MPI_Status* status,  
		  MPI_Comm comm);

  /** Hub allocates the donated workload to its workers. */
  void hubAllocateDonation(char* buf, 
			   MPI_Status* status, 
			   MPI_Comm comm);

  /** Hub balances the workloads of its workers. */
  void hubBalanceWorkers(char* buf, 
			 MPI_Status* status, 
			 MPI_Comm comm);
  
  /** Hub checks the status of its cluster. */
  void hubCheckOwnCluster(int tag, 
			  MPI_Comm comm);
  
  /** A hub reports its status (workload and msg counts) to the master. */
  void hubReportStatus(int tag, 
		       MPI_Comm comm);

  /** A hub unpacks the status of a worker from buf. */
  void hubUpdateCluStatus(char* buf, 
			  MPI_Status* status, 
			  MPI_Comm comm);

  /** Two hubs share their workload. */
  void hubsShareWork(char* buf, 
		     MPI_Status* status, 
		     MPI_Comm comm);

  /** Master balance the workload of hubs. */
  void masterBalanceHubs(MPI_Comm comm);

  /** Master unpack the status of a hub from buf and update system status. */
  void masterUpdateSysStatus(char* buf, 
			     MPI_Status* status, 
			     MPI_Comm comm);

  /** The master re-calculate the system status. */
  void refreshSysStatus();

  /** unpack the incumbent value, then store it and the id of the process 
      having the incumbent in AlpsData. */
  void unpackSetIncumbent(char* buf, 
			  MPI_Status* status, 
			  MPI_Comm comm);

  /** A worker report its status (workload and msg counts) to its hub. */
  void workerReportStatus(int tag, 
			  MPI_Comm comm);
  //@}

  //---------------------------------------------------------------------------

  /** @name Message passing member functions
   *
   */
  //@{
  /** Broadcast the model from source to other processes. */
  void broadcastModel(const int id, 
		      const int source, 
		      MPI_Comm comm);

  /** Broadcast parameters from source to other processes. */
  void broadcastParams(const int id, 
		       const int source, 
		       MPI_Comm comm);

  /** Broadcast the best solution value and the rank of the process that
      fount it. */
  void broadcastIncumbent(MPI_Comm comm);

  /** Send the best solution from the process having it to destination. */
  void collectBestSolution(int destination, 
			   MPI_Comm comm);

  /** Pack an AlpsEncoded instance into buf. Return filled buf and size of 
   packed message. */
  void packEncoded(AlpsEncoded* enc, 
		   char*& buf, 
		   int& size, 
		   int& position);
 
  /** Unpack the given buffer into an AlpsEncoded instance. */
  AlpsEncoded* unpackEncoded(char*& buf);

  /** Receive the message and put it in buffer. Assume that the buffer 
      is large enough.*/
  void receiveBuf(char*& buf, 
		  int sender, 
		  int tag, 
		  MPI_Comm comm, 
		  MPI_Status* status);

  /** Receive the size of buffer, allocate memory for buffer, then 
      receive the message and put it in buffer. */
  void receiveSizeBuf(char*& buf, 
		      int sender, 
		      int tag, 
		      MPI_Comm comm, 
		      MPI_Status* status);

  /** Receive the the contend of a node from the sender process, 
      and unpack it. Assume that buf is large enough. */
  void receiveNode(char*& buf, 
		   int sender, 
		   MPI_Comm comm, 
		   MPI_Status* status);

  /** Receive the size and the contend of a node from the sender process, 
      then unpack it. */
  void receiveSizeNode(int sender, 
		       MPI_Comm comm, 
		       MPI_Status* status);

  /** Send the content of buffer to the target process. Assume that 
      the buffer is large enough. */
  void sendBuf(char* buf, 
	       int size, 
	       int position, 
	       const int target, 
	       const int tag, 
	       MPI_Comm comm);

  /** Send the size of buffer and the content of buffer to the 
      target process. */
  void sendSizeBuf(char* buf, 
		   int size, 
		   int position,
		   const int target, 
		   const int tag, 
		   MPI_Comm comm);

  /** Pop a node from node pool and send it to the target process. 
      Assume that the buffer is large enough. */
  void sendNode(const int target, 
		MPI_Comm comm);

  /** Pop a node from node pool, pack it, then send the size and the
      contend of it to the target process. */
  void sendSizeNode(const int target, 
		    MPI_Comm comm);

  /** Send finish initialization signal to the target process. */
  void sendFinishInit(const int target, 
		      MPI_Comm comm);
  //@}

  //===========================================================================

 public:

  AlpsKnowledgeBrokerMPI()          // Default constructor,
    :                               // need call initializeSolver(...) later
    AlpsKnowledgeBroker()           // Base class constructor
    { 
      init(); 
    }
 
  AlpsKnowledgeBrokerMPI(int argc, 
			 char* argv[], 
			 AlpsModel& model, 
			 AlpsParameterSet& userParams )
    :
    AlpsKnowledgeBroker()          // Base class constructor
    {    
      init(); 
      initializeSolver(argc, argv, model, userParams); 
    }


  ~AlpsKnowledgeBrokerMPI() { 
    if (hubRanks_ != 0) {
      delete hubRanks_; hubRanks_ = 0;
    }
    if (hubLoads_ != 0) {
      delete hubLoads_; hubLoads_ = 0;
    }

    MPI_Finalize();
  }

  //---------------------------------------------------------------------------


  /** Qeury the global rank of the process. */
  int getProcRank() const { return globalRank_; }

  /** This function
   * <ul>
   *  <li> initializes the message environment;
   *  <li> the master reads in ALPS and user's parameter sets. If the model 
   *  data is input from file, then it reads in the model data.
   *  <li> sets up user params and model;
   *  <li> broadcast parameters from the master to all other processes;
   *  <li> creates MPI communicators and groups;
   *  <li> classifies process types, sets up subtree and pools
   *  <li> determines their hub's global rank for workers
   * </ul>
   */
  void initializeSolver(int argc, 
			char* argv[], 
			AlpsModel& model, 
			AlpsParameterSet& userParams);

  /** This function
   * <ul>
   * <li> broadcasts model from the master to all other processes;
   * <li> calls its associated main function to explore the sub tree;
   * <li> collects the best solution found.
   * </ul>
   */
  void solve(AlpsTreeNode* root);

  /** @name Report the best solution 
   *  
   */
  //@{
  /** Get the objective value of the incumbent. */
  virtual double getIncumbentValue() const {
    return incumbentValue_;
  }
  /** Get the objective value of the best solution found. */
  virtual double getBestObjValue() const {
    if (globalRank_ == masterRank_) 
      return getBestKnowledge(ALPS_SOLUTION).second;   
  }

  /** Print out the best solution found. */
  virtual void printBestSolution(char* outputFile = 0) const {
    if (globalRank_ == masterRank_) {
      if (outputFile != 0) {                // Write to outputFile
	std::ofstream os(outputFile);
	dynamic_cast<AlpsSolution* >( 
	   const_cast<AlpsKnowledge* >(getBestKnowledge(ALPS_SOLUTION).first) 
	   )->print(os);
      }
      else {                                // Write to std::cout
	dynamic_cast<AlpsSolution* >( 
	   const_cast<AlpsKnowledge* >(getBestKnowledge(ALPS_SOLUTION).first) 
	   )->print(std::cout);
      }      
    }
  }

  /** Print out the best solution found and its objective value. */
  virtual void printBestResult(char* outputFile = 0) const {
    if (globalRank_ == masterRank_) {
      if (outputFile != 0) {                // Write to outputFile
	std::ofstream os(outputFile);
	os << "Objective value = " << getBestObjValue();
	os << std::endl;
	dynamic_cast<AlpsSolution* >( 
	   const_cast<AlpsKnowledge* >(getBestKnowledge(ALPS_SOLUTION).first) 
	   )->print(os);
      }
      else {                                // Write to std::cout
	std::cout << "Objective value = " << getBestObjValue();
	std::cout << std::endl;
	dynamic_cast<AlpsSolution* >( 
	   const_cast<AlpsKnowledge* >(getBestKnowledge(ALPS_SOLUTION).first) 
	   )->print(std::cout);
      }      
    }
  }
  //@}

};

//#############################################################################

/** A functor class used in calulating total workload. */
class TotalWorkload : public std::unary_function<AlpsTreeNode*, void> {

 private:
  double totalLoad_;
  double incVal_;
  double rho_;

 public:
  TotalWorkload(double incVal) 
    : 
    totalLoad_(0.0), 
    //    incVal_(AlpsData::getIncumbentValue()), 
    incVal_(incVal),
    rho_(AlpsData::parameters()->entry(AlpsOwnParam::rho))  
    {}

  void operator()(AlpsTreeNode*& node) {
    totalLoad_ += pow(fabs(incVal_ - node->getPriority()), rho_);
  }

  double result() const { return totalLoad_; }
};

#endif
//#############################################################################
