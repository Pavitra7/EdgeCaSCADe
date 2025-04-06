#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <sys/stat.h>

#include <sys/time.h>
#include <unistd.h>

#include <basic_simulator.h>
#include <core.h>
#include <elfFile.h>
#include <riscvISA.h>
#include <time.h>
#define DEBUG 0
#define st 1
static int missCounter = 0;
BasicSimulator::BasicSimulator(std::string binaryFile,
                               std::vector<std::string> args,
                               std::string inFile, std::string outFile,
                               std::string tFile, std::string sFile,
                               bool benchmark, std::string dtbFile) {

  mem.resize(DRAM_SIZE, 0);

  for (unsigned i = 0; i < NB_CORES; i++) {
    // Init zero
    memset((char *)&core[i], 0, sizeof(Core));
    
    // Init memory interface
    interfaceIn[i][0].interfaceAck  = 0;
    interfaceIn[i][1].interfaceAck  = 0;
    interfaceOut[i][0].interfaceAck = 0;
    interfaceOut[i][1].interfaceAck = 0;
    interfaceOut[i][2].interfaceAck = 0;
    interfaceOut[i][3].interfaceAck = 0;
    
    interfaceIn[i][0].releaseAck    = 0;
    interfaceIn[i][1].releaseAck    = 0;
    interfaceOut[i][0].releaseAck   = 0;
    interfaceOut[i][1].releaseAck   = 0;
    interfaceOut[i][2].releaseAck   = 0;
    interfaceOut[i][3].releaseAck   = 0;
    
    interfaceOut[i][0].buffer   = 0;
    interfaceOut[i][1].buffer   = 0;
    interfaceOut[i][0].bufferIn = 0;
    interfaceOut[i][1].bufferIn = 0;
    
     // Init Core PC
    core[i].regFile[2] = STACK_INIT;
    core[i].csrUnit.mhartid = ac_int<4, false>(i);
    
#if !SMP
    // One core
    IncompleteMemory<4> * imInterface = new IncompleteMemory<4>(mem.data(), &reservationAMO[i]);
    IncompleteMemory<4> * dmInterface = new IncompleteMemory<4>(mem.data(), &reservationAMO[i]);

    core[i].im = new CacheMemory<4, 16, 64>(imInterface, false);
    core[i].dm = new CacheMemory<4, 16, 64>(dmInterface, false);
#else
    // Multi-core
#if MCVM
    // One core
    IncompleteMemory<4> * imInterface = new IncompleteMemory<4>(mem.data(), &reservationAMO[i]);
    core[i].im = new CacheMemory<4, 16, 64>(imInterface, false);
#else
    core[i].im = new CacheMemoryMulticoreInst<4, 16, 64>(new MulticoreMemory<4>(&interfaceIn[i][0], &interfaceOut[i][0], NULL), false);
#endif
    core[i].dm = new CacheMemoryMulticore<4, 16, 64>(new MulticoreMemory<4>(&interfaceIn[i][1], &interfaceOut[i][1], &interfaceOut[i][2]), false);
#endif

    clint.ir_timer[i] = 0;
    clint.ipi[i]      = 0;
    enable[i]         = 1;
    reservationAMO[i].valid = 0;
    reservationAMO[i].address = 0;
  }

#if SMP
  // Cache L2
  bufferInL2.interfaceAck  = 0;
  bufferOutL2.interfaceAck = 0;
  bufferInL2.rejection     = 0;
  // Special WB input
  bufferInL2.interfaceAckWB = 0;
  bufferInL2.releaseAckWB   = 0;
  
  IncompleteMemory<4> * l2Interface = new IncompleteMemory<4>(mem.data(), &reservationAMOL2);
  cacheL2 = new CacheMemoryL2<4, 16, 128>(l2Interface, false);
#endif

  openFiles(inFile, outFile, tFile, sFile);

  readElf(binaryFile.c_str());

  if(!dtbFile.empty())
    loadDTB(dtbFile);

  pushArgsOnStack(args);

  this->benchmark = benchmark;
}

FILE *fopenCheck(const char *fname, const char *mode) {
  FILE *fp = fopen(fname, mode);
  if (fp == NULL) {
    fprintf(stderr, "Error: cannot open file %s\n", fname);
    exit(-1);
  }
  return fp;
}

