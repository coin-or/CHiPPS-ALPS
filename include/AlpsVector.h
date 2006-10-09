#include "AlpsLicense.h"

#ifndef AlpsVector_h
#define AlpsVector_h

// AlpsVec is modified from BCP_vec. Explicit instantiation is not used.
// This file is fully docified.

#include <memory>
#include "CoinError.hpp"
#include "AlpsOs.h"

/**
   The class AlpsVec serves the same purpose as the vector class in the
   standard template library. The main difference is that while the vector 
   class is <em>likely</em> to be implemented as a memory array, AlpsVec 
   <em>is</em> implemented that way. Also, AlpsVec has a number of extra
   member methods, most of them exist to speed up operations (e.g.,
   there are <em>unchecked</em> versions of the insert member methods,
   i.e., the method does not check whether there is enough space
   allocated to fit the new elements). 
*/

template <class T> class AlpsVec {
public:
   /**@name Type definitions (needed for using the STL) */
   /*@{*/
   ///
   typedef size_t size_type;
   ///
   typedef T value_type;
   ///
   typedef T* iterator;
   ///
   typedef const T* const_iterator;
   ///
   typedef T& reference;
   ///
   typedef const T& const_reference;
   /*@}*/

protected:
   /**@name Internal methods */
   /*@{*/
   /** insert <code>x</code> into the given <code>position</code> in the
       vector. Reallocate the vector if necessary. */
   void insert_aux(iterator position, const_reference x);
   /** allocate raw, uninitialized memory for <code>len</code> entries. */
   inline iterator allocate(size_t len) {
      return static_cast<iterator>(::operator new(len * sizeof(T)));
   }
   /** Destroy the entries in the vector and free the memory allocated for the
       vector. */
   void deallocate();
   /*@}*/

protected:
   /**@name Data members */
   /*@{*/
   /** Iterator pointing to the beginning of the memory array
       where the vector is stored. */ 
   iterator start;
   /** Iterator pointing to right after the last entry in the vector. */ 
   iterator finish;
   /** Iterator pointing to right after the last memory location usable by the
       vector without reallocation. */
   iterator end_of_storage;
   /*@}*/

public:
   /**@name Constructors / Destructor */
   /*@{*/
   /** The default constructor initializes the data members as 0 pointers. */
   AlpsVec() : start(0), finish(0), end_of_storage(0) {}
   /** The copy constructor copies over the content of <code>x</code>. */
   AlpsVec(const AlpsVec<T>& x) : start(0), finish(0), end_of_storage(0) {
      operator=(x);
   }
   /** Construct a <code>AlpsVec</code> with <code>n</code> elements, all
       initialized with the second argument (or initialized with the default
       constructor of <code>T</code> if the second argument is missing). */
   AlpsVec(const size_t n, const_reference value = T());
   /** Construct a <code>AlpsVec</code> by copying the elements from
       <code>first</code> to <code>last-1</code>. */
   AlpsVec(const_iterator first, const_iterator last);
   /** Construct a <code>AlpsVec</code> by copying <code>num</code> objects
       of type <code>T</code> from the memory starting at <code>x</code>. */
   AlpsVec(const T* x, const size_t num);
   /** The destructor deallocates the memory allocated for the
       <code>AlpsVec</code>. */
   virtual ~AlpsVec() { deallocate(); }
   /*@}*/

   /**@name Query methods */
   /*@{*/
   /** Return an iterator to the beginning of the object. */
   iterator begin()                           { return start; }
   /** Return a const iterator to the beginning of the object. */
   const_iterator begin() const               { return start; }

   /** Return an iterator to the end of the object. */
   iterator end()                             { return finish; }
   /** Return a const iterator to the end of the object. */
   const_iterator end() const                 { return finish; }

   /** Return an iterator to the <code>i</code>-th entry. */
   iterator entry(const int i)                { return start + i; }
   /** Return a const iterator to the <code>i</code>-th entry. */
   const_iterator entry(const int i) const    { return start + i; }

   /** Return the index of the entry pointed to by <code>pos</code>. */
   size_t index(const_iterator pos) const     { return size_t(pos - start); }
   /** Return the current number of entries. */
   size_t size() const                        { return finish - start; }
   /** Return the capacity of the object (space allocated for this many
       entries). */
   size_t capacity() const                    { return end_of_storage - start;}
   /** Test if there are any entries in the object. */
   bool empty() const                         { return start == finish; }

   /** Return a reference to the <code>i</code>-th entry. */
   reference operator[](const size_t i)       { return *(start + i); }
   /** Return a const reference to the <code>i</code>-th entry. */
   const_reference operator[](const size_t i) const { return *(start + i); }

   /** Return a reference to the first entry. */
   reference front()                          { return *start; }
   /** Return a const reference to the first entry. */
   const_reference front() const              { return *start; }
   /** Return a reference to the last entry. */
   reference back()                           { return *(finish - 1); }
   /** Return a const reference to the last entry. */
   const_reference back() const               { return *(finish - 1); }
   /*@}*/

   /**@name General modifying methods */
   /*@{*/
   /** Reallocate the object to make space for <code>n</code> entries. */
   void reserve(const size_t n);
   /** Exchange the contents of the object with that of <code>x</code>. */
   void swap(AlpsVec<T>& x);

   /** Copy the contents of <code>x</code> into the object and return a
       reference the the object itself. */
   AlpsVec<T>& operator=(const AlpsVec<T>& x);

   /** Copy <code>num</code> entries of type <code>T</code> starting at the
       memory location <code>x</code> into the object. (<code>x</code> is a
       void pointer since it might be located somewhere in a buffer and
       therefore might not be aligned for type <code>T</code> entries.) */ 
   void assign(const void* x, const size_t num);
   /** Insert <code>num</code> entries starting from memory location
       <code>first</code> into the vector from position <code>pos</code>. */
   void insert(iterator position, const void* first, const size_t num);
   
   /** Insert the entries <code>[first,last)</code> into the vector from
       position <code>pos</code>. */
   void insert(iterator position, const_iterator first, const_iterator last);
   /** Insert <code>n</code> copies of <code>x</code> into the vector from
       position <code>pos</code>. */
   void insert(iterator position, const size_t n, const_reference x);
   /** Insert <code>x</code> (a single entry) into the vector at position
       <code>pos</code>. Return an iterator to the newly inserted entry. */
   iterator insert(iterator position, const_reference x);

   /** Append the entries in <code>x</code> to the end of the vector. */
   void append(const AlpsVec<T>& x) {
      insert(end(), x.begin(), x.end()); }
   /** Append the entries <code>[first,last)</code> to the end of the vector.
    */
   void append(const_iterator first, const_iterator last) {
      insert(end(), first, last); }

   /** Append <code>x</code> to the end of the vector. Check if enough space
       is allocated (reallocate if necessary). */
   inline void push_back(const_reference x) {
      if (finish != end_of_storage) {
	 ALPS_CONSTRUCT(finish++, x);
      } else
	 insert_aux(finish, x);
   }
   /** Append <code>x</code> to the end of the vector. Does not check if
       enough space is allcoated. */
   inline void unchecked_push_back(const_reference x) {
      ALPS_CONSTRUCT(finish++, x);
   }
   /** Delete the last entry. */
   inline void pop_back() {
      ALPS_DESTROY(--finish);
   }

   /** Delete every entry. */
   void clear() {
      if (start) erase(start, finish);
   }

   /** Update those entries listed in <code>positions</code> to the given
       <code>values</code>. The two argument vector must be of equal length.
       Sanity checks are done on the given positions. */
   void update(const AlpsVec<int>& positions,
	       const AlpsVec<T>& values);
   /** Same as the previous method but without sanity checks. */
   void unchecked_update(const AlpsVec<int>& positions,
			 const AlpsVec<T>& values);

   /*@}*/

   //--------------------------------------------------------------------------

   /**@name Methods for selectively keeping entries */
   /*@{*/
   /** Keep only the entry pointed to by <code>pos</code>. */
   void keep(iterator pos);
   /** Keep the entries <code>[first,last)</code>. */
   void keep(iterator first, iterator last);
   /** Keep the entries indexed by <code>indices</code>. Abort if the indices
       are not in increasing order, if there are duplicate indices or if any
       of the indices is outside of the range <code>[0,size())</code>. */
   void keep_by_index(const AlpsVec<int>& positions);
   /** Same as the previous method but without the sanity checks. */
   void unchecked_keep_by_index(const AlpsVec<int>& positions);
   /** Like the other <code>keep_by_index</code> method (including sanity
       checks), just the indices of the entries to be kept are given in
       <code>[firstpos,lastpos)</code>. */
   void keep_by_index(const int * firstpos, const int * lastpos);
   /** Same as the previous method but without the sanity checks. */
   void unchecked_keep_by_index(const int * firstpos, const int * lastpos);
   /*@}*/

   //-------------------------------------------------------------------------

   /**@name Methods for selectively erasing entries */
   /*@{*/
   /** Erase the entry pointed to by <code>pos</code>. */
   void erase(iterator pos);
   /** Erase the entries <code>[first,last)</code>. */
   void erase(iterator first, iterator last);
   /** Erase the entries indexed by <code>indices</code>. Abort if the indices
       are not in increasing order, if there are duplicate indices or if any
       of the indices is outside of the range <code>[0,size())</code>. */
   void erase_by_index(const AlpsVec<int>& positions);
   /** Same as the previous method but without the sanity check. */
   void unchecked_erase_by_index(const AlpsVec<int>& positions);
   /** Like the other <code>erase_by_index</code> method (including sanity
       checks), just the indices of the entries to be erased are given in
       <code>[firstpos,lastpos)</code>. */
   void erase_by_index(const int * firstpos, const int * lastpos);
   /** Same as the previous method but without the sanity checks. */
   void unchecked_erase_by_index(const int * firstpos, const int * lastpos);
   /*@}*/
};

