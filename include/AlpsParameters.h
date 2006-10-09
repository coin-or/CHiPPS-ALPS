#include "AlpsLicense.h"

#ifndef AlpsParameters_h
#define AlpsParameters_h

// AlpsParameters.h is modified from BCP_parameters.hpp
// This file is fully docified.

#include <utility> // for 'pair'
#include <iostream>
#include <fstream>
#include <string>
//#include <strstream> //FIXME: strstream is deprecated
#include <cctype>
#include <cstdio>
#include <algorithm>

#include "CoinError.hpp"
#include "AlpsString.h"
#include "AlpsVector.h"

//#############################################################################
//#############################################################################

class AlpsEncoded;


/** This enumerative constant describes the possible parameter types. */
enum AlpsParameterT{
  /** The type is not yet specified. Used only in the default constructor of
      a Alps parameter. */
  AlpsNoPar,
  /** Character parameter. */
  AlpsCharPar,
  /** Integer parameter. */
  AlpsIntPar,
  /** Double parameter. */
  AlpsDoublePar,
  /** String parameter. */
  AlpsStringPar,
  /** The parameter is an array of strings. (E.g., the names of machines in
      the parallel configuration.) */
  AlpsStringArrayPar
};

//-----------------------------------------------------------------------------

/** This parameter indeintifies a single parameter entry. */

class AlpsParameter {

 private:
  /**@name Data members */
  /*@{*/
  /** The type of the parameter (e.g., AlpsIntPar). */
  AlpsParameterT type_;
  /** The index of this parameter within all parameters of the same type. */
  int index_;
  /*@}*/

 public:
  // default copy constructor and assignment operator are fine 
  /**@name Constructors / Destructor */
  /*@{*/
  /** The default constructor creates a phony parameter. */
  AlpsParameter() : type_(AlpsNoPar), index_(0) {}
  /** Constructor where members are specified. */
  AlpsParameter(const AlpsParameterT t, const int i) :
    type_(t), index_(i) {}
  /** The destructor. */
  ~AlpsParameter() {}
  /*@}*/

  /**@name Query methods */
  /*@{*/
  /** Return the type of the parameter. */
  AlpsParameterT type() const { return type_; }
  /** Return the index of the parameter within all parameters of the same
      type. */
  int index() const            { return index_; }
  /*@}*/
};

//-----------------------------------------------------------------------------

/** This is the class serves as a holder for a set of parameters. 

    For example, Alps stores has a parameter set for each process. Of course,
    the user can use this class for her own parameters. To use this class the
    user must 
    <ol>
      <li> first create a \c struct with the names of the parameters (see,
           e.g., AlpsAllparam.)
      <li> define the member functions
           <code>AlpsParameterSet<USER_par>::createKeywordList()</code> and
           <code>AlpsParameterSet<USER_par>::setDefaultEntries()</code>.
	   For an example look at the file
	  <code>AlpsAllParam.cpp</code>. Essentially, the first
	   method defines what keywords should be looked for in the parameter
	   file, and if one is found which parameter should take the
	   corresponding value; the other method specifies the default values
	   for each parameter.
      <li> in her source code declare a variable 
           "<code>AlpsParameterSet<AlpsAllparam> param;</code>"
    </ol>

    After this the user can read in the parameters from a file, she can
    set/access the parameters in the parameter, etc.
*/

class AlpsParameterSet {
 protected:
  /**@name Data members. All of them are protected. */
  /*@{*/
  /** The keyword, parameter pairs. Used when the parameter file is read in.
   */
  AlpsVec< std::pair<AlpsString, AlpsParameter> > keys;

  /** list of obsolete keywords. If any of these is encountered a warning is
      printed. */
  AlpsVec<AlpsString> obsoleteKeys;

  /** The character parameters. */
  char*                cpar;

  /** The integer parameters. */
  int*                 ipar;

  /** The double parameters. */
  double*              dpar;

  /** The string (actually, AlpsString) parameters. */
  AlpsString*          spar;

  /** The string array parameters */
  AlpsVec<AlpsString>* sapar;

  /*@}*/
  //---------------------------------------------------------------------------

 public:
  /**@name Private methods that must be defined for each parameter set.
     If the user creates a new parameter set, she must define these two
     methods for the class. */
  /*@{*/
  /** Method for creating the list of keyword looked for in the parameter
      file. */
  virtual void createKeywordList() = 0;
  /** Method for setting the default values for the parameters. */
  virtual void setDefaultEntries() = 0;
  /*@}*/
  //---------------------------------------------------------------------------

    
 public:
  /**@name Set methods 
     /**
	First, there is the assignment operator that sets the whole parameter
	set at once.
	Individual members of the parameter set can be set for using the
	overloaded setEntry() method. Using the example in the
	class documentation the user can set a parameter with the
	"<code>param.setEntry(USER_par::parameter_name, param_value)</code>"
	expression. */