void openOrDefault(std::string fname, const char *mode, FILE *def,
                   FILE *&dest) {
  if (fname.empty())
    dest = def;
  else
    dest = fopenCheck(fname.c_str(), mode);
}

void BasicSimulator::openFiles(std::string inFile, std::string outFile,
                               std::string tFile, std::string sFile) {
  openOrDefault(inFile, "rb", stdin, inputFile);
  openOrDefault(outFile, "wb", stdout, outputFile);
  openOrDefault(tFile, "wb", stderr, traceFile);
  openOrDefault(sFile, "wb", NULL, signatureFile);
}

void BasicSimulator::readElf(const char *binaryFile) {
  heapAddress = 0;
  ElfFile elfFile(binaryFile);
  for (auto const &section : elfFile.sectionTable) {
    if (section.address != 0) {
      for (unsigned i = 0; i < section.size; i++) {
        setByte(section.address + i, elfFile.content[section.offset + i]);
      }

      // update the size of the heap
      if (section.name != ".text") {
        if (section.address + section.size > heapAddress)
          heapAddress = section.address + section.size;
      }
    }
  }

  for (unsigned i = 0; i < NB_CORES; i++) { // FIXME: not true for multi-core
    core[i].pc = find_by_name(elfFile.symbols, "_start").offset;
  }

  if (signatureFile != NULL) {
    begin_signature = find_by_name(elfFile.symbols, "begin_signature").offset;
    end_signature = find_by_name(elfFile.symbols, "end_signature").offset;
  }

  if (DEBUG) {
    printf("Elf Reading done.\n");
  }
}

void BasicSimulator::loadDTB(std::string dtbFile) {
  /*std::ifstream dtbStream(dtbFile, std::ifstream::in);
  const int begin = dtbStream.tellg();
  dtbStream.seekg (0, std::ifstream::end);
  const int end = dtbStream.tellg();
  const unsigned fsize = (end-begin);
  dtbStream.seekg (0, std::ifstream::beg);
  char *dtb = (char*)malloc(fsize);
  dtbStream.read(dtb, fsize);
  free(dtb);
  dtbStream.close();*/

  FILE *dtbStream = fopen(dtbFile.c_str(), "rb");
  fseek(dtbStream, 0L, SEEK_END);
  const unsigned int fsize = ftell(dtbStream);
  rewind(dtbStream);
  char *dtb = (char*)malloc(fsize);
  if(fread(dtb, 1, fsize, dtbStream) == 0) {
    free(dtb);
    fclose(dtbStream);
    return;
  }

  for(int i=0; i<fsize; i++) {
    setByte(DTB_BASE+i, dtb[i]);
    //printf("%p\n", dtb[i]);
  }

  free(dtb);
  fclose(dtbStream);

  if(DEBUG) {
    printf("The %d-byte-DTB has been stored.\n", fsize);
  }
}

void BasicSimulator::pushArgsOnStack(const std::vector<std::string> args) {
  unsigned int argc = args.size();

  mem[STACK_INIT >> 2] = argc;

  auto currentPlaceStrings = STACK_INIT + 4 + 4 * argc;
  for (unsigned oneArg = 0; oneArg < argc; oneArg++) {
    mem[(STACK_INIT + 4 * oneArg + 4) >> 2] = currentPlaceStrings;

    int oneCharIndex = 0;
    for (const auto c : args[oneArg]) {
      setByte(currentPlaceStrings + oneCharIndex, c);
      oneCharIndex++;
    }
    setByte(currentPlaceStrings + oneCharIndex, 0);
    currentPlaceStrings += oneCharIndex + 1;
  }
  if (DEBUG) {
    printf("Populate Data Memory done.\n");
  }
}

BasicSimulator::~BasicSimulator() {

  if (inputFile)
    fclose(inputFile);
  if (outputFile)
    fclose(outputFile);
  if (traceFile)
    fclose(traceFile);
  if (signatureFile)
    fclose(signatureFile);
}

