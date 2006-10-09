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
 * Copyright (C) 2001-2004, International Business Machines                  *
 * Corporation, Lehigh University, Yan Xu, Ted Ralphs, Matthew Salzman and   *
 * others. All Rights Reserved.                                              *
 *===========================================================================*/

#ifndef AlpsNodeDesc_h_
#define AlpsNodeDesc_h_

//#include "AlpsModel.h"

class AlpsModel;

//#############################################################################
/** A class to refer to the description of a search tree node.
 *FIXME* : write a better doc...
 */
//#############################################################################
class AlpsNodeDesc {

 protected:

    /** A pointer to model. */
    // Should allow change model due to presolve
    AlpsModel* model_;
     
 public:

    AlpsNodeDesc() {}
    AlpsNodeDesc(AlpsModel* m)
	{ model_ = m; }

    virtual ~AlpsNodeDesc() {}

    inline AlpsModel* getModel() const { return model_; }
    inline void setModel(AlpsModel* m) { model_ = m; }

    /** Pack node description into an encoded. */
    virtual AlpsReturnCode encode(AlpsEncoded *encoded) const {
    	AlpsReturnCode status = ALPS_OK;
	// Should never be called.
	assert(0);
	return status;
    }

    /** Unpack a node description from an encoded. Fill member data. */
    virtual AlpsReturnCode decode(AlpsEncoded &encoded) {
    	AlpsReturnCode status = ALPS_OK;
	assert(0);
	return status;
    }
};

#endif
