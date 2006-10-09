#include "AlpsLicense.h"

#ifndef AlpsKnowledge_h
#define AlpsKnowledge_h

#include <map>
#include <typeinfo>
#include <memory>
#include <cstring>
#include <vector>

#include "CoinError.hpp"
#include "AlpsString.h"
#include "AlpsVector.h"

// AlpsEncoded is modified from BCP_buffer and CoinEncoded

//#############################################################################

/** Type of knowledge like solution, node, cut...*/
enum AlpsKnowledgeType { ALPS_MODEL, ALPS_NODE, ALPS_SOLUTION };

//#############################################################################
/** This data structure is to contain the packed form of an encodable
    object. */
//#############################################################################

class AlpsEncoded {  // Is buffer

 private:

  /** Disable copy constructor and assignment operator */
  ///@{
  AlpsEncoded(const AlpsEncoded&);
  AlpsEncoded& operator=(const AlpsEncoded&);
  ///@}

private:

  /** The next read/write position in the representation. */
  size_t pos_;
  /** The amount of memory allocated for the representation. */
  size_t maxSize_;
  /** A C-style string representing the type of the object. We might use RTTI
      to point this into the static data of the executable :). */
  const char* type_; //why const ??? XY: can only initialize in constructor
  //char* type_;
  /** The size of the packed representation. */
  int         size_;
  /** The encoded/compressed representation of the object. */
  // const char* representation_;   //why const ??? XY
  char* representation_;

 public:

  //===========================================================================

  /**@name Constructors and destructor */
  /*@{*/
  /** The default constructor creates a buffer of size 16 Kbytes with no
      message in it. */
  AlpsEncoded() 
    : 
    pos_(0), 
    maxSize_(0x4000/*16K*/), 
    type_(NULL), 
    size_(0), 
    representation_(new char[maxSize_]) 
    {}
  AlpsEncoded(const char* t) 
    : 
    pos_(0), 
    maxSize_(0x4000/*16K*/), 
    type_(t), 
    size_(0), 
    representation_(new char[maxSize_]) 
    {}
  AlpsEncoded(char* r) 
    : 
    pos_(0), 
    maxSize_(strlen(r) > 0x4000/*16K*/ ? strlen(r) : 0x4000), 
    type_(0), 
    size_(0), 
    representation_(r) 
    { r = 0; }         // Must take over the ownership!
  
  // AlpsEncoded() : size_(0), representation_(NULL) {}
  // can't use "const char*& r" as parameter
  AlpsEncoded(char*& t, const int s, char*& r) 
    : 
    pos_(0),   maxSize_(strlen(r)),
    type_(t),  size_(s),  representation_(r) 
    {
      t = 0;  r = 0;  // Must take over the ownership!
    }                    
  ~AlpsEncoded() {
    if (type_ != 0) { delete type_; type_ = 0; }
    if (representation_ != 0) { 
      delete  representation_; 
      representation_ = 0; 
    }
  }
  /*@}*/

  //===========================================================================
  /**@name Query methods */
  ///@{
  const char* type() const { return type_; }
  int size() const { return size_; }
  const char* representation() const { return representation_; }
  ///@}
  //===========================================================================

   inline void setPosition(const int pos) throw(CoinError) {
    if (pos < 0 || pos >= size()) {
      //     const char msg [100] = "Incorrest position setting.";
      //throw AlpsException(__FILE__, __LINE__, msg);
      throw CoinError("Incorrest position setting.", "setPosition",
		      "AlpsEncoded");
    }
    pos_ = pos;
  }

  inline void setRepresentation(char* buf) {
    representation_ = buf;    // Take over the ownership!
    buf = 0;
  }

#if 0
   // type_ is const, can not allocate memory. Following two methods not used
  inline void setType(const char* type) {
    const int typeLen = strlen(type);
    if (typeLen) {
      type_ = new char[typeLen];
      memcpy(type_, type, typeLen);
    }
    else {
      //      const char msg [100] = "The length of type name <= 0.";
      //throw AlpsException(__FILE__, __LINE__, msg);
      throw CoinError("The length of type name <= 0.", "setType",
		      "AlpsEncoded");
    }
  }

