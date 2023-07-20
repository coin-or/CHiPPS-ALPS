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
 * Copyright (C) 2001-2023, Lehigh University, Yan Xu, Aykut Bulut, and      *
 *                          Ted Ralphs.                                      *
 * All Rights Reserved.                                                      *
 *===========================================================================*/


#include "AlpsKnowledgeBroker.h"
#include "AlpsModel.h"

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

//todo(aykut) Shouldn't this method be a member of either the broker or subtree classes?
//todo(aykut) this should have const modifier
void
AlpsModel::nodeLog(AlpsTreeNode *node, bool force)
{
    int nodeInterval =
        broker_->getModel()->AlpsPar()->entry(AlpsParams::nodeLogInterval);

    int numNodesProcessed = broker_->getNumNodesProcessed();
    int numNodesPartial  = broker_->getNumNodesPartial();

    AlpsTreeNode *bestNode = NULL;

    if ( (broker_->getProcType() != AlpsProcessTypeMaster) &&
         (broker_->getProcType() != AlpsProcessTypeSerial) ) {
        return;
    }

    if ( (broker_->getMsgLevel() > 1) &&
         ( force ||
           (numNodesProcessed % nodeInterval == 0) ) ) {

        double feasBound = ALPS_OBJ_MAX, relBound = ALPS_OBJ_MAX;

        if (broker_->getNumKnowledges(AlpsKnowledgeTypeSolution) > 0) {
            feasBound = (broker_->getBestKnowledge(AlpsKnowledgeTypeSolution)).second;
        }

        bestNode = broker_->getBestNode();

        if (bestNode) {
            relBound = bestNode->getQuality();
        }

        //Take into account pregnant nodes (processed but not branched)
        broker_->messageHandler()->
            message(ALPS_S_NODE_COUNT, broker_->messages())
            << numNodesProcessed
            << numNodesPartial
            << broker_->updateNumNodesLeft() - numNodesPartial
            << relBound
            << feasBound
            << CoinMessageEol;
    }
}

/// Pack AlpsPar_ into a given encode object.
AlpsReturnStatus AlpsModel::encode(AlpsEncoded * encoded) const {
  AlpsPar_->pack(*encoded);
  return AlpsReturnStatusOk;
}

/// Decode the given AlpsEncoded object into this.
AlpsReturnStatus AlpsModel::decodeToSelf(AlpsEncoded & encoded) {
  AlpsPar_->unpack(encoded);
  return AlpsReturnStatusOk;
}
