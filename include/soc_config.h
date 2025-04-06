/*

  SOC CONFIGURATION FILE 
  
  Memory Map:
  
  ------ High address: HA
  |
  #### H1 - 0x1000 : define(STACK_INIT)
  | stack 
  |       
  |
  |
  | dynamic data (heap)
  #### ROM address: Heap address
  | .bss
  | .data
  | .text
  #### Begin ROM define(CLINT)
  | UART
  | CLINT
  ------ Low address
  
  CLINT (Multi-core - [n] 0<= n < NB_CORES)
  | ...
  | ipi[0]
  | ...
  | mtimecmph[0]
  | mtimecmp[0]
  | mtimeh
  | mtime
  CLINT_BASE

*/

#ifndef __SOC_CONFIG_H
#define  __SOC_CONFIG_H

// This variable tells the simulator if we are using a bare-metal program or an OS: 0->bare-metal 1->OS
#define HAS_KERNEL_SYSCALL_HANDLER 1
// This variable tells the simulator if we are using the console or not (1->console enable)
#define PRINT_CONSOLE 1
// This variable tells the simulator that we are running Linux
#define LINUXOS 0
// This variable tells the simulator where to put the DTB (for Linux)
#define DTB_BASE 0x10000
// This variable telles the simulator if we are doing a multi-core unitary test
#define MCVM 0
// This variable tells the simulator how many cores we simulate
#define NB_CORES 1
#define SMP (NB_CORES > 1)
// Memory size
#define ADDR_WIDTH 19
#define DRAM_SIZE ((size_t)1 << ADDR_WIDTH) // ~16 MB
// ROM and RAM address
#define ROMRAM 0x30000
// Stack first address
#define STACK_INIT (DRAM_SIZE - 0x1000) // White space: 32 Byte: 8 lines
// CLINT size and address
#define CLINT_BASE      0x1000 * LINUXOS + 0x10 * (1-LINUXOS)
#define CLINT_LENGTH    0xC000 * LINUXOS + 0x300 * (1-LINUXOS) // clint -> D000
#define CLINT_MSIP_OFF  0 + 0x40 * (1-LINUXOS)
#define CLINT_MTIMECMP_OFF  0x4000 * LINUXOS + 0xF0 * (1-LINUXOS)
#define CLINT_MTIME_OFF 0xBFF8 *LINUXOS + 0x0 * (1-LINUXOS)
// Serial Console size and address
#define IO_BASE         0xD000 * LINUXOS + 0x310 * (1-LINUXOS)// SERIAL/UART (EARLY CONSOLE)
#define IO_LENGTH       0x480
#define IO_LENGTH_LINES (0x480/4)
// Serial Console Litex (LiteUart)
#define UART_BASE_ADDR	 0xE000 * LINUXOS + 0x790 * (1-LINUXOS)
#define UART_SIZE        0x18
#define UART_RXTX	 ((UART_BASE_ADDR) + 0x00)// 124
#define UART_TXFULL	 ((UART_BASE_ADDR) + 0x04)// 125
#define UART_RXEMPTY	 ((UART_BASE_ADDR) + 0x08)// 126
#define UART_EV_STATUS	 ((UART_BASE_ADDR) + 0x0c)// 127
#define UART_EV_PENDING  ((UART_BASE_ADDR) + 0x10)// 128
#define UART_EV_ENABLE	 ((UART_BASE_ADDR) + 0x14)// 129


struct Clint {
  // Interrupt Timer / Interrupt Software
  bool ir_timer[NB_CORES] = {false}, ipi[NB_CORES] = {false};
};

#endif
