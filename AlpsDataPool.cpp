#include "AlpsLicense.h"

#include "AlpsDataPool.h"

//#############################################################################
// Initialization of static members. 

AlpsModel* AlpsDataPool::model_ = 0;

AlpsOwnParams* AlpsDataPool::ownParams_ = 0;

AlpsParameterSet* AlpsDataPool::appParams_ =0; 

//-----------------------------------------------------------------------------

AlpsOwnParams* AlpsDataPool::getOwnParams() {
  if ( ! ownParams_ )
    ownParams_ =  new AlpsOwnParams;
  return ownParams_;
}

AlpsParameterSet* AlpsDataPool::getAppParams() {
  return appParams_;
}

//#############################################################################
