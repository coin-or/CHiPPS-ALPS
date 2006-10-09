#include "AlpsLicense.h"

#ifndef AlpsKnowledgeBrokerSerial_h_
#define AlpsKnowledgeBrokerSerial_h_

#include "AlpsEnumProcessT.h"
#include "AlpsKnowledgeBroker.h"

//#############################################################################

class AlpsKnowledgeBrokerSerial : public AlpsKnowledgeBroker {
 private:
  AlpsKnowledgeBrokerSerial(const AlpsKnowledgeBrokerSerial&);
  AlpsKnowledgeBrokerSerial& operator=(const AlpsKnowledgeBrokerSerial&);
 public:
  AlpsKnowledgeBrokerSerial() 
    : AlpsKnowledgeBroker(new AlpsSubTreeWorker)
    {} 
  //  AlpsKnowledgeBrokerSerial(AlpsSubTree* st):
  //  AlpsKnowledgeBroker(st) {}

  //===========================================================================

  /** Explore the subtree rooted at root by calling the dauft implementation. 
      id and comm are useless for serial program. 
  */
  virtual void exploreSubTree(AlpsTreeNode* root,
			      const int id, 
			      int comm = 91 ){
    
    AlpsKnowledgeBroker::exploreSubTree(root);
  
  }
  
  //===========================================================================

  /** Initialize message environment. */
  virtual void initMsgEnv(int argc, char* argv[],
			  int& myRank, int& procSize, 
			  const int comm = 91) {
    
    myRank = AlpsProcessTypeMaster;        // one process is used
    procSize = 1;
  }

  /** Finalize the message environment. Do nothing in serail program. */
  virtual void finializeMsgEnv() {}

  //===========================================================================

};
#endif
