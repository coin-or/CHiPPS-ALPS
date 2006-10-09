#include "AlpsLicense.h"

#ifndef AlpsHelperFunctions_h_
#define AlpsHelperFunctions_h_

#include  <time.h>
#if !defined(_MSC_VER)
#include <sys/times.h>
#include <sys/resource.h>
#include <unistd.h>
#endif

#if defined(NF_DEBGU)
#include <iostream>
#endif


//#############################################################################
// Adapted from Sbb
/** Returns an approximation of processor time used (in seconds). */
inline double AlpsCpuTime()
{
  double cpu_temp;
#if defined(_MSC_VER)
  unsigned int ticksnow;        /* clock_t is same as int */
  
  ticksnow = static_cast<unsigned int>(clock());
  
  cpu_temp = static_cast<double>(ticksnow)/CLOCKS_PER_SEC;
#else
  struct rusage usage;
  getrusage(RUSAGE_SELF,&usage);
  cpu_temp = double(usage.ru_utime.tv_sec);
  cpu_temp += 1.0e-6 * static_cast<double>(usage.ru_utime.tv_usec);
#endif
  return cpu_temp;
}

//#############################################################################
/** Delay for the specified seconds. */
inline void AlpsSleep(double sec)
{
  double start = AlpsCpuTime();
  while ( (AlpsCpuTime() - start) < sec) { };  
}
#endif