//#############################################################################



//#############################################################################

template <class T> inline void
AlpsVec<T>::deallocate() {
   if (start) {
      while (finish != start) {
	 ALPS_DESTROY(--finish);
      }
      ::operator delete(start);
   }
}

//#############################################################################

template <class T> void
AlpsVec<T>::insert_aux(iterator position, const_reference x){
   if (finish != end_of_storage) {
      ALPS_CONSTRUCT(finish, *(finish - 1));
      std::copy_backward(position, finish - 1, finish);
      *position = x;
      ++finish;
   } else {
      const size_t len = (2*size() + 0x1000) * sizeof(T);
      iterator tmp = allocate(len);
      iterator tmp_finish = std::uninitialized_copy(start, position, tmp);
      ALPS_CONSTRUCT(tmp_finish++, x);
      tmp_finish = std::uninitialized_copy(position, finish, tmp_finish);
      deallocate();
      start = tmp;
      finish = tmp_finish;
      end_of_storage = tmp + len;
   }
}

//#############################################################################

template <class T>
AlpsVec<T>::AlpsVec(const size_t n, const_reference value) :
   start(0), finish(0), end_of_storage(0)
{
   if (n > 0) {
      start = allocate(n);
      end_of_storage = finish = start + n;
      std::uninitialized_fill_n(start, n, value);
   }
}