void BasicSimulator::printCycle() {



if ((core[0].im)->cacheState == 10){
missCounter++;
//printf("Total cache misses are %d \n",missCounter);
//to stop
/*if(missCounter==1064)
{
printEnd();
exit(0);
}*/

/*if((core[0].im)->secret_counter >=6)
{
printEnd();
exit(0);
}*/
//printf("%ld",(core[0].im)->secret_counter);
}
  if (DEBUG && debugOn) {
  
    // hartid zero
    if (!core[0].stallSignals[0] && !core[0].stallIm && !core[0].stallDm) {
      printf("Hartid 0 --> Debug trace : %x [%x] ", (unsigned int)core[0].dctoEx.pc,
             (unsigned int)core[0].dctoEx.we);
      std::cout << printDecodedInstrRISCV(core[0].dctoEx.instruction);

      /*TEMPORARY
      std::cout << "(" << std::hex << core[0].csrUnit.mcause << " - " << mem[(CLINT_BASE+CLINT_MTIME_OFF)>>2] << " - " << mem[(CLINT_BASE+CLINT_MTIME_OFF+4)>>2] << " - " << mem[(CLINT_BASE+CLINT_MTIMECMP_OFF)>>2] << " - " << mem[(CLINT_BASE+CLINT_MTIMECMP_OFF+4)>>2] << ") ";
      // ::*/

      for (const auto &reg : core[0].regFile)
        printf("%x  ", (unsigned int)reg);
      std::cout << std::endl;
    }
    
    // Dual Core Debug : hartid one
    
    if (SMP && !core[1].stallSignals[1] && !core[1].stallIm && !core[1].stallDm) {
      printf("Hartid 1 --> Debug trace : %x [%x] ", (unsigned int)core[1].dctoEx.pc,
             (unsigned int)core[1].dctoEx.we);
      std::cout << printDecodedInstrRISCV(core[1].dctoEx.instruction);
      for (const auto &reg : core[1].regFile)
        printf("%x  ", (unsigned int)reg);
      std::cout << std::endl;
    }
  }
//  printf("final time is %x", clock());
}

void BasicSimulator::printEnd() {
  /*
  RISCV-COMPLIANCE Ending Routine to get the signature
  stored between begin_signature and end_signature.
  */
/*  printf("YOU ARE UNDER ATTACK");*/
  //printf("Total Cache misses are %d\n",missCounter);
 // printf("The secret section cache misses are %ld\n",core[0].im->secret_counter);
   printf("Ending time is %ld",clock());
  if (signatureFile != NULL) {
    const auto addr_offset = 4;
    const auto begin_offset =
        ((begin_signature) % 4); // correct address alignement

    if (DEBUG) {
      printf("BEGIN/END_SIGNATURE: %x/%x (%x)", begin_signature, end_signature,
             begin_offset);
    }

    // memory read
    for (auto wordNumber = begin_signature - begin_offset;
         wordNumber < end_signature - begin_offset; wordNumber += addr_offset) {
      fprintf(signatureFile, "%08x\n", (unsigned int)this->ldw(wordNumber, 0));
    }
  }
  /*
  RISCV Benchmark: Embench, Parsec,...
  */

  if (benchmark) {
    printf(" 1-Valid/0-Incorrect result : %x \n",
           (unsigned int)core[0].regFile[9]);
    printf(" (%lu) : speed <- Guard/Verify(%x) \n",
           (unsigned long)core[0].csrUnit.mhpmcounter4 -
               (unsigned long)core[0].csrUnit.mhpmcounter3,
           (unsigned int)core[0].csrUnit.mscratch);
  }
}

void BasicSimulator::setByte(unsigned addr, ac_int<8, true> value) {
  mem[addr >> 2].set_slc((addr % 4) * 8, value);
}

// Function for handling memory accesses
void BasicSimulator::stb(ac_int<32, false> addr, ac_int<4, false> hartid,
                         ac_int<8, true> value) {
  ac_int<32, false> wordRes = 0;
  bool stall = true;
  bool enable[1]            = {true};
  while (stall){
#if LINUXOS
    core[hartid].dm->nextLevel->process(addr, BYTE, STORE, hartid, value, wordRes, stall);
#else
    core[hartid].dm->process(addr, BYTE, STORE, hartid, value, wordRes, stall);
    doDirectory(this->mem, NB_CORES, enable, this->interfaceIn, this->interfaceOut, NULL);
#endif
  }
}

void BasicSimulator::sth(ac_int<32, false> addr, ac_int<4, false> hartid,
                         ac_int<16, true> value) {
  this->stb(addr + 1, hartid, value.slc<8>(8));
  this->stb(addr + 0, hartid, value.slc<8>(0));
}

