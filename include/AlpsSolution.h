#include "AlpsLicense.h"

#ifndef AlpsSolution_h
#define AlpsSolution_h

#include <map>
#include <vector>

#include "CoinBoostWrapper.h"

class AlpsSolution {
 public:
   virtual ~AlpsSolution() {}
};

/** In the solution pool we assume that the lower the priority value the more
    desirable the solution is. */

class AlpsSolutionPool {
   // *FIXME* ? : we do want to allow solutions with the same priority, but do
   // *FIXME* ? : we want to allow identical solutions?
 private:
   AlpsSolutionPool(const AlpsSolutionPool&);
   AlpsSolutionPool& operator=(const AlpsSolutionPool&);

 private:
   std::multimap< double, CoinSharedPtr<const AlpsSolution> > solutions_;
   int maxNumSolutions_;

 public:
   AlpsSolutionPool(int maxsols = 1) : maxNumSolutions_(maxsols) {}
   ~AlpsSolutionPool() {}

   /** query the current number of solutions */
   int getNumSolutions() const { return solutions_.size(); }
   /** query the maximum number of solutions */
   int getMaxNumSolutions() const { return maxNumSolutions_; }
   /** reset the maximum number of solutions */
   void setMaxNumSolutions(int maxsols) {
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

   /** return true if there are any solution stored in the solution pool */
   bool hasSolution() const { return ! solutions_.empty(); }

   /** Return the best solution. The callee must not delete the returned
       pointer! */
   std::pair<const AlpsSolution*, double> getBestSolution() const {
      return std::make_pair(solutions_.begin()->second.get(),
			    solutions_.begin()->first);
   }

   /** Return all the solutions of the solution pool in the provided argument
       vector. The callee must not delete the returned pointers! */
   void getAllSolutions
      (std::vector<std::pair<const AlpsSolution*, double> >& sols) const {
      sols.reserve(sols.size() + solutions_.size());
      std::multimap< double, CoinSharedPtr<const AlpsSolution> >::
	 const_iterator si;
      for (si = solutions_.begin(); si != solutions_.end(); ++si) {
	 sols.push_back(std::make_pair(si->second.get(), si->first));
      }
   }

   /** Append the solution to the end of the vector of solutions. The solution
       pool takes over the ownership of the solution */
   void addSolution(const AlpsSolution* sol, double priority) {
      CoinSharedPtr<const AlpsSolution> ssol(sol);
      addSolution(ssol, priority);
   }
   void addSolution(CoinSharedPtr<const AlpsSolution> sol, double priority) {
      solutions_.insert(std::make_pair(priority, sol));
      if (maxNumSolutions_ > 0 && solutions_.size() > maxNumSolutions_) {
	 std::multimap< double, CoinSharedPtr<const AlpsSolution> >::
	    iterator si = solutions_.end();
	 --si;
	 solutions_.erase(si);
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
