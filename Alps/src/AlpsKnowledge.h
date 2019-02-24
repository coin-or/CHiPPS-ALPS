/*===========================================================================*
 * This file is part of the Abstract Library for Parallel Search (ALPS).     *
 *                                                                           *
 * ALPS is distributed under the Eclipse Public License as part of the       *
 * COIN-OR repository (http://www.coin-or.org).                              *
 *                                                                           *
 * Authors:                                                                  *
 *                                                                           *
 *          Yan Xu, Lehigh University                                        *
 *          Aykut Bulut, Lehigh University                                   *
 *          Ted Ralphs, Lehigh University                                    *
 *                                                                           *
 * Conceptual Design:                                                        *
 *                                                                           *
 *          Yan Xu, Lehigh University                                        *
 *          Ted Ralphs, Lehigh University                                    *
 *          Laszlo Ladanyi, IBM T.J. Watson Research Center                  *
 *          Matthew Saltzman, Clemson University                             *
 *                                                                           *
 *                                                                           *
 * Copyright (C) 2001-2019, Lehigh University, Yan Xu, Aykut Bulut, and      *
 *                          Ted Ralphs.                                      *
 * All Rights Reserved.                                                      *
 *===========================================================================*/


#ifndef AlpsKnowledge_h_
#define AlpsKnowledge_h_

#include <map>
#include <memory>
#include <typeinfo>

#include "Alps.h"
#include "AlpsEncoded.h"

class AlpsKnowledgeBroker;

/*!
  A function object to perform lexicographic lexicographic comparison
  between two C style strings.
*/

struct AlpsStrLess {
  inline bool operator()(const char* s1, const char* s2) const {
    return strcmp(s1, s2) < 0;
  }
};


/*!

  The abstract base class of Alps knowledges generated during the search. It is
  inherited by #AlpsModel, #AlpsTreeNode, #AlpsSolution and #AlpsSubTree.

  It provides the API for encoding and decoding functions. ::encode/::decode
  functions implemented here will work on simple classes (stored in contigious
  memory), that does not have pointers or STL containers. Complicated classes
  are required to implement their versions of encode/decode functions.

*/

class AlpsKnowledge {
  AlpsKnowledgeType type_;
public:
  AlpsKnowledgeBroker * broker_;

public:
  ///@name Constructor and Destructor
  //@{
  /// Default constructor
  AlpsKnowledge(): type_(AlpsKnowledgeTypeUndefined), broker_(0) {}
  AlpsKnowledge(AlpsKnowledgeType type): type_(type), broker_(0) {}
  AlpsKnowledge(AlpsKnowledgeType type, AlpsKnowledgeBroker * broker);
  /// Destructor
  virtual ~AlpsKnowledge() {}
  //@}

  ///@name Get type and set type functions
  //@{
  /// Get knowledge type.
  AlpsKnowledgeType getType() const { return type_; }
  /// Set knowledge type.
  void setType(AlpsKnowledgeType t) { type_ = t; }
  /// Get pointer to the knowledge broker
  AlpsKnowledgeBroker * broker() { return broker_; }
  /// Get pointer to the knowledge broker
  AlpsKnowledgeBroker const * broker() const { return broker_; }
  /// Set knowledge broker
  void setBroker(AlpsKnowledgeBroker * broker) { broker_=broker; }
  //@}

  ///@name Encoding and Decoding functions
  //@{
  /// Encode the content of this into an AlpsEncoded object and return a
  /// pointer to it.
  AlpsEncoded * encode() const;
  /// Encode the content of this into the given AlpsEncoded object.
  /// Implementation given in this class can not be used when the memory of
  /// data members is not contiguous, i.e., some data members are pointers to
  /// heap locations, STL set, map, etc. These type of user application
  /// sub-classes should implement their own version of this.
  virtual AlpsReturnStatus encode(AlpsEncoded * encoded) const;
  /// Decode the given AlpsEncoded object into a new AlpsKnowledge object and
  /// return a pointer to it. User application sub-classes should implement this
  /// since the returned pointer will point to user sub-class instances.
  virtual AlpsKnowledge * decode(AlpsEncoded & encoded) const = 0;
  /// Decode the given AlpsEncoded object into this.  Implementation given in
  /// this class can not be used when the memory of data members is not
  /// contiguous, i.e., some data members are pointers to heap locations, STL
  /// set, map, etc. These type of user application sub-classes should
  /// implement their own version of this.
  virtual AlpsReturnStatus decodeToSelf(AlpsEncoded & encoded);
  //@}

private:
  /// Disable copy constructor.
  AlpsKnowledge(AlpsKnowledge const &);
  /// Disable copy assignment operator.
  AlpsKnowledge & operator=(AlpsKnowledge const &);

};

#endif
