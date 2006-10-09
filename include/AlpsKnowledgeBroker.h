#include "AlpsLicense.h"

#ifndef AlpsKnowledgeBroker_h_
#define AlpsKnowledgeBroker_h_

#include <iosfwd>
#include <map>

#include "AlpsEnumProcessT.h"
#include "AlpsKnowledge.h"
#include "AlpsKnowledgePool.h"
#include "AlpsParameter.h"
#include "AlpsSolution.h"
#include "AlpsSubTree.h"

//#############################################################################
/** The base class of knowledge broker class. */
class AlpsKnowledgeBroker {
 private:
  AlpsKnowledgeBroker(const AlpsKnowledgeBroker&);
  AlpsKnowledgeBroker& operator=(const AlpsKnowledgeBroker&);

  /** Stores a master copy of any encodable object for decoding purposes. */
  static std::map<const char*, const AlpsKnowledge*, AlpsStrLess>* decodeMap_;
  
 protected:
  AlpSubTree*  subtree_;

  /** A knowledge pool containing the solutions found. */
  AlpsSolutionPool* solpool_;

  /** The collection of pools managed by the knowledge broker. */
  std::map<AlpsKnowledgeType, AlpsKnowledgePool*>* pools_;
  
 public:
  AlpsKnowledgeBroker()
    : 
    subtree_(0),         /* The derived class will take care of it. */
    solpool_ (new  AlpsSolutionPool)
    {}                  /* Need call setupKnowledgePools() later. */
   
  AlpsKnowledgeBroker(AlpSubTree* st) // Used in serial broker
    : 
    subtree_(st),
    solpool_ (new  AlpsSolutionPool)
    { setupKnowledgePools(); }
   
  virtual ~AlpsKnowledgeBroker() {
    if (solpool_ != 0) {
      delete solpool_, solpool_ = 0;
    }
    if (pools_ != 0) {
      delete pools_;     pools_ = 0;
    }
    if (subtree_ != 0) {
      delete subtree_;   subtree_ = 0;
    }
  }

  //---------------------------------------------------------------------------
  /** @name Funcitons related to exploring subtree. 
   *
   */
  //@{
  /** Every user derived knowledge class must register. 
      The register methods register the decode method of the class so that 
      later on we can decode objects from buffers. Invoking this 
      registration for class <code>foo</code> is a single line:<br>
      <code>foo().registerClass(name, userKnowledge);</code> */
  void registerClass(const char * name, AlpsKnowledge* userKnowledge) {
    (*decodeMap_)[name] = userKnowledge;
  }

  /** This method returns the pointer to an empty object of the registered
      class <code>name</code>. Then the <code>decode()</code> method of that
      object can be used to decode a new object of the same type from the
      buffer. This method will be invoked as follows to decode an object
      whose type is <code>name</code>:<br>
      <code>obj = AlpsKnowledge::decoderObject(name)->decode(buf) </code> */
  static const AlpsKnowledge* decoderObject(const char* name) {
    return (*decodeMap_)[name];
  }
  //@}

  //---------------------------------------------------------------------------
  /** @name Funcitons related to exploring subtree. 
   *  
   */
  //@{
  /** Set a subtree for this broker. */
  inline void setSubTree(AlpSubTree* st) {
    subtree_ = st;
  }
 
  /** Do certain initialization (depends on parallell or serial) for search. */
  virtual void initializeSolver(int argc, 
				char* argv[], 
				AlpsModel& model, 
				AlpsParameterSet& userParams) = 0;

  /** Explore the subtree rooted as a given root. */
  virtual void solve(AlpsTreeNode* root) = 0;  
  //@}

  //---------------------------------------------------------------------------
  /** @name Interface with the knowledge pools 
   *  
   */
  //@{
  /** Set up knowledge pools for this broker. */
  inline void setupKnowledgePools() {
    pools_ = new std::map<AlpsKnowledgeType, AlpsKnowledgePool*>;
    
    pools_->insert( std::pair<AlpsKnowledgeType, AlpsKnowledgePool*>
		    ( ALPS_NODE, subtree_->getNodePool() ) );
    pools_->insert( std::pair<AlpsKnowledgeType, AlpsKnowledgePool*>
		    ( ALPS_SOLUTION, solpool_ ) );
  }

  /** Add a knowledge pool into the Knowledge pools */
  inline void addKnowledgePool(AlpsKnowledgeType kt, AlpsKnowledgePool* kp) {
   AlpsKnowledgePool* akp = static_cast<AlpsKnowledgePool*>(kp);
   pools_->insert
     (std::pair<AlpsKnowledgeType, AlpsKnowledgePool*>(kt, kp));
  }
  
  /** Retrieve a knowledge pool in the Knowledge base */
  inline  AlpsKnowledgePool* getKnowledgePool(AlpsKnowledgeType kt) const { 
    return (*pools_)[kt];       //FIXME: check if exit
  }

  /** Query the number of knowledge in the given type of a knowledge pool. */
  virtual int getNumKnowledges(AlpsKnowledgeType kt) const {
    return getKnowledgePool(kt)->getNumKnowledges();
  }

  /** Query the max number of knowledge can be stored in a given 
      type of knowledge pools.*/
  virtual int getMaxNumKnowledges(AlpsKnowledgeType kt) const {
    return getKnowledgePool(kt)->getMaxNumKnowledges();
  }

  /** Set the max number of knowledge can be stored in a given 
      type o fknowledge pools. */
  virtual void setMaxNumKnowledges(AlpsKnowledgeType kt, int num) {
    getKnowledgePool(kt)->setMaxNumKnowledges(num);
  }

  /** Query whether there are knowledges in the given type of 
      knowledge pools. */
  virtual bool hasKnowledge(AlpsKnowledgeType kt) const {
    return getKnowledgePool(kt)->hasKnowledge();
  }

  /** Get a knowledge, but doesn't remove it from the pool*/
  virtual std::pair<const AlpsKnowledge*, double> 
    getKnowledge(AlpsKnowledgeType kt) const {
    
    return getKnowledgePool(kt)->getKnowledge();
  }

  /** Remove the a knowledge from the given type of knowledge pools.*/
  virtual void popKnowledge(AlpsKnowledgeType kt) {
    getKnowledgePool(kt)->popKnowledge();
  } 

  /** Get the best knowledge in the given type of knowledge pools. */
  virtual std::pair<const AlpsKnowledge*, double> 
    getBestKnowledge(AlpsKnowledgeType kt) const 
    { return getKnowledgePool(kt)->getBestKnowledge(); }

  /** Get all knowledges in the given type of knowledge pools. */
  virtual void getAllKnowledges (AlpsKnowledgeType kt, 
				 std::vector<std::pair<const AlpsKnowledge*, 
				 double> >& kls)  const 
    { getKnowledgePool(kt)->getAllKnowledges(kls);  }

  /** Add a knowledge in the given type of knowledge pools. */
  virtual void addKnowledge(AlpsKnowledgeType kt, 
			     const AlpsKnowledge* kl, 
			     double priority=0) 
    { getKnowledgePool(kt)->addKnowledge(kl, priority); }
  //@}

  /** @name Report the best result
   *  
   */
  //@{
 /** Get the objective value of the incumbent. */
  virtual double getIncumbentValue() const = 0;

  /** Get the objective value of the best solution found. */
  virtual double getBestObjValue() const = 0;

  /** Print out the best solution found and its objective value. */
  virtual void printBestResult(char* outputFile = 0) const = 0;
  //@}

  /** Qeury the global rank of the process. Note: not usefue for serial code.*/
  int getProcRank() const { return 0; }

};
#endif
