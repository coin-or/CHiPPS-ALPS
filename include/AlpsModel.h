#include "AlpsLicense.h"

#ifndef AlpsModel_h
#define AlpsModel_h

#include <string>

#include "AlpsKnowledge.h"

class AlpsModel : public AlpsKnowledge { // why don't need headfile?? NEED
 private:
  AlpsModel(const AlpsModel&);
  AlpsModel& operator=(const AlpsModel&);

 protected:
  std::string dataFile_;        /** The file has model data. */
  
 public:
  AlpsModel() {}
  virtual ~AlpsModel() {}

  /** Get the input file. */
  inline std::string getDataFile() const { return dataFile_; }

  /** Set the data file. */
  inline void setDataFile(std::string infile) { dataFile_ = infile; }

  /** Read in the problem data. Default: do nothing */
  virtual void readData(const char* ) = 0; 

};
#endif
