#include "AlpsLicense.h"

#ifndef AlpsKnowledgeBrokerSerial_h_
#define AlpsKnowledgeBrokerSerial_h_

#include "AlpsEnumProcessT.h"
#include "AlpsKnowledgeBroker.h"
#include "AlpsParameter.h"

//#############################################################################

class AlpsKnowledgeBrokerSerial : public AlpsKnowledgeBroker {
 private:
  AlpsKnowledgeBrokerSerial(const AlpsKnowledgeBrokerSerial&);
  AlpsKnowledgeBrokerSerial& operator=(const AlpsKnowledgeBrokerSerial&);

 public:
  AlpsKnowledgeBrokerSerial() 
    // For serial, declaring a broker will create a subtree worker 
    : AlpsKnowledgeBroker(new AlpSubTree) 
    {} 

  AlpsKnowledgeBrokerSerial(int argc, char* argv[], 
			    AlpsModel& model, AlpsParameterSet& userParams ) 
    : AlpsKnowledgeBroker(new AlpSubTree) 
    { initializeSolver(argc, argv, model, userParams); }


  //---------------------------------------------------------------------------
  /** @name Report the best solution 
   *  
   */
  //@{
  /** Get the objective value of the incumbent. */
  virtual double getIncumbentValue() const {
    return getBestObjValue();
  }

  /** Get the objective value of the best solution found. */
  virtual double getBestObjValue() const {
    
    if (AlpsKnowledgeBroker::hasKnowledge(ALPS_SOLUTION))
      return getBestKnowledge(ALPS_SOLUTION).second;
    else 
      return 0;  // FIXME
  }

  /** Output the best solution found to a file or std::out. */
  virtual void printBestSolution(char* outputFile = 0) const {
    if (outputFile != 0) {   // Write to outputFile
      std::ofstream os("outputFile");
      dynamic_cast<AlpsSolution* >( 
	   const_cast<AlpsKnowledge* >(getBestKnowledge(ALPS_SOLUTION).first) 
	   )->print(os);
    }
    else {                   // Write to std::cout
     dynamic_cast<AlpsSolution* >( 
	const_cast<AlpsKnowledge* >(getBestKnowledge(ALPS_SOLUTION).first) 
	)->print(std::cout);
    }
  }

  /** Output the best solution found and its objective value to a file 
      or std::out. */
  virtual void printBestResult(char* outputFile = 0) const {
    if (outputFile != 0) {                  // Write to outputFile
      std::ofstream os(outputFile);
      os << "Objective value = " << getBestObjValue();
      os << std::endl;
      dynamic_cast<AlpsSolution* >( 
	   const_cast<AlpsKnowledge* >(getBestKnowledge(ALPS_SOLUTION).first) 
	   )->print(os);
    }
    else {                                  // Write to std::cout
      std::cout << "Objective value = " << getBestObjValue();
      std::cout << std::endl;
      dynamic_cast<AlpsSolution* >( 
	const_cast<AlpsKnowledge* >(getBestKnowledge(ALPS_SOLUTION).first) 
	)->print(std::cout);
    }
  }
  //@}

  /** Reading in Alps and user parameter sets, and read in model data. */
  void initializeSolver(int argc, 
			char* argv[], 
			AlpsModel& model, 
			AlpsParameterSet& userParams) {

    std::cout << "\nALPS Version 0.6 \n\n";

    // Read in params
    AlpsDataPool::getOwnParams()->readFromFile(argv[1]);
    userParams.readFromFile(argv[1]);
    AlpsDataPool::setAppParams(&userParams);
    
    // Read in model data if NEEDED
    if ( AlpsDataPool::getOwnParams()->entry(AlpsOwnParam::inputFromFile) ) {
      const char* dataFile =
	AlpsDataPool::getOwnParams()->entry(AlpsOwnParam::dataFile).c_str();
      std::cout << "DATA FILE = " << dataFile << std::endl;
      model.readData(dataFile);
    }
 
    // Put model* in AlpsDataPool. MUST do it before register user node class
    AlpsDataPool::setModel(&model);  
  }


  /** Explore the subtree rooted as a given root. */
  inline void solve(AlpsTreeNode* root) {
    root->setKnowledgeBroker(this);
    root->setPriority(0);
    root->setLevel(0);
    root->setIndex(0);
    subtree_->setNextIndex(1);        // one more than root's index
    subtree_->exploreSubTree(root);
  }

};
#endif
