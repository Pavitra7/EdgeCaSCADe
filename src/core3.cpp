
Skip to content
GitLab

    /

    Help

    C Comet
    Project information
    Repository
    Issues 0
    Merge requests 1
    CI/CD
    Deployments
    Monitor
    Analytics
    Wiki
    Snippets

    ROKICKI Simon
    Comet
    Repository

Add cycle counter to the core and more cache monitor work
PATUREL Joseph authored 20 hours ago
3975bdd2

    Comet
    src
    core.cpp

core.cpp
34.10 KiB

#include <ac_int.h>

#include <core.h>

void fetch(ac_int<32, false> pc, struct FtoDC& ftoDC, ac_int<32, false> instruction)

{

  ftoDC.instruction = instruction;

  ftoDC.pc          = pc;

  ftoDC.nextPCFetch = pc + 4;

  ftoDC.we          = 1;

}

void decode(struct FtoDC ftoDC, struct DCtoEx& dctoEx, ac_int<32, true> registerFile[32])

{

  ac_int<32, false> pc          = ftoDC.pc;

  ac_int<32, false> instruction = ftoDC.instruction;

  // R-type instruction

  ac_int<7, false> funct7 = instruction.slc<7>(25);

  ac_int<5, false> rs2    = instruction.slc<5>(20);

  ac_int<5, false> rs1    = instruction.slc<5>(15);

  ac_int<3, false> funct3 = instruction.slc<3>(12);

  ac_int<5, false> funct5 = instruction.slc<5>(27);

  ac_int<5, false> rd     = instruction.slc<5>(7);

  ac_int<7, false> opCode = instruction.slc<7>(0); // could be reduced to 5 bits because 1:0 is always 11

  // Privilege Instructions

  ac_int<12, false> funct12 = instruction.slc<12>(20);

  // Construction of different immediate values

  ac_int<12, false> imm12_S = 0;

  imm12_S.set_slc(5, instruction.slc<7>(25));

  imm12_S.set_slc(0, instruction.slc<5>(7));

  ac_int<12, true> imm12_I_signed = instruction.slc<12>(20);

  ac_int<12, true> imm12_S_signed = 0;

  imm12_S_signed.set_slc(0, imm12_S.slc<12>(0));

  ac_int<13, false> imm13 = 0;

  imm13[12]               = instruction[31];

  imm13.set_slc(5, instruction.slc<6>(25));

  imm13.set_slc(1, instruction.slc<4>(8));

  imm13[11] = instruction[7];

  ac_int<13, true> imm13_signed = 0;

  imm13_signed.set_slc(0, imm13);

  ac_int<32, true> imm31_12 = 0;

  imm31_12.set_slc(12, instruction.slc<20>(12));

  ac_int<21, false> imm21_1 = 0;

  imm21_1.set_slc(12, instruction.slc<8>(12));

  imm21_1[11] = instruction[20];

  imm21_1.set_slc(1, instruction.slc<10>(21));

  imm21_1[20] = instruction[31];

  ac_int<21, true> imm21_1_signed = 0;

  imm21_1_signed.set_slc(0, imm21_1);

  // Register access

  ac_int<32, false> valueReg1 = registerFile[rs1];

  ac_int<32, false> valueReg2 = registerFile[rs2];

  dctoEx.rs1         = rs1;

  dctoEx.rs2         = rs2;

  dctoEx.rs3         = rs2;

  dctoEx.rd          = rd;

  dctoEx.opCode      = opCode;

  dctoEx.funct3      = funct3;

  dctoEx.funct5      = funct5;

  dctoEx.funct7      = funct7;

  dctoEx.funct12     = funct12;

  dctoEx.instruction = instruction;

  dctoEx.pc          = pc;

  // Initialization of control bits

  dctoEx.useRs1   = 0;

  dctoEx.useRs2   = 0;

  dctoEx.useRd    = 0;

  dctoEx.we       = ftoDC.we;

  dctoEx.isBranch = 0;

  switch (opCode) {

    case RISCV_LUI:

      dctoEx.lhs    = imm31_12;

      dctoEx.useRs1 = 0;

      dctoEx.useRs2 = 0;

      dctoEx.useRs3 = 0;

      dctoEx.useRd  = 1;

      break;

    case RISCV_AUIPC:

      dctoEx.lhs    = ftoDC.pc;

      dctoEx.rhs    = imm31_12;

      dctoEx.useRs1 = 0;

      dctoEx.useRs2 = 0;

      dctoEx.useRs3 = 0;

      dctoEx.useRd  = 1;

      break;

    case RISCV_JAL:

      dctoEx.lhs      = ftoDC.pc + 4;

      dctoEx.rhs      = 0;

      dctoEx.nextPCDC = ftoDC.pc + imm21_1_signed;

      dctoEx.useRs1   = 0;

      dctoEx.useRs2   = 0;

      dctoEx.useRs3   = 0;

      dctoEx.useRd    = 1;

      dctoEx.isBranch = 1;

      break;

    case RISCV_JALR:

      dctoEx.lhs    = valueReg1;

      dctoEx.rhs    = imm12_I_signed;

      dctoEx.useRs1 = 1;

      dctoEx.useRs2 = 0;

      dctoEx.useRs3 = 0;

      dctoEx.useRd  = 1;

      break;

    case RISCV_BR:

      dctoEx.lhs    = valueReg1;

      dctoEx.rhs    = valueReg2;

      dctoEx.useRs1 = 1;

      dctoEx.useRs2 = 1;

      dctoEx.useRs3 = 0;

      dctoEx.useRd  = 0;

      break;

    case RISCV_LD:

      dctoEx.lhs    = valueReg1;

      dctoEx.rhs    = imm12_I_signed;

      dctoEx.useRs1 = 1;

      dctoEx.useRs2 = 0;

      dctoEx.useRs3 = 0;

      dctoEx.useRd  = 1;

      break;

    //******************************************************************************************

    // Treatment for: STORE INSTRUCTIONS

    case RISCV_ST:

      dctoEx.lhs    = valueReg1;

      dctoEx.rhs    = imm12_S_signed;

      dctoEx.datac  = valueReg2; // Value to store in memory

      dctoEx.useRs1 = 1;

      dctoEx.useRs2 = 0;

      dctoEx.useRs3 = 1;

      dctoEx.useRd  = 0;

      dctoEx.rd     = 0;

      break;

    case RISCV_OPI:

      dctoEx.lhs    = valueReg1;

      dctoEx.rhs    = imm12_I_signed;

      dctoEx.useRs1 = 1;

      dctoEx.useRs2 = 0;

      dctoEx.useRs3 = 0;

      dctoEx.useRd  = 1;

      break;

    case RISCV_OP:

      dctoEx.lhs    = valueReg1;

      dctoEx.rhs    = valueReg2;

      dctoEx.useRs1 = 1;

      dctoEx.useRs2 = 1;

      dctoEx.useRs3 = 0;

      dctoEx.useRd  = 1;

      break;

    case RISCV_SYSTEM:

      if (funct3 != 0) {

        // We are in a CSR

        dctoEx.useRs1 = 1;

        dctoEx.useRs2 = 0;

        dctoEx.useRs3 = 0;

        dctoEx.useRd  = 1;

        dctoEx.lhs    = valueReg1;

      }

      break;

    case RISCV_AMO:

      if (funct3 == 0x2) {

        switch (funct5) {

          case RISCV_AMO_LRW:

            dctoEx.lhs    = valueReg1;

            dctoEx.useRs1 = 1;

            dctoEx.useRs2 = 0;

            dctoEx.useRs3 = 0;

            dctoEx.useRd  = 1;

            break;

          case RISCV_AMO_SCW:

            dctoEx.lhs    = valueReg1;

            dctoEx.datac  = valueReg2; // value to store

            dctoEx.useRs1 = 1;

            dctoEx.useRs2 = 0;

            dctoEx.useRs3 = 1;

            dctoEx.useRd  = 1; // we must receive an answer 0 or 1

            break;

          case RISCV_AMO_SWAPW:

            dctoEx.lhs    = valueReg1;

            dctoEx.datac  = valueReg2; // value to store

            dctoEx.useRs1 = 1;

            dctoEx.useRs2 = 0;

            dctoEx.useRs3 = 1;

            dctoEx.useRd  = 1;

            break;

#if LINUXOS

          case RISCV_AMO_ADDW:

            dctoEx.lhs    = valueReg1;

            dctoEx.datac  = valueReg2;

            dctoEx.useRs1 = 1;

            dctoEx.useRs2 = 0;

            dctoEx.useRs3 = 1;

            dctoEx.useRd  = 1;

            break;

          case RISCV_AMO_ANDW:

            dctoEx.lhs    = valueReg1;

            dctoEx.datac  = valueReg2;

            dctoEx.useRs1 = 1;

            dctoEx.useRs2 = 0;

            dctoEx.useRs3 = 1;

            dctoEx.useRd  = 1;

            break;

          case RISCV_AMO_ORW:

            dctoEx.lhs    = valueReg1;

            dctoEx.datac  = valueReg2;

            dctoEx.useRs1 = 1;

            dctoEx.useRs2 = 0;

            dctoEx.useRs3 = 1;

            dctoEx.useRd  = 1;

            break;

          case RISCV_AMO_XORW:

            dctoEx.lhs    = valueReg1;

            dctoEx.datac  = valueReg2;

            dctoEx.useRs1 = 1;

            dctoEx.useRs2 = 0;

            dctoEx.useRs3 = 1;

            dctoEx.useRd  = 1;

            break;

#endif

          default:

            dctoEx.we = 0; // This AMO is not supported

            break;

        }

      } // else we are doing double words AMO (64 bits)

      break;

    default:

      dctoEx.we = 0; // invalid instruction for this current implementation

      break;

  }

  // If dest is zero, useRd should be at zero

  if (rd == 0) {

    dctoEx.useRd = 0;

  }

  // If the instruction was dropped, we ensure that isBranch is at zero

  if (!ftoDC.we) {

    dctoEx.isBranch = 0;

    dctoEx.useRd    = 0;

    dctoEx.useRs1   = 0;

    dctoEx.useRs2   = 0;

    dctoEx.useRs3   = 0;

  }

}