  /** Set the Encoded to be a copy of the given data. Use this with care! */
  // Can pass enpty rep[], size == 0;
  inline void setEncoded(const char* type, const char* rep, 
			 const size_t size) {
    if (maxSize_ < size) {
      delete[] representation_;
      representation_ = new char[size];
      maxSize_ = size;
    }
    pos_ = 0;
    size_ = size;
    if (size_)
      memcpy(representation_, rep, size * sizeof(char));

    // Copy type
    int typeLen = strlen(type);
    if (typeLen) {
      type_ = new char[typeLen];
      memcpy(type_, type, typeLen);
    }
    else 
      type_ = NULL;
    if(!size_ < 0 ) {
      //      const char msg [100] = "The size of rep < 0.";
      //      throw AlpsException(__FILE__, __LINE__, msg);
      throw CoinError("The size of rep < 0.", "setEncoded",
		      "AlpsEncoded");
    }
  }
#endif
  //===========================================================================

  /** Reallocate the size of encoded if necessary so that at least
      <code>addsize_</code> number of additional bytes will fit into the
      encoded. */
  inline void make_fit(const int addSize){
    if (maxSize_ < size_ + addSize){
      maxSize_ = 2 * (size_ + addSize + 0x1000/*4K*/);
      char* newRep = new char[maxSize_];
      if (size_)
	memcpy(newRep, representation_, size_);
      delete[] representation_;
      representation_ = newRep;
    }
  }

  /** Completely clear the encoded. Delete and zero out <code>type_, size_,
      pos_</code>. */
  inline void clear(){
    size_ = 0;
    pos_ = 0;
    if (type_ != NULL) { delete type_; type_ = NULL; }
    if (representation_ != NULL) { 
      delete  representation_; 
      representation_ = NULL; 
    }
  }

  //===========================================================================

  /** Pack a single object of type <code>T</code>. Copies
      <code>sizeof(T)</code> bytes from the address of the object. */
  template <class T> AlpsEncoded& pack(const T& value) {
    make_fit( sizeof(T) );
    memcpy(representation_ + size_, &value, sizeof(T));
    size_ += sizeof(T);
    return *this;
  }

  /** Unpack a single object of type <code>T</code>. Copies
      <code>sizeof(T)</code> bytes to the address of the object. */
  template <class T> AlpsEncoded& unpack(T& value){
#ifdef PARANOID
    if (pos_ + sizeof(T) > size_) {
      throw CoinError("Reading over the end of buffer.", 
		      "AlpsEncoded", "pack(const T& value");

    }
#endif
    memcpy(&value, representation_ + pos_, sizeof(T));
    pos_ += sizeof(T);
    return *this;
  }

  //===========================================================================

   /** Pack a C style array of objects of type <code>T</code>. 
       First pack the length, then pack the content of the array */
  
   template <class T> AlpsEncoded& pack(const T* const values,
				       const int length){
     make_fit( sizeof(int) + sizeof(T) * length );
     memcpy(representation_ + size_, &length, sizeof(int));
     size_ += sizeof(int);
     if (length > 0){
       memcpy(representation_ + size_, values, sizeof(T) * length);
       size_ += sizeof(T) * length;
     }
     return *this;
   }

