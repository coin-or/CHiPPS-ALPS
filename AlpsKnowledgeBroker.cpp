#include "AlpsLicense.h"

#include "AlpsKnowledgeBroker.h"


//#############################################################################
// Initialize static member. 
std::map<const char*, const AlpsKnowledge*, AlpsStrLess>*
AlpsKnowledgeBroker::decodeMap_ = new std::map<const char*, 
  const AlpsKnowledge*, AlpsStrLess>;

//#############################################################################