void execute(struct DCtoEx dctoEx, struct ExtoMem& extoMem)

{

  extoMem.pc                = dctoEx.pc;

  extoMem.opCode            = dctoEx.opCode;

  extoMem.rd                = dctoEx.rd;

  extoMem.funct3            = dctoEx.funct3;

  extoMem.funct5            = dctoEx.funct5;

  extoMem.we                = dctoEx.we;

  extoMem.isBranch          = 0;

  extoMem.useRd             = dctoEx.useRd;

  extoMem.isLongInstruction = 0;

  extoMem.instruction       = dctoEx.instruction;

  ac_int<13, false> imm13 = 0;

  imm13[12]               = dctoEx.instruction[31];

  imm13.set_slc(5, dctoEx.instruction.slc<6>(25));

  imm13.set_slc(1, dctoEx.instruction.slc<4>(8));

  imm13[11] = dctoEx.instruction[7];

  ac_int<13, true> imm13_signed = 0;

  imm13_signed.set_slc(0, imm13);

  ac_int<5, false> shamt = dctoEx.instruction.slc<5>(20);

  // switch must be in the else, otherwise external op may trigger default

  // case

  switch (dctoEx.opCode) {

    case RISCV_LUI:

      extoMem.result = dctoEx.lhs;

      break;

    case RISCV_AUIPC:

      extoMem.result = dctoEx.lhs + dctoEx.rhs;

      break;

    case RISCV_JAL:

      // Note: in current version, the addition is made in the decode stage

      // The value to store in rd (pc+4) is stored in lhs

      extoMem.result = dctoEx.lhs;

      break;

    case RISCV_JALR:

      // Note: in current version, the addition is made in the decode stage

      // The value to store in rd (pc+4) is stored in lhs

      extoMem.nextPC   = dctoEx.rhs + dctoEx.lhs;

      extoMem.isBranch = 1;

      extoMem.result = dctoEx.pc + 4;

      break;

    case RISCV_BR:

      extoMem.nextPC = extoMem.pc + imm13_signed;

      switch (dctoEx.funct3) {

        case RISCV_BR_BEQ:

          extoMem.isBranch = (dctoEx.lhs == dctoEx.rhs);

          break;

        case RISCV_BR_BNE:

          extoMem.isBranch = (dctoEx.lhs != dctoEx.rhs);

          break;

        case RISCV_BR_BLT:

          extoMem.isBranch = (dctoEx.lhs < dctoEx.rhs);

          break;

        case RISCV_BR_BGE:

          extoMem.isBranch = (dctoEx.lhs >= dctoEx.rhs);

          break;

        case RISCV_BR_BLTU:

          extoMem.isBranch = ((ac_int<32, false>)dctoEx.lhs < (ac_int<32, false>)dctoEx.rhs);

          break;

        case RISCV_BR_BGEU:

          extoMem.isBranch = ((ac_int<32, false>)dctoEx.lhs >= (ac_int<32, false>)dctoEx.rhs);

          break;

      }

      break;

    case RISCV_LD:

      extoMem.isLongInstruction = 1;

      extoMem.result            = dctoEx.lhs + dctoEx.rhs;

      break;

    case RISCV_ST:

      extoMem.datac  = dctoEx.datac;

      extoMem.result = dctoEx.lhs + dctoEx.rhs;

      break;

    case RISCV_OPI:

      switch (dctoEx.funct3) {

        case RISCV_OPI_ADDI:

          extoMem.result = dctoEx.lhs + dctoEx.rhs;

          break;

        case RISCV_OPI_SLTI:

          extoMem.result = dctoEx.lhs < dctoEx.rhs;

          break;

        case RISCV_OPI_SLTIU:

          extoMem.result = (ac_int<32, false>)dctoEx.lhs < (ac_int<32, false>)dctoEx.rhs;

          break;

        case RISCV_OPI_XORI:

          extoMem.result = dctoEx.lhs ^ dctoEx.rhs;

          break;

        case RISCV_OPI_ORI:

          extoMem.result = dctoEx.lhs | dctoEx.rhs;

          break;

        case RISCV_OPI_ANDI:

          extoMem.result = dctoEx.lhs & dctoEx.rhs;

          break;

        case RISCV_OPI_SLLI: // cast rhs as 5 bits, otherwise generated hardware

                             // is 32 bits

          // & shift amount held in the lower 5 bits (riscv spec)

          extoMem.result = dctoEx.lhs << (ac_int<5, false>)dctoEx.rhs;

          break;

        case RISCV_OPI_SRI:

          if (dctoEx.funct7.slc<1>(5)) // SRAI

            extoMem.result = dctoEx.lhs >> shamt;

          else // SRLI

            extoMem.result = (ac_int<32, false>)dctoEx.lhs >> shamt;

          break;

      }

      break;

    case RISCV_OP:

      if (dctoEx.funct7.slc<1>(0)) // M Extension

      {

      } else {

        switch (dctoEx.funct3) {

          case RISCV_OP_ADD:

            if (dctoEx.funct7.slc<1>(5)) // SUB

              extoMem.result = dctoEx.lhs - dctoEx.rhs;

            else // ADD

              extoMem.result = dctoEx.lhs + dctoEx.rhs;

            break;

          case RISCV_OP_SLL:

            extoMem.result = dctoEx.lhs << (ac_int<5, false>)dctoEx.rhs;

            break;

          case RISCV_OP_SLT:

            extoMem.result = dctoEx.lhs < dctoEx.rhs;

            break;

          case RISCV_OP_SLTU:

            extoMem.result = (ac_int<32, false>)dctoEx.lhs < (ac_int<32, false>)dctoEx.rhs;

            break;

          case RISCV_OP_XOR:

            extoMem.result = dctoEx.lhs ^ dctoEx.rhs;

            break;

          case RISCV_OP_SR:

            if (dctoEx.funct7.slc<1>(5)) // SRA

              extoMem.result = dctoEx.lhs >> (ac_int<5, false>)dctoEx.rhs;

            else // SRL

              extoMem.result = (ac_int<32, false>)dctoEx.lhs >> (ac_int<5, false>)dctoEx.rhs;

            break;

          case RISCV_OP_OR:

            extoMem.result = dctoEx.lhs | dctoEx.rhs;

            break;

          case RISCV_OP_AND:

            extoMem.result = dctoEx.lhs & dctoEx.rhs;

            break;

        }

      }

      break;

    case RISCV_MISC_MEM: // this does nothing because all memory accesses are

                         // ordered and we have only one core - FENCE

      break;

    case RISCV_AMO:

      extoMem.isLongInstruction = 1;

      extoMem.result            = dctoEx.lhs;

      switch (dctoEx.funct5) {

        case RISCV_AMO_LRW:

          break;

        case RISCV_AMO_SCW:

          extoMem.datac = dctoEx.datac;

          break;

        case RISCV_AMO_SWAPW:

          extoMem.datac = dctoEx.datac;

          break;

#if LINUXOS

        case RISCV_AMO_ADDW:

          extoMem.datac = dctoEx.datac;

          break;

        case RISCV_AMO_ANDW:

          extoMem.datac = dctoEx.datac;

          break;

        case RISCV_AMO_ORW:

          extoMem.datac = dctoEx.datac;

          break;

        case RISCV_AMO_XORW:

          extoMem.datac = dctoEx.datac;

          break;

#endif

      }

      break;

  }

  // If the instruction was dropped, we ensure that isBranch is at zero

  if (!dctoEx.we) {

    extoMem.isBranch = 0;

    extoMem.useRd    = 0;

  }

}

