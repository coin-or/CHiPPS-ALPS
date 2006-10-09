#include "AlpsLicense.h"

#include <cstring>

#include "CoinError.hpp"

#include "AlpsData.h"
#include "AlpsKnowledgeBrokerMPI.h"
#include "AlpsMainFun.h"
#include "AlpsMessageTag.h"


//#############################################################################

void 
AlpsKnowledgeBrokerMPI::pack(AlpsEncoded* enc) 
{
  position_          = 0;       // To record current position in buf_.
  int typeSize = strlen(enc->type());
  int repSize  = enc->size();
  size_              = typeSize + repSize + 2*sizeof(int) + 4;
 
  if(buf_ != NULL)
    delete [] buf_;
  buf_               = new char[size_];

  // Pack typeSize, repSize, type_, representation_ of enc
  MPI_Pack(&typeSize, 1, MPI_INT, buf_,  size_, &position_, MPI_COMM_WORLD);
  MPI_Pack(&repSize, 1, MPI_INT, buf_,  size_, &position_, MPI_COMM_WORLD);
  MPI_Pack(const_cast<char*>(enc->type()), typeSize, MPI_CHAR, 
	   buf_, size_, &position_, MPI_COMM_WORLD); 
  MPI_Pack(const_cast<char*>(enc->representation()), repSize, MPI_CHAR, 
	   buf_, size_, &position_, MPI_COMM_WORLD); 
}

//#############################################################################
// Done
AlpsEncoded* 
AlpsKnowledgeBrokerMPI::unpack() 
{
  position_ = 0;

  int typeSize;
  int repSize; 

  // Unpack typeSize, repSize, type and rep from buf_
  MPI_Unpack(buf_, size_, &position_, &typeSize, 1, MPI_INT, MPI_COMM_WORLD);
  std::cout << "Worker: typeSize is " << typeSize 
	    << ";\tsize_ = "<< size_ <<  std::endl;
  MPI_Unpack(buf_, size_, &position_, &repSize, 1, MPI_INT, MPI_COMM_WORLD);
  
  char* type = new char [typeSize];
  char* rep = new char[repSize];

  MPI_Unpack(buf_, size_, &position_, type, typeSize, MPI_CHAR, 
	     MPI_COMM_WORLD);
 
  // NEED this to terminate cstring so that don't read in random char
  // Strange here 12KnapTreeNode is of 14 length, but setting 
  // type[14+1] = '\0' won't work.
  *(type+typeSize) = '\0';        

  std::cout << "Worker: repSize is " << repSize << std::endl;
  std::cout << "Worker: type is " << type << std::endl;
 
  MPI_Unpack(buf_, size_, &position_, rep, repSize, MPI_CHAR, MPI_COMM_WORLD);
  rep[repSize+1] = '\0';

  std::cout << "Worker: rep is " << rep << std::endl;

  // Return a enc of required knowledge type
  return new AlpsEncoded(type, repSize, rep );
}

//#############################################################################
// Done
void
AlpsKnowledgeBrokerMPI::receive(int sender, int tag, 
				MPI_Comm comm, MPI_Status* status) 
{
  size_ = 0;
  // First recv the buf size
  MPI_Recv(&size_, 1, MPI_INT, sender, tag, comm, status);
  if (status->MPI_TAG == AlpsMsgFinishedAlps)
    return;

  // Secondly, allocate memory for buf_
  if (buf_ != NULL)
    delete [] buf_;
  buf_ = new char [size_];

  // Third, receive MPI packed data and put in buf_
  MPI_Recv(buf_, size_, MPI_PACKED, sender, tag, comm, status);
}

//#############################################################################
// Done
void 
AlpsKnowledgeBrokerMPI::send(const int target, const int tag, MPI_Comm comm) 
{
  // If terminate ALPS
  if (tag == AlpsMsgFinishedAlps) {
    MPI_Send(buf_, 0, MPI_PACKED, target, tag, comm); 
    return;
  }

  // If packing successfully, send it to target
  if (size_ > 0) {
    // Need send size_ so that receiver can allocate memory for recv buf
    MPI_Send(&size_, 1, MPI_INT, target, tag, comm);
    MPI_Send(buf_, position_, MPI_PACKED, target, tag, comm); 
  }
  else
    throw CoinError("Msg size is <= 0", "send", "AlpsKnowledgeBrokerMPI");
}

//#############################################################################

void 
AlpsKnowledgeBrokerMPI::exploreSubTree(AlpsTreeNode* root, 
				       const int id, int comm)
{
  if(id == AlpsProcessTypeMaster) 
    AlpsMasterMain(this, subtree_, root, comm);
  else
    AlpsWorkerMain(this, subtree_, comm);
}

void 
AlpsKnowledgeBrokerMPI::initMsgEnv(int argc, char* argv[], 
				   int& myRank, int& procSize, 
				   const int comm = 91)
{
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(comm, &myRank);
  MPI_Comm_size(comm, &procSize);
}

//#############################################################################

void
AlpsKnowledgeBrokerMPI::finializeMsgEnv() 
{
  MPI_Finalize();
}

//#############################################################################
