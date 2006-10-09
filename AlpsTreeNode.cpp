#include "AlpsLicense.h"

#include <algorithm>

#include "AlpsTreeNode.h"
#include "AlpsSubTree.h"

//#############################################################################

void
AlpsTreeNode::removeChild(AlpsTreeNode*& child) throw(CoinError)
{
  AlpsTreeNode** child_p =  // child_p is a pointer to AlpsTreeNode*
    std::find(children_, children_ + numChildren_, child);
  if ( child_p == children_ + numChildren_ ) {
    // *FIXME* : way too informal. design error mechanism
    throw CoinError("Alps_????", "AlpsTreeNode::removeChild",
		      "The argument is not a child of this node.");
  }
  *child_p = children_[--numChildren_];  // Assign the last to *child_p
  child->removeDescendants();            // so that can safely del child . 
  delete child;                          // Storage space never shrinks
  child = 0;         // NOTE: child is pointer that stores the same address
}                    //       as the element of Children we want to remove

//#############################################################################

void
AlpsTreeNode::addChild(AlpsTreeNode*& child)
{
  //FIXME: Is this right? How does this node get deleted?
  //YX: can be used only if enough memory is allocated.
  children_[numChildren_++] = child;
}

//#############################################################################

void
AlpsTreeNode::removeDescendants()
{
   // *FIXME* : Sanity Check. We might want to check whether the status of
   // this node is internal. Otherwise, we run the risk of just changing the
   // status on a node that is a candidate by accident. */
   while ( numChildren_ > 0 ) {
      removeChild(children_[0]);
   }
   status_ = AlpsNodeStatusFathomed;
}

//#############################################################################