void BasicSimulator::stw(ac_int<32, false> addr, ac_int<4, false> hartid,
                         ac_int<32, true> value) {
  this->stb(addr + 3, hartid, value.slc<8>(24));
  this->stb(addr + 2, hartid, value.slc<8>(16));
  this->stb(addr + 1, hartid, value.slc<8>(8));
  this->stb(addr + 0, hartid, value.slc<8>(0));
}

void BasicSimulator::std(ac_int<32, false> addr, ac_int<4, false> hartid,
                         ac_int<64, true> value) {
  this->stb(addr + 7, hartid, value.slc<8>(56));
  this->stb(addr + 6, hartid, value.slc<8>(48));
  this->stb(addr + 5, hartid, value.slc<8>(40));
  this->stb(addr + 4, hartid, value.slc<8>(32));
  this->stb(addr + 3, hartid, value.slc<8>(24));
  this->stb(addr + 2, hartid, value.slc<8>(16));
  this->stb(addr + 1, hartid, value.slc<8>(8));
  this->stb(addr + 0, hartid, value.slc<8>(0));
}

ac_int<8, true> BasicSimulator::ldb(ac_int<32, false> addr,
                                    ac_int<4, false> hartid) {
  ac_int<8, true> result;
  result = mem[addr >> 2].slc<8>(((int)addr.slc<2>(0)) << 3);
  ac_int<32, false> wordRes = 0;
  bool stall = true;
  bool enable[1]            = {true};
  while (stall){
#if LINUXOS
    core[hartid].dm->nextLevel->process(addr, BYTE_U, LOAD, 0, hartid, wordRes, stall);
#else
    core[hartid].dm->process(addr, BYTE_U, LOAD, 0, hartid, wordRes, stall);
    doDirectory(this->mem, NB_CORES, enable, this->interfaceIn, this->interfaceOut, NULL);
#endif
  }

  result = wordRes.slc<8>(0);
  return result;
}

// Little endian version
ac_int<16, true> BasicSimulator::ldh(ac_int<32, false> addr,
                                     ac_int<4, false> hartid) {
  ac_int<16, true> result = 0;
  result.set_slc(8, this->ldb(addr + 1, hartid));
  result.set_slc(0, this->ldb(addr, hartid));
  return result;
}

ac_int<32, true> BasicSimulator::ldw(ac_int<32, false> addr,
                                     ac_int<4, false> hartid) {
  ac_int<32, true> result = 0;
  result.set_slc(24, this->ldb(addr + 3, hartid));
  result.set_slc(16, this->ldb(addr + 2, hartid));
  result.set_slc(8, this->ldb(addr + 1, hartid));
  result.set_slc(0, this->ldb(addr, hartid));
  return result;
}

ac_int<32, true> BasicSimulator::ldd(ac_int<32, false> addr,
                                     ac_int<4, false> hartid) {
  ac_int<32, true> result = 0;
  result.set_slc(56, this->ldb(addr + 7, hartid));
  result.set_slc(48, this->ldb(addr + 6, hartid));
  result.set_slc(40, this->ldb(addr + 5, hartid));
  result.set_slc(32, this->ldb(addr + 4, hartid));
  result.set_slc(24, this->ldb(addr + 3, hartid));
  result.set_slc(16, this->ldb(addr + 2, hartid));
  result.set_slc(8, this->ldb(addr + 1, hartid));
  result.set_slc(0, this->ldb(addr, hartid));

  return result;
}

