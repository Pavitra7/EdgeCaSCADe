#ifndef __CSR_UNIT
#define __CSR_UNIT

#include <ac_int.h>
#include <pipelineRegisters.h>
#include <riscvISA.h>

// Definition of all CSR address: Volume II RISCV PRIVILEGE

// Machine information registers
#define CSR_MVENDORID 0xf11 // Vendor ID : 0
#define CSR_MARCHID 0xf12   // Architecture ID : 0
#define CSR_MIMPID 0xf13    // Implementation ID : 0
#define CSR_MHARTID 0xf14

// Machine trap setup
#define CSR_MSTATUS 0x300 // sstatus (supervisor)
#define CSR_MISA 0x301 // ISA and extensions: 0x40001100
#define CSR_MEDELEG 0x302
#define CSR_MIDELEG 0x303
#define CSR_MIE 0x304
#define CSR_MTVEC 0x305
#define CSR_MCOUNTEREN 0x306

// Machine trap handling
#define CSR_MSCRATCH 0x340
#define CSR_MEPC 0x341
#define CSR_MCAUSE 0x342
#define CSR_MTVAL 0x343
#define CSR_MIP 0x344

// Machine memory protection
#define CSR_PMPCFG0 0x3a0
#define CSR_PMPCFG1 0x3a1
#define CSR_PMPCFG2 0x3a2
#define CSR_PMPCFG3 0x3a3
#define CSR_PMPADDR0 0x3b0
#define CSR_PMPADDR1 0x3b1
#define CSR_PMPADDR2 0x3b2
#define CSR_PMPADDR3 0x3b3
#define CSR_PMPADDR4 0x3b4
#define CSR_PMPADDR5 0x3b5
#define CSR_PMPADDR6 0x3b6
#define CSR_PMPADDR7 0x3b7
#define CSR_PMPADDR8 0x3b8
#define CSR_PMPADDR9 0x3b9
#define CSR_PMPADDR10 0x3ba
#define CSR_PMPADDR11 0x3bb
#define CSR_PMPADDR12 0x3bc
#define CSR_PMPADDR13 0x3bd
#define CSR_PMPADDR14 0x3be
#define CSR_PMPADDR15 0x3bf

// Machine counter/timers
#define CSR_MCYCLE 0xb00
#define CSR_MINSTRET 0xb02
#define CSR_MHPMCOUNTER3 0xb03
#define CSR_MHPMCOUNTER4 0xb04
//...
#define CSR_MCYCLEH 0xb80
#define CSR_MINSTRETH 0xb82
#define CSR_MHPMCOUNTER3H 0xb83
#define CSR_MHPMCOUNTER4H 0xb84
//...

// Machine counter setup
#define CSR_MCOUNTINHIBIT 0x320
#define CSR_MHPMEVENT3 0x323
#define CSR_MHPMEVENT4 0x324
//...

/*
  Interruption Request
*/

#define IR_M_SOFT 3
#define IR_M_TIMER 7
#define IR_M_EXT 11
#define MIE_SSIE 1
#define MSTATUS_MIE 3
#define MSTATUS_MPIE 7

class CsrUnit {
public:
  /*
    Machine Information Registers - CSR - (Production purposes)
    TODO : reduce size
  */
  // Hardwired: Vendor ID
  // Hardwired: Architecture ID
  // Hardwired: Implementation ID
  ac_int<4, false> mhartid; // Hardware thread ID

  /*
    Machine Trap Setup
  */
  ac_int<32, false> mstatus; // Machine status register
  // Hardwired : MISA
  // ac_int<32, false> medeleg; // Volume II: Machine exception delegation register
  // ac_int<32, false> mideleg; // Volume II: Machine interruption delegation register
  ac_int<32, false> mie;   // Machine interrupt-enable register
  ac_int<32, false> mtvec; // Machine trap-handler base address
  // ac_int<32, false> mcounteren; // Machine counter enable

  /*
    Machine Trap Handling
  */
  ac_int<32, false> mscratch; // Scratch register for machine trap handlers
  ac_int<32, false> mepc;     // Machine exception program counter
  ac_int<32, false> mcause;   // Machine trap cause
  ac_int<32, false> mtval;    // Machine bad address or instruction
  ac_int<32, false> mip;      // Machine interrupt pending

