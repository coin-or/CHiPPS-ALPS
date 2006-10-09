#include "AlpsLicense.h"

#if defined(NF_DEBGU)
#include <iostream>
#endif

#include "AlpsFunction.h"

//#############################################################################

void 
AlpsParseCommandLine( const int argnum, const char* const * arglist ) {

  // if (argnum == 2) {  // MPI has argnum = 6 !

#if defined(NF_DEBGU)
  std::cout << "argv[1] is " << argv[1] << std::endl; 
#endif

  AlpsData::setParamFile(arglist[1]);
  // Read in the parameters for ALPS
  AlpsData::parameters()->readFromFile(arglist[1]);
  // }
}

//#############################################################################
