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
 *===========================================================================*/

#ifndef AlpsMessageTag_h
#define AlpsMessageTag_h

// This file is fully docified.

/** This enumerative constant describes the message tags different
    processes of Alps understand. */

enum AlpsMessageTag{

  /** Alps should finish. The worker process receiving this message will
      send back statistics to the hub and then terminate. hub will wait
      for all other processes to terminate. */
  // AlpsMsgFinishedAlps,

  /** The message tag indicating that the send ask the recv to
      continue or terminate. */
  // 0
  AlpsMsgContOrTerm,

  /** Ask the receiver worker to donor workload to the specified worker. */
  // 1
  AlpsMsgAskDonate,

  /** Ask the receiver worker to donor workload to the specified hub. */
  // 2
  AlpsMsgAskDonateToHub,

  /** Aks the receiver hub to share workload with the specified hub. */
  // 3
  AlpsMsgAskHubShare,

  /** The message tag indicating that initialization is finished. */
  // 4
  AlpsMsgFinishInit,

  /** Hub load, msg counts are in the buf. */
  // 5
  AlpsMsgHubLoad,

  /** The message tag indicating that the send ask the recv to
      send its load information. */
  // 6
  AlpsMsgAskLoad,

  /** The message tag indicating that the send ask the recv to
      pause current work immediately or after finishing the work on hand. */
  // 7
  AlpsMsgAskPause,

  /** The message tag indicating that the send ask the recv to terminate. */
  // 8
  AlpsMsgAskTerminate,

  /** The message tag indicating that the process sent the message is idle. */
  // 9
  AlpsMsgIdle,

  /** The message tag indicating that a incumbent is in the message buf. */
  // 10
  AlpsMsgIncumbent,

  /** The message tag indicating that cluster and system workloads are in 
      the message buf. */
  // 11
  AlpsMsgLoadInfo,

  /** The message tag indicating that the sender has no workload. */
  // 12
  AlpsMsgWorkerNeedWork,

  /** The message tag indicating that a model is being sent or received. */
  // 13
  AlpsMsgModel,

  /** The message tag indicating that a node is being sent or received. */
  // 14
  AlpsMsgNode,

  /** The message tag indicating that parameters are being sent or received. */
  // 15
  AlpsMsgParams,

  /** Termination check. */
  // 16
  AlpsMsgTermCheck,

  /** Hub check the status of its workers. */
  // 17
  AlpsMsgHubCheckCluster,
  
  /** Hub periodically check the status of its workers.*/
  // 18
  AlpsMsgHubPeriodCheck,

  /** Hub periodically report its status to master.*/
  // 19
  AlpsMsgHubPeriodReport,
  
  /** Hub's status is in buf. */
  // 20
  AlpsMsgHubStatus,

  /** Worker's status is in buf. */
  // 21
  AlpsMsgWorkerStatus,

  /** Hub's status is in buf, used in termination checking. */
  // 22
  AlpsMsgHubTermStatus,

  /** Worker's status is in buf, used in termination checking. */
  // 23
  AlpsMsgWorkerTermStatus,

  /** The size of the message. */
  // 24
  AlpsMsgSize,

  /** Send or receive a subtree due to master load balancing. */
  // 25
  AlpsMsgSubTreeByMaster,

  /** Send or receive a subtree due to hub load balancing. */
  // 26
  AlpsMsgSubTree,

  /** Tree node size */
  // 27
  AlpsMsgNodeSize,

  // 28
  AlpsMsgTellMasterRecv,

  // 29
  AlpsMsgTellHubRecv,

  // 30
  AlpsMsgHubAskIndices,
  
  // 31
  AlpsMsgIndicesFromMaster,

  // 32
  AlpsMsgWorkerAskIndices,

  // 33
  AlpsMsgIndicesFromHub,

  // 34
  AlpsMsgForceTerm,

  // 35
  AlpsMsgMasterIncumbent,

  // 36
  AlpsMsgHubIncumbent,

  // 37
  AlpsMsgAskHubPause,
  /** Ask the receiver worker to donor workload to the specified worker. */
  // 38
  AlpsMsgAskDonateToWorker,
  
  // 39
  AlpsMsgSubTreeByWorker,

  // 40
  AlpsMsgIncumbentTwo
};

#endif
