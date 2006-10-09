#include "AlpsLicense.h"

#include "AlpsData.h"

//#############################################################################
// Allocate memory for the static member. Can't simply put it in headfile!
AlpsDataPool* AlpsData::ADPool_ = 0;

AlpsParameterSet<AlpsAllParam>* AlpsData::parameters_ =
                                new AlpsParameterSet<AlpsAllParam>;
AlpsString AlpsData::paramFile_;

//#############################################################################

AlpsDataPool*  AlpsData::ADPool() {
  if ( ! ADPool_ )
    ADPool_ = new AlpsDataPool;
  return ADPool_;
}
//#############################################################################