   /** Unpack an array of objects of type <code>T</code>, where T
       <strong>must</strong> be a built-in type (ar at least something that
       can be copied with memcpy).

       If the third argument is true then memory is allocated for the array
       and the array pointer and the length of the array are returned in the
       arguments.

       If the third argument is false then the arriving array's length is
       compared to <code>length</code> and an exception is thrown if they are
       not the same. Also, the array passed as the first argument will be
       filled with the arriving array.
   */
   template <class T> AlpsEncoded& unpack(T*& values, int& length,
					 bool allocate = true)
     //     throw(AlpsException) {
     throw(CoinError) {
     if (allocate) {
#ifdef PARANOID
       if (pos_ + sizeof(int) > size_) {
	 throw CoinError("Reading over the end of buffer.", 
			 "AlpsEncoded", "unpack(T*& values, int& length,...");
       }
#endif
       memcpy(&length, representation_ + pos_, sizeof(int));
       pos_ += sizeof(int);
       if (length > 0){
#ifdef PARANOID
	 if (pos_ + sizeof(T)*length > size_) {
	   throw CoinError("Reading over the end of buffer.", 
			   "AlpsEncoded", "unpack(T*& values, int& le..,...");
	 }
#endif
	 values = new T[length];
	 memcpy(values, representation_ + pos_, sizeof(T)*length);
	 pos_ += sizeof(T) * length;
       }
       
     } else { /* ! allocate */

       int l;
#ifdef PARANOID
       if (pos_ + sizeof(int) > size_) {
	 throw CoinError("Reading over the end of buffer.", 
			 "AlpsEncoded", "unpack(T*& values, int& le..,...");
       }
#endif
       memcpy(&l, representation_ + pos_, sizeof(int));
       pos_ += sizeof(int);
       if (l != length) {
	 throw CoinError("Reading over the end of buffer.", 
			 "AlpsEncoded", "unpack(T*& values, int& le..,...");
       }
       if (length > 0){
#ifdef PARANOID
	 if (pos_ + sizeof(T)*length > size_) {
	   throw CoinError("Reading over the end of buffer.", 
			   "AlpsEncoded", "unpack(T*& values, int& le..,...");
	 }
#endif
	 memcpy(values, representation_ + pos_, sizeof(T)*length);
	 pos_ += sizeof(T) * length;
       }       
     }
     
     return *this;
   }


   /** Pack a <code>AlpsString</code> into the buffer. */
   AlpsEncoded& pack(AlpsString& value){
      // must define here, 'cos in *_message.C we have only templated members
      const int len = value.length();
      make_fit( sizeof(int) + len );
      memcpy(representation_ + size_, &len, sizeof(int));
      size_ += sizeof(int);
      if (len > 0){
	 memcpy(representation_ + size_, value.c_str(), len);
	 size_ += len;
      }
      return *this;
   }
   /** Unpack a <code>AlpsString</code> from the buffer. */
   AlpsEncoded& unpack(AlpsString& value){
      int len;
      unpack(len);
      value.assign(representation_ + pos_, len);
      pos_ += len;
      return *this;
   }

   // packing/unpacking for AlpsVec
   /** Pack a <code>AlpsVec</code> into the buffer. */
   template <class T> AlpsEncoded& pack(const AlpsVec<T>& vec) {
     int objnum = vec.size();
     int new_bytes = objnum * sizeof(T);
     make_fit( sizeof(int) + new_bytes );
     memcpy(representation_ + size_, &objnum, sizeof(int));
     size_ += sizeof(int);
     if (objnum > 0){
       memcpy(representation_ + size_, vec.begin(), new_bytes);
       size_ += new_bytes;
     }
     return *this;
   }

   /** Pack a <code>std::vector</code> into the buffer. */
   template <class T> AlpsEncoded& pack(const std::vector<T>& vec) {
     int objnum = vec.size();
     int new_bytes = objnum * sizeof(T);
     make_fit( sizeof(int) + new_bytes );
     memcpy(representation_ + size_, &objnum, sizeof(int));
     size_ += sizeof(int);
     if (objnum > 0){
       memcpy(representation_ + size_, &vec[0], new_bytes);
       size_ += new_bytes;
     }
     return *this;
   }

