#ifndef __CORE_H__
#define __CORE_H__

#include <ac_int.h>
#include <riscvISA.h>

// all the possible memories
#include <soc_config.h>
#include <cacheMemory.h>
#include <cacheMemory_multicore.h>
#include <cacheMemory_multicore_inst.h>
#include <cacheMemoryL2.h>
#include <memoryInterface.h>
#include <pipelineRegisters.h>
#include <csrUnit.h>
#include <multiplicationUnit.h>
#include <vector>

#ifndef MEMORY_INTERFACE
#define MEMORY_INTERFACE SimpleMemory
#endif

/******************************************************************************************
 * Stall signals enum
 * ****************************************************************************************
 */
typedef enum {
	STALL_FETCH = 0,
	STALL_DECODE = 1,
	STALL_EXECUTE = 2,
	STALL_MEMORY = 3,
	STALL_WRITEBACK = 4
} StallNames;

struct Core {
   
  /* Pipeline */
  FtoDC ftoDC;
  DCtoEx dctoEx;
  ExtoMem extoMem;
  MemtoWB memtoWB;

  /* Memory */
  // Interface size are configured with 4 bytes interface size (32 bits)
#if !SMP
  CacheMemory<4, 16, 64>*dm, *im;
#else
  CacheMemoryMulticore<4, 16, 64>*dm;
#if MCVM
  CacheMemory<4, 16, 64>*im;
#else
  CacheMemoryMulticoreInst<4, 16, 64>*im;
#endif
#endif

  /* Registers */
  ac_int<32, true> regFile[32];
  ac_int<32, false> pc = 0;
  CsrUnit csrUnit; // CSRs - Timer, etc...
  MultiplicationUnit multiplicationUnit; // Mult extension
  
  /* Stall */
  bool stallSignals[5] = {0, 0, 0, 0, 0};
  bool stallIm = 0, stallDm = 0;
  bool stallMultAlu = 0;
  bool flushing = 0;
};

void doCycle(struct Core& core, bool globalStall, bool ir_timer = false, bool ipi = false);

#endif // __CORE_H__
