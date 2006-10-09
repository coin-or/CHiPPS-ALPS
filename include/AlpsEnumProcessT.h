#include "AlpsLicense.h"


#ifndef AlpsEnumProcessT_H
#define AlpsEnumProcessT_H

//-----------------------------------------------------------------------------

/** This enumerative constant describes the various process types. */

enum AlpsProcessType {
   /** */
   AlpsProcessTypeMaster,
   //   AlpsProcessType_TM,
   /** */
   //   BCP_ProcessType_LP,
   AlpsProcessTypeWorker,
   /** */
   AlpsProcessTypeCG,
   /** */
   AlpsProcessTypeVG,
   /** */
   AlpsProcessTypeCP,
   /** */
   AlpsProcessTypeVP,
   /** */
   AlpsProcessTypeAny
};

#endif