/********************************************************************************************************************
**  Software emulation of system calls.
**  This piece of software is useful for bare-metal programs.
** Currently all system calls are solved in the simulator. The function
*solveSyscall check the opcode in the
** extoMem pipeline registers and verifies whether it is a syscall or not. If it
*is, they solve the forwarding,
** and switch to the correct function according to reg[17].
*********************************************************************************************************************/
void BasicSimulator::solveSyscall() {
  if (!HAS_KERNEL_SYSCALL_HANDLER && (core[0].extoMem.opCode == RISCV_SYSTEM) &&
      core[0].extoMem.instruction.slc<12>(20) == 0 &&
      !core[0].stallSignals[2] && !core[0].stallIm && !core[0].stallDm) {

    ac_int<32, true> syscallId = core[0].regFile[17];
    ac_int<32, true> arg1 = core[0].regFile[10];
    ac_int<32, true> arg2 = core[0].regFile[11];
    ac_int<32, true> arg3 = core[0].regFile[12];
    ac_int<32, true> arg4 = core[0].regFile[13];

    if (core[0].memtoWB.useRd && core[0].memtoWB.we &&
        !core[0].stallSignals[3]) {
      if (core[0].memtoWB.rd == 10)
        arg1 = core[0].memtoWB.result;
      else if (core[0].memtoWB.rd == 11)
        arg2 = core[0].memtoWB.result;
      else if (core[0].memtoWB.rd == 12)
        arg3 = core[0].memtoWB.result;
      else if (core[0].memtoWB.rd == 13)
        arg4 = core[0].memtoWB.result;
      else if (core[0].memtoWB.rd == 17)
        syscallId = core[0].memtoWB.result;
    }

    ac_int<32, true> result = 0;

    switch (syscallId) {
    case SYS_exit:
      exitFlag = 1; // Currently we break on ECALL
      break;
    case SYS_read:
      result = this->doRead(arg1, arg2, arg3);
      break;
    case SYS_write:
      result = this->doWrite(arg1, arg2, arg3);
      break;
    case SYS_brk:
      result = this->doSbrk(arg1);
      break;
    case SYS_open:
      result = this->doOpen(arg1, arg2, arg3);
      break;
    case SYS_openat:
      result = this->doOpenat(arg1, arg2, arg3, arg4);
      break;
    case SYS_lseek:
      result = this->doLseek(arg1, arg2, arg3);
      break;
    case SYS_close:
      result = this->doClose(arg1);
      break;
    case SYS_fstat:
      result = this->doFstat(arg1, arg2);
      break;
    case SYS_stat:
      result = this->doStat(arg1, arg2);
      break;
    case SYS_gettimeofday:
      result = this->doGettimeofday(arg1);
      break;
    case SYS_unlink:
      result = this->doUnlink(arg1);
      break;
    case SYS_exit_group:
      fprintf(stderr, "Syscall : SYS_exit_group\n");
      exitFlag = 1;
      break;
    case SYS_getpid:
      fprintf(stderr, "Syscall : SYS_getpid\n");
      exitFlag = 1;
      break;
    case SYS_kill:
      fprintf(stderr, "Syscall : SYS_kill\n");
      exitFlag = 1;
      break;
    case SYS_link:
      fprintf(stderr, "Syscall : SYS_link\n");
      exitFlag = 1;
      break;
    case SYS_mkdir:
      fprintf(stderr, "Syscall : SYS_mkdir\n");
      exitFlag = 1;
      break;
    case SYS_chdir:
      fprintf(stderr, "Syscall : SYS_chdir\n");
      exitFlag = 1;
      break;
    case SYS_getcwd:
      fprintf(stderr, "Syscall : SYS_getcwd\n");
      exitFlag = 1;
      break;
    case SYS_lstat:
      fprintf(stderr, "Syscall : SYS_lstat\n");
      exitFlag = 1;
      break;
    case SYS_fstatat:
      fprintf(stderr, "Syscall : SYS_fstatat\n");
      exitFlag = 1;
      break;
    case SYS_access:
      fprintf(stderr, "Syscall : SYS_access\n");
      exitFlag = 1;
      break;
    case SYS_faccessat:
      fprintf(stderr, "Syscall : SYS_faccessat\n");
      exitFlag = 1;
      break;
    case SYS_pread:
      fprintf(stderr, "Syscall : SYS_pread\n");
      exitFlag = 1;
      break;
    case SYS_pwrite:
      fprintf(stderr, "Syscall : SYS_pwrite\n");
      exitFlag = 1;
      break;
    case SYS_uname:
      fprintf(stderr, "Syscall : SYS_uname\n");
      exitFlag = 1;
      break;
    case SYS_getuid:
      fprintf(stderr, "Syscall : SYS_getuid\n");
      exitFlag = 1;
      break;
    case SYS_geteuid:
      fprintf(stderr, "Syscall : SYS_geteuid\n");
      exitFlag = 1;
      break;
    case SYS_getgid:
      fprintf(stderr, "Syscall : SYS_getgid\n");
      exitFlag = 1;
      break;
    case SYS_getegid:
      fprintf(stderr, "Syscall : SYS_getegid\n");
      exitFlag = 1;
      break;
    case SYS_mmap:
      fprintf(stderr, "Syscall : SYS_mmap\n");
      exitFlag = 1;
      break;
    case SYS_munmap:
      fprintf(stderr, "Syscall : SYS_munmap\n");
      exitFlag = 1;
      break;
    case SYS_mremap:
      fprintf(stderr, "Syscall : SYS_mremap\n");
      exitFlag = 1;
      break;
    case SYS_time:
      fprintf(stderr, "Syscall : SYS_time\n");
      exitFlag = 1;
      break;
    case SYS_getmainvars:
      fprintf(stderr, "Syscall : SYS_getmainvars\n");
      exitFlag = 1;
      break;
    case SYS_rt_sigaction:
      fprintf(stderr, "Syscall : SYS_rt_sigaction\n");
      exitFlag = 1;
      break;
    case SYS_writev:
      fprintf(stderr, "Syscall : SYS_writev\n");
      exitFlag = 1;
      break;
    case SYS_times:
      fprintf(stderr, "Syscall : SYS_times\n");
      exitFlag = 1;
      break;
    case SYS_fcntl:
      fprintf(stderr, "Syscall : SYS_fcntl\n");
      exitFlag = 1;
      break;
    case SYS_getdents:
      fprintf(stderr, "Syscall : SYS_getdents\n");
      exitFlag = 1;
      break;
    case SYS_dup:
      fprintf(stderr, "Syscall : SYS_dup\n");
      exitFlag = 1;
      break;
    // Custom syscalls
    case SYS_threadstart:
      result = 0;
      break;
    case SYS_nbcore:
      result = 1;
      break;
    default:
      fprintf(
          stderr,
          "AT %x, Syscall : Unknown system call, %d (%x) with arguments :\n",
          (unsigned int)core[0].extoMem.pc, syscallId.to_int(),
          syscallId.to_int());
      fprintf(stderr, "%d (%x)\n%d (%x)\n%d (%x)\n%d (%x)\n", arg1.to_int(),
              arg1.to_int(), arg2.to_int(), arg2.to_int(), arg3.to_int(),
              arg3.to_int(), arg4.to_int(), arg4.to_int());
      exitFlag = 1;
      break;
    }

    // We write the result and forward
    core[0].memtoWB.result = result;
    core[0].memtoWB.rd = 10;
    core[0].memtoWB.useRd = 1;

    if (core[0].dctoEx.useRs1 && (core[0].dctoEx.rs1 == 10))
      core[0].dctoEx.lhs = result;
    if (core[0].dctoEx.useRs2 && (core[0].dctoEx.rs2 == 10))
      core[0].dctoEx.rhs = result;
    if (core[0].dctoEx.useRs3 && (core[0].dctoEx.rs3 == 10))
      core[0].dctoEx.datac = result;
  }
}

