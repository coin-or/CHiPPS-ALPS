#include "AlpsLicense.h"

#ifndef AlpsData_h
#define AlpsData_h

#include <string>

#include "AlpsAllParam.h"
#include "AlpsDataPool.h"
#include "AlpsEnumProcessT.h"
#include "AlpsModel.h"
#include "AlpsParameters.h"


//#############################################################################

/** This class stores the global data that are used by the program. 
    All data members and functions are declared to be static, so that 
    the data member can be accessed without declaring a AlpsData instance. 
    The allocated memory will be deallocated by the oprating system 
    upon the program terminates. */
class AlpsData {

 private:
  /** The id of this process. */
  static int processID_;

  /** The data pool stores global data. */
  static AlpsDataPool* ADPool_;

  /** The model created by user application. */
  static AlpsModel* model_;

  /** The parameter set that is used in Alps. */
  static AlpsAllParam* parameters_;

  /** The parameter set that is used in application. */
  static AlpsParameterSet* userParameters_; 

  /** The parameter file. */
  static std::string paramFile_;

  /** Incumbent value. */
  static double incumbentValue_;

  /** The process id that store the incumbent. */
  static  int incumbentID_;

  /** Indicate whether the incumbent value is updated between two 
      checking point. */
  static bool updateIncumbent_;

  /** The workload of this process. */
  static double workload_;

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

  /** Query the process id of this process. */
  static int getProcessID() { return processID_; }

  /** Query the process id of this process. */
  static void setProcessID(int id) { processID_ = id; }

  /** Query the parameter set. */
  static AlpsAllParam* parameters();

  /** Access the user parameter set.*/
  static AlpsParameterSet* userPars();

  /** Set the user parameter set.*/
  static void setUserPars(AlpsParameterSet* pars) { userParameters_ = pars; }

  /** Query the parameter file. */
  static std::string getParamFile()  { return paramFile_; }

  /** Set the parameter file. */
  static void setParamFile(const std::string pf) 
    {  paramFile_ =  pf; }

  /** Query the incumbent value. */
  static double getIncumbentValue() { return incumbentValue_; }

  /** Set the incumbent value. */
  static void setIncumbentValue(double inc) { incumbentValue_ = inc; }

  /** Query the incumbent process ID. */
  static int getIncumbentID() { return incumbentID_; }

  /** Set the incumbent process ID. */
  static void setIncumbentID(int id) { incumbentID_ = id; }

  /** Query whether the whether the incumbent value is updated between two 
      checking point.*/
  static bool getUpdateIncumbent() { return updateIncumbent_; }

  /** Set whether the incumbent value is updated between two 
      checking point.*/
  static bool setUpdateIncumbent(bool ui) { updateIncumbent_ = ui; }

  /** Query the workload in the process. */
  static double getWorkload() { return workload_; }

  /** Update the workload in the process. */
  static void setWorkload(double wl) { workload_ = wl; }
};

#endif
//#############################################################################