template <class T>
AlpsVec<T>::AlpsVec(const_iterator first, const_iterator last) :
   start(0), finish(0), end_of_storage(0)
{
   const size_t len = last - first;
   if (len > 0) {
      start = allocate(len);
      end_of_storage = finish = std::uninitialized_copy(first, last, start);
   }
}

template <class T>
AlpsVec<T>::AlpsVec(const T* x, const size_t num) :
   start(0), finish(0), end_of_storage(0)
{
   if (num > 0) {
      finish = start = allocate(num);
      const T* const lastx = x + num;
      while (x != lastx)
	 ALPS_CONSTRUCT(finish++, *x++);
      end_of_storage = finish;
   }
}

//#############################################################################

template <class T> void
AlpsVec<T>::keep(iterator pos) {
   iterator oldfinish = finish;
   finish = std::copy(pos, pos + 1, start);
   ALPS_DESTROY_RANGE(finish, oldfinish);
}

template <class T> void
AlpsVec<T>::keep(iterator first, iterator last) {
   iterator oldfinish = finish;
   finish = std::copy(first, last, start);
   ALPS_DESTROY_RANGE(finish, oldfinish);
}

//#############################################################################

template <class T> void
AlpsVec<T>::erase(iterator position) {
   if (position + 1 != finish)
      std::copy(position + 1, finish, position);
   ALPS_DESTROY(--finish);
}

