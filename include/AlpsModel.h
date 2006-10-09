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
  std::string infile_;        /** The file has model data. */
  
 public:
  AlpsModel() {}
  virtual ~AlpsModel() {}

  /** Get the input file. */
  inline std::string getInfile() const { return infile_; }

  /** Set the infile. */
  inline void setInfile(std::string infile) { infile_ = infile; }

  /** Read in the problem data. Default: do nothing */
  virtual void readData() {}; 

  /** The method that encodes the solution into a buffer. */
  virtual AlpsEncoded* encode() const = 0;//{ return 0; }
   
  /** The method that decodes the solution from a buffer. */
  // virtual AlpsKnowledge* decode(const AlpsEncoded&) const { return 0; }
  virtual AlpsKnowledge* decode(AlpsEncoded&) const =0;// { return 0; }
};
#endif
