#include "AlpsLicense.h"

#include "iostream"

#include "AlpsData.h"
#include "AlpsEnumProcessT.h"
#include "AlpsKnowledge.h"
#include "AlpsKnowledgeBrokerMPI.h"
#include "AlpsMainFun.h"
#include "AlpsMessageTag.h"
#include "AlpsModel.h"
#include "AlpsSubTree.h"
#include "AlpsTreeNode.h"


//#############################################################################

void 
AlpsMasterMain(AlpsKnowledgeBrokerMPI* broker, AlpsSubTree* subtree, 
AlpsTreeNode* root, MPI_Comm comm) {
  
  int i;
 
  root->setKnowledgeBroker(broker); 

  root->setPriority(0);
  root->setLevel(0);
  root->setIndex(0);
  subtree->setNextIndex(1); // One more than root's index


  int numWorker = 
    AlpsData::parameters()->entry(AlpsAllParam::workerProcessNum);

  //===========================================================================
  // Pack and send model 
  broker->pack( ( (AlpsData::ADPool())->getKnowledge(MODEL))->encode() );
  for (i = 0; i < numWorker; ++i) {
    std::cout << "Master start to send Model to " << i+1 << std::endl; 
    broker->send(i+1, AlpsMsgModel, comm);
    std::cout << "Master finish sending Mode to " << i+1 << std::endl; 
  }
  //===========================================================================
  // Create required number of nodes (or more).
  std::cout << "Master start to create nodes..." << std::endl; 
  dynamic_cast<AlpsSubTreeMaster*>(subtree)->initializeSearch(root);
  std::cout << "Master finish creating nodes." << std::endl; 
  std::cout << "The # of nodes Master generated is " 
	    <<  broker->getNumKnowledges(NODE) << std::endl; 

  // Spawn worker processes  // Not needed for static process management  
 
  //===========================================================================
  if (broker->getNumKnowledges(NODE) == 0) {
    std::cout << "Master failed to generate enought nodes and itself "
	      << "finish search without help from workers" << std::endl;
  }
  else {
    // Send nodes to workers
    const int numNode = broker->getNumKnowledges(NODE);
    int numSent = 0;
    const AlpsTreeNode* node;
    AlpsEncoded* enc;
    while ( numSent < numNode) {
      for (i = 0; i < numWorker; ++i) {
	node = dynamic_cast<const AlpsTreeNode* >
	  (broker->getKnowledge(NODE).first);
	enc = node->encode();
	broker->popKnowledge(NODE);
	broker->pack(enc);
	std::cout << "Master start to send node to " << i+1 <<std::endl; 
	broker->send(i+1, AlpsMsgNode, comm);
	std::cout << "Master finish sending nodes to " << i+1 << std::endl; 
	++numSent;
	if(numSent >= numNode )
	  break;
      }
    }
  }

  // If have solution, send it to nodes?

  // Sent termination tag
  for (i = 0; i < numWorker; ++i)
    broker->send(i+1,  AlpsMsgFinishedAlps, comm);
}

//#############################################################################
void 
AlpsWorkerMain(AlpsKnowledgeBrokerMPI* broker, AlpsSubTree* subtree,
	       MPI_Comm comm) 
{
  // Receive model
  MPI_Status status;
  std::cout << "Worker start to rec Model." << std::endl; 
  broker->receive( AlpsProcessTypeMaster, AlpsMsgModel, comm, &status);
  std::cout << "Worker finish recing Model."<< std::endl; 

  std::cout << "Worker start to unpack model." << std::endl; 
  AlpsEncoded* encodedModel = broker->unpack();
  std::cout << "Worker start to decode model." << std::endl; 
  std::cout << encodedModel->type() << std::endl;
  AlpsModel* model = dynamic_cast<AlpsModel* >( AlpsKnowledge::
	decoderObject(encodedModel->type())->decode(*encodedModel) );
  std::cout << "Worker finish decoding model." << std::endl;
  AlpsData::ADPool()->changeKnowledgeValue(MODEL, model);
  std::cout << "Worker finish adding model." << std::endl;
  // Receive node
  AlpsTreeNode* node;
  while(true) {
    std::cout << "Worker start to recv node." << std::endl; 
    broker->receive(AlpsProcessTypeMaster, MPI_ANY_TAG, comm, &status);
    std::cout << "Worker finish recv node." << std::endl; 
    if (status.MPI_TAG == AlpsMsgFinishedAlps) {
      std::cout << "Worker rec AlpsMsgFinishedAlps... STOP." << std::endl; 
      break;
    }
    std::cout << "Worker start to unpack node." << std::endl; 
    AlpsEncoded* encodedNode = broker->unpack();
    std::cout << "Worker: type() is " << encodedNode->type() << std::endl;
    std::cout << "Worker finish unpacking node." << std::endl; 
    std::cout << "Worker start to decode node." << std::endl; 
   
    const AlpsTreeNode* bugNode = dynamic_cast<const AlpsTreeNode* >(
	   AlpsKnowledge::decoderObject(encodedNode->type()));//Debug
    std::cout <<"Worker: bugNode'a Priority = " 
	      << bugNode->getPriority() << std::endl;

    std::cout << "Worker: finish just decoding node." << std::endl; 

    node = dynamic_cast<AlpsTreeNode* >( AlpsKnowledge::
	decoderObject(encodedNode->type())->decode(*encodedNode) );
    std::cout << "Worker finish docoding node." << std::endl; 
    std::cout << "Worker start to exploreSubTree(node)." << std::endl; 
    // explore node
    static_cast<AlpsKnowledgeBroker*>(broker)->exploreSubTree(node);
    std::cout << "Worker finish exploreSubTree(node)." << std::endl; 
  }
  
  // node = const_cast<AlpsTreeNode*>( dynamic_cast<const AlpsTreeNode* >
  //		      ( subtree->getNodePool()->getKnowledge().first ) );
}
