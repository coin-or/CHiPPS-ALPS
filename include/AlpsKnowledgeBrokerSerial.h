#include "AlpsLicense.h"

#ifndef AlpsKnowledgeBrokerSerial_h_
#define AlpsKnowledgeBrokerSerial_h_

#include "AlpsEnumProcessT.h"
#include "AlpsKnowledgeBroker.h"
#include "AlpsParameters.h"

//#############################################################################

class AlpsKnowledgeBrokerSerial : public AlpsKnowledgeBroker {
 private:
  AlpsKnowledgeBrokerSerial(const AlpsKnowledgeBrokerSerial&);
  AlpsKnowledgeBrokerSerial& operator=(const AlpsKnowledgeBrokerSerial&);

 public:
  AlpsKnowledgeBrokerSerial() 
    // For serial, declaring a broker will create a subtree worker 
    : AlpsKnowledgeBroker(new AlpsSubTreeWorker) 
    {} 

  AlpsKnowledgeBrokerSerial(int argc, char* argv[], 
			    AlpsModel& model, AlpsParameterSet& userParams ) 
    : AlpsKnowledgeBroker(new AlpsSubTreeWorker) 
    { initializeSolver(argc, argv, model, userParams); }

  AlpsKnowledgeBrokerSerial(AlpsSubTree* st)
    : AlpsKnowledgeBroker(st) 
    {}

  //---------------------------------------------------------------------------
  /** @name Report the best solution 
   *  
   */
  //@{
  /** Print out the objective value of the best solution found. */
  virtual void printBestObjValue(std::ostream& os) const {
    os << getBestKnowledge(ALPS_SOLUTION).second;
  }

  /** Print out the best solution found. */
  virtual void printBestSolution(std::ostream& os) const {
      dynamic_cast<AlpsSolution* >( 
	const_cast<AlpsKnowledge* >(getBestKnowledge(ALPS_SOLUTION).first) 
      )->print(os);
  }
  //@}

  void initializeSolver(int argc, 
			char* argv[], 
			AlpsModel& model, 
			AlpsParameterSet& userParams) {

    std::cout << "\nALPS Version 0.6 \n\n";

    // Set param file and read in params
    AlpsData::setParamFile(argv[1]);
    AlpsData::parameters()->readFromFile(argv[1]);
    userParams.readFromFile(argv[1]);
    AlpsData::setUserPars(&userParams);
    
    // Read in model data if NEEDED
    if (AlpsData::parameters()->entry(AlpsAllParam::inputFromFile) == true) {
      const char* dataFile =
	AlpsData::parameters()->entry(AlpsAllParam::dataFile).c_str();
      std::cout << "DATA FILE = " << dataFile << std::endl;
      model.readData(dataFile);
    }
 
    AlpsData::setModel(&model);  // Put model* in AlpsData. MUST do it before
                                 // Register user node class
  }

};
#endif