  /*@{*/
  /// This the one used in readFromStream()
  void setEntry(const AlpsParameter key, const char * val) {
    switch (key.type()){
    case AlpsNoPar: break;
    case AlpsCharPar:        cpar [key.index()] = atoi(val);    break;
    case AlpsIntPar:         ipar [key.index()] = atoi(val);    break;
    case AlpsDoublePar:      dpar [key.index()] = atof(val);    break;
    case AlpsStringPar:      spar [key.index()] = val;          break;
    case AlpsStringArrayPar: sapar[key.index()].push_back(val); break;
    }
  }
  /*@}*/
  //---------------------------------------------------------------------------

  /**@name Read parameters from a stream. */
  /*@{*/
  /** Read the parameters from the stream specified in the argument.

      The stream is interpreted as a lines separated by newline characters.
      The first word on each line is tested for match with the keywords
      specified in the createKeywordList() method. If there is
      a match then the second word will be interpreted as the value for the
      corresponding parameter. Any further words on the line are discarded.
      Every non-matching line is discarded. 

      If the keyword corresponds to a non-array parameter then the new value
      simply overwrites the old one. Otherwise, i.e., if it is a
      StringArrayPar, the value is appended to the list of strings in that
      array. 
  */
  void readFromStream(std::istream& parstream) {
    // Get the lines of the parameter file one-by-one and if a line contains
    // a (keyword, value) pair set the appropriate parameter
    const int MAX_PARAM_LINE_LENGTH = 1024;
    char line[MAX_PARAM_LINE_LENGTH], *endOfLine, *keyword, *value, *ctmp;
    bool quiet = true;

    AlpsVec< std::pair<AlpsString, AlpsParameter> >::const_iterator ind;
    AlpsVec<AlpsString>::const_iterator obsInd;
    printf("\
AlpsParameters::readFromStream   Scanning parameter stream.\n");
    while (!parstream.eof()) {
      parstream.getline(line, MAX_PARAM_LINE_LENGTH);
      const int len = strlen(line);
      if (len == MAX_PARAM_LINE_LENGTH - 1) {
	sprintf(line, "\
There's a too long (>= %i characters) line in the parameter file.\n\
This is absurd.\n", MAX_PARAM_LINE_LENGTH);
	//  throw BCP_fatal_error(line);
	throw CoinError("The line is too long.",
			"readFromStream", " AlpsParameterSet");
      }

      endOfLine = line + len;

      //------------------------ First separate the keyword and value ------
      // keyword = std::find_if(line, endOfLine, isgraph);
      for (keyword = line; keyword < endOfLine; ++keyword) {
	if (isgraph(*keyword))
	  break;
      }
      if (keyword == endOfLine) // empty line
	continue;
      // ctmp = std::find_if(keyword, endOfLine, isspace);
      for (ctmp = keyword; ctmp < endOfLine; ++ctmp) {
	if (isspace(*ctmp))
	  break;
      }
      if (ctmp == endOfLine) // line is just one word. must be a comment
	continue;
      *ctmp++ = 0; // terminate the keyword with a 0 character

      // value = std::find_if(ctmp, endOfLine, isgraph);
      for (value = ctmp; value < endOfLine; ++value) {
	if (isgraph(*value))
	  break;
      }
      if (value == endOfLine) // line is just one word. must be a comment
	continue;

      // ctmp = std::find_if(value, endOfLine, isspace);
      for (ctmp = value; ctmp < endOfLine; ++ctmp) {
	if (isspace(*ctmp))
	  break;
      }
      *ctmp = 0; // terminate the value with a 0 character. this is good
      // even if ctmp == end_ofline

      //--------------- Check if the keyword is a param file ---------------
      if (strcmp(keyword, "ParamFile") == 0) {
	readFromFile(value);
      }

      //--------------- Check if we need to be quiet -----------------------
      if (strcmp(keyword, "Quiet") == 0) {
	int val = atoi(value);
	quiet = (val != 0);
      }

      //--------------- Find the parameter corresponding to  the keyword ---
      for (ind = keys.begin(); ind != keys.end(); ++ind) {
	if (ind->first == keyword) {
	  // The keyword does exists
	  // set_param(ind->second, value);    should work
	  if (!quiet) {
	    printf("%s %s\n", keyword, value);
	  }
	  setEntry((*ind).second, value);
	  break;
	}
      }

      for (obsInd = obsoleteKeys.begin();
	   obsInd != obsoleteKeys.end();
	   ++obsInd) {
	if (*obsInd == keyword) {
	  // The keyword does exists but is obsolete
	  printf("***WARNING*** : Obsolete keyword `%s' is found.\n",
		 keyword);
	  break;
	}
      }
    }
    if (!quiet) {
      printf("\
AlpsParameters::readFromStream   Finished scanning parameter stream.\n\n");
    }
  }
  /*@}*/
  //---------------------------------------------------------------------------

