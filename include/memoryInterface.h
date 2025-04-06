/*
 * memoryInterface.h
 *
 *  Created on: 07 July. 2021
 *      Author: Simon, Logan
 *
 *  This special interface is used for memory communication
 */
#ifndef __MEMORY_INTERFACE_H__
#define __MEMORY_INTERFACE_H__

#include <ac_int.h>
#define MEMMASK 0xffffff

typedef enum { NOMASK = 0, BYTE, HALF, WORD, BYTE_U, HALF_U, LONG } memMask;

typedef struct registrationSet {
  ac_int<1, false> valid;
  ac_int<32, false> address;
} reservationSet;

typedef enum { INVALID = 0, EXCLUSIVE, MODIFIED, SHARED} cacheStatus; // MOESI Protocol

typedef enum {
  NONE = 0,
  LOAD,//1
  STORE,
  AMO_LR,
  AMO_SC,
  AMO_SWAPW,
#if LINUXOS
  AMO_ADDW,
  AMO_ANDW,
  AMO_ORW,
  AMO_XORW,
#endif
  WRITEBACK,
  WRITEHIT,
  MISS_WRITE,// 8
  MISS_READ,// 9
  END_MISS,// 10
} memOpType; // directory Message Types

typedef enum {
  NONE_DIR = 0,
  REQ_INV,
  REQ_DATATRANSFER_SHARED,
  REQ_DATATRANSFER_INV
} directoryOpType;

struct cacheDirectoryInterfaceBase {
  // in or out
  ac_int<32, false> addr;
  ac_int<1, false> interfaceAck;
  ac_int<1, false> releaseAck;
};

typedef struct cacheDirectoryInterfaceOut : public cacheDirectoryInterfaceBase {
  memOpType interfaceRequest;
  // data out
  ac_int<32, false> buffer;
  // data In
  ac_int<32, false> bufferIn;
} cacheDirectoryInterfaceOut;

typedef struct cacheDirectoryInterfaceIn : public cacheDirectoryInterfaceBase {
  ac_int<1, false> protocolMiss;
  directoryOpType interfaceRequest;
} cacheDirectoryInterfaceIn;

// cache L2
typedef struct cacheL2DirectoryInterfaceOut : public cacheDirectoryInterfaceBase {
  // data out
  ac_int<32, false> buffer;
  ac_int<1, false> protocolMiss;
  directoryOpType  interfaceRequest;
  ac_int<4, false> hartid;
  ac_int<1, false> cache;
} cacheL2DirectoryInterfaceOut;

typedef struct cacheL2DirectoryInterfaceIn : public cacheDirectoryInterfaceBase {
  // in to cache
  memOpType interfaceRequest;
  ac_int<4, false> hartid;
  ac_int<1, false> cache;
  // out from cache 
  ac_int<32, false> dataLoad;
  ac_int<1, false> protocolMiss;
  ac_int<1, false> rejection;
  // Special WB input
  ac_int<1, false> interfaceAckWB;
  ac_int<1, false> releaseAckWB;
  ac_int<32, false> addrWB;
  // data in
  ac_int<32, false> buffer;
  ac_int<32, false> bufferWB;
} cacheL2DirectoryInterfaceIn;


// Mother Class
template <unsigned int INTERFACE_SIZE> class MemoryInterface {
protected:
  bool wait = false;

public:
  virtual void process(ac_int<32, false> addr, memMask mask, memOpType opType, ac_int<4, false> hartid,
                       ac_int<INTERFACE_SIZE * 8, false> dataIn, ac_int<INTERFACE_SIZE * 8, false>& dataOut,
                       bool& waitOut) = 0;
};

template <unsigned int INTERFACE_SIZE> class MemoryInterfaceMulticore {
protected:
  bool wait = false;

public:
  virtual void process(ac_int<32, false> addr, memMask mask, memOpType opType, ac_int<4, false> hartid,
                       ac_int<INTERFACE_SIZE * 8, false> dataIn, ac_int<INTERFACE_SIZE * 8, false>& dataOut,
                       bool& waitOut, bool& waitOutDirectory) = 0;
};