void memory(struct ExtoMem extoMem, struct MemtoWB& memtoWB)

{

  memtoWB.we     = extoMem.we;

  memtoWB.useRd  = extoMem.useRd;

  memtoWB.result = extoMem.result;

  memtoWB.rd     = extoMem.rd;

  memtoWB.opType = NONE;

  

  ac_int<32, false> mem_read;

  switch (extoMem.opCode) {

    case RISCV_LD:

      memtoWB.rd      = extoMem.rd;

      memtoWB.address = extoMem.result;

      memtoWB.opType  = LOAD; // isLoad

      //    formatread(extoMem.result, datasize, signenable, mem_read); //TODO

      break;

    case RISCV_ST:

      //        mem_read = dataMemory[extoMem.result >> 2];

      // if(extoMem.we) //TODO0: We do not handle non 32bit writes

      //        	dataMemory[extoMem.result >> 2] = extoMem.datac;

      memtoWB.opType       = STORE; // isStore

      memtoWB.address      = extoMem.result;

      memtoWB.valueToWrite = extoMem.datac;

      memtoWB.byteEnable   = 0xf;

      break;

    case RISCV_AMO:

      switch (extoMem.funct5) {

        case RISCV_AMO_LRW:

          memtoWB.rd      = extoMem.rd;

          memtoWB.address = extoMem.result;

          memtoWB.opType  = AMO_LR; // AMO_LR

          break;

        case RISCV_AMO_SCW:

          memtoWB.opType       = AMO_SC; // AMO_LC

          memtoWB.address      = extoMem.result;

          memtoWB.valueToWrite = extoMem.datac;

          memtoWB.byteEnable   = 0xf;

          break;

        case RISCV_AMO_SWAPW:

          memtoWB.opType       = AMO_SWAPW; // AMO_SWAPW

          memtoWB.address      = extoMem.result;

          memtoWB.valueToWrite = extoMem.datac;

          memtoWB.byteEnable   = 0xf;

          break;

#if LINUXOS

        case RISCV_AMO_ADDW:

          memtoWB.opType       = AMO_ADDW; // AMO_ADDW

          memtoWB.address      = extoMem.result;

          memtoWB.valueToWrite = extoMem.datac;

          memtoWB.byteEnable   = 0xf;

          break;

        case RISCV_AMO_ANDW:

          memtoWB.opType       = AMO_ANDW; // AMO_ANDW

          memtoWB.address      = extoMem.result;

          memtoWB.valueToWrite = extoMem.datac;

          memtoWB.byteEnable   = 0xf;

          break;

        case RISCV_AMO_ORW:

          memtoWB.opType       = AMO_ORW; // AMO_ORW

          memtoWB.address      = extoMem.result;

          memtoWB.valueToWrite = extoMem.datac;

          memtoWB.byteEnable   = 0xf;

          break;

        case RISCV_AMO_XORW:

          memtoWB.opType       = AMO_XORW; // AMO_XORW

          memtoWB.address      = extoMem.result;

          memtoWB.valueToWrite = extoMem.datac;

          memtoWB.byteEnable   = 0xf;

          break;

#endif

      }

      break;

  }

}

