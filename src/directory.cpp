#include <ac_int.h>
#include <core.h>

void memoryStoreLoad(ac_int<32, false>* data, ac_int<32, false> addr, memMask mask, memOpType opType,
                     ac_int<4 * 8, false> dataIn, ac_int<4 * 8, false>& dataOut, bool& waitOut)
{
  // no latency, wait is always set to false
  // if( (addr>>2) > DRAM_SIZE){exit(-1);}

  ac_int<8, true> signedByte;
  ac_int<16, true> signedHalf;
  ac_int<32, true> signedWord;

  switch (opType) {
    case STORE:
      switch (mask) {
        case BYTE:
        case BYTE_U:
          data[(addr >> 2) & MEMMASK].set_slc((((int)addr.slc<2>(0)) << 3), dataIn.template slc<8>(0));
          break;
        case HALF:
        case HALF_U:
          data[(addr >> 2) & MEMMASK].set_slc((addr[1] ? 16 : 0), dataIn.template slc<16>(0));
          break;
        case WORD:
          data[(addr >> 2) & MEMMASK].set_slc(0, dataIn.template slc<32>(0));
          break;
        case LONG:
          data[(addr >> 2) & MEMMASK].set_slc(0, dataIn.template slc<32>(0));
          break;
      }
      //printf("Store : %x @(%x)\n", data[0x40], (addr >> 2) & MEMMASK);
      break;
    case LOAD:
      switch (mask) {
        case BYTE:
          signedByte = data[(addr >> 2) & MEMMASK].template slc<8>((((int)addr.slc<2>(0)) << 3));
          signedWord = signedByte;
          dataOut.set_slc(0, signedWord);
          break;
        case HALF:
          signedHalf = data[(addr >> 2) & MEMMASK].template slc<16>((addr[1] ? 16 : 0));
          signedWord = signedHalf;
          dataOut.set_slc(0, signedWord);
          break;
        case WORD:
          dataOut = data[(addr >> 2) & MEMMASK].template slc<32>(0);
          break;
        case BYTE_U:
          dataOut = data[(addr >> 2) & MEMMASK].template slc<8>((((int)addr.slc<2>(0)) << 3)) & 0xff;
          break;
        case HALF_U:
          dataOut = data[(addr >> 2) & MEMMASK].template slc<16>((addr[1] ? 16 : 0)) & 0xffff;
          break;
        case LONG:
          dataOut = data[(addr >> 2) & MEMMASK].template slc<32>(0);
          break;
      }
  }
  waitOut = true;
}