// Single Core Memory Interface
template <unsigned int INTERFACE_SIZE> class IncompleteMemory : public MemoryInterface<INTERFACE_SIZE> {
public:
  ac_int<32, false>* data;
  ac_int<1, false> pendingWrite = 0;
  ac_int<32, false> valueLoaded = 0;
  reservationSet* reservationAMO;

public:
  IncompleteMemory(ac_int<32, false>* arg, reservationSet* arg_reg)
  {
    data           = arg;
    reservationAMO = arg_reg;
  }
  void process(ac_int<32, false> addr, memMask mask, memOpType opType, ac_int<4, false> hartid,
               ac_int<INTERFACE_SIZE * 8, false> dataIn, ac_int<INTERFACE_SIZE * 8, false>& dataOut, bool& waitOut)
  {
    
    // Incomplete memory only works for 32 bits
    assert(INTERFACE_SIZE == 4);
    ac_int<8, false> t8 = 0;
    ac_int<1, true> bit = 0;
    ac_int<16, false> t16 = 0;
    ac_int<INTERFACE_SIZE * 8, false> mergedAccess = 0;

    if ((!pendingWrite && opType == STORE && mask != WORD) || opType == LOAD || opType == AMO_LR
        || (!pendingWrite && opType == AMO_SWAPW)
#if LINUXOS
        || (!pendingWrite && opType == AMO_ADDW)
        || (!pendingWrite && opType == AMO_ANDW)
        || (!pendingWrite && opType == AMO_ORW)
        || (!pendingWrite && opType == AMO_XORW)
#endif
        ) {

      mergedAccess = data[(addr >> 2) & MEMMASK];
      //printf("Loading at %x : %x\n", addr >> 2, mergedAccess);
      if ((!pendingWrite && opType == STORE && mask != WORD) || opType == AMO_SWAPW
#if LINUXOS
        || opType == AMO_ADDW || opType == AMO_ANDW || opType == AMO_ORW || opType == AMO_XORW
#endif
        ) {
        waitOut      = true;
        valueLoaded  = mergedAccess;
        pendingWrite = 1;
      } else {
        pendingWrite                 = 0;
        waitOut                      = false;
        ac_int<32, false> dataOutTmp = mergedAccess;
        switch (mask) {
          case BYTE:
            t8  = dataOutTmp.slc<8>(((int)addr.slc<2>(0)) << 3);
            bit = t8.slc<1>(7);
            dataOut.set_slc(0, t8);
            dataOut.set_slc(8, (ac_int<24, true>)bit);
            break;
          case HALF:
            t16 = dataOutTmp.slc<16>(addr[1] ? 16 : 0);
            bit = t16.slc<1>(15);
            dataOut.set_slc(0, t16);
            dataOut.set_slc(16, (ac_int<16, true>)bit);
            break;
          case WORD:
            dataOut = dataOutTmp;
            break;
          case BYTE_U:
            dataOut = dataOutTmp.slc<8>(((int)addr.slc<2>(0)) << 3) & 0xff;
            break;
          case HALF_U:
            dataOut = dataOutTmp.slc<16>(addr[1] ? 16 : 0) & 0xffff;
            break;
        }
        if (opType == AMO_LR) {
          reservationAMO->valid   = 1;
          reservationAMO->address = addr;
        }
        //printf("Loading at %x : %x\n", addr >> 2, mergedAccess);
      }

    } else if (opType == STORE || opType == AMO_SC || opType == AMO_SWAPW
#if LINUXOS
      || opType == AMO_ADDW || opType == AMO_ANDW || opType == AMO_ORW || opType == AMO_XORW
#endif
      ) {
      pendingWrite = 0;
      waitOut      = false;

      ac_int<32, false> valToStore = 0;
      switch (mask) {
        case BYTE_U:
        case BYTE:
          valToStore = valueLoaded;
          valToStore.set_slc(((int)addr.slc<2>(0)) << 3, dataIn.template slc<8>(0));
          break;
        case HALF:
        case HALF_U:
          valToStore = valueLoaded;
          valToStore.set_slc(addr[1] ? 16 : 0, dataIn.template slc<16>(0));
          break;
        case WORD:
          valToStore = dataIn;
          break;
      }
      //printf("Store at %x : %x\n", addr >> 2, mergedAccess);
      if (opType == STORE || (opType == AMO_SC && reservationAMO->valid && reservationAMO->address == addr)) {
        data[(addr >> 2) & MEMMASK] = valToStore;
        dataOut                     = 0; // success !
        reservationAMO->valid       = 0;
      } else if (opType == AMO_SC) {
        reservationAMO->valid = 0;
        dataOut               = 1; // try again !
      } else if (opType == AMO_SWAPW) {
        dataOut                     = valueLoaded; // swap
        data[(addr >> 2) & MEMMASK] = valToStore;
      }
#if LINUXOS
      else if(opType == AMO_ADDW) {
        dataOut                     = valueLoaded;
        data[(addr >> 2) & MEMMASK] = valueLoaded + dataIn;
      } else if(opType == AMO_ANDW) {
        dataOut                     = valueLoaded;
        data[(addr >> 2) & MEMMASK] = valueLoaded & dataIn;
      } else if(opType == AMO_ORW) {
        dataOut                     = valueLoaded;
        data[(addr >> 2) & MEMMASK] = valueLoaded | dataIn;
      } else if(opType == AMO_XORW) {
        dataOut                     = valueLoaded;
        data[(addr >> 2) & MEMMASK] = valueLoaded ^ dataIn;
      }
#endif
    }

    // Debug
    /*
    if(opType == AMO_LR){
      printf("AMO_LR %x %x %x \n", dataOut, reservationAMO->valid, reservationAMO->address);
    } else if(opType == AMO_SC){
      printf("AMO_SC %x %x %x %x %x \n", dataOut, dataIn, reservationAMO->valid, reservationAMO->address);
    }
    bool addr_superior = false, addr_inferior = false;
        addr_superior = (addr >= CLINT_BASE);
    addr_inferior = (addr <= IO_BASE);
    if(opType != NONE && addr_superior && addr_inferior){
      printf("** %d CLINT %x %x **\n", hartid, addr, dataIn);
    }
    */

#ifndef __HLS__
    if (opType == STORE && addr == ac_int<32, false>(UART_RXTX)) {
      // UART
      pendingWrite = 0;
      waitOut      = false;
      if (dataIn != 0x1B && PRINT_CONSOLE)
        printf("%c", (char)dataIn);
    }
#endif
  }
};