void writeback(struct MemtoWB memtoWB, struct WBOut& wbOut)

{

  wbOut.we = memtoWB.we;

  if ((memtoWB.rd != 0) && (memtoWB.we) && memtoWB.useRd) {

    wbOut.rd    = memtoWB.rd;

    wbOut.value = memtoWB.result;

    wbOut.useRd = 1;

  }

}

void branchUnit(ac_int<32, false> nextPC_fetch, ac_int<32, false> nextPC_decode, bool isBranch_decode,

                ac_int<32, false> nextPC_execute, bool isBranch_execute, ac_int<32, false>& pc, bool& we_fetch,

                bool& we_decode, bool stall_fetch)

{

  if (!stall_fetch) {

    if (isBranch_execute) {

      we_fetch  = 0;

      we_decode = 0;

      pc        = nextPC_execute;

    } else if (isBranch_decode) {

      we_fetch = 0;

      pc       = nextPC_decode;

    } else {

      pc = nextPC_fetch;

    }

  }

}

void forwardUnit(ac_int<5, false> decodeRs1, bool decodeUseRs1, ac_int<5, false> decodeRs2, bool decodeUseRs2,

                 ac_int<5, false> decodeRs3, bool decodeUseRs3,

                 ac_int<5, false> executeRd, bool executeUseRd, bool executeIsLongComputation,

                 ac_int<5, false> memoryRd, bool memoryUseRd,

                 ac_int<5, false> writebackRd, bool writebackUseRd,

                 bool stall[5], struct ForwardReg& forwardRegisters)

