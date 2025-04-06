#include <ac_int.h>

class MultiplicationUnit {
public:
  ac_int<32, false> quotient = 0, remainder = 0;
  ac_int<6, false> state   = 0;
  bool resIsNeg            = false;
  bool forward_minus_minus = false, forward_minus_plus = false, forward_plus_minus = false;
  bool forward_result_unsigned = false, forward_result_signed = false;
  int i                           = 0;
  ac_int<32, false> dataAUnsigned = 0, dataBUnsigned = 0;

  bool process(struct DCtoEx dctoEx, ac_int<32, false>& result, bool& stall)
  {
    // no need to fill in the output register fields, the first ALU has that
    // taken care of
    bool valRet = false;

    if (dctoEx.opCode == RISCV_OP && dctoEx.funct7 == RISCV_OP_M) {
      if (state == 0) {

        forward_result_unsigned |= (dataAUnsigned ^ dctoEx.lhs) | (dataBUnsigned ^ dctoEx.rhs);
        forward_result_unsigned = !forward_result_unsigned;

        forward_minus_minus |= (dataAUnsigned ^ -dctoEx.lhs) | (dataBUnsigned ^ -dctoEx.rhs);
        forward_minus_minus = !forward_minus_minus;

        forward_minus_plus |= (dataAUnsigned ^ -dctoEx.lhs) | (dataBUnsigned ^ dctoEx.rhs);
        forward_minus_plus = !forward_minus_plus;

        forward_plus_minus |= (dataAUnsigned ^ dctoEx.lhs) | (dataBUnsigned ^ -dctoEx.rhs);
        forward_plus_minus    = !forward_plus_minus;
        forward_result_signed = forward_minus_minus | forward_minus_plus | forward_plus_minus | forward_result_unsigned;

        dataAUnsigned.set_slc(0, dctoEx.lhs);
        dataBUnsigned.set_slc(0, dctoEx.rhs);

        // mult results
        ac_int<64, false> resultU  = dataAUnsigned * dataBUnsigned;
        ac_int<64, false> resultS  = dctoEx.lhs * dctoEx.rhs;
        ac_int<64, false> resultSU = dctoEx.lhs * dataBUnsigned;
        resIsNeg                   = dctoEx.lhs[31] ^ dctoEx.rhs[31];

        switch (dctoEx.funct3) {
          case RISCV_OP_M_MUL:
            result = resultS.slc<32>(0);
            valRet = true;
            break;
          case RISCV_OP_M_MULH:
            result = resultS.slc<32>(32);
            valRet = true;
            break;
          case RISCV_OP_M_MULHSU:
            result = resultSU.slc<32>(32);
            valRet = true;
            break;
          case RISCV_OP_M_MULHU:
            result = resultU.slc<32>(32);
            valRet = true;
            break;
          case RISCV_OP_M_DIV:
            if (dataBUnsigned == 0) { // division by zero
              result = -1;
              valRet = true;
            } else if (dataAUnsigned == 0x80000000 && dataBUnsigned == 0xfffffffc) { // Overflow 32 bits
              result = 0x80000000;
              valRet = true;
            } else if (forward_result_signed) {
              if (resIsNeg)
                result = -quotient;
              else
                result = quotient;
              valRet = true;
            } else {
              state     = 32;
              quotient  = 0;
              remainder = 0;
              if (dctoEx.lhs[31]) {
                dataAUnsigned = -dctoEx.lhs;
              }
              if (dctoEx.rhs[31]) {
                dataBUnsigned = -dctoEx.rhs;
              }
            }
            break;
          case RISCV_OP_M_DIVU:
            if (dataBUnsigned == 0) { // division by zero
              result = -1;
              valRet = true;
            } else if (forward_result_unsigned) {
              result = quotient;
              valRet = true;
            } else {
              state     = 32;
              quotient  = 0;
              remainder = 0;
            }
            break;
          case RISCV_OP_M_REM:
            if (dataBUnsigned == 0) { // division by zero
              result = dataAUnsigned;
              valRet = true;
            } else if (dataAUnsigned == 0x80000000 && dataBUnsigned == 0xfffffffc) { // Overflow
              result = 0;
              valRet = true;
            } else if (forward_result_signed) {
              if (dctoEx.lhs[31])
                result = -remainder;
              else
                result = remainder;
              valRet = true;
            } else {
              state     = 32;
              quotient  = 0;
              remainder = 0;
              if (dctoEx.lhs[31]) {
                dataAUnsigned = -dctoEx.lhs;
              }
              if (dctoEx.rhs[31]) {
                dataBUnsigned = -dctoEx.rhs;
              }
            }
            break;
          case RISCV_OP_M_REMU:
            if (dataBUnsigned == 0) { // division by zero
              result = dataAUnsigned;
              valRet = true;
            } else if (forward_result_unsigned) {
              result = remainder;
              valRet = true;
            } else {
              state     = 32;
              quotient  = 0;
              remainder = 0;
            }
            break;
          default:
            break;
        }
      } else {
        // Loop for the division
        for (i = 0; i < 4; i++) {
          state--;
          remainder    = remainder << 1;
          remainder[0] = dataAUnsigned[state];
          if (remainder >= dataBUnsigned) {
            remainder       = remainder - dataBUnsigned;
            quotient[state] = 1;
          }
        }
        if (state == 0) {
          switch (dctoEx.funct3) {
            case RISCV_OP_M_DIV:
              if (resIsNeg)
                result = -quotient;
              else
                result = quotient;
              valRet = true;
              break;
            case RISCV_OP_M_DIVU:
              result = quotient;
              valRet = true;
              break;
            case RISCV_OP_M_REM:
              if (dctoEx.lhs[31])
                result = -remainder;
              else
                result = remainder;
              valRet = true;
              break;
            case RISCV_OP_M_REMU:
              result = remainder;
              valRet = true;
              break;
            default:
              break;
          }
        }
      }
    }
    stall |= (state != 0);
    return valRet;
  }
};
