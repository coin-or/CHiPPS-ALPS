#include "AlpsLicense.h"

#ifndef AlpsKnowledgeBroker_h_
#define AlpsKnowledgeBroker_h_

#include <iosfwd>
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
  AlpsSubTree*  subtree_;

  /** The collection of pools managed by the knowledge broker. */
  std::map<AlpsKnowledgeType, AlpsKnowledgePool*>* pools_;
  
 public:
  AlpsKnowledgeBroker()
    : 
    subtree_(0)         /* The derived class will take care of it. */
    {}                  /* Need call setupKnowledgePools() later. */
   
  AlpsKnowledgeBroker(AlpsSubTree* st)
    : 
    subtree_(st)
    { setupKnowledgePools(); }
   
  virtual ~AlpsKnowledgeBroker() {
    if (pools_ != 0)
      delete pools_;
    if (subtree_ != 0)
      delete subtree_;
  }


  //---------------------------------------------------------------------------
  /** @name Interface with the subtree. 
   *  
   */
  //@{
  /** Set a subtree for this broker. */
  inline void setSubTree(AlpsSubTree* st) {
    subtree_ = st;
  }

  /** Explore the subtree for certain amount of work/time. */
  inline void doOneUnitWork() {
    subtree_->doOneUnitWork();
  }
 
  /** Explore the subtree rooted as a given root (default implementation). */
  inline void solve(AlpsTreeNode* root) {
    root->setKnowledgeBroker(this);
    root->setPriority(0);
    root->setLevel(0);
    root->setIndex(0);
    subtree_->setNextIndex(1);        // one more than root's index
    subtree_->exploreSubTree(root);
  }
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
		    ( ALPS_SOLUTION, subtree_->getSolutionPool() ) );
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
  virtual bool hasKnowledge(AlpsKnowledgeType kt) {
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

  /** @name Report the best solution 
   *  
   */
  //@{
  /** Print out the objective value of the best solution found. */
  virtual void printBestObjValue(std::ostream& os) const = 0;

  /** Print out the best solution found. */
  virtual void printBestSolution(std::ostream& os) const = 0;
  //@}

};
#endif
