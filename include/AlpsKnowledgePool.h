#include "AlpsLicense.h"

#ifndef AlpsKnowledgePool_h
#define AlpsKnowledgePool_h

#include <vector>
#include "CoinError.hpp"
#include "AlpsKnowledge.h"

//#############################################################################
//#############################################################################

class AlpsKnowledgePool {
 private:
  AlpsKnowledgePool(const AlpsKnowledgePool&);
  AlpsKnowledgePool& operator=(const AlpsKnowledgePool&);

 public:
  AlpsKnowledgePool::AlpsKnowledgePool() {} // Need: otherwise 
  virtual ~AlpsKnowledgePool() {}           // won't compile, why?

  virtual int getNumKnowledges() const = 0;

  /** Get a knowledge, but doesn't remove it from the pool*/
  virtual std::pair<const AlpsKnowledge*, double> getKnowledge() const = 0;

  /** Remove the gotten knowledge from the pool*/
  virtual void popKnowledge() {
    throw CoinError("Can not call popKnowledge()",
		    "popKnowledge()", "AlpsKnowledgePool");
  }

  /** Add a knowledge to pool */
  virtual void addKnowledge(const AlpsKnowledge * nk, double priority=0) {
    throw CoinError("Can not call addKnowledge()",
		    "addKnowledge()", "AlpsKnowledgePool");
  } 

  virtual bool hasKnowledge() const{
    throw CoinError("Can not call hasKnowledge()",
		    "hasKnowledge()", "AlpsKnowledgePool");
  }

  virtual int getMaxNumKnowledges() const {
    throw CoinError("Can not call getMaxNumKnowledges()",
		    "getMaxNumKnowledges()", "AlpsKnowledgePool");
  }
 
  virtual std::pair<const AlpsKnowledge*, double> 
    getBestKnowledge() const {
    throw CoinError("Can not call  getBestKnowledge()",
		    "getBestKnowledge()", "AlpsKnowledgePool");
  }
 
  virtual void getAllKnowledges (std::vector<std::pair<const AlpsKnowledge*, 
				 double> >& kls) const {
    throw CoinError("Can not call  getAllKnowledge()",
		    "getAllKnowledge()", "AlpsKnowledgePool");
  }

  virtual void setMaxNumKnowledges(int num) {
    throw CoinError("Can not call  setMaxNumKnowledges()",
		    "setMaxNumKnowledges()", "AlpsKnowledgePool");
  }
};

#endif
 
