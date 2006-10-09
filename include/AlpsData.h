#include "AlpsLicense.h"

#ifndef AlpsData_h
#define AlpsData_h

#include <string>

#include "AlpsDataPool.h"
#include "AlpsParameters.h"
#include "AlpsAllParam.h"
//#include "AlpsString.h"

//#############################################################################

/** All data members are declared to be static, so that the data member can be 
    accessed without declaring a instance. The class stores the global data
    that are used by the program. */
class AlpsData {
 private:
  /** The data pool stores global data. */
  static AlpsDataPool* ADPool_;

  /** The parameter set that is used in Alps. */
  static AlpsParameterSet<AlpsAllParam>* parameters_;
 
  /** The parameter file. */
  static std::string paramFile_;

 protected:
  AlpsData();
  virtual ~AlpsData();

 public:
  /** Access the data pool. */
  static AlpsDataPool* ADPool(); 

  /** Access the parameter set. */
  static AlpsParameterSet<AlpsAllParam>* parameters() { return parameters_; }

  /** Access the parameter file. */
  static std::string GetParamFile() { return paramFile_; }

  /** Set the parameter file. */
  static void setParamFile(const std::string pf) {
    paramFile_ =  pf;
  }

};

#endif
//#############################################################################
