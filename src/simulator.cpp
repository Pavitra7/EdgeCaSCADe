#include <simulator.h>

void doClint(Clint& clint, std::vector<ac_int<32, false> >& mem)
{

  // We are in the part that control CLINT
  ac_int<32, false>* p_mem = mem.data();
  // mtime (for all harts)
  ac_int<64, false> mtime         = 0;
  ac_int<64, false> mtimecmp_zero = 0;
  ac_int<64, false> mtimecmp_one  = 0;

  int addr_mtimeh = 0, addr_mtimel = 0;
  addr_mtimel = (CLINT_BASE+CLINT_MTIME_OFF)>>2; // 4
  addr_mtimeh = addr_mtimel + 1;   // 5
  mtime.set_slc(0, ac_int<64, false>(p_mem[addr_mtimeh]) << 32 | p_mem[addr_mtimel]);
  mtime++;
  p_mem[addr_mtimel] = mtime.slc<32>(0);
  p_mem[addr_mtimeh] = mtime.slc<32>(32);

  // IPI CPU 0
  ac_int<32, false> ipi_zero = 0;
  ipi_zero                   = p_mem[(CLINT_BASE+CLINT_MSIP_OFF)>>2]; // 20

  // IPI CPU 1
  ac_int<32, false> ipi_one = 0;
  ipi_one                   = p_mem[(CLINT_BASE+CLINT_MSIP_OFF+4)>>2]; // 21

  // Hartid 0
  mtimecmp_zero.set_slc(0, ac_int<64, false>(p_mem[(CLINT_BASE+CLINT_MTIMECMP_OFF+4)>>2]) << 32 | p_mem[(CLINT_BASE+CLINT_MTIMECMP_OFF)>>2]); // 64 - 65
  // Hartid 1
  mtimecmp_one.set_slc(0, ac_int<64, false>(p_mem[(CLINT_BASE+CLINT_MTIMECMP_OFF+12)>>2]) << 32 | p_mem[(CLINT_BASE+CLINT_MTIMECMP_OFF+8)>>2]); // 66 - 67

  // Timer 0
  ac_int<1, false> ir_timer_zero = (mtime >= mtimecmp_zero);
  // Timer 1
  ac_int<1, false> ir_timer_one = (mtime >= mtimecmp_one);
  
  clint.ir_timer[0] = ir_timer_zero;

  if (SMP) {
    clint.ir_timer[1] = ir_timer_one;
  }

  // Inter-Processor Interrupt
  // Hartid 0-1
  clint.ipi[0] = (ipi_zero != 0);
  if (SMP)
    clint.ipi[1] = (ipi_one != 0);
    
  //printf("Timer cmp : %x vs %x %d-%d\n", p_mem[0x40], p_mem[0x4], clint.ir_timer[1], clint.ir_timer[0]);
}


#define LOGLEVEL_EMERG    0 /* system is unusable */
#define LOGLEVEL_ALERT    1 /* action must be taken immediately */
#define LOGLEVEL_CRIT     2 /* critical conditions */
#define LOGLEVEL_ERR      3 /* error conditions */
#define LOGLEVEL_WARNING  4 /* warning conditions */
#define LOGLEVEL_NOTICE   5 /* normal but significant condition */
#define LOGLEVEL_INFO     6 /* informational */
#define LOGLEVEL_DEBUG    7 /* debug-level messages */

void doEarlyConsole(std::vector<ac_int<32, false> >& mem)
{
  ac_int<32, false>* p_mem     = mem.data();
  static int time              = 0;
  static int cur_pos           = 0;
  static bool is_time_to_print = false;
  bool new_msg                 = false;
  const int baudrate           = 30000; // time of the early print
  // Security Section
  // BOOT Security MANAGEMENT
  time++;
  is_time_to_print = (time % baudrate) == 0;

  if (is_time_to_print && PRINT_CONSOLE) {
    //printf("\n----------------------\n");
    while(1) {
      char value_to_print = (char)p_mem[(IO_BASE + cur_pos) >> 2];
      if(value_to_print == 0x00)
        break;

      if (value_to_print == '\001') {
        new_msg = true;
      } else if(new_msg) {
        new_msg = false;
        if(value_to_print >= LOGLEVEL_NOTICE)
          printf("INFO: ");
        else if(value_to_print == LOGLEVEL_WARNING)
          printf("WARN: ");
        else
          printf("ERR: ");
      } else {
        printf("%c", value_to_print);
        p_mem[(IO_BASE + cur_pos) >> 2] = 0; // we delete what we've already read
      }

      cur_pos = (cur_pos + 4)%IO_LENGTH;
    }
    //printf("\n----------EARLY COMET CONSOLE------------\n");
  }
}

void doUART(std::vector<ac_int<32, false> >& mem)
{
  // LiteUart
  ac_int<32, false>* p_mem = mem.data();

  p_mem[(UART_TXFULL >> 2)] = 0; // never full !
}

void amoManagement(int& index, reservationSet reservationAMO[], const int nbCores)
{

  std::vector<int> validSet; // table with indices where reservation set is valid

  for (int i = 0; i < nbCores; i++) { // store indices of valid reservation set
    if (reservationAMO[i].valid) {
      validSet.push_back(i);
      reservationAMO[i].valid = 0; // only one reservation set must be equal to one.
    }
  }

  // choose one core !
  if (validSet.size() == 1) {
    index                       = validSet[0];
    reservationAMO[index].valid = 1;
    return;
  }

  for (int j = 0; j < validSet.size(); j++) {
    if (validSet[j] != index) {
      index                       = validSet[j];
      reservationAMO[index].valid = 1;
      return;
    }
  }
}