template <class T> void
AlpsVec<T>::erase(iterator first, iterator last) {
   iterator oldfinish = finish;
   finish = std::copy(last, finish, first);
   ALPS_DESTROY_RANGE(finish, oldfinish);
}

//#############################################################################
//#############################################################################

template <class T> void
AlpsVec<T>::reserve(const size_t n){
   if (capacity() < n) {
      iterator tmp = allocate(n);
      iterator tmp_finish = std::uninitialized_copy(start, finish, tmp);
      deallocate();
      start = tmp;
      finish = tmp_finish;
      end_of_storage = start + n;
   }
}

//#############################################################################

template <class T> void
AlpsVec<T>::swap(AlpsVec<T>& x) {
   std::swap(start, x.start);
   std::swap(finish, x.finish);
   std::swap(end_of_storage, x.end_of_storage);
}
   
//#############################################################################

template <class T> AlpsVec<T>&
AlpsVec<T>::operator=(const AlpsVec<T>& x){
   if (&x != this) {
      const size_t x_size = x.size();
      if (x_size > capacity()) {
	 deallocate();
	 start = allocate(x_size);
	 end_of_storage = start + x_size;
	 finish = std::uninitialized_copy(x.begin(), x.end(), start);
      } else {
	 if (x_size < size()) {
	    iterator oldfinish = finish;
	    finish = std::copy(x.begin(), x.end(), start);
	    ALPS_DESTROY_RANGE(finish, oldfinish);
	 } else {
	    std::copy(x.begin(), x.entry(size()), start);
	    finish = std::uninitialized_copy(x.entry(size()), x.end(), finish);
	 }
      }
   }
   return *this;
}

//#############################################################################
// need the void* here, since we occasionally want to copy out of a buffer

template <class T> void
AlpsVec<T>::assign(const void* x, const size_t num){
   if (num > capacity()){
      deallocate();
      start = allocate(num);
      end_of_storage = start + num;
   } else {
      ALPS_DESTROY_RANGE(start, finish);
   }
   T entry;
   finish = start;
   const char* charx = static_cast<const char*>(x);
   for (int i = num; i > 0; --i) {
      memcpy(&entry, charx, sizeof(T));
      ALPS_CONSTRUCT(finish++, entry);
      charx += sizeof(T);
   }
}

//#############################################################################