void doSharedBus(const int nb_cores, cacheDirectoryInterfaceOut bufferOut[][3], cacheDirectoryInterfaceIn bufferIn[][2], cacheL2DirectoryInterfaceIn& bufferInL2) {
  
  static int tokencore  = 1;
  static int tokencache = 1;
  static bool wait   = false;
  
  // Temporary variable
  bool isInterfaceRequest = false;
  
  if(wait){
    wait                                         = !bufferInL2.releaseAck && !bufferInL2.rejection;
    bufferOut[tokencore][tokencache].releaseAck  = bufferInL2.releaseAck && !bufferInL2.rejection;
    bufferOut[tokencore][tokencache].bufferIn    = bufferInL2.dataLoad;
    bufferIn[tokencore][tokencache].protocolMiss = bufferInL2.protocolMiss;
    bufferInL2.interfaceAck                      = !bufferInL2.releaseAck && !bufferInL2.rejection;
    //printf("[RRB] Directory (core=%d,cache=%d) current instruction issued ? %d \n", tokencore, tokencache, wait);
  } else if(!wait){
    //printf("[RRB] Ready to get a new instruction... \n");
    /*
      Special Round Robin Algorithm
      
      Examples:
      
      (core,cache) --> priority order (next core + instruction over data)
      (0,0) --> (1,0) : (1, 1) : (0, 1) : (0, 0)
      (1,1) --> (0,0) : (0, 1) : (1, 0) : (1, 1) 
      
    */
    // releaseAck should stay for one cycle to avoid collision
    bufferOut[tokencore][tokencache].releaseAck = 0;
    
    // Store previous token
    int special_tokencache = tokencache;
    int previous_tokencore  = tokencore;
    int previous_tokencache = tokencache;
    // useful variable when processing for the same core
    int tokencore_selected  = tokencore;
    int tokencache_selected = tokencache;
    
    // We want to give each core the ability to move forward
    tokencore  = (tokencore + 1) % nb_cores; 
    // instruction caches have higher priorities
    tokencache = 0;
        
    while(tokencore != previous_tokencore || tokencache != previous_tokencache){
      // is the cache "tokencache" from core "tokencore" is requesting something ?
      if(bufferOut[tokencore][tokencache].interfaceAck && !isInterfaceRequest){
        tokencore_selected  = tokencore;
        tokencache_selected = tokencache;
        isInterfaceRequest  = true;
      }
      // put all release ack to false - stop the core until the directory answer
      bufferOut[tokencore][tokencache].releaseAck = false; 
      // instruction caches have higher priorities
      tokencache = (tokencache+1)%2;
      
      if(tokencache == 0 && tokencore!=previous_tokencore){
        // We want to give each core the ability move forward
        tokencore  = (tokencore + 1) % nb_cores;
        if(tokencore==previous_tokencore){
          tokencache = (previous_tokencache+1)%2;
        }
      }
    }
    
    if(bufferOut[tokencore][tokencache].interfaceAck && !isInterfaceRequest){
        tokencore_selected  = tokencore;
        tokencache_selected = tokencache;
        isInterfaceRequest  = true;
    }   
    
    if(isInterfaceRequest){
      //printf("[RRB] Selected Instruction %d %d %d \n", tokencore_selected, tokencache_selected, bufferOut[tokencore_selected][tokencache_selected].interfaceRequest);
      bufferInL2.interfaceRequest = bufferOut[tokencore_selected][tokencache_selected].interfaceRequest;
      bufferInL2.addr             = bufferOut[tokencore_selected][tokencache_selected].addr;
      bufferInL2.interfaceAck     = bufferOut[tokencore_selected][tokencache_selected].interfaceAck;
      bufferInL2.buffer           = bufferOut[tokencore_selected][tokencache_selected].buffer;
      bufferInL2.hartid           = ac_int<4, false>(tokencore_selected);
      bufferInL2.cache            = ac_int<1, false>(tokencache_selected);
      bufferOut[tokencore_selected][tokencache_selected].releaseAck = false;
      wait                        = true;
      tokencore                   = tokencore_selected;
      tokencache                  = tokencache_selected;
    } else {
      //printf("[RRB] No instructions in the pipeline...\n");
      bufferInL2.interfaceRequest = NONE;
      bufferInL2.interfaceAck     = 0;
    }
  }
}

void doDirectoryRequestAnswers(const int nb_cores, cacheDirectoryInterfaceOut bufferOut[][3], cacheDirectoryInterfaceIn bufferIn[][2], cacheL2DirectoryInterfaceIn& bufferInL2, cacheL2DirectoryInterfaceOut& bufferOutL2, bool& wait, int coreID, int cacheID){
  if(wait){
     // When a directory request is sent to a particular core
     // the core can answer by sending some instructions to the cache l2.
     // It is only the case when the cache L2 asked for a mandatory writeback !
     wait                                   = !bufferIn[coreID][cacheID].releaseAck;
     bufferIn[coreID][cacheID].interfaceAck = wait;
     
     bufferOutL2.releaseAck                 = bufferIn[coreID][cacheID].releaseAck;
     // During a request to a core (REQ_INV/ REQ_DATATRANSFER_X)
     // the core can answer back with some instructions (WB)
     if(bufferOut[coreID][2].interfaceAck) {
       //bufferInL2.interfaceAckWB              = bufferOut[coreID][3].interfaceAck;
       bufferOut[coreID][2].releaseAck        = bufferInL2.releaseAckWB;
       bufferInL2.interfaceAckWB              = !bufferInL2.releaseAckWB;
       bufferInL2.addrWB                      = bufferOut[coreID][2].addr;
       bufferInL2.bufferWB                    = bufferOut[coreID][2].buffer;
     }
     //if(!wait){printf("[DirectoryRequestBus] Instruction done. \n");}
     //if(bufferInL2.interfaceAckWB){printf("[DirectoryRequestBus] Writeback sent to the cache L2. %x \n", bufferInL2.bufferWB);}
  }
}

