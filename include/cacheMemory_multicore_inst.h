/*
 * cacheMemory_multicore_inst.h
 *
 *  Created on: 07 July. 2021
 *      Author: Simon, Logan
 *
 *  This special cache is used for multicore system:

 */

#ifndef INCLUDE_CACHEMEMORY_MULTI_INST_H_
#define INCLUDE_CACHEMEMORY_MULTI_INST_H_

#include <ac_int.h>

/************************************************************************
 * 	Following values are templates:
 * 		- OFFSET_SIZE
 * 		- TAG_SIZE
 * 		- SET_SIZE
 * 		- ASSOCIATIVITY
 *
 * -> bool : missDone 
 * 
 *   This variable aims to force the cpu to have a new instruction 
 *   coming into the cache after the miss. This variable has been added 
 *   because when a miss write occurs at the same time with a directory 
 *   request, the cache will do the directory request (highest priority)
 *   and will lost the knowledge of the previous instruction. 
 *   It is still impossible to differentiate between a new instruction 
 *   or the former one.
 *
 * -> bool : enter_directory_request
 *
 *   This variable aims to trigger the FSM for directory requests. If true,
 *   it means that the directory of the multi-core system is asking for a 
 *   specific request :
 *   (REQ_INV, REQ_DATATRANSFER_SHARED, REQ_DATATRANSFER_INV).
 *   This action is invaldiated in case the cache is in a miss state 
 *   This situation must be strictly avoided by the directory for the moment !
 * 
 * -> bool : directory_request_store
 * 
 *   The directory request has been seen by the cache. It is ready to answer.
 * 
 * -> bool : data_req
 * 
 *   This variable aims to indicate if the directory asked for a data transfer. 
 *   If true, the cache needs to do a write-back.
 * 
 * -> 6-bit variable : cacheStateDir
 * 
 *   This variable is used to have a correct write-back from the cache
 *   in case of a directory request. It is based on the same mechanism
 *   as "cacheState" (used in the miss sub-fsm)
 * 
 * -> bool : forwardCacheState
 *
 *   This variable is used to accelerate cache state searching time
 *   after a directory request:
 * """
 * (placeDirMem==place_current && setStoreDir==0 && forwardCacheState) ? valStateDir 
 *                                           : multicoreCacheState[place_current][0];
 * """ 
 *   This has been added because the HLS synthesis can't find a feedback schedule that
 *   works without a little help to be faster. This action is HLS-oriented !
 * 
 * -> bool : directoryRequestStall
 *
 *    This variable indicates when an instruction/data must be sent to the cacheL2.
 *    This specific boolean is only used when the cacheL2 asked the cache for a data transfer.
 * 
 ************************************************************************/

template <unsigned int INTERFACE_SIZE, int LINE_SIZE, int SET_SIZE>
class CacheMemoryMulticoreInst : public MemoryInterface<INTERFACE_SIZE> {

  static const int LOG_SET_SIZE           = log2const<SET_SIZE>::value;
  static const int LOG_LINE_SIZE          = log2const<LINE_SIZE>::value;
  static const int TAG_SIZE               = (32 - LOG_LINE_SIZE - LOG_SET_SIZE);
  static const int ASSOCIATIVITY          = 4;
  static const int LOG_ASSOCIATIVITY      = 2;
  static const int STATE_CACHE_MISS       = ((LINE_SIZE / INTERFACE_SIZE) * 2 + 2);
  static const int STATE_CACHE_LAST_STORE = ((LINE_SIZE / INTERFACE_SIZE) + 3);
  static const int STATE_CACHE_FIRST_LOAD = ((LINE_SIZE / INTERFACE_SIZE) + 2);
  static const int STATE_CACHE_LAST_LOAD  = 2;
  static const int LOG_INTERFACE_SIZE     = log2const<INTERFACE_SIZE>::value;

public:
  // Specific Protocol for multi-core processing
  MulticoreMemory<INTERFACE_SIZE>* nextLevel;
  
