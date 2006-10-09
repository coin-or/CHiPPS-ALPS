#include "AlpsLicense.h"

#include "AlpsKnowledge.h"

//#############################################################################
// Define the static member. Can't simply put it in headfile!
std::map<const char*, const AlpsKnowledge*, AlpsStrLess>*
AlpsKnowledge::decodeMap_ = new std::map<const char*, const AlpsKnowledge*,
					 AlpsStrLess>;

//#############################################################################
