#include "AlpsSubTreePool.h"

AlpsSubTreePool::AlpsSubTreePool()
  :AlpsKnowledgePool(AlpsKnowledgePoolTypeSubTree) {
  subTreeList_.clear();
}

AlpsSubTreePool::~AlpsSubTreePool() {
  if (!subTreeList_.empty()) {
    deleteGuts();
  }
}

int AlpsSubTreePool::getNumKnowledges() const {
  return static_cast<int> (subTreeList_.size());
}

std::pair<AlpsKnowledge*, double> AlpsSubTreePool::getKnowledge() const {
  return std::make_pair(dynamic_cast<AlpsKnowledge *>(subTreeList_.top()),
                         subTreeList_.top()->getQuality() );
}

std::pair<AlpsKnowledge*, double> AlpsSubTreePool::getBestKnowledge() const {
  std::cerr << "Not implemented. File: " << __FILE__ << " line: "
            << __LINE__ << std::endl;
  throw std::exception();
}

void AlpsSubTreePool::getAllKnowledges (std::vector<std::pair<AlpsKnowledge*,
                                        double> >& kls) const {
  std::cerr << "Not implemented. File: " << __FILE__ << " line: "
            << __LINE__ << std::endl;
  throw std::exception();
}

void AlpsSubTreePool::addKnowledge(AlpsKnowledge* subTree, double priority) {
  AlpsSubTree * st = dynamic_cast<AlpsSubTree* >(subTree);
  subTreeList_.push(st);
}

void AlpsSubTreePool::setMaxNumKnowledges(int num)  {
  std::cerr << "Not implemented. File: " << __FILE__ << " line: "
            << __LINE__ << std::endl;
  throw std::exception();
}

AlpsPriorityQueue<AlpsSubTree*> const &
AlpsSubTreePool::getSubTreeList() const {
  return subTreeList_;
}

void
AlpsSubTreePool::setComparison(AlpsSearchStrategy<AlpsSubTree*> & compare) {
  subTreeList_.setComparison(compare);
}

void AlpsSubTreePool::deleteGuts() {
  std::vector<AlpsSubTree* > treeVec = subTreeList_.getContainer();
  std::for_each(treeVec.begin(), treeVec.end(), DeletePtrObject());
  subTreeList_.clear();
  assert(subTreeList_.size() == 0);
}

double AlpsSubTreePool::getBestQuality() {
  double quality = ALPS_OBJ_MAX;
  std::vector<AlpsSubTree* > subTreeVec = subTreeList_.getContainer();
  std::vector<AlpsSubTree* >::iterator pos1, pos2;
  pos1 = subTreeVec.begin();
  pos2 = subTreeVec.end();
  for (; pos1 != pos2; ++pos1) {
    (*pos1)->calculateQuality();
    if ((*pos1)->getQuality() < quality) {
      quality = (*pos1)->getQuality();
    }
  }
  return quality;
}