{

  if (decodeUseRs1) {

    if (executeUseRd && decodeRs1 == executeRd) {

      if (executeIsLongComputation) {

        stall[0] = 1;

        stall[1] = 1;

      } else {

        forwardRegisters.forwardExtoVal1 = 1;

      }

    } else if (memoryUseRd && decodeRs1 == memoryRd) {

      forwardRegisters.forwardMemtoVal1 = 1;

    } else if (writebackUseRd && decodeRs1 == writebackRd) {

      forwardRegisters.forwardWBtoVal1 = 1;

    }

  }

  if (decodeUseRs2) {

    if (executeUseRd && decodeRs2 == executeRd) {

      if (executeIsLongComputation) {

        stall[0] = 1;

        stall[1] = 1;

      } else {

        forwardRegisters.forwardExtoVal2 = 1;

      }

    } else if (memoryUseRd && decodeRs2 == memoryRd)

      forwardRegisters.forwardMemtoVal2 = 1;

    else if (writebackUseRd && decodeRs2 == writebackRd)

      forwardRegisters.forwardWBtoVal2 = 1;

  }

  if (decodeUseRs3) {

    if (executeUseRd && decodeRs3 == executeRd) {

      if (executeIsLongComputation) {

        stall[0] = 1;

        stall[1] = 1;

      } else {

        forwardRegisters.forwardExtoVal3 = 1;

      }

    } else if (memoryUseRd && decodeRs3 == memoryRd)

      forwardRegisters.forwardMemtoVal3 = 1;

    else if (writebackUseRd && decodeRs3 == writebackRd)

      forwardRegisters.forwardWBtoVal3 = 1;

  }

}

void doCycle(struct Core& core, // Core containing all values

             bool globalStall, bool ir_timer, bool ipi)