ac_int<32, true> BasicSimulator::doRead(ac_int<32, false> file,
                                        ac_int<32, false> bufferAddr,
                                        ac_int<32, false> size,
                                        ac_int<4, false> hartid) {
  char *localBuffer = new char[size.to_int()];
  ac_int<32, true> result;

  if (file == 0 && inputFile)
    result = read(inputFile->_fileno, localBuffer, size);
  else
    result = read(file, localBuffer, size);

  for (int i(0); i < result; i++) {
    this->stb(bufferAddr + i, hartid, localBuffer[i]);
  }

  delete[] localBuffer;
  return result;
}

ac_int<32, true> BasicSimulator::doWrite(ac_int<32, false> file,
                                         ac_int<32, false> bufferAddr,
                                         ac_int<32, false> size,
                                         ac_int<4, false> hartid) {
  char *localBuffer = new char[size.to_int()];

  for (int i = 0; i < size; i++)
    localBuffer[i] = this->ldb(bufferAddr + i, hartid);

  ac_int<32, true> result = 0;
  if (file == 1 && outputFile) // 3 is the first available file descriptor
  {
    fflush(stdout);
    result = write(outputFile->_fileno, localBuffer, size);
  } else {
    if (file == 1)
      fflush(stdout);
    else if (file == 2)
      fflush(stderr);
    result = write(file, localBuffer, size);
  }
  delete[] localBuffer;
  return result;
}

