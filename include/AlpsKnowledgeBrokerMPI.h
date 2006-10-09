#include "AlpsLicense.h"

#ifndef AlpsKnowledgeBrokerMPI_h_
#define AlpsKnowledgeBrokerMPI_h_

#include <cmath>
#include <iosfwd>

#include "mpi.h"

#include "AlpsEnumProcessT.h"
#include "AlpsKnowledge.h"
#include "AlpsKnowledgeBroker.h"
#include "AlpsParameters.h"


//#############################################################################

class AlpsKnowledgeBrokerMPI : public AlpsKnowledgeBroker {
 private:
  AlpsKnowledgeBrokerMPI(const AlpsKnowledgeBrokerMPI&);
  AlpsKnowledgeBrokerMPI& operator=(const AlpsKnowledgeBrokerMPI&);
 
  /** Buffer used to send or receive messages. */
  char* buf_;
  /** Used in MPI_Pack and MPI_Unpack to record current pos in buffer. */
  int position_;
  /** Size of buf_.*/
  int size_;

 private:
  /** Ask a worker to share it workload with the specified receiver worker. */
  void askShareWork(int donorID, int receiverID, 
		    double receiverWorkload, MPI_Comm comm);

  /** Calculate the workload in this process. */
  double calWorkload();

  /** Hub balance the workload of workers. */
  void  hubBalanceLoad(double totalLoad, 
		       std::vector<std::pair<double, int> >& loadIDVector);

  /** Worker send incumbent value and process id to its hub. */
  void sendIncumbentToHub(double incVal, int incID, MPI_Comm comm);

  /** A worker share it workload with the specified worker. */
  void shareWork(char* buf, MPI_Status*,  MPI_Comm comm);

  /** unpack the incumbent value, then store it and the id of the process 
      having the incumbent in AlpsData. */
  void unpackSetIncumbent(char* buf, MPI_Status* status, MPI_Comm comm);

  /** Calculate workload and do termination check. Return 0 means
      the program should be terminate since there is no work anymore, 
      otherwise, continue processing. */
  int takeSurvey(MPI_Comm comm);

 public:
  AlpsKnowledgeBrokerMPI()
    : 
    AlpsKnowledgeBroker(),     // Base class constructor
    buf_(0),
    position_(0),
    size_(0)
    {}
 
  AlpsKnowledgeBrokerMPI(int argc, char* argv[], 
			 AlpsModel& model, AlpsParameterSet& userParams )
    :
    AlpsKnowledgeBroker(),     // Base class constructor
    buf_(0),
    position_(0),
    size_(0)
    { initializeSolver(argc, argv, model, userParams); }


  AlpsKnowledgeBrokerMPI(AlpsSubTree* st)
    : 
    AlpsKnowledgeBroker(st), 
    buf_(0),
    position_(0),
    size_(0)
    {}

  ~AlpsKnowledgeBrokerMPI() { 
    if (buf_ != 0) {
      delete buf_; buf_ = 0;
    }
    finializeMsgEnv(); 
  }

 //---------------------------------------------------------------------------

  void initializeSolver(int argc, 
			char* argv[], 
			AlpsModel& model, 
			AlpsParameterSet& userParams) {
    // Init msg env
    int rank, procSize;
    initMsgEnv(argc, argv, rank, procSize);

    if (rank == AlpsProcessTypeHub) {
      std::cout << "\nALPS Version 0.6 \n\n";
      std::cout << procSize << " processes are launched. " << std::endl;

      // Set param file and read in params
      AlpsData::setParamFile(argv[1]);
      AlpsData::parameters()->readFromFile(argv[1]);
      userParams.readFromFile(argv[1]);
 
      // Read in model data if NEEDED
      if (AlpsData::parameters()->entry(AlpsAllParam::inputFromFile) == true) {
	const char* dataFile = 
	  AlpsData::parameters()->entry(AlpsAllParam::dataFile).c_str();
	std::cout << "DATA FILE = " << dataFile << std::endl;
	model.readData(dataFile);
      }
    }

    AlpsData::setUserPars(&userParams);
    AlpsData::setModel(&model);  // Put model* in AlpsData. MUST do it before
                                 // Register user node class
  }

  /** Hub generates nodes and sends them to workers in Round-Robin way.
      Hub periodically do load balancing and termination check. */
  void hubMain(AlpsTreeNode* root, MPI_Comm comm);

