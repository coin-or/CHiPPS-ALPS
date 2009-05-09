/*===========================================================================*
 * This file is part of the Abstract Library for Parallel Search (ALPS).     *
 *                                                                           *
 * ALPS is distributed under the Common Public License as part of the        *
 * COIN-OR repository (http://www.coin-or.org).                              *
 *                                                                           *
 * Authors:                                                                  *
 *                                                                           *
 *          Yan Xu, Lehigh University                                        *
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
 * Copyright (C) 2001-2009, Lehigh University, Yan Xu, and Ted Ralphs.       *
 *===========================================================================*/


#include "AlpsParameterBase.h"

//##############################################################################

void
AlpsParameterSet::readFromStream(std::istream& parstream)
{
    //------------------------------------------------------
    // Get the lines of the parameter file one-by-one and if a line contains
    // a (keyword, value) pair set the appropriate parameter.
    //------------------------------------------------------

    const int MAX_PARAM_LINE_LENGTH = 1024;
    char line[MAX_PARAM_LINE_LENGTH], *endOfLine, *keyword, *value, *ctmp;
    bool quiet = true;

    std::vector< std::pair<std::string, AlpsParameter> >::const_iterator ind;
    std::vector<std::string>::const_iterator obsInd;
    //printf("\AlpsParameters::readFromStream   Scanning parameter stream.\n");

    while (!parstream.eof()) {
	parstream.getline(line, MAX_PARAM_LINE_LENGTH);
	const int len = static_cast<int> (strlen(line));
	if (len == MAX_PARAM_LINE_LENGTH - 1) {
	    sprintf(line, "\
There's a line that's too long (>= %i characters) in the parameter file.\n\
This is absurd.\n", MAX_PARAM_LINE_LENGTH);
	    throw CoinError("The line is too long.",
			    "readFromStream", " AlpsParameterSet");
	}
	
	endOfLine = line + len;

	//--------------------------------------------------
	//--- First separate the keyword and value ---------
	//--------------------------------------------------

	// keyword = std::find_if(line, endOfLine, isgraph);
	for (keyword = line; keyword < endOfLine; ++keyword) {
	    if (isgraph(*keyword)) break;
	}
	if (keyword == endOfLine) // empty line
	    continue;

	// ctmp = std::find_if(keyword, endOfLine, isspace);
	for (ctmp = keyword; ctmp < endOfLine; ++ctmp) {
	    if (isspace(*ctmp)) break;
	}
	if (ctmp == endOfLine) // line is just one word. must be a comment
	    continue;
	*ctmp++ = 0; // terminate the keyword with a 0 character
	
	// value = std::find_if(ctmp, endOfLine, isgraph);
	for (value = ctmp; value < endOfLine; ++value) {
	    if (isgraph(*value)) break;
	}
	if (value == endOfLine) // line is just one word. must be a comment
	    continue;

	// ctmp = std::find_if(value, endOfLine, isspace);
	for (ctmp = value; ctmp < endOfLine; ++ctmp) {
	    if (isspace(*ctmp)) break;
	}
	*ctmp = 0; // terminate the value with a 0 character. this is good
	           // even if ctmp == end_ofline

	//--------------------------------------------------
	//--- Check if the keyword is a param file ---------
	//--------------------------------------------------

	if ( (strcmp(keyword, "param") == 0) ||
             (strcmp(keyword, "par") == 0) ) {
	    readFromFile(value);
	}

	//--------------------------------------------------	
	//--- Check if we need to be quiet -----------------
	//--------------------------------------------------

	if (strcmp(keyword, "quiet") == 0) {
	    int val = atoi(value);
	    quiet = (val != 0);
	}

	//--------------------------------------------------
	//--- Find the parameter corresponding to the keyword
	//--------------------------------------------------

	for (ind = keys_.begin(); ind != keys_.end(); ++ind) {
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

	for (obsInd = obsoleteKeys_.begin();
	     obsInd != obsoleteKeys_.end();
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

//##############################################################################

void 
AlpsParameterSet::readFromFile(const char * paramfile)
{
    // Open the parameter file
    std::ifstream parstream(paramfile);

    if (!parstream) {
	throw CoinError("Cannot open parameter file.",
			"readFromFile", " AlpsParameterSet");
    }
    
    readFromStream(parstream);
    
    parstream.close();
}

//##############################################################################

void 
AlpsParameterSet::readFromArglist(const int argnum, const char * const * arglist) 
{
    // create a stream
    std::string argstring;
    for (int i = 1; i < argnum; i += 2) {
	// remove leading -
	std::string par = arglist[i];
	std::string sPar;
	
	int length = static_cast<int> (par.length());
	
	std::string::size_type pos = par.find('-');
	
	if (pos != std::string::npos) {
	    // find -, a parameter
	    ++pos;
	    --length;
	    sPar = par.substr(pos, length);
	}
	else {
	    // a value
	    sPar = par;
	}

	if ((sPar == "param") && i != 1) {
            // Need put sPar to be the first in argstring so that 
            // parameter file is read first.
            argstring.insert(0, sPar);
            argstring.insert(length, " ");
            argstring.insert(length+1, arglist[(i+1)]);
            int len2 = static_cast<int> (length + 1 + strlen(arglist[(i+1)]));
            argstring.insert(len2, "\n");
        }
        else {
            argstring += sPar;
            argstring += " ";
            if (i + 1 < argnum) {
                argstring += arglist[i+1];
            }
            argstring += "\n";
        }
    }
    
    ALPS_STRINGSTREAM parstream(argstring.c_str());
    readFromStream(parstream);
}


//##############################################################################

void 
AlpsParameterSet::writeToStream(std::ostream& outstream) const 
{
    const int size = static_cast<int> (keys_.size());
    for (int i = 0; i < size; ++i) {
	const std::string& key = keys_[i].first;
	const AlpsParameter& par = keys_[i].second;
	switch (par.type()) {
	case AlpsBoolPar:
	    outstream << key.c_str() << "   "
		      << static_cast<int>(bpar_[par.index()]) << "\n";
	    break;
	case AlpsIntPar:
	    outstream << key.c_str() << "   "
		      << ipar_[par.index()] << "\n";
	    break;
	case AlpsDoublePar:
	    outstream << key.c_str() << "   "
		      << dpar_[par.index()] << "\n";
	    break;
	case AlpsStringPar:
	    outstream << key.c_str() << "   "
		      << spar_[par.index()].c_str() << "\n";
	    break;
	case AlpsStringArrayPar:
	    for (size_t j = 0; j < sapar_[par.index()].size(); ++j) {
		outstream << key.c_str() << "   "
			  << sapar_[par.index()][j].c_str() << "\n";
	    }
	    break;
	case AlpsNoPar:
	default:
	    throw CoinError("Unrecognized parameter type!",
			    "writeToStream", " AlpsParameterSet");
	}
    }
}

//##############################################################################

//##############################################################################

//##############################################################################