ac_int<32, true> BasicSimulator::doFstat(ac_int<32, false> file,
                                         ac_int<32, false> stataddr,
                                         ac_int<4, false> hartid) {
  ac_int<32, true> result = 0;
  struct stat filestat = {0};

  if (file != 1)
    result = fstat(file, &filestat);

  std(stataddr, hartid, filestat.st_dev);               // unsigned long long
  std(stataddr + 8, hartid, filestat.st_ino);           // unsigned long long
  stw(stataddr + 16, hartid, filestat.st_mode);         // unsigned int
  stw(stataddr + 20, hartid, filestat.st_nlink);        // unsigned int
  stw(stataddr + 24, hartid, filestat.st_uid);          // unsigned int
  stw(stataddr + 28, hartid, filestat.st_gid);          // unsigned int
  std(stataddr + 32, hartid, filestat.st_rdev);         // unsigned long long
  std(stataddr + 40, hartid, filestat.__pad0);          // unsigned long long
  std(stataddr + 48, hartid, filestat.st_size);         // long long
  stw(stataddr + 56, hartid, filestat.st_blksize);      // int
  stw(stataddr + 60, hartid, filestat.__pad0);          // int
  std(stataddr + 64, hartid, filestat.st_blocks);       // long long
  stw(stataddr + 72, hartid, filestat.st_atim.tv_sec);  // long
  stw(stataddr + 76, hartid, filestat.st_atim.tv_nsec); // long
  stw(stataddr + 80, hartid, filestat.st_mtim.tv_sec);  // long
  stw(stataddr + 84, hartid, filestat.st_mtim.tv_nsec); // long
  stw(stataddr + 88, hartid, filestat.st_ctim.tv_sec);  // long
  stw(stataddr + 92, hartid, filestat.st_ctim.tv_nsec); // long
  stw(stataddr + 96, hartid, filestat.__pad0);          // long
  stw(stataddr + 100, hartid, filestat.__pad0);         // long

  return result;
}

ac_int<32, true> BasicSimulator::doOpen(ac_int<32, false> path,
                                        ac_int<32, false> flags,
                                        ac_int<32, false> mode,
                                        ac_int<4, false> hartid) {
  int oneStringElement = this->ldb(path, hartid);
  int index = 0;
  while (oneStringElement != 0) {
    index++;
    oneStringElement = this->ldb(path + index, hartid);
  }

  int pathSize = index + 1;

  char *localPath = new char[pathSize + 1];
  for (int i = 0; i < pathSize; i++)
    localPath[i] = this->ldb(path + i, hartid);
  localPath[pathSize] = '\0';

  // convert riscv flags to unix flags
  int riscvflags = flags;
  std::string str;
  if (riscvflags & SYS_O_WRONLY)
    str += "WRONLY, ";
  else if (riscvflags & SYS_O_RDWR)
    str += "RDWR, ";
  else
    str += "RDONLY, ";
  int unixflags = riscvflags & 3; // O_RDONLY, O_WRITE, O_RDWR are the same
  riscvflags ^= unixflags;
  if (riscvflags & SYS_O_APPEND) {
    unixflags |= O_APPEND;
    riscvflags ^= SYS_O_APPEND;
    str += "APPEND, ";
  }
  if (riscvflags & SYS_O_CREAT) {
    unixflags |= O_CREAT;
    riscvflags ^= SYS_O_CREAT;
    str += "CREAT, ";
  }
  if (riscvflags & SYS_O_TRUNC) {
    unixflags |= O_TRUNC;
    riscvflags ^= SYS_O_TRUNC;
    str += "TRUNC, ";
  }
  if (riscvflags & SYS_O_EXCL) {
    unixflags |= O_EXCL;
    riscvflags ^= SYS_O_EXCL;
    str += "EXCL, ";
  }
  if (riscvflags & SYS_O_SYNC) {
    unixflags |= O_SYNC;
    riscvflags ^= SYS_O_SYNC;
    str += "SYNC, ";
  }
  if (riscvflags & SYS_O_NONBLOCK) {
    unixflags |= O_NONBLOCK;
    riscvflags ^= SYS_O_NONBLOCK;
    str += "NONBLOCK, ";
  }
  if (riscvflags & SYS_O_NOCTTY) {
    unixflags |= O_NOCTTY;
    riscvflags ^= SYS_O_NOCTTY;
    str += "NOCTTY";
  }
  int result = open(localPath, unixflags, mode.to_int());

  delete[] localPath;
  return result;
}

