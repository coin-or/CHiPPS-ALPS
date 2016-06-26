#include "AlpsNodePool.h"

AlpsNodePool::AlpsNodePool()
  : AlpsKnowledgePool(AlpsKnowledgePoolTypeNode) {
  candidateList_.clear();
}


AlpsNodePool::~AlpsNodePool() {
  if (!candidateList_.empty()) {
    deleteGuts();
  }
}

void AlpsNodePool::setMaxNumKnowledges(int num) {
  std::cerr << "Not implemented. "
            << "file: " __FILE__ << " line: "
            << __LINE__ << std::endl;
  throw std::exception();
}

int AlpsNodePool::getNumKnowledges() const {
  return static_cast<int> (candidateList_.size());
}

std::pair<AlpsKnowledge*, double> AlpsNodePool::getKnowledge() const {
  return std::make_pair( static_cast<AlpsKnowledge *>
                         (candidateList_.top()),
                         candidateList_.top()->getQuality() );
}

std::pair<AlpsKnowledge*, double> AlpsNodePool::getBestKnowledge() const {
  std::cerr << "This should not happe. File: " << __FILE__
            << " line: " << __LINE__  << std::endl;
  throw std::exception();
}

void AlpsNodePool::getAllKnowledges (std::vector<std::pair<AlpsKnowledge*,
                                     double> >& kls) const {
  std::cerr << "This should not happe. File: " << __FILE__
            << " line: " << __LINE__  << std::endl;
  throw std::exception();
}


double AlpsNodePool::getBestKnowledgeValue() const {
  const std::vector<AlpsTreeNode *>& pool=candidateList_.getContainer();
  int k;
  int size = static_cast<int> (pool.size());
  double bestQuality = ALPS_OBJ_MAX;
  AlpsTreeNode * node = NULL;
  for (k = 0; k < size; ++k) {
    node = pool[k];
    if (node->getQuality() < bestQuality) {
      bestQuality = node->getQuality();
    }
  }
  return bestQuality;
}

AlpsTreeNode * AlpsNodePool::getBestNode() const {
  const std::vector<AlpsTreeNode *>& pool=candidateList_.getContainer();
  int k;
  int size = static_cast<int> (pool.size());
  double bestQuality = ALPS_OBJ_MAX;
  AlpsTreeNode * bestNode = NULL;
  AlpsTreeNode * node = NULL;

  for (k = 0; k < size; ++k) {
    node = pool[k];
    if (node->getQuality() < bestQuality) {
      bestQuality = node->getQuality();
      bestNode = node;
    }
  }
  return bestNode;
}

void AlpsNodePool::addKnowledge(AlpsKnowledge* node, double priority) {
  AlpsTreeNode * nn = dynamic_cast<AlpsTreeNode*>(node);
  candidateList_.push(nn);
}

AlpsPriorityQueue<AlpsTreeNode*> const &
AlpsNodePool::getCandidateList() const {
  return candidateList_;
}

void
AlpsNodePool::setNodeSelection(AlpsSearchStrategy<AlpsTreeNode*> & compare) {
  candidateList_.setComparison(compare);
}

void AlpsNodePool::deleteGuts() {
  std::vector<AlpsTreeNode* > nodeVec = candidateList_.getContainer();
  std::for_each(nodeVec.begin(), nodeVec.end(), DeletePtrObject());
  candidateList_.clear();
  assert(candidateList_.size() == 0);
}