  /** Worker first receive nodes, then start to explore them. Worker also
      peroidically check message and process message. */
  void workerMain(MPI_Comm comm);

  /** Explore the search tree rooted at root. id is the process id. 
      comm is the MPI communicator, and default is MPI_COMM_WORLD.  */
  void solve(AlpsTreeNode* root, MPI_Comm comm = MPI_COMM_WORLD);

  //---------------------------------------------------------------------------

  /** @name Message passing member functions
   *
   */
  //@{
  /** Pack an AlpsEncoded instance into buf. */
  void packEncoded(AlpsEncoded* enc);
 
  /** Unpack the broker buffer into an AlpsEncoded instance. */
  AlpsEncoded* unpackEncoded();
  
  /** Unpack the given buffer into an AlpsEncoded instance. */
  AlpsEncoded* unpackEncoded(char* buf);

  /** Receive the message and put it in buffer. Assume that the buffer 
      is large enough.*/
  void receiveBuf(int sender, int tag, MPI_Comm comm, MPI_Status* status);

  /** Receive the size of buffer, allocate memory for buffer, then 
      receive the message and put it in buffer. */
  void receiveSizeBuf(int sender, int tag, MPI_Comm comm, MPI_Status* status);

  /** Receive the the contend of a node from the sender process, 
      and unpack it. Assume that buf is large enough. */
  void receiveNode(char* buf, int sender, MPI_Comm comm, MPI_Status* status);

  /** Receive the size and the contend of a node from the sender process, 
      then unpack it. */
  void receiveSizeNode(int sender, MPI_Comm comm, MPI_Status* status);

  /** Send the content of buffer to the target process. Assume that 
      the buffer is large enough. */
  void sendBuf(const int target, const int tag, MPI_Comm comm);

  /** Send the size of buffer and the content of buffer to the 
      target process. */
  void sendSizeBuf(const int target, const int tag, MPI_Comm comm);

  /** Pop a node from node pool and send it to the target process. 
      Assume that the buffer is large enough. */
  void sendNode(const int target, MPI_Comm comm);

  /** Pop a node from node pool, pack it, then send the size and the
      contend of it to the target process. */
  void sendSizeNode(const int target, MPI_Comm comm);

  /** Broadcast the model from source to other processes. */
  void broadcastModel(const int id, const int source, MPI_Comm comm);

  /** Broadcast parameters from source to other processes. */
  void broadcastParams(const int id, const int source, MPI_Comm comm);

  /** Broadcast the best solution value. */
  void broadcastIncumbent(MPI_Comm comm);

  /** Gather the best solution from the process having it to destination. */
  void gatherBestSolution(int destination, MPI_Comm comm);

  /** Send finish initialization signal to the target process. */
  void sendFinishInit(const int target, MPI_Comm comm);

  /** Initialize message environment by calling MPI_init(). */
  void initMsgEnv(int argc, char* argv[], 
		  int& myRank, int& procSize);

  /** Finalize the message environment. */
  void finializeMsgEnv();
  //@}

  //---------------------------------------------------------------------------

  /** @name Report the best solution 
   *  
   */
  //@{
  /** Print out the objective value of the best solution found. */
  virtual void printBestObjValue(std::ostream& os) const {
    const int rank = AlpsData::getProcessID();
    if (rank == AlpsProcessTypeHub) {
      os << getBestKnowledge(ALPS_SOLUTION).second;   
    }
  }

  /** Print out the best solution found. */
  virtual void printBestSolution(std::ostream& os) const {
    const int rank = AlpsData::getProcessID();
    if (rank == AlpsProcessTypeHub) {
      dynamic_cast<AlpsSolution* >( 
	const_cast<AlpsKnowledge* >(getBestKnowledge(ALPS_SOLUTION).first) 
      )->print(os);
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
  TotalWorkload() 
    : 
    totalLoad_(0.0), 
    incVal_(AlpsData::getIncumbentValue()), 
    rho_(AlpsData::parameters()->entry(AlpsAllParam::rho))  
    {}

  void operator()(AlpsTreeNode*& node) {
    totalLoad_ += pow(fabs(incVal_ - node->getPriority()), rho_);
  }

  double result() const { return totalLoad_; }
};

#endif
//#############################################################################
