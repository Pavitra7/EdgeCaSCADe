#ifndef __SIMULATOR_H__
#define __SIMULATOR_H__

#include <core.h>
#include <vector>

// Core Local Interrupt - Emulated
void doClint(Clint& clint, std::vector<ac_int<32, false> >& mem);
// Early Console
void doEarlyConsole(std::vector<ac_int<32, false> >& mem);
// UART LiteUart
void doUART(std::vector<ac_int<32, false> >& mem);
// AMO management
void amoManagement(int& index, reservationSet reservationAMO[], const int nbCores);
// Directory protocol multicore
void doDirectory(std::vector<ac_int<32, false> >& mem, const int nb_cores, bool enable[],
                 cacheDirectoryInterfaceIn bufferIn[][2], cacheDirectoryInterfaceOut bufferOut[][3],
                 reservationSet reservationAMO[]);
// Round-Robin Bus
void doSharedBus(const int nb_cores, cacheDirectoryInterfaceOut bufferOut[][3], cacheDirectoryInterfaceIn bufferIn[][2], cacheL2DirectoryInterfaceIn& bufferInL2);
// Directory Bus
void doDirectoryRequestAnswers(const int nb_cores, cacheDirectoryInterfaceOut bufferOut[][3], cacheDirectoryInterfaceIn bufferIn[][2], cacheL2DirectoryInterfaceIn& bufferInL2, cacheL2DirectoryInterfaceOut& bufferOutL2, bool& wait, int coreID, int cacheID);
void doDirectoryRequestSend(const int nb_cores, cacheDirectoryInterfaceOut bufferOut[][3], cacheDirectoryInterfaceIn bufferIn[][2], cacheL2DirectoryInterfaceIn& bufferInL2, cacheL2DirectoryInterfaceOut& bufferOutL2, bool& wait, int& coreID, int& cacheID);

class Simulator {
protected:
  // Multiple cores possible
  Core core[NB_CORES];
  Clint clint;
  std::vector<ac_int<32, false>> mem;
#if SMP
  cacheL2DirectoryInterfaceIn bufferInL2;
  cacheL2DirectoryInterfaceOut bufferOutL2;
  CacheMemoryL2<4, 16, 128>*cacheL2;
  reservationSet reservationAMOL2;
#endif
  // OS Console (I/O)
  bool exitFlag;
  // Enable
  bool enable[NB_CORES];
  
  /* Multi-core System */
  static const int LOG_NB_CORES = log2const<NB_CORES>::value;
  // AMO reservation set (address + valid)
  reservationSet reservationAMO[NB_CORES];
  // AMO selection algorithm
  int reservationIndex = 0;
  // Multi-core directory cache coherence
  cacheDirectoryInterfaceIn interfaceIn[NB_CORES][2];
  cacheDirectoryInterfaceOut interfaceOut[NB_CORES][3];
  
  bool waitDirBus  = false;
  int coreIDDirBus  = 0;
  int cacheIDDirBus = 0;
  

public:
  int breakpoint;
  int timeout;
  int debugCycle;
  bool debugOn;
  virtual void run()
  {
    exitFlag = false;
#if LINUXOS
    for (unsigned i = 0; i < NB_CORES; i++) {
      if (enable[i]) {
        core[i].regFile[11] = DTB_BASE;
      }
    }
#endif
    while (!exitFlag) {
      for (unsigned i = 0; i < NB_CORES; i++) {
        if (enable[i]) {
          doCycle(core[i], 0, clint.ir_timer[i], clint.ipi[i]);
        }
      }
      solveSyscall();
#if SMP
      //doDirectory(mem, NB_CORES, enable, interfaceIn, interfaceOut, reservationAMO);      
      doSharedBus(NB_CORES, interfaceOut, interfaceIn, bufferInL2);
      doDirectoryRequestAnswers(NB_CORES, interfaceOut, interfaceIn, bufferInL2, bufferOutL2, waitDirBus, coreIDDirBus, cacheIDDirBus);
      cacheL2->processL2(bufferInL2, bufferOutL2);
      doDirectoryRequestSend(NB_CORES, interfaceOut, interfaceIn, bufferInL2, bufferOutL2, waitDirBus, coreIDDirBus, cacheIDDirBus);
#endif
      doClint(clint, mem);
      doEarlyConsole(mem);
      doUART(mem);
      extend();
      debugOn = this->core[0].csrUnit.mcycle >= debugCycle;
      printCycle();
      // We handle breakpoints
      if (this->breakpoint != -1 && core[0].csrUnit.mcycle == this->breakpoint) {
        printf("Reached break\n");
      }

      if (this->timeout != -1 && this->core[0].csrUnit.mcycle >= this->timeout) {
        printf("Timeout!\n");
        break;
      }
      // handle AMO LR:SC
      amoManagement(reservationIndex, reservationAMO, NB_CORES);
    }
    printEnd();
  }

  virtual void printCycle()   = 0;
  virtual void printEnd()     = 0;
  virtual void extend()       = 0;
  virtual void solveSyscall() = 0;
};

#endif // __SIMULATOR_H__
