
#include "AlpsLicense.h"

#ifndef AlpsKnowledgeBase_h
#define AlpsKnowledgeBase_h

#include "AlpsSolution.h"
#include "AlpsTreeNode.h"

class AlpsKnowledgeBase {
 private:
  AlpsKnowledgeBase(const AlpsKnowledgeBase&);
  AlpsKnowledgeBase& operator=(const AlpsKnowledgeBase&);
  
  /** The collection of pools managed by the knowledge broker */
  std::map<AlpsKnowledgeType, AlpsKnowledgePool*>* pools_;

 public:
  AlpsKnowledgeBase() //  Default contructor
    : 
    pools_(new std::map<AlpsKnowledgeType, AlpsKnowledgePool*>) {}
  
  ~AlpsKnowledgeBase() {
    delete pools_;
  }
    
    //  AlpsKnowledgeBase(AlpsNodePool* np)  // Constructor
    //  :
    
    //solpool_(new AlpsSolutionPool(2))
    // {
    //  pools.insert(std::pair<const AlpsKnowledgeType, AlpsKnowledgePool* >
    //		    (SOLUTION, solpool_) );
    // nodepool_ = np;  // Get from subtree 
    // pools.insert(std::pair<const AlpsKnowledgeType, AlpsKnowledgePool* >
    //	    (NODE, nodepool_) );
    // }

  /** Add a knowledge pool into the Knowledge base */
  void addKnowledgePool(AlpsKnowledgeType kt, AlpsKnowledgePool* kp) {


   /** Add a node to node pool. The node pool takes over the 
       ownership of the node */
    //   void addKnowledge(const AlpsKnowledge* node, double priority=0) {
    // const AlpsTreeNode * nn = dynamic_cast<const AlpsTreeNode*>(node);
     //     if(!nn) {
     //  AlpsTreeNode * nonnn = const_cast<AlpsTreeNode*>(nn);
     //  candidateList_.push(nonnn);
       //     }
       //    else 
       // std::cout << "Add node failed\n";
     //     else
     // throw CoinError();
    // }

   AlpsKnowledgePool* akp = static_cast<AlpsKnowledgePool*>(kp);


   pools_->insert
     (std::pair<AlpsKnowledgeType, AlpsKnowledgePool*>(kt, kp));
  }

  /** Retrieve a knowledge pool in the Knowledge base */
  AlpsKnowledgePool* getKnowledgePool(AlpsKnowledgeType kt) const { 
    return (*pools_)[kt];       //FIXME: check if exit
  }
};
#endif