  /**@name Read parameters from a file. */
  /*@{*/
  /** Simply invoke reading from a stream. */
  void readFromFile(const char * paramfile) {
    // Open the parameter file

#if defined(NF_DEBUG)
    std::cout << "paramfile = "<< paramfile << std::endl;
#endif

    std::ifstream parstream(paramfile);
    if (!parstream)
      // throw BCP_fatal_error("Cannot open parameter file");
      throw CoinError("Cannot open parameter file.",
		      "readFromFile", " AlpsParameterSet");
    readFromStream(parstream);

    parstream.close();
  }
  /*@}*/
  //---------------------------------------------------------------------------
#if 0  // FIXME: strstream is deprecated
  /**@name Read parameters from the command line */
  /*@{*/
  /** Simply invoke reading from a stream. */
  void readFromArglist(const int argnum, const char * const * arglist) {
    // create a stream
    std::string argstring;
    for (int i = 1; i < argnum; i += 2) {
      argstring += arglist[i];
      argstring += " ";
      if (i+1 < argnum) {
	argstring += arglist[i+1];
      }
      argstring += "\n";
    }
    //#if defined(__GNUC__) && (__GNUC__ >=3)
    std::istringstream parstream(argstring.c_str());
    //#else
    // std::istrstream parstream(argstring.c_str()); // deprecated header
    //#endif
    readFromStream(parstream);
  }
  /*@}*/
#endif


  //---------------------------------------------------------------------------
  /**@name Write parameters to a stream. */
  /*@{*/
  /** Write keyword-value pairs to the stream specified in the argument.

      Each keyword-value pair is separated by a newline character. 
  */
  void writeToStream(std::ostream& outstream) const {
      
    const int size = keys.size();
    for (int i = 0; i < size; ++i) {
      const AlpsString& key = keys[i].first;
      const AlpsParameter& par = keys[i].second;
      switch (par.type()) {
      case AlpsCharPar:
	outstream << key.c_str() << "   "
		  << static_cast<int>(cpar[par.index()]) << "\n";
	break;
      case AlpsIntPar:
	outstream << key.c_str() << "   "
		  << ipar[par.index()] << "\n";
	break;
      case AlpsDoublePar:
	outstream << key.c_str() << "   "
		  << dpar[par.index()] << "\n";
	break;
      case AlpsStringPar:
	outstream << key.c_str() << "   "
		  << spar[par.index()].c_str() << "\n";
	break;
      case AlpsStringArrayPar:
	for (size_t j = 0; j < sapar[par.index()].size(); ++j) {
	  outstream << key.c_str() << "   "
		    << sapar[par.index()][j].c_str() << "\n";
	}
	break;
      case AlpsNoPar:
      default:
	// error
	//    throw BCP_fatal_error("\
	//AlpsParameters::writeToStream   ERROR: Unrecognized parameter type!\n");
	throw CoinError("Unrecognized parameter type!",
			"writeToStream", " AlpsParameterSet");

	break;
      }
	 
    }
       
  }

  //---------------------------------------------------------------------------

  /**@name Packing/unpacking methods */
  /*@{*/
  /** Pack the parameter set into the buffer. */
  virtual void pack(AlpsEncoded& buf) = 0;

  /** Unpack the parameter set from the buffer. */
  virtual void unpack(AlpsEncoded& buf) = 0;
  /*@}*/

  //---------------------------------------------------------------------------

  /**@name Constructors and destructor. */
  /*@{*/
  /** The constructor allocate memory for parameters. */
  AlpsParameterSet(int c, int i, int d, int s, int sa) :
    keys(),
    cpar(new char[c]),
    ipar(new int[i]),
    dpar(new double[d]),
    spar(new AlpsString[s]),
    sapar(new AlpsVec<AlpsString>[sa])
    {}

  /** The destructor deletes all data members. */
  virtual ~AlpsParameterSet() {
    delete[] cpar; cpar = 0;
    delete[] ipar; ipar = 0;
    delete[] dpar; dpar = 0;
    delete[] spar; spar = 0;
    delete[] sapar; sapar = 0;
  }
  /*@}*/
};

#endif
