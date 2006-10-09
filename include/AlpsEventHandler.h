#include "AlpsLicense.h"

#ifndef AlpsEventHandler_h_
#define AlpsEventHandler_h_

#include "AlpsSolution.h"
#include "AlpsSubTree.h"

class AlpsEventHandler {
 private:
   AlpsSubTreeWorker* subtree_;
   AlpsSolutionPool* solpool_;

 public:
   AlpsEventHandler(AlpsSubTreeWorker* st, AlpsSolutionPool* sp)
      :
      subtree_(st),
      solpool_(sp)
      {}
   virtual ~AlpsEventHandler() {}

   /** methods to interface with the subtree */
   virtual void exploreSubTree(AlpsTreeNode* root){
      root->setEventHandler(this);
      root->setPriority(0);
      root->setLevel(0);
      root->setIndex(0);
      subtree_->setNextIndex(1); // one more than root's index
      subtree_->exploreSubTree(root);
   }
#if 0
   virtual void getPriorityThreshold(){
      subtree_->getPriorityThreshold();
   }

   virtual void setPriorityThreshold(double threshold){
      subtree_->setPriorityThreshold(threshold);
   }
#endif
   /** methods to interface with the solution pool */
   virtual int getNumSolutions() const {
      return solpool_->getNumSolutions();
   }
   virtual int getMaxNumSolutions() const {
      return solpool_->getMaxNumSolutions();
   }
   virtual void setMaxNumSolutions(int num) {
      solpool_->setMaxNumSolutions(num);
   }
   virtual bool hasSolution() const {
      return solpool_->hasSolution();
   }
   virtual std::pair<const AlpsSolution*, double> getBestSolution() const {
      return solpool_->getBestSolution();
   }
   virtual void getAllSolutions
      (std::vector<std::pair<const AlpsSolution*, double> >& sols) const {
      return solpool_->getAllSolutions(sols);
   }
   virtual void addSolution(const AlpsSolution* sol, double priority) {
      solpool_->addSolution(sol, priority);
   }
   virtual void addSolution
      (CoinSharedPtr<const AlpsSolution> sol, double priority) {
      solpool_->addSolution(sol, priority);
   }
};

#endif
