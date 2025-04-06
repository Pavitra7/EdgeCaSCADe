/*
 * cacheMemoryL2.h
 *
 *  Created on: 07 July. 2021
 *      Author: Simon, Logan
 *
 *  This special cache is used for multicore system Level 2:
 */

#ifndef INCLUDE_CACHEMEMORYL2_H_
#define INCLUDE_CACHEMEMORYL2_H_

#include <ac_int.h>
#include <memoryInterface.h>
/************************************************************************
 * 	Following values are templates:
 * 		- OFFSET_SIZE
 * 		- TAG_SIZE
 * 		- SET_SIZE
 * 		- ASSOCIATIVITY
 ************************************************************************/
 
typedef struct directoryStruct {
  cacheStatus cacheStateDir = INVALID;             // cache status
  ac_int<log2const<NB_CORES>::value, false> owner; // Who is the owner ?
  ac_int<NB_CORES, false> sharers   = 0;           // Which cores are sharing the data ?
  ac_int<NB_CORES, false> coreToken = 0;           // Does a core use this directory line ?
  ac_int<1, false> cache            = 0;
} directoryStruct;


template <unsigned int INTERFACE_SIZE, int LINE_SIZE, int SET_SIZE>
class CacheMemoryL2 : public MemoryInterface<INTERFACE_SIZE> {

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

  typedef struct instructionQueue {
    ac_int<4, false> hartid;
    ac_int<1, false> cache;
    directoryOpType interfaceRequest;
    ac_int<32, false> addr;
    ac_int<1, false> valid;
  } instructionQueue;
  
  IncompleteMemory<INTERFACE_SIZE>* nextLevel;

  ac_int<TAG_SIZE + LINE_SIZE * 8, false> cacheMemory[SET_SIZE][ASSOCIATIVITY];
  ac_int<40, false> age[SET_SIZE][ASSOCIATIVITY];
  ac_int<1, false> dataValid[SET_SIZE][ASSOCIATIVITY];
  // Directory
  directoryStruct directory[SET_SIZE][ASSOCIATIVITY];
  
  ac_int<6, false> cacheState;                // Used for the internal state machine
  ac_int<LOG_ASSOCIATIVITY, false> older = 0; // Set where the miss occurs

  // Variables for next level access
  ac_int<LINE_SIZE * 8 + TAG_SIZE, false> newVal, oldVal;
  
  // Toward Memory
  ac_int<32, false> nextLevelAddr;
  memOpType nextLevelOpType;
  ac_int<INTERFACE_SIZE * 8, false> nextLevelDataIn;
  ac_int<INTERFACE_SIZE * 8, false> nextLevelDataOut;
  
  // Directory Internal FSM
  directoryStruct directoryStore;
  // Is the core busy ? (directory has requested this core before)
  ac_int<NB_CORES, false> busyCore = 0;
  
  // Which core is chosen to be the emitter ? (if necessary)
  int interProcConn[NB_CORES] = {0};
  instructionQueue fifoInstruction;// only one for the moment
  bool waitingForCacheAnswer;
  bool cacheModif;
  bool isCycleModified;
  bool specialWBRequest;
  bool directoryModified;
  // Used for the internal state machine
  ac_int<6, false> cacheStateMissRequest;
  ac_int<1, false> protocolMiss;
  // AMO operations
  // AMO reservation set (address + valid)
  reservationSet reservationAMO[NB_CORES];
  
  ac_int<40, false> cycle;
  ac_int<LOG_ASSOCIATIVITY, false> setMiss;
  bool isValid;
  directoryStruct missDirectoryState;

  bool wasStore = false;
  ac_int<LOG_ASSOCIATIVITY, false> setStore;
  ac_int<LOG_SET_SIZE, false> placeStore;
  ac_int<LINE_SIZE * 8 + TAG_SIZE, false> valStore;
  ac_int<INTERFACE_SIZE * 8, false> dataOutStore;

  bool nextLevelWaitOut;

  bool VERBOSE = false;

  bool addr_superior = false, addr_inferior = false;

  // Stats
  unsigned long numberAccess, numberMiss;

  CacheMemoryL2(IncompleteMemory<INTERFACE_SIZE>* nextLevel, bool v)
  {
    this->nextLevel = nextLevel;
    VERBOSE         = v;
    initCache();
  }

  void initCache()
  {
    for (int oneSetElement = 0; oneSetElement < SET_SIZE; oneSetElement++) {
      for (int oneSet = 0; oneSet < ASSOCIATIVITY; oneSet++) {
        cacheMemory[oneSetElement][oneSet] = 0;
        age[oneSetElement][oneSet]         = 0;
        dataValid[oneSetElement][oneSet]   = 0;
        initDirectory(directory[oneSetElement][oneSet]);
      }
    }
    cycle            = 0;
    numberAccess     = 0;
    numberMiss       = 0;
    nextLevelWaitOut = false;
    wasStore         = false;
    cacheState       = 0;
    nextLevelOpType  = NONE;
    cacheStateMissRequest = 0;
    waitingForCacheAnswer = false;
    cacheModif = false;
    isCycleModified = true;
    specialWBRequest = false;
    directoryModified = true;
    protocolMiss = 0;
  }
  