   /** Unpack a <code>AlpsVec</code> from the buffer. */
   template <class T> AlpsEncoded& unpack(AlpsVec<T>& vec) {
     int objnum;
#ifdef PARANOID
     if (pos_ + sizeof(int) > size_)
      throw CoinError("Reading over the end of buffer.", 
		      "AlpsEncoded", "unpack(AlpsVec<T>& vec");
#endif
     memcpy(&objnum, representation_ + pos_, sizeof(int));
     pos_ += sizeof(int);
     vec.clear();
     if (objnum > 0){
#ifdef PARANOID
       if (pos_ + sizeof(T)*objnum > size_)
	 throw CoinError("Reading over the end of buffer.", 
			 "AlpsEncoded", "unpack(AlpsVec<T>& vec");
#endif
       vec.reserve(objnum);
       vec.insert(vec.end(), representation_ + pos_, objnum);
       pos_ += objnum * sizeof(T);
     }
     return *this;
   }

   /** Unpack a <code>std::vector</code> from the buffer. */
   template <class T> AlpsEncoded& unpack(std::vector<T>& vec) {
     int objnum;
#ifdef PARANOID
     if (pos_ + sizeof(int) > size_)
       throw CoinError("Reading over the end of buffer.", 
		       "AlpsEncoded", "unpack(std::vector<T>& vec");
#endif
     memcpy(&objnum, representation_ + pos_, sizeof(int));
     pos_ += sizeof(int);
     vec.clear();
     if (objnum > 0){
#ifdef PARANOID
       if (pos_ + sizeof(T)*objnum > size_)
	 throw CoinError("Reading over the end of buffer.", 
			 "AlpsEncoded", "unpack(std::vector<T>& vec");
#endif
       vec.insert(vec.end(), objnum, T());
       memcpy(&vec[0], representation_ + pos_, objnum * sizeof(T));
       pos_ += objnum * sizeof(T);
     }
     return *this;
   }

   /*@}*/

};


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
  static std::map<const char*, const AlpsKnowledge*, AlpsStrLess>* decodeMap_;
  
  //  static std::map<const char*, AlpsKnowledge*, AlpsStrLess>* decodeMap_;
  /** The encoded object in an encoded form (could be compressed!) */
  //FIXME: For now, we just use a regular pointer here to get it to compile.
  //CoinPtr<AlpsEncoded> encoded_;
  AlpsEncoded* encoded_;
   
 public:
  AlpsKnowledge() : encoded_(0) {}
  virtual ~AlpsKnowledge() {}

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

  /** This method should encode the content of the object and return a
      pointer to the encoded form. 
      
      This default implementation can not be
      used when the memory of data members is not continously allocated,
      for example, some data members are pointers, STL set, map, etc. */
  virtual AlpsEncoded* encode() const {
    AlpsEncoded* encoded = new AlpsEncoded(typeid(*this).name());
    encoded->pack(*this);
    return encoded;
  }

  /** This method should decode and return a pointer to a <em>brand new
      object<em>, i.e., the method must create a new object on the heap from
      the decoded data instead of filling up the object for which the method
      was invoked. 
      
      This default implementation can not be
      used when the memory of data members is not continously allocated,
      for example, some data members are pointers, STL set, map, etc.
  */
  //  virtual AlpsKnowledge* decode(const AlpsEncoded&) const = 0;
  virtual AlpsKnowledge* decode(AlpsEncoded& encoded) const{
    AlpsKnowledge* kl = new AlpsKnowledge;
    encoded.unpack(kl);
    return kl;
  }
  
  /** This method returns the pointer to an empty object of the registered
      class <code>name</code>. Then the <code>decode()</code> method of that
      object can be used to decode a new object of the same type from the
      buffer. This method will be invoked as follows to decode an object
      whose type is <code>name</code>:<br>
      <code>obj = AlpsKnowledge::decoderObject(name)->decode(buf) </code> */
  static const AlpsKnowledge* decoderObject(const char* name) {
    return (*decodeMap_)[name];
  }

  /** */
  // CoinPtr<AlpsEncoded>&
  inline AlpsEncoded* getEncoded() const { return encoded_; }
   
  /** */
  // CoinPtr<AlpsEncoded>&
  inline void setEncoded(AlpsEncoded* e) { encoded_ = e; }
   
};

//#############################################################################
//#############################################################################

#endif
