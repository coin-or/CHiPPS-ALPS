#include "AlpsLicense.h"

#ifndef AlpsSolution_h
#define AlpsSolution_h

#include <iosfwd>
#include <map>
#include <vector>

#include "CoinBoostWrapper.h"
#include "CoinBuffer.h"

#include "AlpsKnowledge.h"
#include "AlpsKnowledgePool.h"

//#############################################################################
//#############################################################################

class AlpsSolution : public AlpsKnowledge { 
 private:
  AlpsSolution(const AlpsSolution&);
  AlpsSolution& operator=(const AlpsSolution&);
 public:
  AlpsSolution() {}
  virtual ~AlpsSolution() {}

  /** Query the value of solution.*/
  virtual double getObjValue() const = 0;

  /** Print out the solution.*/
  virtual void print(std::ostream& os) const{
     throw CoinError("Can not call print()",
		     "print()", "AlpsSolution");
  }

};



//#############################################################################
//#############################################################################

/** In the solution pool we assume that the lower the priority value the more
    desirable the solution is. */
class AlpsSolutionPool : public AlpsKnowledgePool {
   // *FIXME* ? : we do want to allow solutions with the same priority, but do
   // *FIXME* ? : we want to allow identical solutions?
 private:
   AlpsSolutionPool(const AlpsSolutionPool&);
   AlpsSolutionPool& operator=(const AlpsSolutionPool&);

 private:
   std::multimap< double, CoinSharedPtr<const AlpsSolution> > solutions_;
   int maxNumSolutions_;


 private:
   inline void addSolution(CoinSharedPtr<const AlpsSolution> sol, 
			   double priority) {
      solutions_.insert(std::make_pair(priority, sol));
      if (maxNumSolutions_ > 0 && solutions_.size() > maxNumSolutions_) {
	 std::multimap< double, CoinSharedPtr<const AlpsSolution> >::
	    iterator si = solutions_.end();
	 --si;
	 solutions_.erase(si);
      }
   }

 public:
   AlpsSolutionPool(int maxsols = 1) : maxNumSolutions_(maxsols) {}
   virtual ~AlpsSolutionPool() {}

   /** query the current number of solutions */
   //   int getNumSolutions() const { return solutions_.size(); }
   inline int getNumKnowledges() const { return solutions_.size(); }

   /** return true if there are any solution stored in the solution pool */
   inline bool hasKnowledge() const 
     { return solutions_.empty() ? false : true; }

   /**Get a solution from solution pool, doesn't remove it from the pool. 
      It is implemented same as getBestKnowledge(). */
   inline std::pair<const AlpsKnowledge*, double> getKnowledge() const {
     return std::make_pair(solutions_.begin()->second.get(),
			   solutions_.begin()->first);
   }

   /**Remove a solution from the pool*/
   inline void popKnowledge() {
     throw CoinError("Can not call popKnowledge()",
		     "popKnowledge()", "AlpsSolutionPool");
   }

   /** Append the solution to the end of the vector of solutions. The solution
       pool takes over the ownership of the solution */
   //   void addSolution(const AlpsSolution* sol, double priority) {
   //   CoinSharedPtr<const AlpsSolution> ssol(sol);
   //   addSolution(ssol, priority);
   // }

   inline void addKnowledge(const AlpsKnowledge* sol, double priority=0) {
      CoinSharedPtr<const AlpsSolution> 
	ssol( dynamic_cast<const AlpsSolution*>(sol) );
      addSolution(ssol, priority);
   }

   /** query the maximum number of solutions */
   inline int getMaxNumKnowledges() const { return maxNumSolutions_; }

   /** reset the maximum number of solutions */
   inline void setMaxNumKnowledges(int maxsols) {
      if (maxsols > 0) {
	 if (solutions_.size() > maxsols) {
	    std::multimap< double, CoinSharedPtr<const AlpsSolution> >::
	       iterator si = solutions_.begin();
	    for (int i = 0; i < maxsols; ++i)
	       ++si;
	    solutions_.erase(si, solutions_.end());
	 }
      }
      maxNumSolutions_ = maxsols;
   }

   /** Return the best solution. The callee must not delete the returned
       pointer! */
   inline std::pair<const AlpsKnowledge*, double> getBestKnowledge() const {
      return std::make_pair(solutions_.begin()->second.get(),
			    solutions_.begin()->first);
   }

   /** Return all the solutions of the solution pool in the provided argument
       vector. The callee must not delete the returned pointers! */
   inline void getAllKnowledges
      (std::vector<std::pair<const AlpsKnowledge*, double> >& sols) const {
      sols.reserve(sols.size() + solutions_.size());
      std::multimap< double, CoinSharedPtr<const AlpsSolution> >::
	 const_iterator si;
      for (si = solutions_.begin(); si != solutions_.end(); ++si) {
	 sols.push_back(std::make_pair(si->second.get(), si->first));
      }
   }

};


#define AlpsSolutionInterface(ref)					\
int getNumSolutions() const {						\
   (ref).getNumSolutions();						\
}									\
int getMaxNumSolutions() const {					\
   return (ref).getMaxNumSolutions();					\
}									\
void setMaxNumSolutions(int num) {					\
   (ref).setMaxNumSolutions(num);					\
}									\
bool hasSolution() const {						\
   return (ref).hasSolution();						\
}									\
std::pair<const AlpsSolution*, double> getBestSolution() const {	\
   return (ref).getBestSolution();					\
}									\
void getAllSolutions							\
   (std::vector<std::pair<const AlpsSolution*, double> >& sols) {	\
   return (ref).getAllSolutions(sols);					\
}									\
void addSolution(const AlpsSolution* sol, double priority) {		\
   (ref).addSolution(sol, priority);					\
}
   
#endif
