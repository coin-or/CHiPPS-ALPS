#include "AlpsLicense.h"

#ifndef AlpsDataPool_h
#define AlpsDataPool_h

#include <map>
#include <string>

#include "AlpsKnowledge.h"
#include "AlpsKnowledgePool.h"

/** Data pool stores static data for Alps. */
class AlpsDataPool : public AlpsKnowledgePool {
 private:
   AlpsDataPool(const AlpsDataPool&);
   AlpsDataPool& operator=(const AlpsDataPool&);

 private:
   // FIXME: Using regular point make it working first.
   std::map<AlpsKnowledgeType, AlpsKnowledge* > data_;

 public:
   AlpsDataPool() {}

   virtual int getNumKnowledges() const {
     return data_.size();
   }

   /** Get a MODEL */
   virtual std::pair<const AlpsKnowledge*, double> getKnowledge() const {
     throw CoinError("Can not call method getKnowledge()", 
		     "getKnowledge()", "AlpsDataPool");
   }

   // FIXME: Why cannot use const qualifier

   /** Retrieve static data from data pool. */
   AlpsKnowledge* getKnowledge(const AlpsKnowledgeType kt) /*const*/ {
     return data_[kt];
   }

    /** Add static data into data pool. */
   void addKnowledge(const AlpsKnowledgeType kt, AlpsKnowledge* kg) { 
     // FIXME: take over ownership or not?
     data_.insert(std::make_pair(kt, kg));
   }

   /** Change the value of kt if kt is not exit; otherwise call addKnowledge().
    */
   void changeKnowledgeValue(const AlpsKnowledgeType kt, AlpsKnowledge* kg){
     std::map<AlpsKnowledgeType, AlpsKnowledge* >::iterator pos
       = data_.find(kt);
     if (pos != data_.end())
       pos->second = kg;
     else
       addKnowledge(kt, kg);
   }
};

//AlpsDataPool* ALPSDATAPOOL;

#endif
