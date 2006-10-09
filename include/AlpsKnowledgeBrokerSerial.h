#include "AlpsLicense.h"

#ifndef AlpsKnowledgeBrokerSerial_h_
#define AlpsKnowledgeBrokerSerial_h_

#include "AlpsKnowledgeBroker.h"
#include "AlpsEnumProcessT.h"

//#############################################################################

class AlpsKnowledgeBrokerSerial : public AlpsKnowledgeBroker {
 private:
  AlpsKnowledgeBrokerSerial(const AlpsKnowledgeBrokerSerial&);
  AlpsKnowledgeBrokerSerial& operator=(const AlpsKnowledgeBrokerSerial&);
 public:
  AlpsKnowledgeBrokerSerial() 
    : AlpsKnowledgeBroker(new AlpsSubTreeWorker)
    {} 
  // AlpsKnowledgeBrokerSerial(AlpsSubTree* st):
  //  AlpsKnowledgeBroker(st) {}
    
  void initMsgEnv(int argc, char* argv[],
		  int& myRank, int& procSize, const int comm = 91) {
    myRank = AlpsProcessTypeMaster;        // one process is used
    procSize = 1;
  }

  void finializeMsgEnv() {}                // Do nothing in serial code
};
#endif
