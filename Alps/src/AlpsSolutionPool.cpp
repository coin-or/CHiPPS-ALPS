#include "AlpsSolutionPool.h"

AlpsSolutionPool::AlpsSolutionPool(int maxsols)
  : AlpsKnowledgePool(AlpsKnowledgePoolTypeSolution),
  maxNumSolutions_(maxsols) {
}

AlpsSolutionPool::~AlpsSolutionPool() {
  if (! solutions_.empty()) {
    clean();
  }
}

int AlpsSolutionPool::getNumKnowledges() const {
  return static_cast<int> (solutions_.size());
}

bool AlpsSolutionPool::hasKnowledge() const {
  return solutions_.empty() ? false : true;
}

std::pair<AlpsKnowledge*, double> AlpsSolutionPool::getKnowledge() const {
  return std::make_pair(static_cast<AlpsKnowledge *>
                        (solutions_.begin()->second),
                        solutions_.begin()->first);
}

void AlpsSolutionPool::popKnowledge() {
  throw CoinError("Can not call popKnowledge()",
                  "popKnowledge()", "AlpsSolutionPool");
}

void AlpsSolutionPool::addKnowledge(AlpsKnowledge* sol, double priority) {
  std::pair<const double, AlpsSolution*>
    ps(priority, dynamic_cast<AlpsSolution*>(sol));
  solutions_.insert(ps);
  if ((maxNumSolutions_ > 0) &&
      (static_cast<int>(solutions_.size()) > maxNumSolutions_)) {
    std::multimap< double, AlpsSolution* >::iterator si =
      solutions_.end();
    --si;
    AlpsSolution* sol = si->second;
    solutions_.erase(si);
    delete sol;
  }
}

//todo(aykut) This looks buggy to me.
void AlpsSolutionPool::setMaxNumKnowledges(int maxsols) {
  if (maxsols > 0) {
    if (static_cast<int>(solutions_.size()) > maxsols) {
      std::multimap<double, AlpsSolution*>::iterator si = solutions_.begin();
      for (int i = 0; i < maxsols; ++i) {
        ++si;
      }
      //todo(aykut) why not use solution_.begin()+maxsols
      solutions_.erase(si, solutions_.end());
    }
  }
  maxNumSolutions_ = maxsols;
}

std::pair<AlpsKnowledge*, double> AlpsSolutionPool::getBestKnowledge() const {
  return std::make_pair(static_cast<AlpsKnowledge *>
                        (solutions_.begin()->second),
                        solutions_.begin()->first);
}

void AlpsSolutionPool::getAllKnowledges (std::vector<std::pair<AlpsKnowledge*,
                                         double> >& sols) const {
  sols.reserve(sols.size() + solutions_.size());
  std::multimap<double, AlpsSolution*>::const_iterator si;
  for (si = solutions_.begin(); si != solutions_.end(); ++si) {
    sols.push_back(std::make_pair(static_cast<AlpsKnowledge *>
                                  (si->second), si->first));
  }
}

void AlpsSolutionPool::clean() {
  while (!solutions_.empty()) {
    std::multimap< double, AlpsSolution* >::iterator si=solutions_.end();
    --si;
    AlpsSolution * sol = si->second;
    solutions_.erase(si);
    delete sol;
    sol = NULL;
  }
}