void doDirectoryRequestSend(const int nb_cores, cacheDirectoryInterfaceOut bufferOut[][3], cacheDirectoryInterfaceIn bufferIn[][2], cacheL2DirectoryInterfaceIn& bufferInL2, cacheL2DirectoryInterfaceOut& bufferOutL2, bool& wait, int& coreID, int& cacheID) {
    
  if (!wait && bufferOutL2.interfaceAck){
     //printf("[DirectoryRequestBus] Issuing an instruction...\n");
     coreID                                     = bufferOutL2.hartid;
     cacheID                                    = bufferOutL2.cache;
     bufferIn[coreID][cacheID].interfaceAck     = 1;
     bufferIn[coreID][cacheID].addr             = bufferOutL2.addr;
     bufferIn[coreID][cacheID].interfaceRequest = bufferOutL2.interfaceRequest;
     wait                                       = true;
     //printf("[DirectoryRequestBus] Asking Core %d Cache %d to %d \n", coreID, cacheID, bufferOutL2.interfaceRequest);
  }
}

void doDirectory(std::vector<ac_int<32, false> >& mem, const int nb_cores, bool enable[],
                 cacheDirectoryInterfaceIn bufferIn[][2], cacheDirectoryInterfaceOut bufferOut[][3],
                 reservationSet reservationAMO[])
{
  // Directory Architecture
  // Compressed 
  ac_int<32, false>* p_mem             = mem.data();
  static const int LINE_SIZE           = 16;
  static const int LOG_LINE_SIZE       = log2const<LINE_SIZE>::value;  // 4
  static const int DIRECTORY_ADDR_SIZE = (ADDR_WIDTH - LOG_LINE_SIZE); // 28
  static const size_t DIRECTORY_SIZE   = ((size_t)1 << DIRECTORY_ADDR_SIZE);
  static std::vector<directoryStruct> directory(DIRECTORY_SIZE);

  static ac_int<NB_CORES, false> busyCore        = 0;
  static int connexionMatrix[NB_CORES][NB_CORES] = {0};
  static int interProcConn[NB_CORES]             = {0};
  
  bool cacheWait = false;
  for (unsigned int i = 0; i < nb_cores; i++) { // i is the id of the core
  
    if (bufferOut[i][2].interfaceRequest == WRITEBACK && bufferOut[i][2].interfaceAck) {
          // cache L2 update
          ac_int<32, false> dataOut_temp = 0;
          //printf("%d dirBus WRITEBACK: %x %x\n", i, bufferOut[i][2].addr, bufferOut[i][2].buffer);
          memoryStoreLoad(p_mem, bufferOut[i][2].addr, WORD, STORE, bufferOut[i][2].buffer, dataOut_temp, cacheWait);
          bufferOut[i][2].releaseAck = 1;
    }
    for (unsigned int j = 0; j < 2; j++) {      // 0: IM ; 1: DM

      // concurrent reading !
      ac_int<DIRECTORY_ADDR_SIZE, false> addr_request = bufferOut[i][j].addr.slc<DIRECTORY_ADDR_SIZE>(LOG_LINE_SIZE);
      directoryStruct directoryState                  = directory[addr_request];

      if (bufferOut[i][j].releaseAck) // releaseAck should stay for one cycle to avoid collision
        bufferOut[i][j].releaseAck = 0;

      if (bufferIn[i][j].releaseAck)
        bufferIn[i][j].interfaceAck = 0;

      if (bufferOut[i][j].interfaceAck == 1) {

        bufferOut[i][j].releaseAck = 0;

        enable[i] = true;

        if (bufferOut[i][j].interfaceRequest == STORE) { // CLINT-UART
          ac_int<32, false> dataOut_temp = 0;
          memoryStoreLoad(p_mem, bufferOut[i][j].addr, LONG, bufferOut[i][j].interfaceRequest,
                          bufferOut[i][j].buffer, dataOut_temp, cacheWait);
          bufferOut[i][j].releaseAck = cacheWait;
          //printf("Verif Store : %x @(%x)\n", p_mem[0x40], (bufferOut[i][j].addr>>2) & MEMMASK);
          //printf("%d %d STORE: %x %x\n", i, j, (bufferOut[i][j].addr>>2), bufferOut[i][j].buffer);
        } else if (bufferOut[i][j].interfaceRequest == LOAD) { // CLINT-UART - Cache L2 data acquisition
          // cache L2 call
          ac_int<32, false> dataIn_temp = 0;
          bufferOut[i][j].bufferIn         = 0;
          memoryStoreLoad(p_mem, bufferOut[i][j].addr, LONG, bufferOut[i][j].interfaceRequest,
                          dataIn_temp, bufferOut[i][j].bufferIn , cacheWait);
          bufferOut[i][j].releaseAck    = cacheWait;
          //printf("%d %d ACQ/LOAD: %x %x\n", i, j, bufferOut[i][j].addr, bufferOut[i][j].bufferIn );
        } else if (bufferOut[i][j].interfaceRequest == AMO_SC) {
          if (reservationAMO[i].valid && reservationAMO[i].address == bufferOut[i][j].addr) {
            ac_int<32, false> dataOut_temp = 0;
            memoryStoreLoad(p_mem, bufferOut[i][j].addr, LONG, STORE, bufferOut[i][j].buffer,
                            dataOut_temp, cacheWait); // not optimized --> should know whether it is in cache or not !
            bufferOut[i][j].releaseAck     = cacheWait;
            if(cacheWait)
              reservationAMO[i].valid    = 0;
            bufferOut[i][j].bufferIn  = 0; // success !
          } else {
            bufferOut[i][j].bufferIn       = 1; // try again !
            bufferOut[i][j].releaseAck = 1;
            reservationAMO[i].valid    = 0;
          }
          // printf("END AMO_SC addr(%x) output(%x) data(%x)\n", bufferOut[i][j].addr, bufferOut[i][j].bufferIn ,
          // bufferOut[i][j].buffer);
        } else if (bufferOut[i][j].interfaceRequest == AMO_LR) {
          // printf("AMO_LR addr(%x) %x sharer(%d %d) %d %d \n", bufferOut[i][j].addr,
          // directory[addr_request].cacheStateDir, directoryState.sharers.slc<1>(0), directoryState.sharers.slc<1>(1),
          // directoryState.sharers.slc<1>(i), i);
          bool releaseAck = true;

          if (directoryState.coreToken == 0 || directoryState.coreToken[i] == 1) {
            // check for each core if it is a sharer
            for (int t = 0; t < nb_cores; t++) {
              // is it a sharer ?
              if (t != i && directoryState.sharers.slc<1>(t) == ac_int<1, false>(1)) {
                // is the sharer core busy ?
                if (busyCore[t] != 1) {

                  if (interProcConn[i] == 0) {
                    // core t is chosen !
                    interProcConn[i] = t + 1;
                  }
                  releaseAck = false;
                  // SEND DATA_INV to core t !
                  bufferIn[t][j].interfaceAck     = 1;
                  bufferIn[t][j].addr             = bufferOut[i][j].addr;
                  bufferIn[t][j].interfaceRequest = (interProcConn[i] == t + 1) ? REQ_DATATRANSFER_INV : REQ_INV;
                  busyCore[t]                     = 1;
                  connexionMatrix[i][t]           = 1;
                } else if (connexionMatrix[i][t]) {
                  // Wait the release Ack from core t !
                  directory[addr_request].sharers[t] = !bufferIn[t][j].releaseAck;
                  connexionMatrix[i][t]              = !bufferIn[t][j].releaseAck;
                  busyCore[t]                        = !bufferIn[t][j].releaseAck;
                  releaseAck                         = releaseAck && bufferIn[t][j].releaseAck;
                  ;
                } else {
                  releaseAck = false;
                }
              }
            }
            // when invalidation are completed you can release the ack.
            directory[addr_request].coreToken[i]  = !releaseAck;
            directory[addr_request].cacheStateDir = (directoryState.sharers.slc<1>(i) == ac_int<1, false>(1)) ? EXCLUSIVE : INVALID;

            if (releaseAck) {
              reservationAMO[i].valid       = 1;
              reservationAMO[i].address     = bufferOut[i][j].addr;
              ac_int<32, false> dataIn_temp = 0;
              bufferOut[i][j].bufferIn          = 0;
              memoryStoreLoad(p_mem, bufferOut[i][j].addr, WORD, LOAD, dataIn_temp, bufferOut[i][j].bufferIn , cacheWait);
              bufferOut[i][j].releaseAck    = cacheWait;
              // printf("END AMO_LR addr(%x) sharer(%d %d) %x data(%x)\n", bufferOut[i][j].addr,
              // directory[addr_request].sharers.slc<1>(0), directory[addr_request].sharers.slc<1>(1),
              // directory[addr_request].cacheStateDir, bufferOut[i][j].bufferIn);
            }
          }
        } else if (bufferOut[i][j].interfaceRequest == AMO_SWAPW) {
          // printf("AMO_SWAPW addr(%x) %x sharer(%d %d)\n", bufferOut[i][j].addr,
          // directory[addr_request].cacheStateDir, directory[addr_request].sharers.slc<1>(0),
          // directory[addr_request].sharers.slc<1>(1));
          bool releaseAck = true;

          if (directoryState.coreToken == 0 || directoryState.coreToken[i] == 1) {
            // check for each core if it is a sharer
            for (int t = 0; t < nb_cores; t++) {
              // is it a sharer ?
              if (t != i && directoryState.sharers[t] == 1) {
                // is the sharer core busy ?
                if (busyCore[t] != 1) {

                  if (interProcConn[i] == 0) {
                    // core t is chosen !
                    interProcConn[i] = t + 1;
                  }
                  releaseAck = false;
                  // SEND DATA_INV to core t !
                  bufferIn[t][j].interfaceAck     = 1;
                  bufferIn[t][j].addr             = bufferOut[i][j].addr;
                  bufferIn[t][j].interfaceRequest = (interProcConn[i] == t + 1) ? REQ_DATATRANSFER_INV : REQ_INV;
                  busyCore[t]                     = 1;
                  connexionMatrix[i][t]           = 1;
                } else if (connexionMatrix[i][t]) {
                  // Wait the release Ack from core t !
                  directory[addr_request].sharers[t] = !bufferIn[t][j].releaseAck;
                  connexionMatrix[i][t]              = !bufferIn[t][j].releaseAck;
                  busyCore[t]                        = !bufferIn[t][j].releaseAck;
                  releaseAck                         = releaseAck && bufferIn[t][j].releaseAck;
                  ;
                } else {
                  releaseAck = false;
                }
              }
            }
            // when invalidation are completed you can release the ack.
            directory[addr_request].coreToken[i] = !releaseAck;
            directory[addr_request].cacheStateDir =
                (directoryState.sharers.slc<1>(i) == ac_int<1, false>(1)) ? MODIFIED : INVALID;

            if (releaseAck) {
              ac_int<32, false> dataOut_temp = 0;
              ac_int<32, false> dataIn_temp  = 0;
              memoryStoreLoad(p_mem, bufferOut[i][j].addr, WORD, LOAD, dataIn_temp, bufferOut[i][j].bufferIn, cacheWait);
              memoryStoreLoad(p_mem, bufferOut[i][j].addr, WORD, STORE, bufferOut[i][j].buffer, dataOut_temp, cacheWait);
              bufferOut[i][j].releaseAck = cacheWait;
              // printf("END AMO_SWAPW addr(%x) %x sharer(%d %d) SWAP(%x (into reg) - %x (into mem))\n",
              // bufferOut[i][j].addr, directory[addr_request].cacheStateDir, directory[addr_request].sharers.slc<1>(0),
              // directory[addr_request].sharers.slc<1>(1), bufferOut[i][j].bufferIn , bufferOut[i][j].buffer);
            }
          }
        } else if (bufferOut[i][j].interfaceRequest == WRITEHIT) {
          //printf("Core %d Cache %d Update other caches addr(%x) sharer(%d %d)\n", i, j,  bufferOut[i][j].addr,
          //       (unsigned int)directoryState.sharers.slc<1>(0), (unsigned int)directoryState.sharers.slc<1>(1));
          // Which cores need to be updated ?
          // two cores can't process the same directory at the same time : sequential processing
          bool releaseAck = false;

          if (directoryState.coreToken == 0 || directoryState.coreToken[i] == 1) {
            releaseAck = true;
            // check for each core if it is a sharer
            for (int t = 0; t < nb_cores; t++) {
              // is it a sharer ?
              if (t != i && directoryState.sharers[t] == 1) {
                // is the sharer core busy ?
                if (busyCore[t] != 1) {
                  releaseAck = false;
                  // SEND DATA_INV to core t !
                  bufferIn[t][j].interfaceAck     = 1;
                  bufferIn[t][j].addr             = bufferOut[i][j].addr;
                  bufferIn[t][j].interfaceRequest = REQ_INV;
                  busyCore[t]                     = 1;
                  connexionMatrix[i][t]           = 1;
                } else if (connexionMatrix[i][t]) {
                  // Wait the release Ack from core t !
                  directory[addr_request].sharers[t] = !bufferIn[t][j].releaseAck;
                  connexionMatrix[i][t]              = !bufferIn[t][j].releaseAck;
                  busyCore[t]                        = !bufferIn[t][j].releaseAck;
                  releaseAck                         = releaseAck && bufferIn[t][j].releaseAck;
                } else {
                  //printf("busy core !\n");
                  releaseAck = false;
                }
              }
            }
            // when invalidation are completed you can release the ack.
            bufferOut[i][j].releaseAck            = releaseAck;
            directory[addr_request].coreToken[i]  = !releaseAck;
            directory[addr_request].owner         = i;
            directory[addr_request].sharers[i]    = 1;
            directory[addr_request].cacheStateDir = MODIFIED;
          }
          //printf("Core %d Cache %d Update other caches %d addr(%x) sharer(%d %d)\n", i, j, releaseAck, bufferOut[i][j].addr,
          //       (unsigned int)directory[addr_request].sharers.slc<1>(0), (unsigned int)directory[addr_request].sharers.slc<1>(1));
        } else if (bufferOut[i][j].interfaceRequest == MISS_WRITE || bufferOut[i][j].interfaceRequest == MISS_READ) {
          //printf("// Core %d Cache %d MISS Request %d is requested at addr(%x) state(%d) sharers(%d %d) owner(%d)\n", i,
          //j, (unsigned int) bufferOut[i][j].interfaceRequest, bufferOut[i][j].addr, (unsigned int)directory[addr_request].cacheStateDir,
          //directory[addr_request].sharers.slc<1>(0), directory[addr_request].sharers.slc<1>(1),  (unsigned
          //int)directory[addr_request].owner);
          // directoryState.sharers.slc<1>(0));
          // who have this cache block ?
          // invalidate all and transmit the data
          // coreDirectoryInterface.interProcessorCommunication = WAITSTATE;
          // coreDirectoryInterface.cacheToDirectoryAck = 0;
          // two cores can't process the same directory at the same time : sequential processing
          bool enter_miss = true;
          bool releaseAck = true;
          // does the directory is free to manage the miss ?
          // OR
          // does the directory current management is linked with core i ?
          if (directoryState.coreToken == 0 || directoryState.coreToken[i] == 1) {

            ac_int<1, false> protocolMiss = 0; // TODO: 0: mem call 1: cache proc --> EXCLUSIVE : SHARED

            // does a core t need the core i ?
            for (int t = 0; t < nb_cores; t++) {
              if (connexionMatrix[t][i]) {
                enter_miss = false;
              }
            }

            if (enter_miss) {

              directory[addr_request].coreToken[i] = 1;
              busyCore[i]                          = 1; // grant miss request authorization
              directory[addr_request].sharers[i]   = 1; // who is the owner ?

              bool need_writeback = directory[addr_request].cacheStateDir == MODIFIED;

              for (int t = 0; t < nb_cores; t++) {
                if (t != i && directoryState.sharers.slc<1>(t) == 1) {
                  protocolMiss = 1;
                  if (busyCore[t] != 1) {
                    // choose a sharer that will send the data
                    if (interProcConn[i] == 0) {
                      // core t is chosen !
                      interProcConn[i] = t + 1;
                    }
                    // SEND Req to core t ! Optimized !
                    bufferIn[t][j].addr = bufferOut[i][j].addr;
                    bufferIn[t][j].interfaceRequest =
                        (need_writeback && bufferOut[i][j].interfaceRequest == MISS_READ)
                            ? REQ_DATATRANSFER_SHARED
                            : (need_writeback && bufferOut[i][j].interfaceRequest == MISS_WRITE &&
                               interProcConn[i] == t + 1)
                                  ? REQ_DATATRANSFER_INV
                                  : (bufferOut[i][j].interfaceRequest == MISS_WRITE) ? REQ_INV : NONE_DIR;
                    bufferIn[t][j].interfaceAck = (bufferIn[t][j].interfaceRequest == NONE_DIR) ? 0 : 1;
                    busyCore[t]                 = (bufferIn[t][j].interfaceRequest == NONE_DIR) ? 0 : 1;
                    connexionMatrix[i][t]       = (bufferIn[t][j].interfaceRequest == NONE_DIR) ? 0 : 1;
                    releaseAck                  = releaseAck && !busyCore[t];
                    // printf("// Special %x %d MISS Request \n", bufferIn[t][j].interfaceRequest, t);
                  } else if (connexionMatrix[i][t]) {
                    // Wait the release Ack from core t !
                    directory[addr_request].sharers[t] =
                        (bufferOut[i][j].interfaceRequest == MISS_READ) ? 1 : !bufferIn[t][j].releaseAck;
                    connexionMatrix[i][t] = !bufferIn[t][j].releaseAck;
                    busyCore[t]           = !bufferIn[t][j].releaseAck;
                    releaseAck            = releaseAck && bufferIn[t][j].releaseAck;
                  } else {
                    releaseAck = false;
                  }
                }
              }
            }
            bufferOut[i][j].releaseAck  = releaseAck && enter_miss;
            bufferIn[i][j].protocolMiss = protocolMiss;
            if (releaseAck) {
              directory[addr_request].owner =
                  (directory[addr_request].cacheStateDir == INVALID || bufferOut[i][j].interfaceRequest == MISS_WRITE)
                      ? i
                      : (int)directory[addr_request].owner;
              // || (directory[addr_request].cacheStateDir == EXCLUSIVE && directoryState.sharers.slc<1>(i) == 1)
              directory[addr_request].cacheStateDir =
                  ((directory[addr_request].cacheStateDir == INVALID && bufferOut[i][j].interfaceRequest == MISS_READ))
                      ? EXCLUSIVE
                      : ((directory[addr_request].cacheStateDir == EXCLUSIVE ||
                          directory[addr_request].cacheStateDir == SHARED) &&
                         bufferOut[i][j].interfaceRequest == MISS_READ)
                            ? SHARED
                            : (directory[addr_request].cacheStateDir == MODIFIED &&
                               bufferOut[i][j].interfaceRequest == MISS_READ)
                                  ? SHARED
                                  : MODIFIED;
            }
          }
          //printf("// Core %d Cache %d MISS Request Answer : %d %d sharers(%d %d) owner(%d) \n", i, j, (unsigned
          //int)directory[addr_request].cacheStateDir, releaseAck, directory[addr_request].sharers.slc<1>(0),
          //directory[addr_request].sharers.slc<1>(1), (unsigned int)directory[addr_request].owner);

        } else if (bufferOut[i][j].interfaceRequest == WRITEBACK) {
          // cache L2 update
          ac_int<32, false> dataOut_temp = 0;
          //printf("%d %d WRITEBACK: %x %x\n", i, j, bufferOut[i][j].addr, bufferOut[i][j].buffer);
          memoryStoreLoad(p_mem, bufferOut[i][j].addr, WORD, STORE, bufferOut[i][j].buffer, dataOut_temp, cacheWait);
          bufferOut[i][j].releaseAck = cacheWait;
          // Does this writeback come from a miss eviction interfaceAck == 0 ? or a directory request ?
          // Optimized writeback because the ownership can be transfered. 
          bool getAOwner = false;
          if (!bufferIn[i][j].interfaceAck){// || (bufferIn[i][j].interfaceAck && bufferIn[i][j].interfaceRequest != REQ_DATATRANSFER_SHARED) ) {
            directory[addr_request].sharers[i] = 0;
            for (int k = 0; k < nb_cores; k++) {
              if (k != i && directoryState.sharers.slc<1>(k) == 1) {
                directory[addr_request].owner = k;
                getAOwner                     = true;
                break;
              }
            }
            if (!getAOwner) {
              directory[addr_request].cacheStateDir = INVALID;
            }
          }

        } else if (bufferOut[i][j].interfaceRequest == END_MISS) {
          //printf("END_MISS \n");
          enable[i]                            = 1;
          bufferOut[i][j].releaseAck           = 1;
          busyCore[i]                          = 0;
          directory[addr_request].coreToken[i] = 0;
          interProcConn[i]                     = 0;
          for (int t = 0; t < nb_cores; t++) {
            connexionMatrix[i][t] = 0;
          }
        }
        // Debug
        /*
        if(bufferOut[i][j].interfaceRequest == AMO_LR){
          printf("AMO_LR %x %x %x \n", bufferOut[i][j].bufferIn , reservationAMO[i].valid, reservationAMO[i].address);
        } else if(bufferOut[i][j].interfaceRequest == AMO_SC){
          printf("AMO_SC %x %x %x %x %x \n", bufferOut[i][j].bufferIn , bufferOut[i][j].buffer, reservationAMO[i].valid,
        reservationAMO[i].address);
        }
        bool addr_superior = false, addr_inferior = false;
        addr_superior = (bufferOut[i][j].addr >= CLINT_BASE);
        addr_inferior = (bufferOut[i][j].addr <= IO_BASE);
        if(bufferOut[i][j].interfaceRequest != NONE && addr_superior && addr_inferior){
          printf("** %d CLINT %x %x **\n", i, bufferOut[i][j].addr, bufferOut[i][j].buffer);
        }
        */
      } // interface ack ?
    }   // for j
  }     // for i
}
