#include "AlpsLicense.h"

#include "AlpsParameters.h"
#include "AlpsAllParam.h"

using std::make_pair;


void AlpsAllParam::createKeywordList() {

   // Create the list of keywords for parameter file reading
   //--------------------------------------------------------------------------
   // CharPar
   keys.push_back(make_pair(AlpsString("Alps_inputFromFile"),
			    AlpsParameter(AlpsCharPar, 
					  inputFromFile)));
   //--------------------------------------------------------------------------
   // BoolArrayPar

   //--------------------------------------------------------------------------
   // IntPar
   keys.push_back(make_pair(AlpsString("Alps_processNum"),
			    AlpsParameter(AlpsIntPar, 
					  processNum)));
   //
   keys.push_back(make_pair(AlpsString("Alps_workerProcessNum"),
			    AlpsParameter(AlpsIntPar, 
					  workerProcessNum)));
   //
   keys.push_back(make_pair(AlpsString("Alps_initialNodeNum"),
			    AlpsParameter(AlpsIntPar, 
					  initialNodeNum)));
   //
   keys.push_back(make_pair(AlpsString("Alps_typeSize"),
			    AlpsParameter(AlpsIntPar, 
					  typeSize)));
   //
   keys.push_back(make_pair(AlpsString("Alps_unitWorkNode"),
			    AlpsParameter(AlpsIntPar, 
					  unitWorkNodes)));
   //
   keys.push_back(make_pair(AlpsString("Alps_maxNumSolustion"),
			    AlpsParameter(AlpsIntPar, 
					  maxNumSolustion)));
   //
   keys.push_back(make_pair(AlpsString("Alps_smallSize"),
			    AlpsParameter(AlpsIntPar, 
					  smallSize)));
   //
   keys.push_back(make_pair(AlpsString("Alps_mediumSize"),
			    AlpsParameter(AlpsIntPar, 
					  mediumSize)));
   //
   keys.push_back(make_pair(AlpsString("Alps_largeSize"),
			    AlpsParameter(AlpsIntPar, 
					  largeSize)));
   //
   keys.push_back(make_pair(AlpsString("Alps_bufSpare"),
			    AlpsParameter(AlpsIntPar, 
					  bufSpare)));
   //
   keys.push_back(make_pair(AlpsString("Alps_minNodeNum"),
			    AlpsParameter(AlpsIntPar, 
					  minNodeNum)));
   //--------------------------------------------------------------------------
   // DoublePar
   keys.push_back(make_pair(AlpsString("Alps_unitWorkTime"),
			    AlpsParameter(AlpsDoublePar, 
					  unitWorkTime)));
   //
   keys.push_back(make_pair(AlpsString("Alps_balancePeriod"),
			    AlpsParameter(AlpsDoublePar, 
					  balancePeriod)));
   //
   keys.push_back(make_pair(AlpsString("Alps_rho"),
			    AlpsParameter(AlpsDoublePar, rho)));
   //
   keys.push_back(make_pair(AlpsString("Alps_zeroLoad"),
			    AlpsParameter(AlpsDoublePar, zeroLoad)));
   //
   keys.push_back(make_pair(AlpsString("Alps_donorThreshold"),
			    AlpsParameter(AlpsDoublePar, donorThreshold)));
   //
   keys.push_back(make_pair(AlpsString("Alps_receiverThreshold"),
			    AlpsParameter(AlpsDoublePar, receiverThreshold)));
   
   //--------------------------------------------------------------------------
   // StringPar
   keys.push_back(make_pair(AlpsString("Alps_dataFile"),
			    AlpsParameter(AlpsStringPar, dataFile)));
}

//#############################################################################

void AlpsAllParam::setDefaultEntries() {
  //--------------------------------------------------------------------------
  // CharPar
  setEntry(inputFromFile, true);
  //--------------------------------------------------------------------------
  // IntPar
  setEntry(processNum, 3);
  setEntry(workerProcessNum, 2);
  setEntry(initialNodeNum, 4);
  setEntry(typeSize, 32);
  setEntry(unitWorkNodes, 1);
  setEntry(maxNumSolustion, 1);
  setEntry(smallSize, 256);      // 2^8
  setEntry(mediumSize, 4096);    // 2^12
  setEntry(largeSize, 1048576);  // 2^20
  setEntry(bufSpare, 256);
  setEntry(minNodeNum, 1);
  //--------------------------------------------------------------------------
  // DoublePar
  setEntry(unitWorkTime, 0.1);
  setEntry(balancePeriod, 0.1);
  setEntry(rho, 0.0);
  setEntry(zeroLoad, 1.0e-6);
  setEntry(donorThreshold, 1.7);
  setEntry(receiverThreshold, 0.2);
  //--------------------------------------------------------------------------
  // StringPar
  setEntry(dataFile, "");
}