{

  // printf("PC : %x\n", core.pc);

  bool localStall = globalStall;

  core.stallSignals[0] = 0;

  core.stallSignals[1] = 0;

  core.stallSignals[2] = 0;

  core.stallSignals[3] = 0;

  core.stallSignals[4] = 0;

  core.stallIm         = false;

  core.stallDm         = false;

  core.stallMultAlu    = false;

  // declare temporary structs

  struct FtoDC ftoDC_temp;

  ftoDC_temp.pc          = 0;

  ftoDC_temp.instruction = 0;

  ftoDC_temp.nextPCFetch = 0;

  ftoDC_temp.we          = 0;

  struct DCtoEx dctoEx_temp;

  dctoEx_temp.isBranch = 0;

  dctoEx_temp.rs1      = 0;

  dctoEx_temp.useRs1   = 0;

  dctoEx_temp.rs2      = 0;

  dctoEx_temp.useRs2   = 0;

  dctoEx_temp.rs3      = 0;

  dctoEx_temp.useRs3   = 0;

  dctoEx_temp.useRd    = 0;

  dctoEx_temp.we       = 0;

  dctoEx_temp.funct12  = 0;

  dctoEx_temp.funct7   = 0;

  dctoEx_temp.funct5   = 0;

  dctoEx_temp.funct3   = 0;

  dctoEx_temp.lhs      = 0;

  dctoEx_temp.rhs      = 0;

  dctoEx_temp.datac    = 0;

  dctoEx_temp.nextPCDC = 0;

  dctoEx_temp.we       = 0;

  struct ExtoMem extoMem_temp;

  extoMem_temp.useRd             = 0;

  extoMem_temp.isBranch          = 0;

  extoMem_temp.we                = 0;

  extoMem_temp.pc                = 0;

  extoMem_temp.instruction       = 0;

  extoMem_temp.result            = 0;

  extoMem_temp.rd                = 0;

  extoMem_temp.isLongInstruction = 0;

  extoMem_temp.opCode            = 0;

  extoMem_temp.funct3            = 0;

  extoMem_temp.funct5            = 0;

  extoMem_temp.datac             = 0;

  extoMem_temp.nextPC            = 0;

  struct MemtoWB memtoWB_temp;

  memtoWB_temp.useRd        = 0;

  memtoWB_temp.we           = 0;

  memtoWB_temp.opType       = NONE;

  memtoWB_temp.result       = 0;

  memtoWB_temp.rd           = 0;

  memtoWB_temp.address      = 0;

  memtoWB_temp.valueToWrite = 0;

  memtoWB_temp.byteEnable   = 0;

  struct WBOut wbOut_temp;

  wbOut_temp.useRd = 0;

  wbOut_temp.we    = 0;

  wbOut_temp.rd    = 0;

  struct ForwardReg forwardRegisters;

  forwardRegisters.forwardExtoVal1  = 0;

  forwardRegisters.forwardExtoVal2  = 0;

  forwardRegisters.forwardExtoVal3  = 0;

  forwardRegisters.forwardMemtoVal1 = 0;

  forwardRegisters.forwardMemtoVal2 = 0;

  forwardRegisters.forwardMemtoVal3 = 0;

  forwardRegisters.forwardWBtoVal1  = 0;

  forwardRegisters.forwardWBtoVal2  = 0;

  forwardRegisters.forwardWBtoVal3  = 0;

  // declare temporary register fileut

  ac_int<32, false> nextInst, csrResult = 0, multResult = 0;

  

#if LINUXOS || MCVM

  //printf(" IM (%d)\n", (unsigned int) core.csrUnit.mhartid.slc<4>(0));

  core.im->nextLevel->process(core.pc, WORD, (!localStall) ? LOAD : NONE, core.csrUnit.mhartid.slc<4>(0), 0, nextInst,

                   core.stallIm);

#else

  core.im->process(core.pc, WORD, (!localStall) ? LOAD : NONE, core.csrUnit.mhartid.slc<4>(0), 0, nextInst,

                   core.stallIm);

#endif

  fetch(core.pc, ftoDC_temp, nextInst);

  decode(core.ftoDC, dctoEx_temp, core.regFile);

  /* EXECUTION STAGE */

  // ALU

  execute(core.dctoEx, extoMem_temp);

  // MULT

  // idempotent by construction

  bool multUsed = core.multiplicationUnit.process(core.dctoEx, multResult, core.stallMultAlu);

  core.stallSignals[STALL_EXECUTE] = core.stallMultAlu; // do not transmit the result - not valid yet !

  core.stallSignals[STALL_DECODE]  = core.stallMultAlu; // block the propagation of future instructions

  core.stallSignals[STALL_FETCH]   = core.stallMultAlu; // block the propagation of future instructions

  // CSR UNIT

  bool setCSRPC = false;

  bool csrUsed  = false;

  csrUsed       = core.csrUnit.process(core.dctoEx, ir_timer, ipi, csrResult, setCSRPC,

                                 localStall || core.stallDm || core.stallIm ||

                                     core.stallSignals[STALL_DECODE]); // not idempotent !!!

  core.csrUnit.mcycle++;

  if (multUsed)

    extoMem_temp.result = multResult;

  if (csrUsed)

    extoMem_temp.result = csrResult;

  if (setCSRPC) {

    extoMem_temp.nextPC   = csrResult;

    extoMem_temp.isBranch = true;

  }

  /*

    Handling an enventual trap/interruption

    Machine Interrupt Cause CSR mcause mcause[-1] = 1/0 --> Interrupt/Exception

    / mcause[-2:0] --> Exception Code mstatus : most important csr -> control

    and track the hart's current operating state

  */

  if (((core.csrUnit.mip[IR_M_TIMER] && core.csrUnit.mie[IR_M_TIMER]) ||

       (core.csrUnit.mip[IR_M_SOFT] && core.csrUnit.mie[IR_M_SOFT])) &&

      core.csrUnit.mstatus[MSTATUS_MIE]) {

    //printf("trap %d %d %d\n", core.memtoWB.we, core.extoMem.we, core.dctoEx.we);

    // flushing strategy

    if (!core.flushing) {

      core.csrUnit.mepc = extoMem_temp.isBranch ? extoMem_temp.nextPC : (core.ftoDC.we ? core.ftoDC.pc : core.pc);

      core.flushing     = true;

    }

    // We have to trap : we wait for the pipeline to be emptied

    dctoEx_temp.we                 = 0;

    core.ftoDC.we                  = 0;

    ftoDC_temp.we                  = 0;

    core.stallSignals[STALL_FETCH] = 1;

    if (!(core.memtoWB.we || core.extoMem.we || core.dctoEx.we)) {

      //printf("TRap int %x (%d %d)\n", core.csrUnit.mepc, (unsigned int)core.csrUnit.mip[IR_M_TIMER], (unsigned int)core.csrUnit.mip[IR_M_SOFT]);

      core.stallSignals[STALL_FETCH]     = 0; // free branch unit to choose the right PC

      core.csrUnit.mstatus[MSTATUS_MPIE] = core.csrUnit.mstatus[MSTATUS_MIE];

      core.csrUnit.mstatus[MSTATUS_MIE]  = 0;

      core.flushing                      = false;

      core.csrUnit.mcause = 0x80000000 | ((core.csrUnit.mip[IR_M_TIMER] && core.csrUnit.mie[IR_M_TIMER]) ? IR_M_TIMER : IR_M_SOFT);

      ftoDC_temp.nextPCFetch = core.csrUnit.mtvec;

    }

  }

  /*

    ---------------------

  */

  memory(core.extoMem, memtoWB_temp);

  writeback(core.memtoWB, wbOut_temp);

  // resolve stalls, forwards

  if (!localStall)

    forwardUnit(dctoEx_temp.rs1, dctoEx_temp.useRs1, dctoEx_temp.rs2, dctoEx_temp.useRs2, dctoEx_temp.rs3,

                dctoEx_temp.useRs3, extoMem_temp.rd, extoMem_temp.useRd, extoMem_temp.isLongInstruction,

                memtoWB_temp.rd, memtoWB_temp.useRd, wbOut_temp.rd, wbOut_temp.useRd, core.stallSignals,

                forwardRegisters);

  memMask mask;

  // TODO: carry the data size to memToWb

  switch (core.extoMem.funct3) {

    case 0:

      mask = BYTE;

      break;

    case 1:

      mask = HALF;

      break;

    case 2:

      mask = WORD;

      break;

    case 4:

      mask = BYTE_U;

      break;

    case 5:

      mask = HALF_U;

      break;

    // Should NEVER happen

    default:

      mask = WORD;

      break;

  }

  memOpType opType = (!core.stallSignals[STALL_MEMORY] && !localStall && !core.stallIm && memtoWB_temp.we)

                         ? memtoWB_temp.opType

                         : NONE;

#if LINUXOS

  core.dm->nextLevel->process(memtoWB_temp.address, mask, opType, core.csrUnit.mhartid.slc<4>(0), memtoWB_temp.valueToWrite,

                   memtoWB_temp.result, core.stallDm);

#else

  core.dm->process(memtoWB_temp.address, mask, opType, core.csrUnit.mhartid.slc<4>(0), memtoWB_temp.valueToWrite,

                   memtoWB_temp.result, core.stallDm);

#endif

  // commit the changes to the pipeline register

  if (!core.stallSignals[STALL_FETCH] && !localStall && !core.stallIm && !core.stallDm) {

    core.ftoDC = ftoDC_temp;

  }

  if (!core.stallSignals[STALL_DECODE] && !localStall && !core.stallIm && !core.stallDm) {

    core.dctoEx = dctoEx_temp;

    if (forwardRegisters.forwardExtoVal1 && extoMem_temp.we)

      core.dctoEx.lhs = extoMem_temp.result;

    else if (forwardRegisters.forwardMemtoVal1 && memtoWB_temp.we)

      core.dctoEx.lhs = memtoWB_temp.result;

    else if (forwardRegisters.forwardWBtoVal1 && wbOut_temp.we)

      core.dctoEx.lhs = wbOut_temp.value;

    if (forwardRegisters.forwardExtoVal2 && extoMem_temp.we)

      core.dctoEx.rhs = extoMem_temp.result;

    else if (forwardRegisters.forwardMemtoVal2 && memtoWB_temp.we)

      core.dctoEx.rhs = memtoWB_temp.result;

    else if (forwardRegisters.forwardWBtoVal2 && wbOut_temp.we)

      core.dctoEx.rhs = wbOut_temp.value;

    if (forwardRegisters.forwardExtoVal3 && extoMem_temp.we)

      core.dctoEx.datac = extoMem_temp.result;

    else if (forwardRegisters.forwardMemtoVal3 && memtoWB_temp.we)

      core.dctoEx.datac = memtoWB_temp.result;

    else if (forwardRegisters.forwardWBtoVal3 && wbOut_temp.we)

      core.dctoEx.datac = wbOut_temp.value;

  }

  if (core.stallSignals[STALL_DECODE] && !core.stallSignals[STALL_EXECUTE] && !core.stallIm && !core.stallDm &&

      !localStall) { // TODO: energy consumption to flip the bits --> must be

                     // carefully handle ! insert a NOP

    core.dctoEx.we          = 0;

    core.dctoEx.useRd       = 0;

    core.dctoEx.isBranch    = 0;

    core.dctoEx.instruction = 0;

    core.dctoEx.pc          = 0;

  }

  if (!core.stallSignals[STALL_EXECUTE] && !localStall && !core.stallIm && !core.stallDm) {

    core.extoMem = extoMem_temp;

  }

  if (!core.stallSignals[STALL_MEMORY] && !localStall && !core.stallIm && !core.stallDm) {

    core.memtoWB = memtoWB_temp;

  }

  if (wbOut_temp.we && wbOut_temp.useRd && !localStall && !core.stallIm && !core.stallDm) {

    core.regFile[wbOut_temp.rd] = wbOut_temp.value;

  }

  branchUnit(ftoDC_temp.nextPCFetch, dctoEx_temp.nextPCDC, dctoEx_temp.isBranch, extoMem_temp.nextPC,

             extoMem_temp.isBranch, core.pc, core.ftoDC.we, core.dctoEx.we,

             core.stallSignals[STALL_FETCH] || core.stallIm || core.stallDm || localStall);

  /*

    CSR Write

  */

  if (wbOut_temp.we && !localStall && !core.stallIm && !core.stallDm) {

    core.csrUnit.minstret++; // the number of instructions retired : CPI

  }

  //increment the cycle counter

  core.cycle++;

  //printf("%d Debug: --> %x (%d, %d)\n", (int)core.csrUnit.mhartid, (unsigned int)core.pc, core.stallIm, core.stallDm);

}