template <class T> void
AlpsVec<T>::insert(iterator position, const void* first, const size_t n){
   if (n == 0) return;
   T entry;
   const char* charx = static_cast<const char*>(first);
   if ((size_t) (end_of_storage - finish) >= n) {
      const size_t to_move = finish - position;
      if (to_move <= n) {
	 std::uninitialized_copy(position, finish, position + n);
	 finish += n;
	 size_t i = n;
	 for ( ; i > to_move; --i) {
	    memcpy(&entry, charx, sizeof(T));
	    ALPS_CONSTRUCT(position++, entry);
	    charx += sizeof(T);
	 }
	 for ( ; i > 0; --i) {
	    memcpy(&entry, charx, sizeof(T));
	    *position++ = entry; 
	    charx += sizeof(T);
	 }
      } else {
	 std::uninitialized_copy(finish - n, finish, finish);
	 std::copy_backward(position, finish - n, finish);
	 finish += n;
	 for (int i = n; i > 0; --i) {
	    memcpy(&entry, charx, sizeof(T));
	    *position++ = entry; 
	    charx += sizeof(T);
	 }
      }
   } else {
      const size_t new_size = (2*size() + n) * sizeof(T);
      iterator tmp = allocate(new_size);
      iterator tmp_finish = std::uninitialized_copy(start, position, tmp);
      for (int i = n; i > 0; --i) {
	 memcpy(&entry, charx, sizeof(T));
	 ALPS_CONSTRUCT(tmp_finish++, entry);
	 charx += sizeof(T);
      }
      tmp_finish = std::uninitialized_copy(position, finish, tmp_finish);
      deallocate();
      start = tmp;
      finish = tmp_finish;
      end_of_storage = tmp + new_size;
   }
}

//#############################################################################

template <class T> void
AlpsVec<T>::insert(iterator position,
		   const_iterator first, const_iterator last){
   if (first == last) return;
   const size_t n = last - first;
   if ((size_t) (end_of_storage - finish) >= n) {
      const size_t to_move = finish - position;
      if (to_move <= n) {
	 std::uninitialized_copy(position, finish, position + n);
	 std::copy(first, first + to_move, position);
	 std::uninitialized_copy(first + to_move, last, finish);
      } else {
	 std::uninitialized_copy(finish - n, finish, finish);
	 std::copy_backward(position, finish - n, finish);
	 std::copy(first, last, position);
      }
      finish += n;
   } else {
      const size_t new_size = (2*size() + n) * sizeof(T);
      iterator tmp = allocate(new_size);
      iterator tmp_finish = std::uninitialized_copy(start, position, tmp);
      tmp_finish = std::uninitialized_copy(first, last, tmp_finish);
      tmp_finish = std::uninitialized_copy(position, finish, tmp_finish);
      deallocate();
      start = tmp;
      finish = tmp_finish;
      end_of_storage = tmp + new_size;
   }
}

//#############################################################################

template <class T> void
AlpsVec<T>::insert(iterator position, const size_t n, const_reference x) {
   if (n == 0) return;
   if ((size_t) (end_of_storage - finish) >= n) {
      const size_t to_move = finish - position;
      if (to_move <= n) {
	 std::uninitialized_copy(position, finish, position + n);
	 std::fill_n(position, to_move, x);
	 std::uninitialized_fill_n(finish, n - to_move, x);
      } else {
	 std::uninitialized_copy(finish - n, finish, finish);
	 std::copy_backward(position, finish - n, finish);
	 std::fill_n(position, n, x);
      }
      finish += n;
   } else {
      const size_t new_size = (2*size() + n) * sizeof(T);
      iterator tmp = allocate(new_size);
      iterator tmp_finish = std::uninitialized_copy(start, position, tmp);
      std::uninitialized_fill_n(tmp_finish, n, x);
      tmp_finish += n;
      tmp_finish = std::uninitialized_copy(position, finish, tmp_finish);
      deallocate();
      start = tmp;
      finish = tmp_finish;
      end_of_storage = tmp + new_size;
   }
}

//#############################################################################

template <class T> typename AlpsVec<T>::iterator
AlpsVec<T>::insert(iterator position, const_reference x){
   const size_t n = position - start;
   if (finish != end_of_storage && position == finish) {
      ALPS_CONSTRUCT(finish++, x);
   } else {
      insert_aux(position, x);
   }
   return start + n;
}

//#############################################################################