  void initDirectory(directoryStruct& directoryElement){
    directoryElement.cacheStateDir = INVALID;             // cache status
    directoryElement.owner     = 0; // Who is the owner ?
    directoryElement.sharers   = 0; // Which cores are sharing the data ?
    directoryElement.coreToken = 0; // Does a core use this directory line ?
    directoryElement.cache     = 0;
  }

  // Simple copy of the values to make some changes and then do the store
  void transferDirectoryStruct(directoryStruct& directoryFrom, directoryStruct& directoryTo){
    directoryTo.cacheStateDir = directoryFrom.cacheStateDir;
    directoryTo.owner         = directoryFrom.owner;
    directoryTo.sharers       = directoryFrom.sharers;
    directoryTo.coreToken     = directoryFrom.coreToken;
    directoryTo.cache         = directoryFrom.cache;
  }

  // store new instruction
  void pushInstruction(instructionQueue newinstruction, instructionQueue& instructionQueueSystem){
    instructionQueueSystem.hartid = newinstruction.hartid;
    instructionQueueSystem.cache = newinstruction.cache;
    instructionQueueSystem.interfaceRequest = newinstruction.interfaceRequest;
    instructionQueueSystem.addr = newinstruction.addr;
    instructionQueueSystem.valid = 1;
  }
  
  /*
  Core <--> Cache L1 <-->
   
        ...                 RRBus + DirBus <--> Cache L2 <--> Memory
  
  Core <--> Cache L1 <-->  
  */
  
  void process(ac_int<32, false> addr, memMask mask, memOpType opType, ac_int<4, false> hartid,
               ac_int<INTERFACE_SIZE * 8, false> dataIn, ac_int<INTERFACE_SIZE * 8, false>& dataOut, bool& waitOut){};

  void processL2(cacheL2DirectoryInterfaceIn& bufferIn, cacheL2DirectoryInterfaceOut& bufferOut);
};