// ac_int<32, false> imData[DRAM_SIZE]

// ac_int<32, false> dmData[DRAM_SIZE]

// Memory

// IncompleteMemory<4> imInterface = IncompleteMemory<4>(imData, &clint);

// IncompleteMemory<4> dmInterface = IncompleteMemory<4>(dmData, &clint);

void initCore(Core& core)

{

  //init the cycle counter

  core.cycle = 0;

  //init reg file

  core.regFile[0]  = 0;

  core.regFile[1]  = 0;

  core.regFile[2]  = 0;

  core.regFile[3]  = 0;

  core.regFile[4]  = 0;

  core.regFile[5]  = 0;

  core.regFile[6]  = 0;

  core.regFile[7]  = 0;

  core.regFile[8]  = 0;

  core.regFile[9]  = 0;

  core.regFile[10] = 0;

  core.regFile[11] = 0;

  core.regFile[12] = 0;

  core.regFile[13] = 0;

  core.regFile[14] = 0;

  core.regFile[15] = 0;

  core.regFile[16] = 0;

  core.regFile[17] = 0;

  core.regFile[18] = 0;

  core.regFile[19] = 0;

  core.regFile[20] = 0;

  core.regFile[21] = 0;

  core.regFile[22] = 0;

  core.regFile[23] = 0;

  core.regFile[24] = 0;

  core.regFile[25] = 0;

  core.regFile[26] = 0;

  core.regFile[27] = 0;

  core.regFile[28] = 0;

  core.regFile[29] = 0;

  core.regFile[30] = 0;

  core.regFile[31] = 0;

  core.pc          = 0;

  // Stall

  core.stallSignals[0] = 0;

  core.stallSignals[1] = 0;

  core.stallSignals[2] = 0;

  core.stallSignals[3] = 0;

  core.stallSignals[4] = 0;

  core.stallIm         = 0;

  core.stallDm         = 0;

  core.stallMultAlu    = 0;

  core.flushing        = 0;

  // CSR

  core.csrUnit.mstatus       = 0;

  core.csrUnit.mie           = 0;

  core.csrUnit.mepc          = 0;

  core.csrUnit.mcause        = 0;

  core.csrUnit.mip           = 0;

  core.csrUnit.mtval         = 0;

  core.csrUnit.mcountinhibit = 0;

  core.csrUnit.mhpmevent3    = 0;

  core.csrUnit.mhpmevent4    = 0;

  core.csrUnit.mhpmcounter3  = 0;

  core.csrUnit.mhpmcounter4  = 0;

  core.csrUnit.minstret      = 0;

  core.csrUnit.mcycle        = 0;

  core.csrUnit.mscratch      = 0;

  // ftodc

  core.ftoDC.pc          = 0;

  core.ftoDC.instruction = 0;

  core.ftoDC.nextPCFetch = 0;

  core.ftoDC.we          = 0;

  // dctoex

  core.dctoEx.pc          = 0;

  core.dctoEx.instruction = 0;

  core.dctoEx.opCode      = 0;

  core.dctoEx.funct7      = 0;

  core.dctoEx.funct3      = 0;

  core.dctoEx.funct12     = 0;

  core.dctoEx.lhs         = 0;

  core.dctoEx.rhs         = 0;

  core.dctoEx.datac       = 0;

  core.dctoEx.nextPCDC    = 0;

  core.dctoEx.isBranch    = 0;

  core.dctoEx.useRs1      = 0;

  core.dctoEx.useRs2      = 0;

  core.dctoEx.useRs3      = 0;

  core.dctoEx.useRd       = 0;

  core.dctoEx.rs1         = 0;

  core.dctoEx.rs2         = 0;

  core.dctoEx.rs3         = 0;

  core.dctoEx.rd          = 0;

  core.dctoEx.we          = 0;

  // extoMem

  core.extoMem.pc                = 0;

  core.extoMem.instruction       = 0;

  core.extoMem.result            = 0;

  core.extoMem.rd                = 0;

  core.extoMem.useRd             = 0;

  core.extoMem.isLongInstruction = 0;

  core.extoMem.opCode            = 0;

  core.extoMem.funct3            = 0;

  core.extoMem.datac             = 0;

  core.extoMem.nextPC            = 0;

  core.extoMem.isBranch          = 0;

  core.extoMem.we                = 0;

  // memtoWB

  core.memtoWB.result       = 0;

  core.memtoWB.rd           = 0;

  core.memtoWB.useRd        = 0;

  core.memtoWB.address      = 0;

  core.memtoWB.valueToWrite = 0;

  core.memtoWB.byteEnable   = 0;

  core.memtoWB.opType       = NONE;

  core.memtoWB.we           = 0;

}