template <class T> void
AlpsVec<T>::keep_by_index(const AlpsVec<int>& positions){
   AlpsVecSanityCheck(positions.begin(), positions.end(), size());
   unchecked_keep_by_index(positions.begin(), positions.end());
}
//-----------------------------------------------------------------------------

template <class T> void
AlpsVec<T>::unchecked_keep_by_index(const AlpsVec<int>& positions){
   unchecked_keep_by_index(positions.begin(), positions.end());
}

//#############################################################################

template <class T> void
AlpsVec<T>::erase_by_index(const AlpsVec<int>& positions){
   AlpsVecSanityCheck(positions.begin(), positions.end(), size());
   unchecked_erase_by_index(positions.begin(), positions.end());
}

//-----------------------------------------------------------------------------

template <class T> void
AlpsVec<T>::unchecked_erase_by_index(const AlpsVec<int>& positions){
   unchecked_erase_by_index(positions.begin(), positions.end());
}

//#############################################################################

template <class T> void
AlpsVec<T>::keep_by_index(AlpsVec<int>::const_iterator firstpos,
			  AlpsVec<int>::const_iterator lastpos) {
   AlpsVecSanityCheck(firstpos, lastpos, size());
   unchecked_keep_by_index(firstpos, lastpos);
}

//-----------------------------------------------------------------------------

template <class T> void
AlpsVec<T>::unchecked_keep_by_index(AlpsVec<int>::const_iterator firstpos,
				    AlpsVec<int>::const_iterator lastpos) {
   if (firstpos == lastpos) {
      clear();
   } else {
      iterator target = start;
      while ( firstpos != lastpos )
	 *target++ = operator[](*firstpos++);
      ALPS_DESTROY_RANGE(target, finish);
      finish = target;
   }
}

//#############################################################################

template <class T> void
AlpsVec<T>::erase_by_index(AlpsVec<int>::const_iterator firstpos,
			   AlpsVec<int>::const_iterator lastpos) {
   AlpsVecSanityCheck(firstpos, lastpos, size());
   unchecked_erase_by_index(firstpos, lastpos);
}

//-----------------------------------------------------------------------------

template <class T> void
AlpsVec<T>::unchecked_erase_by_index(AlpsVec<int>::const_iterator firstpos,
				     AlpsVec<int>::const_iterator lastpos) {
   if (firstpos == lastpos)
      return;
   --lastpos;
   int source;
   iterator target = entry(*firstpos);
   while (firstpos != lastpos){
      source = *firstpos + 1;
      ++firstpos;
      if (*firstpos > source)
	 target = std::copy( entry(source), entry(*firstpos), target );
   }
   iterator oldfinish = finish;
   finish = std::copy( entry(*firstpos + 1), end(), target );
   ALPS_DESTROY_RANGE(finish, oldfinish);
}

//#############################################################################

template <class T> void
AlpsVec<T>::update(const AlpsVec<int>& positions,
		   const AlpsVec<T>& values){
   if (positions.size() != values.size())
      throw CoinError("AlpsVec::update() called with unequal sizes.",
		      "update", "AlpsVec<T>");
   AlpsVecSanityCheck(positions.begin(), positions.end(), size());
   unchecked_update(positions, values);
}

//-----------------------------------------------------------------------------

template <class T> void
AlpsVec<T>::unchecked_update(const AlpsVec<int>& positions,
			     const AlpsVec<T>& values){
   if (positions.size() == 0)
      return;
   const_iterator val = values.begin();
   AlpsVec<int>::const_iterator pos = positions.begin();
   const AlpsVec<int>::const_iterator lastpos = positions.end();
   while (pos != lastpos)
      operator[](*pos++) = *val++;
}

//#############################################################################

template <class T>
bool operator==(const AlpsVec<T>& x, const AlpsVec<T>& y) {
   return x.size() == y.size() && equal(x.begin(), x.end(), y.begin());
}

template <class T>
bool operator< (AlpsVec<T>& x, AlpsVec<T>& y) {
   return lexicographical_compare(x.begin(), x.end(), y.begin(), y.end());
}