template <unsigned int INTERFACE_SIZE, int LINE_SIZE, int SET_SIZE>
void CacheMemoryL2<INTERFACE_SIZE, LINE_SIZE, SET_SIZE>::processL2(cacheL2DirectoryInterfaceIn& bufferIn, cacheL2DirectoryInterfaceOut& bufferOut)
{
        
    ac_int<32, false> addr                   = bufferIn.addr;
    memMask mask                             = LONG;
    memOpType opType                         = bufferIn.interfaceRequest;
    ac_int<INTERFACE_SIZE * 8, false> dataIn = bufferIn.buffer;
    ac_int<4, false> hartid                  = bufferIn.hartid;
    ac_int<1, false> cache                   = bufferIn.cache;
    // stay one cycle high to avoid conflict (unless burst of hit+release)
    bufferIn.releaseAck                 = 0;
    // high for one cycle to avoid collisions
    bufferIn.releaseAckWB               = false;
    bufferIn.rejection                  = false;
    
    // bit size is the log(setSize)
    ac_int<LOG_SET_SIZE, false> place = addr.slc<LOG_SET_SIZE>(LOG_LINE_SIZE);
    // startAddress is log(lineSize) + log(setSize) + 2
    ac_int<TAG_SIZE, false> tag = addr.slc<TAG_SIZE>(LOG_LINE_SIZE + LOG_SET_SIZE);
    // bitSize is log(lineSize), start address is 2(because of #bytes in a word)
    ac_int<LOG_LINE_SIZE, false> offset = addr.slc<LOG_LINE_SIZE - 2>(2);
    
    // Same decomposition of place/tag
    ac_int<LOG_SET_SIZE, false> placeWB = bufferIn.addrWB.slc<LOG_SET_SIZE>(LOG_LINE_SIZE);
    ac_int<TAG_SIZE, false> tagWB       = bufferIn.addrWB.slc<TAG_SIZE>(LOG_LINE_SIZE + LOG_SET_SIZE);
    ac_int<LOG_LINE_SIZE, false> offsetWB = bufferIn.addrWB.slc<LOG_LINE_SIZE - 2>(2);
    // CLINT-UART
    addr_superior = (addr >= CLINT_BASE);
    addr_inferior = (addr <= UART_BASE_ADDR + UART_SIZE);
    
    // Send Instruction to core
    if(waitingForCacheAnswer){
      waitingForCacheAnswer      = !bufferOut.releaseAck;
      bufferOut.interfaceAck     = !bufferOut.releaseAck;
      if(!waitingForCacheAnswer)
        fifoInstruction.valid = 0;
      //fifoInstruction.pop_back();
    }
    if(busyCore != 0 && busyCore[hartid] == 0 && bufferIn.interfaceAck) {
      bufferIn.rejection = true;
      //printf(" Rejection... \n");
    } else if ((!nextLevelWaitOut && bufferIn.interfaceAck && !waitingForCacheAnswer) || bufferIn.interfaceAckWB) {
      cycle++;
      
      if (wasStore || cacheState == 1) {
        // do we want to write a new value ?
        if(cacheModif){
          cacheMemory[placeStore][setStore] = valStore;
          dataValid[placeStore][setStore]   = 1;
        }
        // do we change the cycle (aging wb) ?
        if(isCycleModified) {
          age[placeStore][setStore] = cycle;
        }
        // do we change the directory status ?
        if(directoryModified){
          // directory Write
          transferDirectoryStruct(directoryStore, directory[placeStore][setStore]);
        }
        // AMOSWAP = Write + Load. Otherwise, we need to inform of a success (AMO_SC case)
        bufferIn.dataLoad                 = (opType == AMO_SWAPW || opType == AMO_LR) ? dataOutStore : ac_int<INTERFACE_SIZE * 8, false>(0);// AMO success
        bufferIn.releaseAck               = (cacheState == 1 || specialWBRequest) ? 0 : 1;
        // Miss
        bufferIn.protocolMiss             = protocolMiss;
        // 0 memory call 1 Shared
        protocolMiss                      = 0; 
        // FSM Init state
        wasStore                          = false;
        if(!specialWBRequest)
          cacheState                        = 0;
        cacheModif                        = false;
        isCycleModified                   = false;
        directoryModified                 = false;
        // End WB request
        bufferIn.releaseAckWB             = specialWBRequest ? true : false;
        specialWBRequest                  = false;
        //printf("[CacheL2] Successfully written the new data where ? (%x,%x) rl(%d) @(%x) sh(%x) state(%x) data(%x)\n", placeStore, setStore, bufferIn.releaseAck, addr, directory[placeStore][setStore].sharers, directory[placeStore][setStore].cacheStateDir, valStore.template slc<32>(offset*8*4));
      } else if (opType != NONE || bufferIn.interfaceAckWB) {
        ac_int<LOG_SET_SIZE, false> place_current = bufferIn.interfaceAckWB ? placeWB : place;
        ac_int<TAG_SIZE, false> tag_current       = bufferIn.interfaceAckWB ? tagWB : tag;
        
        ac_int<LINE_SIZE * 8 + TAG_SIZE, false> val1 = cacheMemory[place_current][0];
        ac_int<LINE_SIZE * 8 + TAG_SIZE, false> val2 = cacheMemory[place_current][1];
        ac_int<LINE_SIZE * 8 + TAG_SIZE, false> val3 = cacheMemory[place_current][2];
        ac_int<LINE_SIZE * 8 + TAG_SIZE, false> val4 = cacheMemory[place_current][3];

        ac_int<1, false> valid1 = dataValid[place_current][0];
        ac_int<1, false> valid2 = dataValid[place_current][1];
        ac_int<1, false> valid3 = dataValid[place_current][2];
        ac_int<1, false> valid4 = dataValid[place_current][3];

        ac_int<40, false> age1 = age[place_current][0];
        ac_int<40, false> age2 = age[place_current][1];
        ac_int<40, false> age3 = age[place_current][2];
        ac_int<40, false> age4 = age[place_current][3];
        
        directoryStruct directoryState1 = directory[place_current][0];
        directoryStruct directoryState2 = directory[place_current][1];
        directoryStruct directoryState3 = directory[place_current][2];
        directoryStruct directoryState4 = directory[place_current][3];

        if (cacheState == 0 && addr_superior && addr_inferior) {
          // CLINT - Forward
          nextLevelAddr   = addr;
          nextLevelOpType = opType;
          nextLevelDataIn = dataIn;
          cacheState      = 2; // avoid the first "if" statement
          //printf("**L2 %d CLINT %x %c **\n", (unsigned int)hartid, (unsigned int)addr>>2, (unsigned int)dataIn);
        } else if (cacheState == 2 && addr_superior && addr_inferior) {
          cacheState          = 0;
          bufferIn.dataLoad   = nextLevelDataOut;
          bufferIn.releaseAck = 1;
          nextLevelOpType = NONE; // avoid a memory call
          // printf("** END_CLINT %x %x **\n", (unsigned int)dataOut, (unsigned
          // int)addr>>2);
        } else if (cacheState == 0 || bufferIn.interfaceAckWB) {
          numberAccess++;

          ac_int<TAG_SIZE, false> tag1 = val1.template slc<TAG_SIZE>(0);
          ac_int<TAG_SIZE, false> tag2 = val2.template slc<TAG_SIZE>(0);
          ac_int<TAG_SIZE, false> tag3 = val3.template slc<TAG_SIZE>(0);
          ac_int<TAG_SIZE, false> tag4 = val4.template slc<TAG_SIZE>(0);

          bool hit1 = (tag1 == tag_current) && valid1;
          bool hit2 = (tag2 == tag_current) && valid2;
          bool hit3 = (tag3 == tag_current) && valid3;
          bool hit4 = (tag4 == tag_current) && valid4;
          bool hit  = hit1 | hit2 | hit3 | hit4;
          
          //printf("tag1 %x tagcurrent %x \n", tag1, tag_current);

          ac_int<LOG_ASSOCIATIVITY, false> set = 0;
          ac_int<LINE_SIZE * 8, false> selectedValue;
          ac_int<TAG_SIZE, false> tag;
          directoryStruct directoryStateHit;
          
          if (hit1) {
            selectedValue = val1.template slc<LINE_SIZE * 8>(TAG_SIZE);
            tag           = tag1;
            set           = 0;
            directoryStateHit = directoryState1;
          }

          if (hit2) {
            selectedValue = val2.template slc<LINE_SIZE * 8>(TAG_SIZE);
            tag           = tag2;
            set           = 1;
            directoryStateHit = directoryState2;
          }

          if (hit3) {
            selectedValue = val3.template slc<LINE_SIZE * 8>(TAG_SIZE);
            tag           = tag3;
            set           = 2;
            directoryStateHit = directoryState3;
          }

          if (hit4) {
            selectedValue = val4.template slc<LINE_SIZE * 8>(TAG_SIZE);
            tag           = tag4;
            set           = 3;
            directoryStateHit = directoryState4;
          }

          ac_int<8, true> signedByte;
          ac_int<16, true> signedHalf;
          ac_int<32, true> signedWord;
          
          if(hit && bufferIn.interfaceAckWB){
            // there must be a hit !
            // load new value
            ac_int<LINE_SIZE * 8 + TAG_SIZE, false> localValStore = 0;
            localValStore.set_slc(TAG_SIZE, selectedValue);
            localValStore.set_slc(0, tag);
            localValStore.set_slc(TAG_SIZE + INTERFACE_SIZE * 8 * offsetWB, bufferIn.bufferWB.template slc<32>(0));
            
            valStore              = localValStore;
            // once fsm cycle
            wasStore              = true;
            // where to store ?
            placeStore            = place_current;
            setStore              = set;
            // use to release the ack
            specialWBRequest      = true;
            // do I want to write a new value ? Yes
            cacheModif            = true;
            // do not modify the cycle (see Miss state)
            isCycleModified       = false;
            // no modification of the directory (see Writehit and Miss state)
            directoryModified     = false;
            
            //printf("[CacheL2] DirWriteBack @%x data(%x)\n", bufferIn.addrWB, valStore.template slc<32>(TAG_SIZE + INTERFACE_SIZE * 8 * offsetWB) );
          } else if (!hit && bufferIn.interfaceAckWB) {
            //printf("[Cache L2] Error \n");
            //while(1);
          } else if (hit) {
            ac_int<LINE_SIZE * 8 + TAG_SIZE, false> localValStore = 0;
            localValStore.set_slc(TAG_SIZE, selectedValue);
            localValStore.set_slc(0, tag);
            if (opType == AMO_SC) {
              // AMO_SC can only happen after an AMO_LR
              // two hypothesis here : 
              // 1) AMO_SC is done in the cache L1 but a write in the cache L2 does not affect anything
              // 2) AMO_SC is done in the cache L2 bc cache L1 does not have it and AMO_LR fetched the data
              //    without WB (miss state instead of here otherwise)
              //    between the AMO_LR and the AMO_SC (these isntructions should be very temporarily closed)
              if (reservationAMO[hartid].valid && reservationAMO[hartid].address == addr) {
                transferDirectoryStruct(directoryStateHit, directoryStore);
                directoryStore.cacheStateDir = MODIFIED;
                // AMO write
                localValStore.set_slc(TAG_SIZE + 4 * 8 * offset, dataIn.template slc<32>(0));
                valStore = localValStore;
                // directory write ? no
                directoryModified     = (directoryStateHit.sharers[hartid] == 0) ? false : true;
                cacheModif            = true;
                isCycleModified       = true;
                // do we need to write a new value ? yes
                wasStore              = true;
                // where ?
                placeStore            = place_current;
                setStore              = set;
              } else {
                bufferIn.dataLoad     = 1; // try again !
                bufferIn.releaseAck   = true;
                //printf("AMO try again \n");
              }
              // This part can be avoided with strict amo management
              for(int t = 0; t < NB_CORES; t++){
                if(reservationAMO[t].address == addr){
                  reservationAMO[t].valid    = 0;
                }
              }
            } else if(opType == AMO_SWAPW && cacheStateMissRequest == 0){
              // Which cores need to be updated ?
              // two cores can't process the same directory at the same time : sequential processing
              // Some check could be done
              bool isExclusive = (directoryStateHit.cacheStateDir == EXCLUSIVE || directoryStateHit.cacheStateDir == MODIFIED) && (directoryStateHit.sharers[hartid] == 1);
              placeStore       = place_current;
              setStore         = set;
	      transferDirectoryStruct(directoryStateHit, directoryStore);
              
              if(isExclusive){
                localValStore.set_slc(TAG_SIZE + 4 * 8 * offset, dataIn.template slc<32>(0));
                valStore           = localValStore;
                dataOutStore       = selectedValue.template slc<INTERFACE_SIZE * 8>(4 * 8 * offset);
                directoryStore.cacheStateDir = MODIFIED;
                cacheModif         = true;
                wasStore           = true;
                directoryModified  = true;
                isCycleModified    = true;
                //printf("// AMO SWAPW (%d %d) \n", (unsigned int)directoryStateHit.sharers[0], (unsigned int)directoryStateHit.sharers[1]);
              } else { 
                cacheStateMissRequest = NB_CORES+1;
              }
            } else if (opType == AMO_SWAPW) {
              if(cacheStateMissRequest == 1){
                cacheModif            = true;
                wasStore              = true;
                directoryModified     = true;
                isCycleModified       = true;
                directoryStore.cacheStateDir   = directoryStateHit.sharers[hartid] == 1 ? MODIFIED : INVALID;
                cacheStateMissRequest          = 0;
                // new value to write
                localValStore.set_slc(TAG_SIZE + 4 * 8 * offset, dataIn.template slc<32>(0));
                valStore              = localValStore;
                dataOutStore          = selectedValue.template slc<INTERFACE_SIZE * 8>(4 * 8 * offset);
                //printf("// AMO SWAPW (%d %d) \n", (unsigned int)directoryStateHit.sharers[0], (unsigned int)directoryStateHit.sharers[1]);
              } else if (cacheStateMissRequest-2 != int(hartid) && directoryStateHit.sharers[cacheStateMissRequest-2] == 1) {
                // SEND DATA_INV to core t !
                instructionQueue newInstruction;
                newInstruction.hartid           = cacheStateMissRequest-2;
                newInstruction.cache            = directoryStateHit.cache;
                newInstruction.interfaceRequest = REQ_INV;
                newInstruction.addr             = addr;
                pushInstruction(newInstruction, fifoInstruction);
                //fifoInstruction.push_back(newInstruction);
                directoryStore.sharers[cacheStateMissRequest-2] = 0;
                cacheStateMissRequest--;
                //printf("// AMO SWAPW Special REQ addr(%x) status(%d,%d) inv --> core %d MISS Request \n", addr, (int)directoryStore.sharers[0], (int)directoryStore.sharers[1], cacheStateMissRequest-2);
              } else {
                cacheStateMissRequest--;
              }
            } else if (opType == WRITEBACK) {
              // This writeback can only happen because of an eviction
              localValStore.set_slc(TAG_SIZE + 4 * 8 * offset, dataIn.template slc<32>(0));
              
              // need to store a new value
              placeStore      = place_current;
              setStore        = set;
              valStore        = localValStore;
              wasStore        = true;
              // we need to write the new value
              cacheModif      = true;
              // bc it is a writeback we do not effectively use it
              isCycleModified = false;
              // we need to modify the directory status
              directoryModified = true;
              
              bool getAOwner = false;
              transferDirectoryStruct(directoryStateHit, directoryStore);
              directoryStore.sharers[hartid] = 0; // because of the eviction
              
              for (int k = 0; k < NB_CORES; k++) {
                if(k != int(hartid) && directoryStateHit.sharers[k] == 1 && !getAOwner) {
                  // SHARED --> EXCLUSIVE not done
                  // new owner
                  directoryStore.owner = k;
                  // someone is the owner now
                  getAOwner            = true;
                }
              } 
              if(!getAOwner) {
                // Modified --> Invalid = No one has it
                directoryStore.cacheStateDir = INVALID;
              }
              //printf("[CacheL2] WriteBack @%x data(%x)\n", addr, valStore.template slc<32>(TAG_SIZE + INTERFACE_SIZE * 8 * offset) );
            } else if (opType == LOAD) {
              
              ac_int<INTERFACE_SIZE * 8, false> dataOut = selectedValue.template slc<INTERFACE_SIZE * 8>(4 * 8 * offset);
              
              bufferIn.dataLoad                  = dataOut;
              bufferIn.releaseAck                = 1;
              //printf("[CacheL2] Hit Read %x @(%x) \n", dataOut, addr);
            } else if(opType == AMO_LR && cacheStateMissRequest == 0){
              // Which cores need to be updated ?
              // two cores can't process the same directory at the same time : sequential processing
              // Some check could be done
              bool isExclusive = (directoryStateHit.cacheStateDir == EXCLUSIVE) && (directoryStateHit.sharers[hartid] == 1);
              placeStore       = place_current;
              setStore         = set;
		      transferDirectoryStruct(directoryStateHit, directoryStore);
              directoryStore.owner = hartid;
              directoryStore.cacheStateDir = (directoryStateHit.sharers[hartid] == 0) ? INVALID : (directoryStateHit.cacheStateDir == SHARED) ? EXCLUSIVE : directoryStore.cacheStateDir;
              
              if(isExclusive){
                // reservation
                reservationAMO[hartid].valid     = 1;
                reservationAMO[hartid].address   = addr;
                
                ac_int<INTERFACE_SIZE * 8, false> dataOut = selectedValue.template slc<INTERFACE_SIZE * 8>(4 * 8 * offset);
              
                bufferIn.dataLoad                  = dataOut;
                bufferIn.releaseAck                = 1;
                //printf("[CacheL2] AMO_LR %x @(%x) \n", dataOut, addr);
              } else { 
                cacheStateMissRequest = NB_CORES+1;
              }
            } else if (opType == AMO_LR) {
              if(cacheStateMissRequest == 1){
                // reservation
                reservationAMO[hartid].valid     = 1;
                reservationAMO[hartid].address   = addr;
                cacheModif            = false;
                wasStore              = true;
                directoryModified     = true;
                isCycleModified       = true;
                ac_int<INTERFACE_SIZE * 8, false> dataOut = selectedValue.template slc<INTERFACE_SIZE * 8>(4 * 8 * offset);
                dataOutStore          = dataOut;
                cacheStateMissRequest = 0;
                //printf("[CacheL2] Hit AMO_LR %x @(%x) \n", dataOut, addr);
              } else if (cacheStateMissRequest-2 != int(hartid) && directoryStateHit.sharers[cacheStateMissRequest-2] == 1) {
                
                bool need_writeback = (directoryStateHit.cacheStateDir == MODIFIED);

		        directoryOpType interfaceRequest = (need_writeback) ? REQ_DATATRANSFER_INV : REQ_INV;
                // SEND DATA_INV to core t !
                instructionQueue newInstruction;
                newInstruction.hartid           = cacheStateMissRequest-2;
                newInstruction.cache            = directoryStateHit.cache;
                newInstruction.interfaceRequest = interfaceRequest;
                newInstruction.addr             = addr;
                pushInstruction(newInstruction, fifoInstruction);
                //fifoInstruction.push_back(newInstruction);
                directoryStore.sharers[cacheStateMissRequest-2] = 0;
                cacheStateMissRequest--;
                //printf("// AMO LR Special REQ addr(%x) status(%d,%d) %x --> core %d MISS Request \n", addr, (int)directoryStore.sharers[0], (int)directoryStore.sharers[1], interfaceRequest, cacheStateMissRequest-2);
              } else {
                cacheStateMissRequest--;
              }
            } else if(opType == END_MISS) {
              //printf("END_MISS \n");
              placeStore = place_current;
              setStore   = set;
              busyCore[hartid] = 0;
              
              transferDirectoryStruct(directoryStateHit, directoryStore);
              
              directoryStore.coreToken[hartid] = 0;
               
              cacheModif            = false;
              directoryModified     = true;
              isCycleModified       = false;
              wasStore              = true;
              // Have we issued the instruction ? Not yet
            } else if((opType == MISS_WRITE || opType == MISS_READ) && cacheStateMissRequest == 0) {
              
              bool isInvalid = directoryStateHit.cacheStateDir == INVALID;
              bool isShared = directoryStateHit.cacheStateDir == SHARED;
              bool isExclusive = directoryStateHit.cacheStateDir == EXCLUSIVE;
              placeStore = place_current;
              setStore   = set;
		      transferDirectoryStruct(directoryStateHit, directoryStore);
              if(opType == MISS_READ && (isInvalid || isShared || isExclusive)){
                // nothing to do...
                directoryStore.sharers[hartid]   = 1;
                busyCore[hartid]                 = 1; 
		        // who is the owner ?
		        directoryStore.owner = (directoryStateHit.cacheStateDir == INVALID)
		                                  ? (int)hartid : (int)directoryStateHit.owner;
		           
		        directoryStore.cacheStateDir = (directoryStateHit.cacheStateDir == INVALID) 
		                                       ? EXCLUSIVE : SHARED;
		        // need to write a new directory status
		        wasStore              = true;
		        cacheModif            = false;
                directoryModified     = true;
                isCycleModified       = true;
                // Have we issued the instruction ? Not yet
                //printf("MISS WRITE/RD nothing todo (%d,%d) \n", (int)directoryStore.sharers[0], (int)directoryStore.sharers[1]);
              } else {
                cacheStateMissRequest = NB_CORES+1;
                //printf("%d MISS WRITE/RD must do @(%x,%x) (%d,%d) \n", cache, place_current, set, (int)directoryStore.sharers[0], (int)directoryStore.sharers[1]);
              }
            } else if (opType == MISS_WRITE || opType == MISS_READ) {
              // Two ways to get a releaseAck equal to true --> Miss Write (all invalidated) or Miss Read for all sharers
	          // None_DIR --> directory status is not in the Modified state (change thanks to a writeback)
	          if (cacheStateMissRequest == 1) {
	            directoryStore.sharers[hartid]   = 1;
	            //printf("[CacheL2] The miss can be released after a directory write...\n");
	            directoryStore.owner = (directoryStateHit.cacheStateDir == INVALID || opType == MISS_WRITE) ? 
                                       (int)hartid : (int)directoryStateHit.owner;
	            directoryStore.cacheStateDir = (directoryStateHit.cacheStateDir == INVALID && opType == MISS_READ)
	                                           ? EXCLUSIVE
	                                           : ((directoryStateHit.cacheStateDir == EXCLUSIVE ||
	                                           directoryStateHit.cacheStateDir == SHARED) && opType == MISS_READ)
	                                           ? SHARED : (directoryStateHit.cacheStateDir == MODIFIED && opType == MISS_READ) 
	                                           ? SHARED : MODIFIED;
                cacheModif            = false;
                wasStore              = true;
                directoryModified     = true;
                isCycleModified       = true;
                busyCore[hartid]      = 1;
                cacheStateMissRequest = 0;
                //printf("MISS WRITE/RD granted status(%d,%d) \n", (int)directoryStore.sharers[0], (int)directoryStore.sharers[1]);
              } else if (directoryStateHit.sharers[cacheStateMissRequest-2] == 1) {
                protocolMiss = 1;
		        // We can't send a directory request to a core that is already in a busy state
		        // core cacheStateMissRequest will never need core hartid to become available
		        bool need_writeback = directoryStateHit.cacheStateDir == MODIFIED;

		        directoryOpType interfaceRequest = (need_writeback && opType == MISS_READ)
		                                   ? REQ_DATATRANSFER_SHARED
		                                   : (need_writeback && opType == MISS_WRITE)
		                                   ? REQ_DATATRANSFER_INV
		                                   : (opType == MISS_WRITE) ? REQ_INV : NONE_DIR;
		        if(interfaceRequest != NONE_DIR){
		          // SEND Req to core cacheStateMissRequest ! Optimized !
		          instructionQueue newInstruction;
                  newInstruction.hartid           = cacheStateMissRequest-2;
                  newInstruction.cache            = directoryStateHit.cache;
                  newInstruction.interfaceRequest = interfaceRequest;
                  newInstruction.addr             = addr;
                  pushInstruction(newInstruction, fifoInstruction);
                  //fifoInstruction.push_back(newInstruction);
		          directoryStore.sharers[cacheStateMissRequest-2] = (interfaceRequest==REQ_DATATRANSFER_SHARED) ? 1 : 0;
		        }
		        //printf("// %d Special addr(%x) @(%x,%x) status(%d,%d) %x %d MISS Request \n", cache, addr, place_current, set, (int)directoryStore.sharers[0], (int)directoryStore.sharers[1], interfaceRequest, (cacheStateMissRequest-2) );
		        cacheStateMissRequest--;
              } else {
                 cacheStateMissRequest--;
	          }      
            } else if(opType == WRITEHIT && cacheStateMissRequest == 0){
              // Which cores need to be updated ?
              // two cores can't process the same directory at the same time : sequential processing
              // Some check could be done
              bool isExclusive = (directoryStateHit.cacheStateDir == EXCLUSIVE) && (directoryStateHit.sharers[hartid] == 1);
              placeStore       = place_current;
              setStore         = set;
	          transferDirectoryStruct(directoryStateHit, directoryStore);
              
              if(isExclusive){
                directoryStore.cacheStateDir = MODIFIED;
                cacheModif         = false;
                wasStore           = true;
                directoryModified  = true;
                isCycleModified    = true;
              } else { 
                cacheStateMissRequest = NB_CORES+1;
              }
            } else if (opType == WRITEHIT) {
              if(cacheStateMissRequest == 1){
                cacheModif            = false;
                wasStore              = true;
                directoryModified     = true;
                isCycleModified       = true;
                directoryStore.owner           = hartid;
                directoryStore.sharers[hartid] = 1;
                directoryStore.cacheStateDir   = MODIFIED;
                cacheStateMissRequest          = 0;
              } else if (cacheStateMissRequest-2 != int(hartid) && directoryStateHit.sharers[cacheStateMissRequest-2] == 1) {
                // SEND DATA_INV to core t !
                instructionQueue newInstruction;
                newInstruction.hartid           = cacheStateMissRequest-2;
                newInstruction.cache            = directoryStateHit.cache;
                newInstruction.interfaceRequest = REQ_INV;
                newInstruction.addr             = addr;
                pushInstruction(newInstruction, fifoInstruction);
                //fifoInstruction.push_back(newInstruction);
                directoryStore.sharers[cacheStateMissRequest-2] = 0;
                cacheStateMissRequest--;
              } else {
                cacheStateMissRequest--;
              }
            }
          } else {
            numberMiss++;
            cacheState            = STATE_CACHE_MISS;
            cacheStateMissRequest = NB_CORES;
            //printf("[CacheL2] Entering in a miss... place(%x) addr(%x)\n", place_current, addr);
          }
        } else {
          //printf("Miss %d\n", (unsigned int)cacheState);
          bool doMiss                                   = true;
          if (cacheState == STATE_CACHE_MISS) {
            newVal  = tag;
            setMiss = (age1 <= age2 && age1 <= age3 && age1 <= age4)
                          ? 0
                          : ((age2 <= age1 && age2 <= age3 && age2 <= age4)
                                 ? 1
                                 : ((age3 <= age2 && age3 <= age1 && age3 <= age4) ? 2 : 3));
            oldVal = (age1 <= age2 && age1 <= age3 && age1 <= age4)
                         ? val1
                         : ((age2 <= age1 && age2 <= age3 && age2 <= age4)
                                ? val2
                                : ((age3 <= age2 && age3 <= age1 && age3 <= age4) ? val3 : val4));
            isValid = (age1 <= age2 && age1 <= age3 && age1 <= age4)
                          ? valid1
                          : ((age2 <= age1 && age2 <= age3 && age2 <= age4)
                                 ? valid2
                                 : ((age3 <= age2 && age3 <= age1 && age3 <= age4) ? valid3 : valid4));             
            missDirectoryState = (age1 <= age2 && age1 <= age3 && age1 <= age4)
                                   ? directoryState1
                                     : ((age2 <= age1 && age2 <= age3 && age2 <= age4)
                                       ? directoryState2
                                         : ((age3 <= age2 && age3 <= age1 && age3 <= age4) ? directoryState3 : directoryState4));
                                         
            ac_int<32, false> cacheL1Address = (((int)oldVal.template slc<TAG_SIZE>(0)) << (LOG_LINE_SIZE + LOG_SET_SIZE)) |
                                               (((int)place_current) << LOG_LINE_SIZE);
            if (!isValid) {
              cacheState = STATE_CACHE_LAST_STORE - 1;
              doMiss = true;
              cacheStateMissRequest = 0;
              //printf("[CacheL2] No necessary writeback... Loading data from memory... age1,2,3,4 %d %d %d %d\n", age1, age2, age3, age4); 
            } else if (missDirectoryState.cacheStateDir != MODIFIED && missDirectoryState.cacheStateDir != INVALID){
              if (cacheStateMissRequest == 0){
                doMiss = true;
              } else {
                if (missDirectoryState.sharers[cacheStateMissRequest-1] == 1) {
                  // SEND DATA_INV to core t !
                  instructionQueue newInstruction;
                  newInstruction.hartid           = cacheStateMissRequest-1;
                  newInstruction.cache            = missDirectoryState.cache;
                  newInstruction.interfaceRequest = REQ_INV;
                  newInstruction.addr             = cacheL1Address;
                  pushInstruction(newInstruction, fifoInstruction);
                  //fifoInstruction.push_back(newInstruction);  
                }
                cacheStateMissRequest--;
                doMiss = false;
                //printf("[CacheL2] Necessary writeback... cache %d @L1(%x) set(%d) State of the cache %d age1,2,3,4 %d %d %d %d\n", missDirectoryState.cache, cacheL1Address, setMiss, missDirectoryState.cacheStateDir, age1, age2, age3, age4);
              }
            } else if (missDirectoryState.cacheStateDir == MODIFIED){
              if(cacheStateMissRequest == 0) {
                doMiss = true;
              } else {
                if (missDirectoryState.sharers[cacheStateMissRequest-1] == 1) {
                  // SEND DATA_INV to core t ! (only one core is a sharer)
                  instructionQueue newInstruction;
                  newInstruction.hartid           = cacheStateMissRequest-1;
                  newInstruction.cache            = missDirectoryState.cache;
                  newInstruction.interfaceRequest = REQ_DATATRANSFER_INV;
                  newInstruction.addr             = cacheL1Address;
                  pushInstruction(newInstruction, fifoInstruction);
                  //fifoInstruction.push_back(newInstruction);
                  //printf("Sending instruction to Core %d Cache %d \n", cacheStateMissRequest-1 , cache);
                } 
                cacheStateMissRequest--;
                doMiss = false;
                //printf("[CacheL2] (Modified) Necessary writeback... %d @L1(%x) set(%d) State of the cache %d age1,2,3,4 %d %d %d %d \n", missDirectoryState.cache, cacheL1Address, setMiss, missDirectoryState.cacheStateDir, age1, age2, age3, age4);
              }
            } else {
              doMiss = true;// Invalid --> wb
              cacheStateMissRequest = 0;
              //printf("[CacheL2] Necessary writeback... Invalid state age1,2,3,4 %d %d %d %d \n", age1, age2, age3, age4);
            }
            // printf("TAG is %x\n", oldVal.slc<TAG_SIZE>(0));
        }

          if(doMiss){
          
            ac_int<32, false> oldAddress = (((int)oldVal.template slc<TAG_SIZE>(0)) << (LOG_LINE_SIZE + LOG_SET_SIZE)) |
                                           (((int)place_current) << LOG_LINE_SIZE);
            // First we write back the four memory values in upper level

            if (cacheState >= STATE_CACHE_LAST_STORE) {
              // We store all values into next memory interface
              nextLevelAddr   = oldAddress + (((int)(cacheState - STATE_CACHE_LAST_STORE)) << LOG_INTERFACE_SIZE);
              nextLevelDataIn = oldVal.template slc<INTERFACE_SIZE * 8>((cacheState - STATE_CACHE_LAST_STORE) * INTERFACE_SIZE * 8 + TAG_SIZE);
              nextLevelOpType = STORE;

              //printf("[CacheL2] Writing back %x %x at %x\n", (unsigned int)nextLevelAddr,
              //                                     (unsigned int)nextLevelDataIn, (unsigned int)nextLevelAddr);

            } else if (cacheState >= STATE_CACHE_LAST_LOAD) {
              // Then we read values from next memory level
              if (cacheState != STATE_CACHE_FIRST_LOAD) {
                newVal.set_slc(((unsigned int)(cacheState - STATE_CACHE_LAST_LOAD)) * INTERFACE_SIZE * 8 + TAG_SIZE,
                               nextLevelDataOut); // at addr +1
                //printf("[CacheL2] Loading data from memory... %x %x\n", nextLevelDataOut, nextLevelAddr); 
              }

              if (cacheState != STATE_CACHE_LAST_LOAD) {
                // We initiate the load at the address determined by next cache
                // state
                nextLevelAddr = (((int)addr.slc<32 - LOG_LINE_SIZE>(LOG_LINE_SIZE)) << LOG_LINE_SIZE) +
                                ((cacheState - STATE_CACHE_LAST_LOAD - 1) << LOG_INTERFACE_SIZE);
                nextLevelOpType = LOAD;
              }
            }

            cacheState--;
          }
          
          if (cacheState == 1) {
            //printf("[CacheL2] End loading data from memory... \n");
            placeStore = place_current;
            setStore   = setMiss;
            valStore   = newVal;
            directoryStore.cacheStateDir = INVALID;
            directoryStore.owner   = 0;
            directoryStore.sharers = 0;
            directoryStore.coreToken = 0;
            directoryStore.cache   = cache;
            nextLevelOpType = NONE;
            cacheModif         = true;
            directoryModified  = true;
            isCycleModified    = true;
          }
        }
      }
    }
    
    /*
    
      Memory Interface
    
    */
    
    // no hartid because it is the LLC !
    this->nextLevel->process(nextLevelAddr, WORD, nextLevelOpType, 0, nextLevelDataIn, nextLevelDataOut,
                             nextLevelWaitOut);
    
    /*
    
      Multicore Bus Interface
    
    */
    
    // Send Instruction to core
    if (!waitingForCacheAnswer && fifoInstruction.valid == 0){
      bufferOut.interfaceAck     = 0;
      bufferOut.interfaceRequest = NONE_DIR;
    } else if (!waitingForCacheAnswer) {
      bufferOut.interfaceAck     = 1;
      bufferOut.interfaceRequest = fifoInstruction.interfaceRequest;
      bufferOut.hartid           = fifoInstruction.hartid;
      bufferOut.cache            = fifoInstruction.cache;
      bufferOut.addr             = fifoInstruction.addr;
      waitingForCacheAnswer      = true;
    }
        
}

#endif /* INCLUDE_CACHEMEMORYL2_H_ */
