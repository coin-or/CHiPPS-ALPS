#include "AlpsLicense.h"

#include "AlpsKnowledge.h"

//#############################################################################
// Initialize static member. 
std::map<const char*, const AlpsKnowledge*, AlpsStrLess>*
AlpsKnowledge::decodeMap_ = new std::map<const char*, const AlpsKnowledge*, 
  AlpsStrLess>;

//#############################################################################
