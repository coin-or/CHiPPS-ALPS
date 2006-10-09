#include "AlpsLicense.h"

#include "AlpsOwnParam.h"

using std::make_pair;

void AlpsOwnParam::createKeywordList() {

   // Create the list of keywords for parameter file reading
   //--------------------------------------------------------------------------
   // CharPar
   keys_.push_back(make_pair(AlpsString("Alps_inputFromFile"),
			    AlpsParameter(AlpsCharPar, 
					  inputFromFile)));
   keys_.push_back(make_pair(AlpsString("Alps_interClusterBalance"),
			    AlpsParameter(AlpsCharPar, 
					  interClusterBalance)));
   keys_.push_back(make_pair(AlpsString("Alps_intraClusterBalance"),
			    AlpsParameter(AlpsCharPar, 
					  intraClusterBalance)));
   //--------------------------------------------------------------------------
   // BoolArrayPar

   //--------------------------------------------------------------------------
   // IntPar
   keys_.push_back(make_pair(AlpsString("Alps_processNum"),
			     AlpsParameter(AlpsIntPar, 
					   processNum)));
   //
   keys_.push_back(make_pair(AlpsString("Alps_hubNum"),
			     AlpsParameter(AlpsIntPar, 
					   hubNum)));
   //
   keys_.push_back(make_pair(AlpsString("Alps_masterInitNodeNum"),
			     AlpsParameter(AlpsIntPar, 
					   masterInitNodeNum)));
   //
   keys_.push_back(make_pair(AlpsString("Alps_hubInitNodeNum"),
			     AlpsParameter(AlpsIntPar, 
					   hubInitNodeNum)));
   //
   keys_.push_back(make_pair(AlpsString("Alps_unitWorkNodes"),
			     AlpsParameter(AlpsIntPar, 
					   unitWorkNodes)));
   //
   keys_.push_back(make_pair(AlpsString("Alps_maxNumSolustion"),
			     AlpsParameter(AlpsIntPar, 
					   maxNumSolustion)));
   //
   keys_.push_back(make_pair(AlpsString("Alps_smallSize"),
			     AlpsParameter(AlpsIntPar, 
					   smallSize)));
   //
   keys_.push_back(make_pair(AlpsString("Alps_mediumSize"),
			     AlpsParameter(AlpsIntPar, 
					   mediumSize)));
   //
   keys_.push_back(make_pair(AlpsString("Alps_largeSize"),
			     AlpsParameter(AlpsIntPar, 
					   largeSize)));
   //
   keys_.push_back(make_pair(AlpsString("Alps_bufSpare"),
			     AlpsParameter(AlpsIntPar, 
					   bufSpare)));
   //
   keys_.push_back(make_pair(AlpsString("Alps_minNodeNum"),
			     AlpsParameter(AlpsIntPar, 
					   minNodeNum)));
   //
   keys_.push_back(make_pair(AlpsString("Alps_display"),
			     AlpsParameter(AlpsIntPar, 
					   display)));
   //
   keys_.push_back(make_pair(AlpsString("Alps_nodeInterval"),
			     AlpsParameter(AlpsIntPar, 
					   nodeInterval)));
   //
   keys_.push_back(make_pair(AlpsString("Alps_keyNodeNum"),
			     AlpsParameter(AlpsIntPar, 
					   keyNodeNum)));
   //--------------------------------------------------------------------------
   // DoublePar
   keys_.push_back(make_pair(AlpsString("Alps_unitWorkTime"),
			     AlpsParameter(AlpsDoublePar, 
					   unitWorkTime)));
   //
   keys_.push_back(make_pair(AlpsString("Alps_masterBalancePeriod"),
			     AlpsParameter(AlpsDoublePar, 
					   masterBalancePeriod)));

   //
   keys_.push_back(make_pair(AlpsString("Alps_hubReportPeriod"),
			     AlpsParameter(AlpsDoublePar, 
					   hubReportPeriod)));
   //
   keys_.push_back(make_pair(AlpsString("Alps_workerAskPeriod"),
			     AlpsParameter(AlpsDoublePar, 
					   workerAskPeriod)));
   //
   keys_.push_back(make_pair(AlpsString("Alps_rho"),
			     AlpsParameter(AlpsDoublePar, rho)));
   //
   keys_.push_back(make_pair(AlpsString("Alps_zeroLoad"),
			     AlpsParameter(AlpsDoublePar, zeroLoad)));
   //
   keys_.push_back(make_pair(AlpsString("Alps_donorThreshold"),
			     AlpsParameter(AlpsDoublePar, donorThreshold)));
   //
   keys_.push_back(make_pair(AlpsString("Alps_receiverThreshold"),
			     AlpsParameter(AlpsDoublePar, receiverThreshold)));
   
   //--------------------------------------------------------------------------
   // StringPar
   keys_.push_back(make_pair(AlpsString("Alps_dataFile"),
			     AlpsParameter(AlpsStringPar, dataFile)));
}

//#############################################################################

void AlpsOwnParam::setDefaultEntries() {
  //--------------------------------------------------------------------------
  // CharPar
  setEntry(inputFromFile, true);
  setEntry(interClusterBalance, true);
  setEntry(intraClusterBalance, true);
  //--------------------------------------------------------------------------
  // IntPar
  setEntry(processNum, 6);
  setEntry(hubNum, 2);
  setEntry(masterInitNodeNum, 2);
  setEntry(hubInitNodeNum, 2);
  setEntry(unitWorkNodes, 10);
  setEntry(maxNumSolustion, 1);
  setEntry(smallSize, 256);      // 2^8
  setEntry(mediumSize, 4096);    // 2^12
  setEntry(largeSize, 1048576);  // 2^21
  setEntry(bufSpare, 256);
  setEntry(minNodeNum, 1);
  setEntry(display, 1);
  setEntry(nodeInterval, 1000);
  setEntry(keyNodeNum, 10);

  //--------------------------------------------------------------------------
  // DoublePar
  setEntry(unitWorkTime, 0.1);
  setEntry(masterBalancePeriod, 0.05);
  setEntry(hubReportPeriod, 0.5);
  setEntry(workerAskPeriod, 0.5);
  setEntry(rho, 0.0);
  setEntry(zeroLoad, 1.0e-6);
  setEntry(donorThreshold, 1.5);
  setEntry(receiverThreshold, 0.5);
  //--------------------------------------------------------------------------
  // StringPar
  setEntry(dataFile, "");
}
