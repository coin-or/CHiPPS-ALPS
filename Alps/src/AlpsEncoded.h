#ifndef AlpsEncoded_h
#define AlpsEncoded_h

#include <cstring>
#include <memory>
#include <vector>

#include "CoinError.hpp"

// AlpsEncoded is modified from BCP_buffer and CoinEncoded

//#############################################################################
/** This data structure is to contain the packed form of an encodable
 *    knowledge. It servers two purposes:
 *  <ul>
 *  <li> used as a buffer when passing messages
 *  <li> allow Alps to manipulate the user derived knowledge 
 *  </ul>
 */
//#############################################################################

class AlpsEncoded {  

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
	to point this into the static data of the executable :-)
	Can only be initialized during constructing. User take care of memory.*/
    const char* type_;   
    
    //char* type_;
    /** The size of the packed representation. */
    int size_;
    /** The encoded/compressed representation of the object. */
    // const char* representation_;   //why const ??? XY
    char* representation_;
    
 public:
    
    /**@name Constructors and destructor */
    /*@{*/
    /** The default constructor creates a buffer of size 16 Kbytes with no
	message in it. */
    
#if 0
    AlpsEncoded() 
	: 
	pos_(0), 
	maxSize_(0x4000/*16K*/), 
	type_(NULL), 
	size_(0), 
	representation_(new char[maxSize_]) 
	{}