  // cache system 
  ac_int<TAG_SIZE + LINE_SIZE * 8, false> cacheMemory[SET_SIZE][ASSOCIATIVITY];
  ac_int<40, false> age[SET_SIZE][ASSOCIATIVITY];
  // cache status
  cacheStatus multicoreCacheState[SET_SIZE][ASSOCIATIVITY];

  ac_int<6, false> cacheState; // Used for the internal state machine

  // Variables for next level access
  ac_int<LINE_SIZE * 8 + TAG_SIZE, false> newVal, oldVal;
  ac_int<32, false> nextLevelAddr;
  memOpType nextLevelOpType;
  ac_int<INTERFACE_SIZE * 8, false> nextLevelDataIn;
  ac_int<INTERFACE_SIZE * 8, false> nextLevelDataOut;
  memMask                           nextLevelMask;
  ac_int<40, false> cycle;
  ac_int<LOG_ASSOCIATIVITY, false> setMiss;

  // do we need to writeback the cache lines ?
  cacheStatus missCacheStatus;

  bool wasStore;
  ac_int<LOG_ASSOCIATIVITY, false> setStore;
  ac_int<LOG_SET_SIZE, false> placeStore;
  ac_int<LINE_SIZE * 8 + TAG_SIZE, false> valStore;
  ac_int<INTERFACE_SIZE * 8, false> dataOutStore;
  cacheStatus cacheStateStore;

  bool nextLevelWaitOut;
  
  // depreciated
  bool VERBOSE = false;

  // I/O - CLINT - UART - Specific Memory Request (Enclave)
  // Configuration file : soc_config.h 
  bool addrSuperior, addrInferior;

  // Multicore

  bool enter_miss;
  bool missDone;
  bool enter_directory_request;
  bool directory_request_store;
  bool data_req;
  bool forwardCacheState;
  bool directoryRequestStall;

  ac_int<6, false> cacheStateDir;
  cacheStatus valStateDir, previous_state;
  ac_int<LOG_ASSOCIATIVITY, false> setStoreDir;
  ac_int<LINE_SIZE * 8 + TAG_SIZE, false> valDir;
  ac_int<LOG_SET_SIZE, false> placeDirMem;

  // AMO
  // do we have an AMO request ?
  bool amoDataReq;
  // Does the data/addr is in the cache ?
  bool isInCache;
  // Do we need a memory answer ?
  bool grantAMOSC;
  // Do we have an AMO SWAP ? HLS-oriented to get faster
  bool swapamo;
  // Do we have a AMO Store-Conditonal ? HLS-oriented to get faster
  bool scamo;

  // Stats
  unsigned long numberAccess, numberMiss;

  CacheMemoryMulticoreInst(MulticoreMemory<INTERFACE_SIZE>* nextLevel, bool v)
  {
    this->nextLevel = nextLevel;
    VERBOSE         = v;
  }

  void initCache()
  {
    for (int oneSetElement = 0; oneSetElement < SET_SIZE; oneSetElement++) {
      for (int oneSet = 0; oneSet < ASSOCIATIVITY; oneSet++) {
        cacheMemory[oneSetElement][oneSet] = 0;
        age[oneSetElement][oneSet]         = 0;
        multicoreCacheState[oneSetElement][oneSet] = INVALID;
      }
    }
    numberAccess            = 0;
    numberMiss              = 0;
    nextLevelWaitOut        = false;
    wasStore                = false;
    cacheState              = 0;
    nextLevelOpType         = NONE;
    directory_request_store = false;
    data_req                = false;
    enter_miss              = false;
    enter_directory_request = false;
    cacheStateDir           = 0;
    placeDirMem             = 0;
    cacheStateStore         = INVALID;
    addrSuperior            = false;
    addrInferior            = false;
    amoDataReq              = false;
    isInCache               = false;
    grantAMOSC              = false;
    swapamo                 = false;
    scamo                   = false;
    missDone                = false;
    forwardCacheState       = false;
    directoryRequestStall   = false;
  }

