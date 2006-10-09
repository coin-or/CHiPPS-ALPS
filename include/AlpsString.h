#include "AlpsLicense.h"

#ifndef AlpsString_h
#define AlpsString_h

// AlpsString is modified from BCP_string.
// This file is fully docified.

#include <cstring>

/** This class is a very simple impelementation of a constant length string.
    Using it one can avoid some memory errors related to using functions
    operating on C style strings. */
class AlpsString {

 private:
  /* the length of the string */
  int    len_;
  /* the data in the string */
  char * data_;

 public:
  /* The default constructor creates an empty sting. */
  AlpsString() : len_(0), data_(0) {};
  /* Create a <code>AlpsString</code> from a C style string. */
  AlpsString(const char * str) :
    len_(strlen(str)), data_(new char[strlen(str)+1]) {
    memcpy(data_, str, len_);
    data_[len_] = 0;
  }
  /* Make a copy of the argument string. */
  AlpsString(const AlpsString& str) :
    len_(str.length()), data_(new char[str.length()+1]) {
    memcpy(data_, str.c_str(), len_);
    data_[len_] = 0;
  }
  /* Delete the data members. */
  ~AlpsString() {
    delete[] data_;
  }

 public:
  /* Return the length of the string. */
  int length() const         { return len_; }
  /* Return a pointer to the data stored in the string. I.e., return a C
     style string. */
  const char * c_str() const { return data_; }

  /* This methods replaces the current <code>AlpsString</code> with one
     create from the first <code>len</code> bytes in <code>source</code>. */
  AlpsString& assign(const char * source, const int len) {
    delete[] data_;
    len_ = len;
    data_ = new char[len_+1];
    memcpy(data_, source, len_);
    data_[len_] = 0;
    return *this;
  }
  /* replace the current <code>AlpsString</code> with a copy of the argument
   */ 
  AlpsString& operator= (const AlpsString& str) {
    return assign(str.c_str(), str.length());
  }
  /* replace the current <code>AlpsString</code> with a copy of the argument
     C style string. */ 
  AlpsString& operator= (const char * str) {
    return assign(str, strlen(str));
  }

};

/** Equality tester for a <code>AlpsString</code> and a C style string. */
inline bool operator==(const AlpsString& s0, const char* s1) {
  if (s0.c_str() == 0)
    return s1 == 0;
  return s1 == 0 ? false : (strcmp(s0.c_str(), s1) == 0);
}

/** Equality tester for two <code>AlpsString</code>s. */
inline bool
operator==(const AlpsString& s0, const AlpsString& s1) {
  return s0 == s1.c_str();   // Use above overloaded ==
}

/** Equality tester for a C style string and a <code>AlpsString</code>. */
inline bool
operator==(const char* s0, const AlpsString& s1) {
  return s1 == s0;
}

#endif
