#include "AlpsLicense.h"

#ifndef AlpsDataPool_h
#define AlpsDataPool_h

#include <string>

#include "AlpsEnumProcessT.h"
#include "AlpsModel.h"
#include "AlpsOwnParams.h"
#include "AlpsParameter.h"


//#############################################################################

/** This class stores the data that are used by the program. 
    All data members and functions are declared to be static, so that 
    the data member can be accessed without declaring instances. 
    The allocated memory will be deallocated by the oprating system 
    upon the program terminates. */
class AlpsDataPool {

 private:
  /** The model created by user application. */
  static AlpsModel* model_;

  /** The parameter set that is used in Alps. */
  static AlpsOwnParams* ownParams_;

  /** The parameter set that is used in application. */
  static AlpsParameterSet* appParams_; 

 
 private:
  AlpsDataPool() {};
  AlpsDataPool(const AlpsDataPool&);
  AlpsDataPool& operator=(const AlpsDataPool&);
  ~AlpsDataPool();

 public:
 
  /** Query the problem model. */
  static AlpsModel* getModel() { return model_; }

  /** Set the problem model. */
  static void setModel(AlpsModel* m) { model_ = m; }

  /** Access the parameter set of Alps. */
  static AlpsOwnParams* getOwnParams();

  /** Set the parameter set for Alps. */
  static void setOwnParams(AlpsOwnParams* pars) { ownParams_ = pars; }

  /** Access the application parameter set.*/
  static AlpsParameterSet* getAppParams();

  /** Set the application parameter set.*/
  static void setAppParams(AlpsParameterSet* pars) { appParams_ = pars; }
};

#endif
//#############################################################################
