//#include <iostream>
#include "AlpsLicense.h"

#include"CoinError.hpp"

#include "AlpsSubTree.h"
#include "AlpsData.h"


//#############################################################################

void
AlpsSubTree::removeDeadNodes(AlpsTreeNode*& node) throw(CoinError)
{
   if (!node->isFathomed()) {
      //FIXME: Need to work on the error throwing mechanism
      //throw CoinError(0, Alps_NodeNotDead, "AlpsSubTree::removeDeadNodes");
     throw CoinError();
   }

   AlpsTreeNode* parent = node->getParent();
   if (parent) {
      parent->removeChild(node);
      if (parent->getNumChildren() == 0) {
	 parent->setStatus(AlpsNodeStatusFathomed);
	 removeDeadNodes(parent);
      }
   } else {
      // We are in the root
      node->setStatus(AlpsNodeStatusFathomed);
   }
}

//#############################################################################

void
AlpsSubTree::replaceNode(AlpsTreeNode* oldNode, AlpsTreeNode* newNode)
{
   AlpsTreeNode* parent = oldNode->getParent();

   oldNode->removeDescendants();

   if (parent) {
      parent->removeChild(oldNode);
      parent->addChild(newNode);
      newNode->setParent(parent);
   } else {
      delete root_;
      root_ = newNode;
   }
}


//#############################################################################

void
AlpsSubTreeWorker::exploreSubTree(AlpsTreeNode* root)
{
   AlpsTreeNode* node = 0;

   /*Set the root node and put it into the queue*/
   root_ = root;
   //  candidateList_.push(root_);
   nodepool_->addKnowledge(root_);

   //   while (! candidateList_.empty()) {
   while (! nodepool_->empty()) {
     // node = candidateList_.top();
     node = dynamic_cast<AlpsTreeNode*>
       (const_cast<AlpsKnowledge*>(nodepool_->getKnowledge().first) ); 
    
     //     node = nodepool_.getKnowledge();      

//      candidateList_.pop();
     nodepool_->popKnowledge();

     switch (node->getStatus()) {
      case AlpsNodeStatusPregnant : {
       std::vector< CoinTriple<AlpsNodeDesc*, AlpsNodeStatus, double> > 
	    children = node->branch();
	 createChildren(node, children);
	 break;
      }
      case AlpsNodeStatusCandidate :
      case AlpsNodeStatusEvaluated :
	 node->setActive(true);
	 node->process();
	 node->setActive(false);

	 switch (node->getStatus()) {
	 case AlpsNodeStatusCandidate :
	 case AlpsNodeStatusEvaluated :
	 case AlpsNodeStatusPregnant :
	   // candidateList_.push(node);
	   nodepool_->addKnowledge(node);
	    break;
	 case AlpsNodeStatusFathomed :
	    // *FIXME* : based on a parameter, we should decide whether to
	    // clean up the tree or preserve it for posterity
	    removeDeadNodes(node);
	    break;
	 default : // AlpsNodeStatus::branched ==> this is impossible
	    //FIXME: Error Mechanism
	    //throw CoinError(0, Alps_ProcessReturnedBranched,
	    //		    "AlpsSubTreeWorker::process");
	   throw CoinError();
	 }
	 break;
      default : // branched or fathomed
	 //FIXME: Error Mechanism
 	 //throw CoinError(0, Alps_NextNodeBadStatus,
	 //		 "AlpsSubTreeWorker::process");
	 throw CoinError();
      }
   }
}   


void 
AlpsSubTreeWorker:: doOneUnitWork()
{
  AlpsTreeNode* node  = 0;
  int numNode         = 0;
  const  int unitWork =
    AlpsData::parameters()->entry(AlpsAllParam::unitWorkNodes);
  const int unitTime =
    AlpsData::parameters()->entry(AlpsAllParam::unitWorkNodes);

  //  while (! nodepool_->empty()) {
  while (! nodepool_->empty() && numNode < unitWork) { 
    ++numNode;
    node = dynamic_cast<AlpsTreeNode*>
       (const_cast<AlpsKnowledge*>(nodepool_->getKnowledge().first) ); 
     
     nodepool_->popKnowledge();

     switch (node->getStatus()) {
     case AlpsNodeStatusPregnant : {
	std::vector< CoinTriple<AlpsNodeDesc*, AlpsNodeStatus, double> > 
	  children = node->branch();
	createChildren(node, children);
	break;
     }
     case AlpsNodeStatusCandidate :
     case AlpsNodeStatusEvaluated :
	 node->setActive(true);
	 node->process();
	 node->setActive(false);

	 switch (node->getStatus()) {
	 case AlpsNodeStatusCandidate :
	 case AlpsNodeStatusEvaluated :
	 case AlpsNodeStatusPregnant :
	   nodepool_->addKnowledge(node);
	   break;
	 case AlpsNodeStatusFathomed :
	   // *FIXME* : based on a parameter, we should decide whether to
	   // clean up the tree or preserve it for posterity
	   removeDeadNodes(node);
	   break;
	 default : // AlpsNodeStatus::branched ==> this is impossible
	   throw CoinError("Bad status: branched", 
			   "doOneUnitWork", "AlpsSubTreeWorker"); 
	 }
	 break;
     default :    // branched or fathomed
       throw CoinError("Bad status: branched or fathomed", 
		       "doOneUnitWork", "AlpsSubTreeWorker"); 
     }
   }
}

