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


#include "AlpsModel.h"

//##############################################################################

/** Pack Alps portion of node into an encoded object. */
AlpsReturnCode 
AlpsModel::encodeAlps(AlpsEncoded *encoded) const
{
    AlpsReturnCode status = ALPS_OK;
    
    //assert(encode);
    AlpsPar_->pack(*encoded);
    
    return status;
}

//##############################################################################

/** Unpack Alps portion of node from an encoded object. */
AlpsReturnCode 
AlpsModel::decodeAlps(AlpsEncoded &encoded)
{
    AlpsReturnCode status = ALPS_OK;
    
    //assert(encode);
    AlpsPar_->unpack(encoded);

    //AlpsPar_->writeToStream(std::cout);
    
    return status;
}

//##############################################################################

/** Write out parameters. */
void 
AlpsModel::writeParameters(std::ostream& outstream) const
{
    AlpsPar_->writeToStream(outstream);
}

//##############################################################################

/** Read in Alps parameters. */
void 
AlpsModel::readParameters(const int argnum, const char * const * arglist) 
{
    AlpsPar_->readFromArglist(argnum, arglist);
} 

//##############################################################################
