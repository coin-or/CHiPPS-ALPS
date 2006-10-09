#include "AlpsLicense.h"

#ifndef AlpsKnowledgeBroker_h_
#define AlpsKnowledgeBroker_h_

#include <map>

#include "AlpsEnumProcessT.h"
#include "AlpsKnowledge.h"
#include "AlpsKnowledgePool.h"
#include "AlpsSolution.h"
#include "AlpsSubTree.h"

//#############################################################################

class AlpsKnowledgeBroker {
 private:
  AlpsKnowledgeBroker(const AlpsKnowledgeBroker&);
  AlpsKnowledgeBroker& operator=(const AlpsKnowledgeBroker&);

 protected:
  AlpsSubTreeWorker*  subtree_;

  /** The collection of pools managed by the knowledge broker. */
  std::map<AlpsKnowledgeType, AlpsKnowledgePool*>* pools_;
  
 public:
  AlpsKnowledgeBroker()
    : 
    subtree_(0),   /*Its derived class will allocate memory.*/
    pools_(new std::map<AlpsKnowledgeType, AlpsKnowledgePool*>)
    {
      pools_->insert( std::pair<AlpsKnowledgeType, AlpsKnowledgePool*>
		      ( NODE, subtree_->getNodePool() ) );
      pools_->insert( std::pair<AlpsKnowledgeType, AlpsKnowledgePool*>
		      ( SOLUTION, subtree_->getSolutionPool() ) );
    }
   
  AlpsKnowledgeBroker(AlpsSubTreeWorker* st)
    : 
    subtree_(st),
    pools_(new std::map<AlpsKnowledgeType, AlpsKnowledgePool*>)
    {
      pools_->insert( std::pair<AlpsKnowledgeType, AlpsKnowledgePool*>
		      ( NODE, subtree_->getNodePool() ) );
      pools_->insert( std::pair<AlpsKnowledgeType, AlpsKnowledgePool*>
		      ( SOLUTION, subtree_->getSolutionPool() ) );
    }
   
  virtual ~AlpsKnowledgeBroker() {
    if (pools_ != 0)
      delete pools_;
    if (subtree_ != 0)
      delete subtree_;
  }

  //===========================================================================

  /** Function to interface with the subtree (default implementation). */
  void exploreSubTree(AlpsTreeNode* root) {
    root->setKnowledgeBroker(this);
    root->setPriority(0);
    root->setLevel(0);
    root->setIndex(0);
    subtree_->setNextIndex(1);        // one more than root's index
    subtree_->exploreSubTree(root);
  }

  /** Pure virtual explore subtree function will be called by serial and 
      parallel program. */
  virtual void exploreSubTree(AlpsTreeNode* root,
			      const int id, /* process id */	
			      int comm = 91) = 0;

  //===========================================================================

  /** @name Interface with the knowledge pools 
   *  
   */
  //@{
  /** Add a knowledge pool into the Knowledge pools */
  void addKnowledgePool(AlpsKnowledgeType kt, AlpsKnowledgePool* kp) {
   AlpsKnowledgePool* akp = static_cast<AlpsKnowledgePool*>(kp);
   pools_->insert
     (std::pair<AlpsKnowledgeType, AlpsKnowledgePool*>(kt, kp));
  }
  
  /** Retrieve a knowledge pool in the Knowledge base */
  AlpsKnowledgePool* getKnowledgePool(AlpsKnowledgeType kt) const { 
    return (*pools_)[kt];       //FIXME: check if exit
  }

  virtual int getNumKnowledges(AlpsKnowledgeType kt) const {
    return getKnowledgePool(kt)->getNumKnowledges();
  }

  virtual int getMaxNumKnowledges(AlpsKnowledgeType kt) const {
    return getKnowledgePool(kt)->getMaxNumKnowledges();
  }

  virtual void setMaxNumKnowledges(AlpsKnowledgeType kt, int num) {
    getKnowledgePool(kt)->setMaxNumKnowledges(num);
  }

  virtual bool hasKnowledge(AlpsKnowledgeType kt) {
    return getKnowledgePool(kt)->hasKnowledge();
  }

  /** Get a knowledge, but doesn't remove it from the pool*/
  virtual std::pair<const AlpsKnowledge*, double> 
    getKnowledge(AlpsKnowledgeType kt) const {
    
    return getKnowledgePool(kt)->getKnowledge();
  }

  /** Remove the gotten knowledge from the pool*/
  virtual void popKnowledge(AlpsKnowledgeType kt) {
    getKnowledgePool(kt)->popKnowledge();
  } 

  virtual std::pair<const AlpsKnowledge*, double> 
    getBestKnowledge(AlpsKnowledgeType kt) const {
    
    return getKnowledgePool(kt)->getBestKnowledge();
  }

  virtual void getAllKnowledges (AlpsKnowledgeType kt, 
				 std::vector<std::pair<const AlpsKnowledge*, 
				 double> >& kls)  const {
   
    getKnowledgePool(kt)->getAllKnowledges(kls);
  }

  virtual void  addKnowledge(AlpsKnowledgeType kt, 
			     const AlpsKnowledge* kl, 
			     double priority=0) {
    
    getKnowledgePool(kt)->addKnowledge(kl, priority);
  }
  //@}

  //===========================================================================

  /** @name Message environemnt setting member functions 
   *
   */
  //@{
  /** Initialize the message environemnt. */
  virtual void initMsgEnv(int argc, char* argv[],
			  int& myRank, int& procSize, 
			  const int comm = 91) = 0;

  /** Finalize the message environemnt. */
  virtual void finializeMsgEnv() = 0;
  //@}

 //===========================================================================
};
#endif