// Multi-core Interface
template <unsigned int INTERFACE_SIZE> class MulticoreMemory : public MemoryInterfaceMulticore<INTERFACE_SIZE> {
public:
  cacheDirectoryInterfaceIn* bufferIn;
  cacheDirectoryInterfaceOut* bufferOut;
  cacheDirectoryInterfaceOut* bufferOutWB;
  bool stateDirectoryRequest = false;

public:
  MulticoreMemory(cacheDirectoryInterfaceIn* arg_bufferIn, cacheDirectoryInterfaceOut* arg_bufferOut, cacheDirectoryInterfaceOut* arg_bufferOutDir)
  {
    bufferIn    = arg_bufferIn;
    bufferOut   = arg_bufferOut;
    bufferOutWB = arg_bufferOutDir;
  }
  void process(ac_int<32, false> addr, memMask mask, memOpType opType, ac_int<4, false> hartid,
               ac_int<INTERFACE_SIZE * 8, false> dataIn, ac_int<INTERFACE_SIZE * 8, false>& dataOut, bool& waitOut, bool& waitOutDirectory)
  {
    // Incomplete memory only works for 32 bits
    assert(INTERFACE_SIZE == 4);
    bool printIO = false;
    if(waitOutDirectory){
      if(!stateDirectoryRequest){
        bufferOutWB->addr             = addr;
        bufferOutWB->interfaceAck     = 1;
        bufferOutWB->interfaceRequest = opType;
        bufferOutWB->buffer           = dataIn;
        stateDirectoryRequest         = true;
        //printf("//Directory Bus Asking...\n");
      } else {
        waitOutDirectory          = !bufferOutWB->releaseAck;
        bufferOutWB->interfaceAck = !bufferOutWB->releaseAck;
        stateDirectoryRequest     = !bufferOutWB->releaseAck;
        //(!stateDirectoryRequest){printf("//Directory Bus response...\n");}
      }
    } else if (this->wait) {
      waitOut                 = !bufferOut->releaseAck;
      bufferOut->interfaceAck = !bufferOut->releaseAck;
      dataOut                 = bufferOut->bufferIn;
      this->wait              = !bufferOut->releaseAck;
      //if(!this->wait){printf("//Directory response... %x %x %d\n", bufferOut->addr,  bufferOut->bufferIn,
      //   bufferOut->releaseAck );}
    } else if (opType != NONE) {
      bufferOut->addr             = addr;
      bufferOut->interfaceAck     = 1;
      bufferOut->interfaceRequest = opType;
      bufferOut->buffer           = dataIn;
      waitOut                     = true;
      this->wait                  = waitOut;
      printIO                     = true;
      //printf("//Request to the directory... %x %x %d\n", bufferOut->addr, opType,
      //        dataIn);
    } else {
      bufferOut->interfaceAck = 0; // STORE-WRITEBACK case
    }

#ifndef __HLS__
    if (PRINT_CONSOLE && opType == STORE && addr == UART_RXTX && !SMP) {
      // UART
      if (dataIn != 0x1B) {
        printf("%c", (char)dataIn);
      }
    }
#endif
  }

  void processInstruction(ac_int<32, false> addr, memMask mask, memOpType opType, ac_int<4, false> hartid,
               ac_int<INTERFACE_SIZE * 8, false> dataIn, ac_int<INTERFACE_SIZE * 8, false>& dataOut, bool& waitOut, bool& waitOutDirectory)
  {
    // Incomplete memory only works for 32 bits
    assert(INTERFACE_SIZE == 4);
    if (this->wait) {
      waitOut                 = !bufferOut->releaseAck;
      bufferOut->interfaceAck = !bufferOut->releaseAck;
      dataOut                 = bufferOut->bufferIn;
      this->wait              = !bufferOut->releaseAck;
      //if(!this->wait){printf("//Directory response... %x %x %d\n", bufferOut->addr, bufferOut->bufferIn,
      //   bufferOut->releaseAck );}
    } else if (opType != NONE) {
      bufferOut->addr             = addr;
      bufferOut->interfaceAck     = 1;
      bufferOut->interfaceRequest = opType;
      bufferOut->buffer           = dataIn;
      waitOut                     = true;
      this->wait                  = waitOut;
      //printf("//Request to the directory... %x %x %d\n", bufferOut->addr, opType,
      //        dataIn);
    } else {
      bufferOut->interfaceAck = 0; // STORE-WRITEBACK case
    }
  }
};

#endif //__MEMORY_INTERFACE_H__
