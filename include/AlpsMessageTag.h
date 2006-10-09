#include "AlpsLicense.h"

#ifndef AlpsMessageTag_h
#define AlpsMessageTag_h

// This file is fully docified.

/** This enumerative constant describes the message tags different
    processes of Alps understand. */

enum AlpsMessageTag{


  /** Alps has finished. The worker process receiving this message will
      send back statistics to the hub and then terminate. hub will wait
      for all other processes to terminate. */
  AlpsMsgFinishedAlps,

  /** Master send model to workers. */
  AlpsMsgModel,

  /** Master send node to workers. */
  AlpsMsgNode,

  /** Master send parameter to workers. */
  AlpsMsgParameters
};

#endif