  /*
     Machine Memory Protection
  */
  /*
  ac_int<32, false> pmpcfg0; // Physical memory protection configuration.
  ac_int<32, false> pmpcfg;
  ac_int<32, false> pmpcfg2;
  ac_int<32, false> pmpcfg3;
  ac_int<32, false> pmpaddr0; // Physical memory protection address register.
  ac_int<32, false> pmpaddr1;
  ac_int<32, false> pmpaddr2;
  ac_int<32, false> pmpaddr3;
  ac_int<32, false> pmpaddr4;
  ac_int<32, false> pmpaddr5;
  ac_int<32, false> pmpaddr6;
  ac_int<32, false> pmpaddr7;
  ac_int<32, false> pmpaddr8;
  ac_int<32, false> pmpaddr9;
  ac_int<32, false> pmpaddr10;
  ac_int<32, false> pmpaddr11;
  ac_int<32, false> pmpaddr12;
  ac_int<32, false> pmpaddr13;
  ac_int<32, false> pmpaddr14;
  ac_int<32, false> pmpaddr15;
*/

  /*
    Machine Counter/Timers
  */
  ac_int<64, false> mcycle;       // Machine cycle counter
  ac_int<64, false> minstret;     // Machine instructions-retired counter
  ac_int<64, false> mhpmcounter3; // Machine performance-monitoring counter
  ac_int<64, false> mhpmcounter4;
  /*
  ac_int<64, false> mhpmcounter4;
  ac_int<64, false> mhpmcounter5;
  ac_int<64, false> mhpmcounter6;
  ac_int<64, false> mhpmcounter7;
  ac_int<64, false> mhpmcounter8;
  ac_int<64, false> mhpmcounter9;
  ac_int<64, false> mhpmcounter10;
  ac_int<64, false> mhpmcounter11;
  ac_int<64, false> mhpmcounter12;
  ac_int<64, false> mhpmcounter13;
  ac_int<64, false> mhpmcounter14;
  ac_int<64, false> mhpmcounter15;
  ac_int<64, false> mhpmcounter16;
  ac_int<64, false> mhpmcounter17;
  ac_int<64, false> mhpmcounter18;
  ac_int<64, false> mhpmcounter19;
  ac_int<64, false> mhpmcounter20;
  ac_int<64, false> mhpmcounter21;
  ac_int<64, false> mhpmcounter22;
  ac_int<64, false> mhpmcounter23;
  ac_int<64, false> mhpmcounter24;
  ac_int<64, false> mhpmcounter25;
  ac_int<64, false> mhpmcounter26;
  ac_int<64, false> mhpmcounter27;
  ac_int<64, false> mhpmcounter28;
  ac_int<64, false> mhpmcounter29;
  ac_int<64, false> mhpmcounter30;
  ac_int<64, false> mhpmcounter31;
  */

  /*
    Machine Counter Setup
  */

  ac_int<32, false> mcountinhibit; // Machine counter-inhibit register
  ac_int<32, false> mhpmevent3;    // Machine performance-monitoring event selector.
  ac_int<32, false> mhpmevent4;
  /*
  ac_int<32, false> mhpmevent3; // Machine performance-monitoring event selector.
  ac_int<32, false> mhpmevent4;
  ac_int<32, false> mhpmevent5;
  ac_int<32, false> mhpmevent6;
  ac_int<32, false> mhpmevent7;
  ac_int<32, false> mhpmevent8;
  ac_int<32, false> mhpmevent9;
  ac_int<32, false> mhpmevent10;
  ac_int<32, false> mhpmevent11;
  ac_int<32, false> mhpmevent12;
  ac_int<32, false> mhpmevent13;
  ac_int<32, false> mhpmevent14;
  ac_int<32, false> mhpmevent15;
  ac_int<32, false> mhpmevent16;
  ac_int<32, false> mhpmevent17;
  ac_int<32, false> mhpmevent18;
  ac_int<32, false> mhpmevent19;
  ac_int<32, false> mhpmevent20;
  ac_int<32, false> mhpmevent21;
  ac_int<32, false> mhpmevent22;
  ac_int<32, false> mhpmevent23;
  ac_int<32, false> mhpmevent24;
  ac_int<32, false> mhpmevent25;
  ac_int<32, false> mhpmevent26;
  ac_int<32, false> mhpmevent27;
  ac_int<32, false> mhpmevent28;
  ac_int<32, false> mhpmevent29;
  ac_int<32, false> mhpmevent30;
  ac_int<32, false> mhpmevent31;
  */
  ac_int<32, false> result_reg;
  bool setPC_reg;
  bool csrUsed_reg;