#endif 
    
    AlpsEncoded(const char* t) 
	: 
	pos_(0), 
	maxSize_(0), 
	type_(t), 
	size_(0), 
	representation_(0) 
	{}

    // AlpsEncoded() : size_(0), representation_(NULL) {}
    // can't use "const char*& r" as parameter
    AlpsEncoded(const char* t, const int s, char*& r) 
	: 
	pos_(0),   
	maxSize_(s + 4),
	type_(t),  
	size_(s),
	representation_(r) 
	{
	    r = 0;  // Must take over the ownership! 
	}                    
    
    /** Destructor. */
    ~AlpsEncoded() {
	//if (type_ != 0) { delete type_; type_ = 0; }
	if (representation_ != 0) { 
	    delete [] representation_; 
	    representation_ = 0; 
	}
    }
    /*@}*/
    
    /**@name Query methods */
    ///@{
    const char* type() const { return type_; }
    int size() const { return size_; }
    const char* representation() const { return representation_; }
    ///@}

    inline void setPosition(const int pos) throw(CoinError) {
	if (pos < 0 || pos >= size()) {
	    //     const char msg [100] = "Incorrest position setting.";
	    //throw AlpsException(__FILE__, __LINE__, msg);
	    throw CoinError("Incorrest position setting.", "setPosition",
			    "AlpsEncoded");
	}
	pos_ = pos;
    }
  
    inline void setRepresentation(char*& buf) {
	maxSize_ = strlen(buf) + 1;   //> 0x4000/*16K*/ ? (strlen(buf)+1) : 0x4000;
    
#if 0    // Seg faulted. strlen(buf) = 1, Don't know why.
	std::cout << "setRep: strlen = " << strlen(buf) << std::endl;
	if (representation_ != 0) {
	    delete [] representation_; representation_ = 0;
	}
	representation_ = new char [maxSize_]; //= buf;   //new char [maxSize_];
	memcpy(representation_, buf, maxSize_ - 1);
#endif 

	representation_ = buf;
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
    inline void setEncoded(const char* type, const char* rep, const size_t size) {
    if (maxSize_ < size) {
	delete[] representation_;
	representation_ = new char[size];
	maxSize_ = size;
    }
    pos_ = 0;
    size_ = size;
    if (size_) memcpy(representation_, rep, size * sizeof(char));
    
    // Copy type
    int typeLen = strlen(type);
    if (typeLen) {
	type_ = new char[typeLen];
	memcpy(type_, type, typeLen);
    }
    else 
	type_ = 0;
    if(!size_ < 0 ) {
	//      const char msg [100] = "The size of rep < 0.";
	//      throw AlpsException(__FILE__, __LINE__, msg);
	throw CoinError("The size of rep < 0.", "setEncoded",
			"AlpsEncoded");
    }
    }
#endif

  /** Reallocate the size of encoded if necessary so that at least
      <code>addsize_</code> number of additional bytes will fit into the
      encoded. */
    inline void make_fit(const int addSize){
	assert(addSize > 0);
	size_t addSize1 = static_cast<size_t>(addSize);
	
	if (maxSize_ < size_ + addSize1){
	    maxSize_ = 4 * (size_ + addSize1 + 0x1000/*4K*/);
	    char* newRep = new char[maxSize_];
	    if (size_)
		memcpy(newRep, representation_, size_);
	    delete[] representation_;
	    representation_ = newRep;
	}
    }

    /** Completely clear the encoded. Delete and zero out <code>type_, size_,
	pos_</code>. */
    inline void clean(){
	size_ = 0;
	pos_ = 0;
	if (type_ != 0) { delete type_; type_ = 0; }
	if (representation_ != 0) { 
	    delete  representation_; 
	    representation_ = 0; 
	}
    }

    
    //------------------------------------------------------
    // Following functiosn are used in parallel code only.
    //------------------------------------------------------

    /** Write a single object of type <code>T</code> in <code>repsentation_
	</code>. Copies <code>sizeof(T)</code> bytes from 
	the address of the object. */
    template <class T> AlpsEncoded& writeRep(const T& value) {
	make_fit( sizeof(T) );
	memcpy(representation_ + size_, &value, sizeof(T));
	size_ += sizeof(T);
	return *this;
    }

    /** Read a single object of type <code>T</code> from <code>repsentation_
	</code>. Copies <code>sizeof(T)</code> bytes to the address of 
	the object. */
    template <class T> AlpsEncoded& readRep(T& value){
#ifdef PARANOID
	if (pos_ + sizeof(T) > size_) {
	    throw CoinError("Reading over the end of buffer.", 
			    "readRep(const T& value)", "AlpsEncoded");
	}
#endif
	memcpy(&value, representation_ + pos_, sizeof(T));
	pos_ += sizeof(T);
	return *this;
    }


    /** Write a C style array of objects of type <code>T</code> in 
	<code>repsentation_</code>. First write the length, 
	then write the content of the array */
    template <class T> AlpsEncoded& writeRep(const T* const values,
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

    /** Read an array of objects of type <code>T</code> from <code>
	repsentation_</code>, where T <strong>must</strong> be a 
	built-in type (ar at least something that can be copied with memcpy).
	
	If the third argument is true then memory is allocated for the array
	and the array pointer and the length of the array are returned in the
	arguments.

	If the third argument is false then the arriving array's length is
	compared to <code>length</code> and an exception is thrown if they are
	not the same. Also, the array passed as the first argument will be
	filled with the arriving array.
    */
    template <class T> AlpsEncoded& readRep(T*& values, 
					    int& length,
					    bool allocate = true)
	//     throw(AlpsException) {
	throw(CoinError) {
	if (allocate) {
	    // Need allocate memeory for arrary values.

#ifdef PARANOID
	    if (pos_ + sizeof(int) > size_) {
		throw CoinError("Reading over the end of buffer.", 
				"readRep(T*& values, int& length,...", 
				"AlpsEncoded");
	    }
#endif
	    memcpy(&length, representation_ + pos_, sizeof(int));
	    pos_ += sizeof(int);
	    if (length > 0){
#ifdef PARANOID
		if (pos_ + sizeof(T)*length > size_) {
		    throw CoinError("Reading over the end of buffer.", 
				    "readRep(T*& values, int& length,...",
				    "AlpsEncoded");
		}
#endif
		values = new T[length];
		memcpy(values, representation_ + pos_, sizeof(T)*length);
		pos_ += sizeof(T) * length;
	    }
	    
	} 
	else { /* ! allocate */

	    int l;
#ifdef PARANOID
	    if (pos_ + sizeof(int) > size_) {
		throw CoinError("Reading over the end of buffer.", 
				"readRep(T*& values, int& length,...",
				"AlpsEncoded");
	    }
#endif
	    memcpy(&l, representation_ + pos_, sizeof(int));
	    pos_ += sizeof(int);
	    if (l != length) {
		throw CoinError("Reading over the end of buffer.", 
				"readRep(T*& values, int& length,...", 
				"AlpsEncoded");
	    }
	    if (length > 0){
#ifdef PARANOID
		if (pos_ + sizeof(T)*length > size_) {
		    throw CoinError("Reading over the end of buffer.", 
				    "readRep(T*& values, int& length,...",
				    "AlpsEncoded");
		}
#endif
		memcpy(values, representation_ + pos_, sizeof(T)*length);
		pos_ += sizeof(T) * length;
	    }       
	}
	
	return *this;
    }

    /** Read a <code>std::string</code> in <code>repsentation_ </code>. */
    AlpsEncoded& writeRep(std::string& value){
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

    /** Read a <code>std::string</code> from <code>repsentation_ </code>. */
    AlpsEncoded& readRep(std::string& value){
	int len;
	readRep(len);
	value.assign(representation_ + pos_, len);
	pos_ += len;
	return *this;
    }
    
    /** Write a <code>std::vector</code> into <code>repsentation_ </code>. */
    template <class T> AlpsEncoded& writeRep(const std::vector<T>& vec) {
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
    
   /** Read a <code>std::vector</code> from <code>repsentation_ </code>. */
   template <class T> AlpsEncoded& readRep(std::vector<T>& vec) {
       int objnum;
#ifdef PARANOID
       if (pos_ + sizeof(int) > size_)
	   throw CoinError("Reading over the end of buffer.", 
			   "AlpsEncoded", "readRep(std::vector<T>& vec");
#endif
       memcpy(&objnum, representation_ + pos_, sizeof(int));
       pos_ += sizeof(int);
       vec.clear();
       if (objnum > 0){
#ifdef PARANOID
	   if (pos_ + sizeof(T)*objnum > size_)
	       throw CoinError("Reading over the end of buffer.", 
			       "AlpsEncoded", "readRep(std::vector<T>& vec");
#endif
	   vec.insert(vec.end(), objnum, T());
	   memcpy(&vec[0], representation_ + pos_, objnum * sizeof(T));
	   pos_ += objnum * sizeof(T);
       }
       return *this;
   }
   /*@}*/

};

#endif
