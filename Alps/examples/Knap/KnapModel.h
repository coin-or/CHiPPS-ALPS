/*===========================================================================*
 * This file is part of the Abstract Library for Parallel Search (ALPS).     *
 *                                                                           *
 * ALPS is distributed under the Eclipse Public License as part of the       *
 * COIN-OR repository (http://www.coin-or.org).                              *
 *                                                                           *
 * Authors: Yan Xu, Lehigh University                                       *
 *          Ted Ralphs, Lehigh University                                    *
 *          Laszlo Ladanyi, IBM T.J. Watson Research Center                  *
 *          Matthew Saltzman, Clemson University                             *
 *                                                                           * 
 *                                                                           *
 * Copyright (C) 2001-2013, Lehigh University, Yan Xu, and Ted Ralphs.       *
 *===========================================================================*/

#ifndef KnapModel_h_
#define KnapModel_h_


#include "AlpsKnowledge.h"
#include "AlpsModel.h"
#include "KnapParams.h"

//#############################################################################

class KnapModel : public AlpsModel {

 private:

  /** Capacity of the knapsack */
  int capacity_;
   
  /** List of sizes and profits of the items */
  std::vector< std::pair<int, int> > items_;

  /** The descent sequence based on ratio: profit/size. */
  int* sequence_;

  /** Knap parameters. */
  KnapParams *KnapPar_;

 public: 

  KnapModel() : capacity_(0), sequence_(0), KnapPar_(new KnapParams) {}
  KnapModel(int cap, std::vector<std::pair<int, int> > items, int* seq)
      :
      capacity_(cap),
      sequence_(seq),
      KnapPar_(new KnapParams) 
      { items_.insert(items_.begin(), items.begin(), items.end()); }
  
  ~KnapModel() { 
      if (sequence_ != 0) { 
	  delete [] sequence_; 
	  sequence_ = 0; 
      }
      delete KnapPar_;
  }

  /** Get the capacity of the knapsack */
  inline int getCapacity() const { return capacity_; }

  /** Get the number of items in the knapsack */
  inline int getNumItems() const { return static_cast<int> (items_.size()); }

  /** Get the sequence of items in the knapsack */
  inline int* getSequence() const { return sequence_; }   

//############################################################################ 

  /** Read in Alps and Knap parameters. */
  virtual void readParameters(const int argnum, const char * const * arglist){
      AlpsPar_->readFromArglist(argnum, arglist);
      int msgLevel = AlpsPar_->entry(AlpsParams::msgLevel);
      if (msgLevel > 0) {
	  std::cout << "Reading in KNAP parameters ..." << std::endl;
	  std::cout << "Reading in ALPS parameters ..." << std::endl;
      }
      KnapPar_->readFromArglist(argnum, arglist);
  }
   
  /** Get the size of item i */
  inline std::pair<int, int> getItem(int i) const { 
    return(items_[sequence_[i]]); 
  }
   
  /** Set the capacity of the knapsack */
  inline void setCapacity(int capacity) { capacity_ = capacity; }

  /** Set the sequence of items in the knapsack */
  void setSequence(const int * seq);  

  /** Set the size of item i */
  inline void addItem(int size, int cost)
    { items_.push_back(std::pair<int, int>(size, cost)); }

  /** Read in the problem data */
  void readInstance(const char* dataFile);

  /** Order the items based on their cost/size */
  void orderItems();

  /** The method that encodes the solution into a buffer. */
  virtual AlpsEncoded* encode() const;

  /** The method that decodes model data from the encoded form and 
      fill member data. */
  virtual void decodeToSelf(AlpsEncoded&);
};

//#############################################################################

#endif
