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
  
  //  AlpsKnowledgeBase *    base_;

 public:
  AlpsKnowledgeBroker()
    : 
    subtree_(NULL), /*Its derived class will allocate memory.*/
    pools_(new std::map<AlpsKnowledgeType, AlpsKnowledgePool*>)
  //   base_(new  AlpsKnowledgeBase) 
    {
      pools_->insert( std::pair<AlpsKnowledgeType, AlpsKnowledgePool*>
		      ( NODE, subtree_->getNodePool() ) );
      pools_->insert( std::pair<AlpsKnowledgeType, AlpsKnowledgePool*>
		      ( SOLUTION, subtree_->getSolutionPool() ) );
  
      // base_->addKnowledgePool(NODE, subtree_->getNodePool());
      // base_->addKnowledgePool(SOLUTION, subtree_->getSolutionPool());
    }
   
  AlpsKnowledgeBroker(AlpsSubTreeWorker* st)
    : 
    subtree_(st),
    pools_(new std::map<AlpsKnowledgeType, AlpsKnowledgePool*>)
    //    base_(new  AlpsKnowledgeBase) 
    {
      pools_->insert( std::pair<AlpsKnowledgeType, AlpsKnowledgePool*>
		      ( NODE, subtree_->getNodePool() ) );
      pools_->insert( std::pair<AlpsKnowledgeType, AlpsKnowledgePool*>
		      ( SOLUTION, subtree_->getSolutionPool() ) );
      // base_->addKnowledgePool(NODE, subtree_->getNodePool());
      // base_->addKnowledgePool(SOLUTION, subtree_->getSolutionPool());
    }
   
  virtual ~AlpsKnowledgeBroker() {
    if (pools_ != NULL)
      delete pools_;
    if (subtree_ != NULL)
      delete subtree_;
  }

  /** methods to interface with the subtree */
  void exploreSubTree(AlpsTreeNode* root) {
    root->setKnowledgeBroker(this);
    root->setPriority(0);
    root->setLevel(0);
    root->setIndex(0);
    subtree_->setNextIndex(1);        // one more than root's index
    subtree_->exploreSubTree(root);
  }

  /** methods to interface with the subtree for serial and parallel program. */
  virtual void exploreSubTree(AlpsTreeNode* root,
			      const int id,	
			      int comm = 91){	
    exploreSubTree(root);
  }

  /** methods to interface with the knowledge pools */
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

  //  virtual int getNumKnowledges(AlpsKnowledgeType kt) const ()
  //  Why const does not work ??? YX: now work
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
    getKnowledge(AlpsKnowledgeType kt) const
    {
      return getKnowledgePool(kt)->getKnowledge();
    }

  /** Remove the gotten knowledge from the pool*/
  virtual void popKnowledge(AlpsKnowledgeType kt) {
    getKnowledgePool(kt)->popKnowledge();
  } 

  virtual std::pair<const AlpsKnowledge*, double> 
    getBestKnowledge(AlpsKnowledgeType kt) const
    {
      return getKnowledgePool(kt)->getBestKnowledge();
    }

  virtual void 
    getAllKnowledges (AlpsKnowledgeType kt, 
		      std::vector<std::pair<const AlpsKnowledge*, 
		      double> >& kls)  const
    {
      getKnowledgePool(kt)->getAllKnowledges(kls);
    }

  virtual void 
    addKnowledge(AlpsKnowledgeType kt, const AlpsKnowledge* kl, 
		 double priority=0) 
    {
      getKnowledgePool(kt)->addKnowledge(kl, priority);
    }

};
#endif
