/*===========================================================================*
 * This file is part of the Abstract Library for Parallel Search (ALPS).     *
 *                                                                           *
 * ALPS is distributed under the Common Public License as part of the        *
 * COIN-OR repository (http://www.coin-or.org).                              *
 *                                                                           *
 * Authors: Yan Xu, SAS Institute Inc.                                       *
 *          Ted Ralphs, Lehigh University                                    *
 *          Laszlo Ladanyi, IBM T.J. Watson Research Center                  *
 *          Matthew Saltzman, Clemson University                             *
 *                                                                           * 
 *                                                                           *
 * Copyright (C) 2001-2006, Lehigh University, Yan Xu, and Ted Ralphs.       *
 * Corporation, Lehigh University, Yan Xu, Ted Ralphs, Matthew Salzman and   *
 *===========================================================================*/

#ifndef AlpsKnowledge_h
#define AlpsKnowledge_h

#include <map>
#include <memory>
#include <typeinfo>

#include "AlpsEncoded.h"

//#############################################################################
// *FIXME* : For now, we use RTTI in the following methods. 
// *FIXME* : Indices would probably be more efficient.  We have to check this.

/** A function object to perform lexicographic lexicographic comparison
    between two C style strings. */
//#############################################################################

struct AlpsStrLess {
  inline bool operator()(const char* s1, const char* s2) const
  {
    return strcmp(s1, s2) < 0;
  }
};

//#############################################################################
/** The abstract base class of any user-defined class that Alps has to know
    about in order to encode/decode. These classes must all be registered so
    that the proper decode method can be called. */
//#############################################################################

class AlpsKnowledge {

 private:
  AlpsKnowledge(const AlpsKnowledge&);
  AlpsKnowledge& operator=(const AlpsKnowledge&);

 private:

  /** Stores a master copy of any encodable object for decoding purposes. */
  //static std::map<const char*, const AlpsKnowledge*, AlpsStrLess>* decodeMap_;


  //  static std::map<const char*, AlpsKnowledge*, AlpsStrLess>* decodeMap_;
  /** The encoded object in an encoded form (could be compressed!) */
  //FIXME: For now, we just use a regular pointer here to get it to compile.
  //CoinPtr<AlpsEncoded> encoded_;
  AlpsEncoded* encoded_;
   
 public:
  AlpsKnowledge() : encoded_(0) {}
  virtual ~AlpsKnowledge() {}

#if 0
  /** Every subclass that is derived from this base class must register. 
      The register methods register the decode method of the class so that 
      later on we can decode objects from buffers. Invoking this registration 
      for class <code>foo</code> is a single line:<br>
      <code>foo().registerClass();</code> 
  */
  void registerClass() {
    AlpsEncoded* enc = encode();
    (*decodeMap_)[typeid(*this).name()] = decode(*enc);
  }

  /** Overloaded version of <code> registerClass() </code>. Invoking this 
      registration for class <code>foo</code> is a single line:<br>
      <code>foo().registerClass(name);</code> */
  void registerClass(const char * name) {
    AlpsEncoded* enc = encode();
    (*decodeMap_)[name] = decode(*enc);
  }
#endif

  /** This method should encode the content of the object and return a
      pointer to the encoded form. 
      
      NOTE: This default implementation can not be
      used when the memory of data members is not continously allocated,
      for example, some data members are pointers, STL set, map, etc. */
  virtual AlpsEncoded* encode() const;
  
  /** This method should decode and return a pointer to a \em brand \em new
      \em object, i.e., the method must create a new object on the heap from
      the decoded data instead of filling up the object for which the method
      was invoked. 
      
      NOTE: This default implementation can not be
      used when the memory of data members is not continously allocated,
      for example, some data members are pointers, STL set, map, etc.
  */
  virtual AlpsKnowledge* decode(AlpsEncoded& encoded) const;
  
#if 0
  /** This method returns the pointer to an empty object of the registered
      class <code>name</code>. Then the <code>decode()</code> method of that
      object can be used to decode a new object of the same type from the
      buffer. This method will be invoked as follows to decode an object
      whose type is <code>name</code>:<br>
      <code>obj = AlpsKnowledge::decoderObject(name)->decode(buf) </code> */
  static const AlpsKnowledge* decoderObject(const char* name) {
    return (*decodeMap_)[name];
  }
#endif

  /** */
  // CoinPtr<AlpsEncoded>&
  inline AlpsEncoded* getEncoded() const { return encoded_; }
   
  /** */
  // CoinPtr<AlpsEncoded>&
  inline void setEncoded(AlpsEncoded* e) { encoded_ = e; }
   
};

//#############################################################################

#endif
