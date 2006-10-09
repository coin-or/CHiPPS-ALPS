#include "AlpsLicense.h"

#include "AlpsData.h"

//#############################################################################
// Initialization of static members. 

AlpsDataPool* AlpsData::ADPool_ = 0;

AlpsModel* AlpsData::model_ = 0;

AlpsOwnParam* AlpsData::parameters_ = 0;

AlpsParameterSet* AlpsData::userParameters_ =0; 

//-----------------------------------------------------------------------------

AlpsDataPool* AlpsData::ADPool() {
  if ( ! ADPool_ )
    ADPool_ = new AlpsDataPool;
  return ADPool_;
}

AlpsOwnParam* AlpsData::parameters() {
  if ( ! parameters_ )
    parameters_ =  new AlpsOwnParam;
  return parameters_;
}

AlpsParameterSet* AlpsData::userPars() {
  return userParameters_;
}


//#############################################################################
