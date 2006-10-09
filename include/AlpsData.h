#include "AlpsLicense.h"

#ifndef AlpsData_h
#define AlpsData_h

#include <string>

#include "AlpsDataPool.h"
#include "AlpsEnumProcessT.h"
#include "AlpsModel.h"
#include "AlpsOwnParam.h"
#include "AlpsParameter.h"


//#############################################################################

/** This class stores the global data that are used by the program. 
    All data members and functions are declared to be static, so that 
    the data member can be accessed without declaring a AlpsData instance. 
    The allocated memory will be deallocated by the oprating system 
    upon the program terminates. */
class AlpsData {

 private:
  /** The data pool stores global data. */
  static AlpsDataPool* ADPool_;

  /** The model created by user application. */
  static AlpsModel* model_;

  /** The parameter set that is used in Alps. */
  static AlpsOwnParam* parameters_;

  /** The parameter set that is used in application. */
  static AlpsParameterSet* userParameters_; 

 
 private:
  AlpsData();
  AlpsData(const AlpsData&);
  AlpsData& operator=(const AlpsData&);
  ~AlpsData();

 public:
  /** Query the data pool. */   // Do we still need AlpsDataPool?
  static AlpsDataPool* ADPool(); 
 
  /** Query the problem model. */
  static AlpsModel* getModel() { return model_; }

  /** Query the problem model. */
  static void setModel(AlpsModel* m) { model_ = m; }

  /** Query the parameter set. */
  static AlpsOwnParam* parameters();

  /** Access the user parameter set.*/
  static AlpsParameterSet* userPars();

  /** Set the user parameter set.*/
  static void setUserPars(AlpsParameterSet* pars) { userParameters_ = pars; }
};

#endif
//#############################################################################
