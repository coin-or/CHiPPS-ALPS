#include "AlpsLicense.h"

#include <cfloat>     // For DBL_MAX

#include "AlpsData.h"

//#############################################################################
// Initialization of static members. 

int AlpsData::processID_;

AlpsDataPool* AlpsData::ADPool_ = 0;

AlpsModel* AlpsData::model_ = 0;

AlpsAllParam* AlpsData::parameters_ = 0;

AlpsParameterSet* AlpsData::userParameters_ =0; 

std::string AlpsData::paramFile_;

double AlpsData::incumbentValue_ = DBL_MAX;        // Assume minimization

int AlpsData::incumbentID_;

bool AlpsData::updateIncumbent_ = false;

double AlpsData::workload_ = 0.0;

//-----------------------------------------------------------------------------

AlpsDataPool* AlpsData::ADPool() {
  if ( ! ADPool_ )
    ADPool_ = new AlpsDataPool;
  return ADPool_;
}

AlpsAllParam* AlpsData::parameters() {
  if ( ! parameters_ )
    parameters_ =  new AlpsAllParam;
  return parameters_;
}

AlpsParameterSet* AlpsData::userPars() {
  return userParameters_;
}


//#############################################################################