  void process(ac_int<32, false> addr, memMask mask, memOpType opType, ac_int<4, false> hartid,
               ac_int<INTERFACE_SIZE * 8, false> dataIn, ac_int<INTERFACE_SIZE * 8, false>& dataOut, bool& waitOut)
  {
    // bit size is the log(setSize)
    ac_int<LOG_SET_SIZE, false> place = addr.slc<LOG_SET_SIZE>(LOG_LINE_SIZE);
    // startAddress is log(lineSize) + log(setSize) + 2
    ac_int<TAG_SIZE, false> tag = addr.slc<TAG_SIZE>(LOG_LINE_SIZE + LOG_SET_SIZE);
    // bitSize is log(lineSize), start address is 2(because of #bytes in a word)
    ac_int<LOG_LINE_SIZE, false> offset = addr.slc<LOG_LINE_SIZE - 2>(2);

    // Interface: cacheDirectoryInterface cacheControllerReg
    // Same decomposition of place/tag
    ac_int<LOG_SET_SIZE, false> placeDir = this->nextLevel->bufferIn->addr.template slc<LOG_SET_SIZE>(LOG_LINE_SIZE);
    ac_int<TAG_SIZE, false> tag_dir =
        this->nextLevel->bufferIn->addr.template slc<TAG_SIZE>(LOG_LINE_SIZE + LOG_SET_SIZE);

    // CLINT
    addrSuperior = (addr >= CLINT_BASE);
    addrInferior = (addr <= UART_BASE_ADDR + UART_SIZE);

    // enter_miss cannot be true when we receive a directory request ! The following boolean is a security barrier
    // (debug purpose)
    enter_directory_request = this->nextLevel->bufferIn->interfaceAck && !enter_miss;

    this->nextLevel->bufferIn->releaseAck = 0;
    missDone                              = false;

    //printf("Core %d : %d %d %d \n", hartid, nextLevelWaitOut, enter_directory_request, directoryRequestStall);
    if ((!nextLevelWaitOut || enter_directory_request) && !directoryRequestStall) {
      cycle++;

      if ((directory_request_store && !data_req) || ((wasStore || cacheState == 1 || amoDataReq) && !nextLevelWaitOut)) {
        ac_int<LOG_SET_SIZE, false> placeTemp;// cache sets block (4 associative sets)
        ac_int<LOG_ASSOCIATIVITY, false> setTemp;// cache associativity
        bool cacheStateModified = true;// do we need to modify the cache status ?
        bool cacheWrite = (!amoDataReq && (wasStore || cacheState == 1));// do we have a cache write (Hit W / Miss R-W) ?
        bool amoWrite   = (isInCache && ((scamo && dataOutStore[0]) || swapamo));// do we have an AMO write ?
        
        if(directory_request_store && !data_req) {
          // we should never see a miss at the same time with a directory request
          this->nextLevel->bufferIn->releaseAck     = 1; // stay for one cycle
          placeTemp                                 = placeDir;
          setTemp                                   = setStoreDir;
          forwardCacheState                         = true;
          placeDirMem                               = placeDir;
        } else if ((!isInCache || scamo) && amoDataReq && !grantAMOSC) {
          // AMO specific requests should wait a memory answer => one more state machine (grantAMOSC) :
          // AMO addr not in cache => !isInCache => the answer comes from the memory
          // AMO Store-Conditional => registrationSet => valid or not ?
          dataOutStore                              = nextLevelDataOut;
          grantAMOSC                                = true;
          cacheStateModified                        = false;
        } else {
          // cache write
          if (cacheWrite || amoWrite) {
            cacheMemory[placeStore][setStore]       = valStore;
            age[placeStore][setStore]               = cycle;
          } else {
            // do not modify the cache state !
            // AMO Load-Reserve
            // AMO addr not in cache
            cacheStateModified                      = false;
          }
          dataOut                                   = dataOutStore;
          grantAMOSC                                = false;
          wasStore                                  = false;
          cacheState                                = 0;
          missDone                                  = true;
          amoDataReq                                = false;
          isInCache                                 = false;
          enter_miss                                = false;
          placeTemp                                 = placeStore;
          setTemp                                   = setStore;
          forwardCacheState                         = false;
          swapamo                                   = false;
          scamo                                     = false;
        }
        
        if(cacheStateModified)
          multicoreCacheState[placeTemp][setTemp]   = (directory_request_store && !data_req) ? valStateDir : cacheStateStore;
          
        nextLevelOpType                             = NONE;
        directory_request_store                     = false;
        //printf("Core %d END MISS/Directory/Store ! %d %x %x \n", (unsigned int)hartid, (unsigned int)multicoreCacheState[placeTemp][setTemp], (unsigned int)placeTemp, (unsigned int)setTemp);
      } else if (opType != NONE || enter_directory_request) {
        ac_int<LOG_SET_SIZE, false> place_current = enter_directory_request ? placeDir : place;
        ac_int<TAG_SIZE, false> tag_current       = enter_directory_request ? tag_dir : tag;

        ac_int<LINE_SIZE * 8 + TAG_SIZE, false> val1 = cacheMemory[place_current][0];
        ac_int<LINE_SIZE * 8 + TAG_SIZE, false> val2 = cacheMemory[place_current][1];
        ac_int<LINE_SIZE * 8 + TAG_SIZE, false> val3 = cacheMemory[place_current][2];
        ac_int<LINE_SIZE * 8 + TAG_SIZE, false> val4 = cacheMemory[place_current][3];

        ac_int<40, false> age1 = age[place_current][0];
        ac_int<40, false> age2 = age[place_current][1];
        ac_int<40, false> age3 = age[place_current][2];
        ac_int<40, false> age4 = age[place_current][3];

        cacheStatus multicoreCacheState1 = (placeDirMem==place_current && setStoreDir==0 && forwardCacheState) ? valStateDir : multicoreCacheState[place_current][0];
        cacheStatus multicoreCacheState2 = (placeDirMem==place_current && setStoreDir==1 && forwardCacheState) ? valStateDir : multicoreCacheState[place_current][1];
        cacheStatus multicoreCacheState3 = (placeDirMem==place_current && setStoreDir==2 && forwardCacheState) ? valStateDir : multicoreCacheState[place_current][2];
        cacheStatus multicoreCacheState4 = (placeDirMem==place_current && setStoreDir==3 && forwardCacheState) ? valStateDir : multicoreCacheState[place_current][3];
        forwardCacheState = false;
       
        if (cacheState == 0 && addrSuperior && addrInferior && !enter_directory_request) {
          // CLINT - Forward
          nextLevelAddr   = addr;
          nextLevelOpType = opType;
          nextLevelDataIn = dataIn;
          nextLevelMask   = mask;
          cacheState      = 2; // avoid the first "if" statement
          //printf("** CLINT %x %x **\n", (unsigned int)addr>>2, (unsigned int)dataIn);
        } else if (cacheState == 2 && addrSuperior && addrInferior && !enter_directory_request) {
          // CLINT data acquisition
          cacheState      = 0;
          dataOut         = nextLevelDataOut;
          nextLevelOpType = NONE; // avoid a memory call
          //printf("** END_CLINT %x %x **\n", (unsigned int)dataOut, (unsigned int)addr>>2);
        } else if ((cacheState == 0 && !enter_directory_request) ||
                   (enter_directory_request && !directory_request_store && !data_req)) {
          numberAccess++;
          
          ac_int<TAG_SIZE, false> tag1 = val1.template slc<TAG_SIZE>(0);
          ac_int<TAG_SIZE, false> tag2 = val2.template slc<TAG_SIZE>(0);
          ac_int<TAG_SIZE, false> tag3 = val3.template slc<TAG_SIZE>(0);
          ac_int<TAG_SIZE, false> tag4 = val4.template slc<TAG_SIZE>(0);

          bool hit1 = (tag1 == tag_current) && (multicoreCacheState1 != INVALID);
          bool hit2 = (tag2 == tag_current) && (multicoreCacheState2 != INVALID);
          bool hit3 = (tag3 == tag_current) && (multicoreCacheState3 != INVALID);
          bool hit4 = (tag4 == tag_current) && (multicoreCacheState4 != INVALID);
          bool hit  = hit1 | hit2 | hit3 | hit4;

          ac_int<LOG_ASSOCIATIVITY, false> set = 0;
          ac_int<LINE_SIZE * 8, false> selectedValue;
          ac_int<TAG_SIZE, false> tag;

          if (hit1) {
            selectedValue  = val1.template slc<LINE_SIZE * 8>(TAG_SIZE);
            tag            = tag1;
            set            = 0;
            previous_state = multicoreCacheState1;
          }

          if (hit2) {
            selectedValue  = val2.template slc<LINE_SIZE * 8>(TAG_SIZE);
            tag            = tag2;
            set            = 1;
            previous_state = multicoreCacheState2;
          }

          if (hit3) {
            selectedValue  = val3.template slc<LINE_SIZE * 8>(TAG_SIZE);
            tag            = tag3;
            set            = 2;
            previous_state = multicoreCacheState3;
          }

          if (hit4) {
            selectedValue  = val4.template slc<LINE_SIZE * 8>(TAG_SIZE);
            tag            = tag4;
            set            = 3;
            previous_state = multicoreCacheState4;
          }

          ac_int<8, true> signedByte;
          ac_int<16, true> signedHalf;
          ac_int<32, true> signedWord;

          if (hit && enter_directory_request) {
            setStoreDir = set;
            valStateDir = (this->nextLevel->bufferIn->interfaceRequest == REQ_INV ||
                           this->nextLevel->bufferIn->interfaceRequest == REQ_DATATRANSFER_INV)
                              ? INVALID
                              : SHARED;
            directory_request_store = true;
            data_req                = (this->nextLevel->bufferIn->interfaceRequest == REQ_INV) ? false : true;
            cacheStateDir           = (this->nextLevel->bufferIn->interfaceRequest == REQ_INV) ? 0 : STATE_CACHE_MISS;
            //printf(" Core %d directory request ! %d %x %x \n", hartid, this->nextLevel->bufferIn->interfaceRequest, placeDir, setStoreDir);
          } else if (!hit && enter_directory_request) {
            //printf("Error place %x \n", place_current);
            //while(1);
          } else if (hit) {
            ac_int<LINE_SIZE * 8 + TAG_SIZE, false> localValStore = 0;
            localValStore.set_slc(TAG_SIZE, selectedValue);
            localValStore.set_slc(0, tag);
            placeStore = place;
            setStore   = set;
            cacheStateStore = MODIFIED;
            
            // First we handle the store
            if (opType == STORE || opType == AMO_SC) {
              /*
               -- Write Hit Directory Message --
               1) If we are in a Modified state, do not send a message to the directory.
               2) If we are in a Shared, Exclusive or Owned state, you must invalidate all caches that have the same
               block or change the status of the directory.
              */
              switch (mask) {
                case BYTE:
                case BYTE_U:
                  localValStore.set_slc((((int)addr.slc<2>(0)) << 3) + TAG_SIZE + 4 * 8 * offset,
                                        dataIn.template slc<8>(0));
                  break;
                case HALF:
                case HALF_U:
                  localValStore.set_slc((addr[1] ? 16 : 0) + TAG_SIZE + 4 * 8 * offset, dataIn.template slc<16>(0));
                  break;
                case WORD:
                  localValStore.set_slc(TAG_SIZE + 4 * 8 * offset, dataIn.template slc<32>(0));
                  break;
                case LONG:
                  localValStore.set_slc(TAG_SIZE + 4 * 8 * offset, dataIn);
                  break;
              }
              // Hit write
              wasStore        = true;
              // AMO_SC
              // Optimized !
              nextLevelOpType = (opType == AMO_SC) ? AMO_SC : (previous_state == MODIFIED) ? NONE : WRITEHIT;
              nextLevelAddr   = addr;                     // bufferOut->addr = addr;
              nextLevelDataIn = dataIn;
              amoDataReq      = (opType == AMO_SC) ? true : false;
              isInCache       = (opType == AMO_SC) ? true : false;
              scamo           = (opType == AMO_SC) ? true : false;
              //printf("// %d Hit write at %x: %x amo(%d)\n", (unsigned int) hartid, (unsigned int)nextLevelAddr, (unsigned int)dataIn, amoDataReq);
            } else if (opType == LOAD || opType == AMO_LR || opType == AMO_SWAPW) {
              /*
               -- Read Hit Directory Message --
               Do not send a message to the directory.
              */
              switch (mask) {
                case BYTE:
                  signedByte = selectedValue.template slc<8>((((int)addr.slc<2>(0)) << 3) + 4 * 8 * offset);
                  signedWord = signedByte;
                  dataOut.set_slc(0, signedWord);
                  break;
                case HALF:
                  signedHalf = selectedValue.template slc<16>((addr[1] ? 16 : 0) + 4 * 8 * offset);
                  signedWord = signedHalf;
                  dataOut.set_slc(0, signedWord);
                  break;
                case WORD:
                  dataOut = selectedValue.template slc<32>(4 * 8 * offset);
                  break;
                case BYTE_U:
                  dataOut = selectedValue.template slc<8>((((int)addr.slc<2>(0)) << 3) + 4 * 8 * offset) & 0xff;
                  break;
                case HALF_U:
                  dataOut = selectedValue.template slc<16>((addr[1] ? 16 : 0) + 4 * 8 * offset) & 0xffff;
                  break;
                case LONG:
                  dataOut = selectedValue.template slc<INTERFACE_SIZE * 8>(4 * 8 * offset);
                  break;
              }
              // AMO_LR && AMO_SWAPW
              dataOutStore    = dataOut;
              localValStore.set_slc(TAG_SIZE + 4 * 8 * offset, dataIn.template slc<32>(0));
              nextLevelOpType = (opType != LOAD) ? opType : NONE;
              nextLevelAddr   = addr;
              nextLevelDataIn = dataIn;
              amoDataReq      = (opType != LOAD) ? true : false;
              isInCache       = (opType != LOAD) ? true : false;
              swapamo         = (opType == AMO_SWAPW) ? true : false;
              //printf("// %d Hit read %x at %x amo(%d)\n", (unsigned int) hartid, (unsigned int)dataOut, (unsigned int)addr, amoDataReq);
            }
            
            valStore = localValStore;

          } else {
            numberMiss++;
            cacheState = STATE_CACHE_MISS;
            amoDataReq = (opType == AMO_LR || opType == AMO_SC || opType == AMO_SWAPW) ? true : false;
            nextLevelOpType =
                (opType == AMO_LR || opType == AMO_SC || opType == AMO_SWAPW)
                    ? opType
                    : (opType == STORE) ? MISS_WRITE : MISS_READ;
            nextLevelAddr   = addr;
            nextLevelDataIn = dataIn;
            //printf("// Core: %d waiting for a Miss grant... addr(%x) opType(%x) %x %d \n", (unsigned int)hartid, (unsigned int)addr, (unsigned int)opType, (unsigned int) place_current, multicoreCacheState3);
          }
        } else {
          // printf("%d Miss %d %d\n", hartid, (unsigned int)cacheState, cacheStateDir);

          if (cacheState == STATE_CACHE_MISS && !data_req) {

            /*
              --- ENTERING MISS STATE ---

              The directory let us enter into the 'miss state'
              
            */
            enter_miss = true;

            newVal  = tag;
            setMiss = (age1 < age2 && age1 < age3 && age1 < age4)
                          ? 0
                          : ((age2 < age1 && age2 < age3 && age2 < age4)
                                 ? 1
                                 : ((age3 < age2 && age3 < age1 && age3 < age4) ? 2 : 3));
            oldVal = (age1 < age2 && age1 < age3 && age1 < age4)
                         ? val1
                         : ((age2 < age1 && age2 < age3 && age2 < age4)
                                ? val2
                                : ((age3 < age2 && age3 < age1 && age3 < age4) ? val3 : val4));
            missCacheStatus = (age1 < age2 && age1 < age3 && age1 < age4)
                                 ? multicoreCacheState1
                                 : ((age2 < age1 && age2 < age3 && age2 < age4)
                                        ? multicoreCacheState2
                                        : ((age3 < age2 && age3 < age1 && age3 < age4) ? multicoreCacheState3
                                                                                : multicoreCacheState4));
            // Optimized !
            if (missCacheStatus == INVALID) {
              cacheState = STATE_CACHE_FIRST_LOAD;
            }
            // printf("TAG is %x %x\n", missCacheStatus, oldVal);
          } else if (cacheStateDir == STATE_CACHE_MISS && data_req) {
            oldVal = (setStoreDir == 0 ? val1 : (setStoreDir == 1 ? val2 : (setStoreDir == 2 ? val3 : val4)));
          }

          ac_int<32, false> oldAddress = (((int)oldVal.template slc<TAG_SIZE>(0)) << (LOG_LINE_SIZE + LOG_SET_SIZE)) |
                                         (((int)place_current) << LOG_LINE_SIZE);
          // ac_int<32, false> directoryAddress = this->nextLevel->bufferIn->addr;

          // First we write back the four memory values in upper level

          if (cacheState >= STATE_CACHE_LAST_STORE || cacheStateDir >= STATE_CACHE_LAST_STORE) {
            // We store all values into next memory interface
            nextLevelAddr = !data_req
                                ? oldAddress + (((int)(cacheState - STATE_CACHE_LAST_STORE)) << LOG_INTERFACE_SIZE)
                                : oldAddress + (((int)(cacheStateDir - STATE_CACHE_LAST_STORE)) << LOG_INTERFACE_SIZE);
            nextLevelDataIn = !data_req ? oldVal.template slc<INTERFACE_SIZE * 8>(
                                              (cacheState - STATE_CACHE_LAST_STORE) * INTERFACE_SIZE * 8 + TAG_SIZE)
                                        : oldVal.template slc<INTERFACE_SIZE * 8>(
                                              (cacheStateDir - STATE_CACHE_LAST_STORE) * INTERFACE_SIZE * 8 + TAG_SIZE);
            nextLevelOpType = WRITEBACK;
            directoryRequestStall = data_req ? true : false;
            //printf("//Core %x Writing back %x %x at %x - %d\n", (unsigned int)hartid, (unsigned int)nextLevelDataIn,
            //       (unsigned int)nextLevelDataIn, (unsigned int)nextLevelAddr, data_req);

          } else if (cacheState >= STATE_CACHE_LAST_LOAD) {
            // Then we read values from next memory level
            if (cacheState != STATE_CACHE_FIRST_LOAD) {
              newVal.set_slc(((unsigned int)(cacheState - STATE_CACHE_LAST_LOAD)) * INTERFACE_SIZE * 8 + TAG_SIZE,
                             nextLevelDataOut); // at addr +1
              //printf("Core %x LOAD Value from Memory %x \n", (unsigned int)hartid, this->nextLevel->bufferIn->buffer);
            }

            if (cacheState != STATE_CACHE_LAST_LOAD) {
              // printf("LOAD Value from Memory %x %d\n", addr, cacheState);
              // We initiate the load at the address determined by next cache state
              nextLevelAddr   = (((int)addr.slc<32 - LOG_LINE_SIZE>(LOG_LINE_SIZE)) << LOG_LINE_SIZE) +
                                ((cacheState - STATE_CACHE_LAST_LOAD - 1) << LOG_INTERFACE_SIZE);
              nextLevelOpType = LOAD;//(cacheState == STATE_CACHE_FIRST_LOAD) ? DATA_FIRST_ACQ : DATA_ACQ;
              nextLevelMask   = LONG;
            }
          }
          
          if(!data_req)
            cacheState--;
            
          data_req = (cacheStateDir <= STATE_CACHE_LAST_STORE) ? false : true;

          if (data_req)
            cacheStateDir--;
          else
            cacheStateDir = 0;

          if (cacheState == 1) {

            cacheStateStore = SHARED;

            if (opType == STORE) {
              switch (mask) {
                case BYTE:
                case BYTE_U:
                  newVal.set_slc((((int)addr.slc<2>(0)) << 3) + TAG_SIZE + 4 * 8 * offset, dataIn.template slc<8>(0));
                  break;
                case HALF:
                case HALF_U:
                  newVal.set_slc((addr[1] ? 16 : 0) + TAG_SIZE + 4 * 8 * offset, dataIn.template slc<16>(0));
                  break;
                case WORD:
                  newVal.set_slc(TAG_SIZE + 4 * 8 * offset, dataIn.template slc<32>(0));
                  break;
                case LONG:
                  newVal.set_slc(TAG_SIZE + 4 * 8 * offset, dataIn);
                  break;
              }
              cacheStateStore = MODIFIED;
            }

            placeStore = place;
            setStore   = setMiss;
            valStore   = newVal;

            nextLevelOpType = END_MISS;
            nextLevelAddr   = addr;

            ac_int<8, true> signedByte;
            ac_int<16, true> signedHalf;
            ac_int<32, true> signedWord;

            switch (mask) {
              case BYTE:
                signedByte = newVal.template slc<8>((((int)addr.slc<2>(0)) << 3) + 4 * 8 * offset + TAG_SIZE);
                signedWord = signedByte;
                dataOut.set_slc(0, signedWord);
                break;
              case HALF:
                signedHalf = newVal.template slc<16>((addr[1] ? 16 : 0) + 4 * 8 * offset + TAG_SIZE);
                signedWord = signedHalf;
                dataOut.set_slc(0, signedWord);
                break;
              case WORD:
                dataOut = newVal.template slc<32>(4 * 8 * offset + TAG_SIZE);
                break;
              case BYTE_U:
                dataOut = newVal.template slc<8>((((int)addr.slc<2>(0)) << 3) + 4 * 8 * offset + TAG_SIZE) & 0xff;
                break;
              case HALF_U:
                dataOut = newVal.template slc<16>((addr[1] ? 16 : 0) + 4 * 8 * offset + TAG_SIZE) & 0xffff;
                break;
              case LONG:
                dataOut = newVal.template slc<INTERFACE_SIZE * 8>(4 * 8 * offset);
                break;
            }
            // printf("After Miss read %x at %x\n", (unsigned int)dataOut.slc<32>(0), (unsigned int)addr);
            dataOutStore = dataOut;
          }
        }
      } // opType != NONE
    }   //! nextLevelWaitOut
    this->nextLevel->processInstruction(nextLevelAddr, nextLevelMask, nextLevelOpType, hartid, nextLevelDataIn, nextLevelDataOut, nextLevelWaitOut, directoryRequestStall);
    waitOut = (nextLevelWaitOut && opType != NONE) || cacheState || wasStore || amoDataReq ||
              (this->nextLevel->bufferIn->interfaceAck && opType != NONE && !missDone);
  }
};

#endif /* INCLUDE_CACHEMEMORY_MULTI_INST_H_ */