//#############################################################################

//============================================================================

// template <class T>
// bool operator==(const AlpsVec<T>& x, const AlpsVec<T>& y);

// template <class T>
// bool operator< (AlpsVec<T>& x, AlpsVec<T>& y);

//#############################################################################
//#############################################################################

/** This function purges the entries <code>[first,last)</code> from the vector
    of pointers <code>pvec</code>.
    Since these entries are pointers, first <code>operator delete</code> is
    invoked for each of them, then the pointers are erased from the vector. */

template <class T> void purgePtrVector(AlpsVec<T*>& pvec,
					 typename AlpsVec<T*>::iterator first,
					 typename AlpsVec<T*>::iterator last)
{
   typename AlpsVec<T*>::iterator origfirst = first;
   while (first != last) {
      delete *first;
      *first = 0;
      ++first;
   }
   pvec.erase(origfirst, last);
}


/** This function purges all the entries from the vector of pointers
    <code>pvec</code>. 
    Since these entries are pointers, first <code>operator delete</code> is
    invoked for each of them, then the pointers are erased from the vector. */

template <class T> void purgePtrVector(AlpsVec<T*>& pvec)
{
   purgePtrVector(pvec, pvec.begin(), pvec.end());
}


/** This function purges the entries indexed by <code>[first,last)</code> from
    the vector of pointers <code>pvec</code>. 
    Since these entries are pointers, first <code>operator delete</code> is
    invoked for each of them, then the pointers are erased from the vector. */

template <class T>
void purgePtrVectorByIndex(AlpsVec<T*>& pvec,
			       typename AlpsVec<int>::const_iterator first,
			       typename AlpsVec<int>::const_iterator last)
{
   AlpsVec<int>::const_iterator origfirst = first;
   while (first != last) {
      delete pvec[*first];
      pvec[*first] = 0;
      ++first;
   }
   pvec.erase_by_index(origfirst, last);
}


/** This function keeps only the entries indexed by <code>[first,last)</code>
    from the vector of pointers <code>pvec</code>. 
    No sanity check is performed. */

template <class T>
void keepPtrVectorByIndex(AlpsVec<T*>& pvec,
			      typename AlpsVec<int>::const_iterator first,
			      typename AlpsVec<int>::const_iterator last)
{
   AlpsVec<int>::const_iterator origfirst = first;
   const int pvec_size = pvec.size();
   int i;

   for (i = 0;  i < pvec_size && first != last; ++i) {
      if (i != *first) {
	 delete pvec[i];
	 pvec[i] = 0;
      } else {
	 ++first;
      }
   }

   for ( ; i < pvec_size; ++i) {
      delete pvec[i];
      pvec[i] = 0;
   }
   pvec.keep_by_index(origfirst, last);
}

template <class T>
void AlpsVecSanityCheck(typename AlpsVec<T>::const_iterator firstpos,
			typename AlpsVec<T>::const_iterator lastpos,
			const int maxsize)
{
   if (firstpos == lastpos)
      return;
   if (*firstpos < 0)
     throw CoinError("Negative entry in a AlpsVecSanityCheck.", 
		     "AlpsVecSanityCheck", "AlpsVecSanityCheck");
   if (*(lastpos - 1) >= maxsize)
     throw CoinError("Too big entry in a AlpsVecSanityCheck.", 
		     "AlpsVecSanityCheck", "AlpsVecSanityCheck");
   int prev = -1;
   while (firstpos != lastpos){
      if (*firstpos < prev)
	throw CoinError("Entry list is not ordered in AlpsVecSanityCheck.", 
			"AlpsVecSanityCheck", "AlpsVecSanityCheck");
      if (*firstpos == prev)
	throw CoinError("Duplicate entry in AlpsVecSanityCheck.", 
			"AlpsVecSanityCheck", "AlpsVecSanityCheck");
      prev = *firstpos;
      ++firstpos;
   }
}

#endif
