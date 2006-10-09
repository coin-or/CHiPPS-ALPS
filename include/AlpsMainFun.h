#include "AlpsLicense.h"

#ifndef AlpsMainFun_h_
#define AlpsMainFun_h_

#include "mpi.h"

//#############################################################################

class AlpsKnowledgeBrokerMPI;
class AlpsSubTree;
class AlpsTreeNode;

//#############################################################################

/** Master receive a root and intialize the search */
void AlpsMasterMain(AlpsKnowledgeBrokerMPI* broker, AlpsSubTree* subtree, 
		    AlpsTreeNode* root, MPI_Comm comm);
/** Workers receive nodes and process the search */

void AlpsWorkerMain(AlpsKnowledgeBrokerMPI* broker, AlpsSubTree* subtree, 
		    MPI_Comm comm);

#endif
//#############################################################################
