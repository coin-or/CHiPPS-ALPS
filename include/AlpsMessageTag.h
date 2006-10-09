#include "AlpsLicense.h"

#ifndef AlpsMessageTag_h
#define AlpsMessageTag_h

// This file is fully docified.

/** This enumerative constant describes the message tags different
    processes of Alps understand. */

enum AlpsMessageTag{

  /** Alps should finish. The worker process receiving this message will
      send back statistics to the hub and then terminate. hub will wait
      for all other processes to terminate. */
  AlpsMsgFinishedAlps,

  /** The message tag indicating that a model is being sent or received. */
  AlpsMsgModel,

  /** The message tag indicating that a node is being sent or received. */
  AlpsMsgNode,

  /** The message tag indicating that parameters are being sent or received. */
  AlpsMsgParams,

  /** The message tag indicating that initialization is finished. */
  AlpsMsgFinishInit,

  /** The message tag indicating that hub thread should be invoked. */
  AlpsMsgHub,

  /** The message tag indicating that the process sent the message is idle. */
  AlpsMsgIdle,

  /** The message tag indicating that the send ask the recv to
      pause current work immediately or after finishing the work on hand. */
  AlpsMsgAskPause,

  /** The message tag indicating that the send ask the recv to
      continue its work. */
  AlpsMsgAskCont,

  /** The message tag indicating that the send ask the recv to
      terminate. */
  AlpsMsgAskStop,

  /** The message tag indicating that the send ask the recv to
      send its load information. */
  AlpsMsgAskLoad,

  /** The message tag indicating that cluster and system workloads are in 
      the message. */
  AlpsMsgLoadInfo,

  /** The message tag indicating that a incumbent is in 
      the message. */
  AlpsMsgIncumbent,

  /** Ask the reiver to donor workload. */
  AlpsMsgAskDonor
};

#endif