//#############################################################################

void 
AlpsSubTreeWorker::createChildren(AlpsTreeNode* parent,
				  std::vector< CoinTriple<AlpsNodeDesc*, 
				  AlpsNodeStatus, double> >& children)
{
   const int numChildren = children.size();

   parent->setNumChildren(numChildren);
   for (int i = 0; i < numChildren; ++i) {
      AlpsTreeNode* child = parent->createNewTreeNode(children[i].first);
      parent->setChild(i, child);
      child->setStatus(children[i].second);
      child->setBestBound(children[i].third);
      child->setPriority(children[i].third);
      child->setParent(parent);
      child->setKnowledgeBroker(parent->getKnowledgeBroker());
      child->setActive(false);
      child->setLevel(parent->getLevel() + 1);
      child->setIndex(nextIndex_++);
   }

   for (int i = 0; i < numChildren; ++i) {
      AlpsTreeNode* child = parent->getChild(i);
      switch (child->getStatus()) {
      case AlpsNodeStatusCandidate :
      case AlpsNodeStatusEvaluated :
      case AlpsNodeStatusPregnant :
	// candidateList_.push(child);
	nodepool_->addKnowledge(child);
	break;
      case AlpsNodeStatusFathomed :
	 // *FIXME* : based on a parameter, we should decide whether to
	 // clean up the tree or preserve it for posterity
	 removeDeadNodes(child);
	 break;
      default: // AlpsNodeStatus::branched ==> this is impossible
	 throw CoinError("AlpsNodeStatus::branched, this is impossible",
			 "createChildren", "AlpsSubTree");
      }
   }
}

#if 0
//#############################################################################

bool 
AlpsSubTreeWorker::fathom()
{
   //Here, priority == lowerBound
   if (activeNode_->getPriority() > priorityThreshold_)
      return(true);
   else
      return(false);
}

#endif


// The implementation is almost same as exploreSubtree, except it will return
// when the required number of nodes is generated or the nodepool is empty(
// which means the hub already solve the problem). During this step,
// hub may find solutions, and later it will let workers know the solutions?
void 
AlpsSubTreeHub::initializeSearch(AlpsTreeNode* root) 
{
   AlpsTreeNode* node;

   /*Set the root node and put it into the queue*/
   root_ = root;
   nodepool_->addKnowledge(root_);
   const int nodeNum =  AlpsData::parameters()->
                           entry(AlpsAllParam::initialNodeNum);

   //  while (! nodepool_->empty()) {
   while(! nodepool_->empty() && nodepool_->getNumKnowledges() < nodeNum) { 
 
     node = static_cast<AlpsTreeNode*>
       (const_cast<AlpsKnowledge*>(nodepool_->getKnowledge().first) ); 
    
     nodepool_->popKnowledge();

     switch (node->getStatus()) {
      case AlpsNodeStatusPregnant : {
       std::vector< CoinTriple<AlpsNodeDesc*, AlpsNodeStatus, double> > 
	    children = node->branch();
	 createChildren(node, children);
	 break;
      }
      case AlpsNodeStatusCandidate :
      case AlpsNodeStatusEvaluated :
	 node->setActive(true);
	 node->process();
	 node->setActive(false);

	 switch (node->getStatus()) {
	 case AlpsNodeStatusCandidate :
	 case AlpsNodeStatusEvaluated :
	 case AlpsNodeStatusPregnant :
	   // candidateList_.push(node);
	   nodepool_->addKnowledge(node);
	    break;
	 case AlpsNodeStatusFathomed :
	    // *FIXME* : based on a parameter, we should decide whether to
	    // clean up the tree or preserve it for posterity
	    removeDeadNodes(node);
	    break;
	 default : // AlpsNodeStatus::branched ==> this is impossible
	    //FIXME: Error Mechanism
	    //throw CoinError(0, Alps_ProcessReturnedBranched,
	    //		    "AlpsSubTreeWorker::process");
	   throw CoinError();
	 }
	 break;
      default : // branched or fathomed
	 //FIXME: Error Mechanism
 	 //throw CoinError(0, Alps_NextNodeBadStatus,
	 //		 "AlpsSubTreeWorker::process");
	 throw CoinError();
      }
   }
}