ac_int<32, true> BasicSimulator::doOpenat(ac_int<32, false> dir,
                                          ac_int<32, false> path,
                                          ac_int<32, false> flags,
                                          ac_int<32, false> mode) {
  fprintf(stderr, "Syscall : SYS_openat not implemented yet...\n");
  exit(-1);
}

ac_int<32, true> BasicSimulator::doClose(ac_int<32, false> file) {
  if (file > 2) // don't close simulator's stdin, stdout & stderr
  {
    return close(file);
  }

  return 0;
}

ac_int<32, true> BasicSimulator::doLseek(ac_int<32, false> file,
                                         ac_int<32, false> ptr,
                                         ac_int<32, false> dir) {
  int result = lseek(file, ptr, dir);
  return result;
}

ac_int<32, true> BasicSimulator::doStat(ac_int<32, false> filename,
                                        ac_int<32, false> stataddr,
                                        ac_int<4, false> hartid) {
  int oneStringElement = this->ldb(filename, hartid);
  int index = 0;
  while (oneStringElement != 0) {
    index++;
    oneStringElement = this->ldb(filename + index, hartid);
  }

  int pathSize = index + 1;

  char *localPath = new char[pathSize + 1];
  for (int i = 0; i < pathSize; i++)
    localPath[i] = this->ldb(filename + i, hartid);
  localPath[pathSize] = '\0';

  struct stat filestat;
  int result = stat(localPath, &filestat);

  std(stataddr, hartid, filestat.st_dev);               // unsigned long long
  std(stataddr + 8, hartid, filestat.st_ino);           // unsigned long long
  stw(stataddr + 16, hartid, filestat.st_mode);         // unsigned int
  stw(stataddr + 20, hartid, filestat.st_nlink);        // unsigned int
  stw(stataddr + 24, hartid, filestat.st_uid);          // unsigned int
  stw(stataddr + 28, hartid, filestat.st_gid);          // unsigned int
  std(stataddr + 32, hartid, filestat.st_rdev);         // unsigned long long
  std(stataddr + 40, hartid, filestat.__pad0);          // unsigned long long
  std(stataddr + 48, hartid, filestat.st_size);         // long long
  stw(stataddr + 56, hartid, filestat.st_blksize);      // int
  stw(stataddr + 60, hartid, filestat.__pad0);          // int
  std(stataddr + 64, hartid, filestat.st_blocks);       // long long
  stw(stataddr + 72, hartid, filestat.st_atim.tv_sec);  // long
  stw(stataddr + 76, hartid, filestat.st_atim.tv_nsec); // long
  stw(stataddr + 80, hartid, filestat.st_mtim.tv_sec);  // long
  stw(stataddr + 84, hartid, filestat.st_mtim.tv_nsec); // long
  stw(stataddr + 88, hartid, filestat.st_ctim.tv_sec);  // long
  stw(stataddr + 92, hartid, filestat.st_ctim.tv_nsec); // long
  stw(stataddr + 96, hartid, filestat.__pad0);          // long
  stw(stataddr + 100, hartid, filestat.__pad0);         // long

  delete[] localPath;
  return result;
}

ac_int<32, true> BasicSimulator::doSbrk(ac_int<32, false> value) {
  ac_int<32, true> result;
  if (value == 0) {
    result = heapAddress;
  } else {
    heapAddress = value;
    result = value;
  }

  return result;
}

ac_int<32, true> BasicSimulator::doGettimeofday(ac_int<32, false> timeValPtr,
                                                ac_int<4, false> hartid) {
  struct timeval oneTimeVal;
  int result = gettimeofday(&oneTimeVal, NULL);

  this->stw(timeValPtr, hartid, oneTimeVal.tv_sec);
  this->stw(timeValPtr + 4, hartid, oneTimeVal.tv_usec);

  return result;
}

ac_int<32, true> BasicSimulator::doUnlink(ac_int<32, false> path,
                                          ac_int<4, false> hartid) {
  int oneStringElement = this->ldb(path, hartid);
  int index = 0;
  while (oneStringElement != '\0') {
    index++;
    oneStringElement = this->ldb(path + index, hartid);
  }

  int pathSize = index + 1;

  char *localPath = new char[pathSize + 1];
  for (int i = 0; i < pathSize; i++)
    localPath[i] = this->ldb(path + i, hartid);
  localPath[pathSize] = '\0';

  int result = unlink(localPath);

  delete[] localPath;
  return result;
}
