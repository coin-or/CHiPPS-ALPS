#include "AlpsLicense.h"

#ifndef AlpsPriorityQueue_h
#define AlpsPriorityQueue_h

#include <queue>
#include <vector>

//#############################################################################

template<class T, class Container = std::vector<T>, class Compare 
                              = std::less<typename Container::value_type> >
class AlpsPriorityQueue : public std::priority_queue<T, Container, Compare> {
  public:
  const Container& getContainer() const { return c; }
};
#endif
