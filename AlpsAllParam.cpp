#include "AlpsLicense.h"

#include "AlpsParameters.h"
#include "AlpsAllParam.h"

using std::make_pair;

template <>
void AlpsParameterSet<AlpsAllParam>::createKeywordList() {

   // Create the list of keywords for parameter file reading
   //--------------------------------------------------------------------------
   // CharPar

   //--------------------------------------------------------------------------
   // BoolArrayPar

   //--------------------------------------------------------------------------
   // IntPar
   keys.push_back(make_pair(AlpsString("Alps_workerProcessNum"),
			    AlpsParameter(AlpsIntPar, 
					  workerProcessNum)));
   //
   keys.push_back(make_pair(AlpsString("Alps_intialNodeNum"),
			    AlpsParameter(AlpsIntPar, 
					  intialNodeNum)));
   //
   keys.push_back(make_pair(AlpsString("Alps_typeSize"),
			    AlpsParameter(AlpsIntPar, 
					  typeSize)));
   //--------------------------------------------------------------------------
   // DoublePar
   
   //--------------------------------------------------------------------------
   // StringPar

}

//#############################################################################

template <>
void AlpsParameterSet<AlpsAllParam>::setDefaultEntries() {
   //--------------------------------------------------------------------------
   // CharPar

   //--------------------------------------------------------------------------
   // IntPar
  setEntry(workerProcessNum, 2);
  setEntry(intialNodeNum, 4);
  setEntry(typeSize, 32);
  //--------------------------------------------------------------------------
  // DoublePar
  
  //--------------------------------------------------------------------------
  // StringPar
  
}