  bool process(struct DCtoEx dctoEx, ac_int<1, false> ir_timer, ac_int<1, false> ipi, ac_int<32, false>& result,
               bool& setPC, bool stall_cpu)
  {

    // CSR Write/ Update Logic

    // External Interrupts
    this->mip[IR_M_SOFT]  = ipi;      // & this->mie[IR_M_SOFT]; // inter processor interrupt
    this->mip[IR_M_TIMER] = ir_timer; // & this->mie[IR_M_TIMER]; // timer interrupt (mtime, mtimecmp)

    // ZiCSR : CSRRW, CSRRS, CSRRC, CSRRWI, CSRRSI, CSRRCI
    if (!stall_cpu && dctoEx.we && dctoEx.opCode == RISCV_SYSTEM &&
        (dctoEx.funct3 == RISCV_SYSTEM_CSRRW || dctoEx.funct3 == RISCV_SYSTEM_CSRRS ||
         dctoEx.funct3 == RISCV_SYSTEM_CSRRC || dctoEx.funct3 == RISCV_SYSTEM_CSRRWI ||
         dctoEx.funct3 == RISCV_SYSTEM_CSRRSI || dctoEx.funct3 == RISCV_SYSTEM_CSRRCI)) {

      // We are in a CSR function
      ac_int<12, false> csr     = dctoEx.instruction.slc<12>(20);
      ac_int<32, false> andMask = 0xffffffff;
      ac_int<32, false> orMask  = 0;

      if (dctoEx.funct3 == RISCV_SYSTEM_CSRRW) {
        // Its a read/write so the value to write is lhs
        andMask = 0;
        orMask  = dctoEx.lhs;
      } else if (dctoEx.funct3 == RISCV_SYSTEM_CSRRWI) {
        // Its a read/write so the value to write is lhs
        andMask = 0;
        orMask  = dctoEx.rs1;
      } else if (dctoEx.funct3 == RISCV_SYSTEM_CSRRS && dctoEx.lhs != 0) {
        // Its a read/write so the value to write is lhs
        andMask = 0xffffffff;
        orMask  = dctoEx.lhs;
      } else if (dctoEx.funct3 == RISCV_SYSTEM_CSRRSI && dctoEx.rs1 != 0) {
        // Its a read/write so the value to write is lhs
        andMask = 0xffffffff;
        orMask  = dctoEx.rs1;
      } else if (dctoEx.funct3 == RISCV_SYSTEM_CSRRC && dctoEx.lhs != 0) {
        // Its a read/write so the value to write is lhs
        andMask = !dctoEx.lhs;
        orMask  = 0;
      } else if (dctoEx.funct3 == RISCV_SYSTEM_CSRRCI && dctoEx.rs1 != 0) {
        // Its a read/write so the value to write is lhs
        andMask = !dctoEx.rs1;
        orMask  = 0;
      } else {
        andMask = 0xffffffff;
        orMask  = 0;
      }

      switch (csr) {
        // Machine Information Registers - CSR - (Production purposes)
        case CSR_MVENDORID:
          result_reg = 0;
          break;
        case CSR_MARCHID:
          result_reg = 0;
          break;
        case CSR_MIMPID:
          result_reg = 0;
          break;
        case CSR_MHARTID:
          result_reg = this->mhartid;
          break;
        // Machine Trap Setup
        case CSR_MSTATUS:
          result_reg    = this->mstatus;
          this->mstatus = (result_reg & andMask) | orMask; // XS and FS should be zero anyway [15:13]
          break;
        case CSR_MISA:
          result_reg = 0x40001100;
          break;
        /*case CSR_MEDELEG:
                result = this->medeleg;
                this->medeleg = (result & andMask) | orMask;
        break;
        case CSR_MIDELEG:
                result = this->mideleg;
                this->mideleg = (result & andMask) | orMask;
        break;*/
        case CSR_MIE:
          // We can protect that write in order to prevent the system to enable unsupported interrupts
          result_reg = this->mie;
          this->mie  = (result_reg & andMask) | orMask;
          break;
        case CSR_MTVEC:
          result_reg  = this->mtvec;
          this->mtvec = (result_reg & andMask) | orMask;
          break;
        // Machine Trap Handling
        case CSR_MSCRATCH:
          result_reg     = this->mscratch;
          this->mscratch = (result_reg & andMask) | orMask;
          break;
        case CSR_MEPC:
          result_reg = this->mepc;
          this->mepc = (result_reg & andMask) | orMask;
          break;
        case CSR_MCAUSE:
          result_reg   = this->mcause;
          this->mcause = (result_reg & andMask) | orMask;
          break;
        case CSR_MTVAL:
          result_reg  = this->mtval;
          this->mtval = (result_reg & andMask) | orMask;
          break;
        case CSR_MIP:
          result_reg = this->mip;
          this->mip  = (result_reg & andMask) | orMask;
          break;
        // Machine Counter/Timers
        case CSR_MCYCLE:
          result_reg = this->mcycle.slc<32>(0);
          this->mcycle.set_slc(0, (result_reg & andMask) | orMask);
          break;
        case CSR_MCYCLEH:
          result_reg = this->mcycle.slc<32>(32);
          this->mcycle.set_slc(32, (result_reg & andMask) | orMask);
          break;
        case CSR_MINSTRET:
          result_reg = this->minstret.slc<32>(0);
          this->minstret.set_slc(0, (result_reg & andMask) | orMask);
          break;
        case CSR_MINSTRETH:
          result_reg = this->minstret.slc<32>(32);
          this->minstret.set_slc(32, (result_reg & andMask) | orMask);
          break;
        case CSR_MHPMCOUNTER3:
          result_reg = this->mhpmcounter3.slc<32>(0);
          this->mhpmcounter3.set_slc(0, (result_reg & andMask) | orMask);
          break;
        case CSR_MHPMCOUNTER3H:
          result_reg = this->mhpmcounter3.slc<32>(32);
          this->mhpmcounter3.set_slc(32, (result_reg & andMask) | orMask);
          break;
        case CSR_MHPMCOUNTER4:
          result_reg = this->mhpmcounter4.slc<32>(0);
          this->mhpmcounter4.set_slc(0, (result_reg & andMask) | orMask);
          break;
        case CSR_MHPMCOUNTER4H:
          result_reg = this->mhpmcounter4.slc<32>(32);
          this->mhpmcounter4.set_slc(32, (result_reg & andMask) | orMask);
          break;
        // Machine Counter Setup
        case CSR_MCOUNTINHIBIT:
          result_reg          = this->mcountinhibit;
          this->mcountinhibit = (result_reg & andMask) | orMask;
          break;
        case CSR_MHPMEVENT3:
          result_reg       = this->mhpmevent3;
          this->mhpmevent3 = (result_reg & andMask) | orMask;
          break;
        case CSR_MHPMEVENT4:
          result_reg       = this->mhpmevent4;
          this->mhpmevent4 = (result_reg & andMask) | orMask;
          break;
        default:
          break;
      }
      setPC_reg   = false;
      csrUsed_reg = true;
    } else if (!stall_cpu && dctoEx.we && dctoEx.opCode == RISCV_SYSTEM && dctoEx.funct3 == RISCV_PRIV) {
      // We handle a ecall or a ebreak
      // Wait For Interrupt (WFI) : NOP instructions for the moment
      switch (dctoEx.funct12) {
        case RISCV_SYSTEM_ENV_ECALL:
          // ECALL jumps to the IRQ handler code, whose adress is stored in mtvec lower bits
          this->mepc   = dctoEx.pc;
          this->mstatus[7] = this->mstatus[3];
          this->mcause = 11; // environment call from machine mode 0x00000 -> exception
          result_reg   = mtvec;
          setPC_reg    = true & HAS_KERNEL_SYSCALL_HANDLER;
          break;
        case RISCV_SYSTEM_ENV_MRET:
          // MRET jumps back to the address stored in mepc
          result_reg = this->mepc;
          setPC_reg  = true & HAS_KERNEL_SYSCALL_HANDLER;
          // We change interrupt enable bits (MIE = MPIE and MPIE = 1)
          //if (!stall_cpu) {
          this->mstatus[3] = this->mstatus[7];
          this->mstatus[7] = 0;
          break;
        case RISCV_SYSTEM_ENV_EBREAK:
          this->mepc   = dctoEx.pc;
          this->mcause = 3; // break debug !
          result_reg   = mtvec;
          setPC_reg    = true & HAS_KERNEL_SYSCALL_HANDLER;
          break;
        default:
          setPC_reg = false;
          break;
      }
      csrUsed_reg = false;
    } else if(!stall_cpu){
      result_reg  = false;
      setPC_reg   = false;
      csrUsed_reg = false;
    }
    
    result = result_reg;
    setPC  = setPC_reg;
    return csrUsed_reg;
  }
};

#endif