// One hart

#if !SMP

void doCore(ac_int<32, false> boot_address, ac_int<4, false> mhartid, bool globalStall, bool ir_timer, bool ipi,

            ac_int<32, false> imData[DRAM_SIZE], ac_int<32, false> dmData[DRAM_SIZE])

{

  static Core core;

  static bool initialized = false;

  // AMO This register is mandatory for multicore synchro between

  // harts. We use it to elaborate Load-Reserve and Store-Conditional !

  static reservationSet reservationAMO;

  static IncompleteMemory<4> imInterface = IncompleteMemory<4>(imData, &reservationAMO);

  static IncompleteMemory<4> dmInterface = IncompleteMemory<4>(dmData, &reservationAMO);

  static CacheMemory<4, 16, 64> dmCache = CacheMemory<4, 16, 64>(&dmInterface, false);

  static CacheMemory<4, 16, 64> imCache = CacheMemory<4, 16, 64>(&imInterface, false);

  if (!initialized) {

    initialized = true;

    initCore(core);

    core.csrUnit.mhartid = mhartid; // Core Unique ID

    core.im              = &imCache;

    core.dm              = &dmCache;

    // Init

    core.pc         = boot_address;

    core.regFile[2] = STACK_INIT;

  }

#pragma pipeline_init_interval 1

  doCycle(core, globalStall, ir_timer, ipi);

}

#elif !MCVM

// Multiple harts

void doCore(ac_int<32, false> boot_address, ac_int<4, false> mhartid,

            bool globalStall, bool ir_timer, bool ipi,

            cacheDirectoryInterfaceIn* bufferInIM, cacheDirectoryInterfaceIn* bufferInDM, cacheDirectoryInterfaceOut* bufferOutIM,

            cacheDirectoryInterfaceOut* bufferOutDM, cacheDirectoryInterfaceOut* bufferOutWBDM, cacheDirectoryInterfaceOut* bufferOutWBIM) {

  static Core core;

  static bool initialized = false;

  static MulticoreMemory<4> imInterface = MulticoreMemory<4>(bufferInIM, bufferOutIM, bufferOutWBIM);

  static MulticoreMemory<4> dmInterface = MulticoreMemory<4>(bufferInDM, bufferOutDM, bufferOutWBDM);

  static CacheMemoryMulticore<4, 16, 64> dmCache = CacheMemoryMulticore<4, 16, 64>(&dmInterface, false);

  static CacheMemoryMulticoreInst<4, 16, 64> imCache = CacheMemoryMulticoreInst<4, 16, 64>(&imInterface, false);

  if (!initialized) {

    initialized = true;

    initCore(core);

    core.csrUnit.mhartid = mhartid; // Core Unique ID

    core.im = &imCache;

    core.dm = &dmCache;

    // Init

    core.pc = boot_address;

    core.regFile[2] = STACK_INIT;

  }

  #pragma pipeline_init_interval 1

  doCycle(core, globalStall, ir_timer, ipi);

}

void doL2(cacheL2DirectoryInterfaceIn& bufferIn, cacheL2DirectoryInterfaceOut& bufferOut, ac_int<32, false> mem[DRAM_SIZE]) {

  static reservationSet reservationAMO;

  static IncompleteMemory<4> l2Interface = IncompleteMemory<4>(mem, &reservationAMO);

  static CacheMemoryL2<4,16,128> cacheL2 = CacheMemoryL2<4, 16, 128>(&l2Interface, false);

  #pragma pipeline_init_interval 1

  cacheL2.processL2(bufferIn, bufferOut);

}

#endif

