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
 * Copyright (C) 2001-2007, Lehigh University, Yan Xu, and Ted Ralphs.       *
 *===========================================================================*/

#ifndef AlpsMessageTag_h
#define AlpsMessageTag_h

// This file is fully docified.

/** This enumerative constant describes the message tags different
    processes of Alps understand. */

enum AlpsMessageTag{

  /** The message tag indicating that the send ask the recv to
      continue or terminate. */
  // 0
  AlpsMsgContOrTerm = 0,

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
  AlpsMsgIndicesFromMaster,

  // 31
  AlpsMsgWorkerAskIndices,

  // 32
  AlpsMsgForceTerm,

  // 33
  AlpsMsgMasterIncumbent,

  // 34
  AlpsMsgHubIncumbent,

  // 35
  AlpsMsgAskHubPause,

  /** Ask the receiver worker to donor workload to the specified worker. */
  // 36
  AlpsMsgAskDonateToWorker,
  
  // 37
  AlpsMsgSubTreeByWorker,

  // 38
  AlpsMsgIncumbentTwo,
  
  /** The message tag indicating that knowledge generated during rampup about
      model is being sent or received. */
  // 39
  AlpsMsgModelGenRampUp,

  /** The message tag indicating that knowledge generated during search about
      model is being sent or received. */
  // 40
  AlpsMsgModelGenSearch,

  /** When requested by master during inter balance, hub failed to identify 
      a donor worker. */
  // 41
  AlpsMsgHubFailFindDonor,

  /** Load info during ramp up */
  // 42
  AlpsMsgRampUpLoad,

  /** Donate during ramp up */
  // 43
  AlpsMsgRampUpDonate,

  /** Hub finished ramp up. Used in spiral method. */
  // 44
  AlpsMsgFinishInitHub,

  /** Error code. */
  AlpsMsgErrorCode
};

#endif
