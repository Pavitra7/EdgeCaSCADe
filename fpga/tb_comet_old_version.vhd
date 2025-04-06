-------------------------------------------------------------------------------
-- Title      : tb_comet
-- Project    : Flodam
-------------------------------------------------------------------------------
-- File       : tb_comet.vhd
-- Author     : Cédric JAYET-GRIFFON
-- Company    : Temento System
-- Created    : 2020-04-03
-- Last update: 2020-04-03
-- Platform   : 
-- Standard   : VHDL
-------------------------------------------------------------------------------
-- Description: 
-------------------------------------------------------------------------------
-- Copyright (c) 2019
-------------------------------------------------------------------------------
-- Revisions  :
-- Date         Version       Author           Description
-- 2020-04-03     1.0      Jayet-Griffon        Creation
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use IEEE.NUMERIC_STD.ALL;

library std;
use STD.TEXTIO.all;
use ieee.std_logic_textio.all;

------------------------------------------
-- Entity                               --
------------------------------------------
entity tb_comet is

end tb_comet;

------------------------------------------
-- Architecture                         --
------------------------------------------
architecture arch of tb_comet is

------------------------------------------
-- Options                              --
------------------------------------------

------------------------------------------
-- Constants                            --
------------------------------------------  	
  constant CLK_FREQ               : natural := 200E6;
  constant CLK_PERIOD             : time    := 1 sec / CLK_FREQ;
  constant RESET_DURATION         : time    := 2*CLK_PERIOD;	
 
  constant ADD_MAX_INSTRUCTION    : std_logic_vector(23 downto 0) := x"005FFF";
  constant ADD_MIN_INSTRUCTION    : std_logic_vector(23 downto 0) := x"004000";
  
  constant ADD_MAX_DATA           : std_logic_vector(23 downto 0) := x"009FFF";
  constant ADD_MIN_DATA           : std_logic_vector(23 downto 0) := x"008000";
  
  constant BIT_POS_EOT            : natural := 0;  -- Bit position of the End Of Test in status register. Position already fixed
  constant BIT_POS_HANG           : natural := 1;  -- Bit position of the Hang in status register.        !!!!!! Position not fixed yet !!!!!!!
  constant BIT_POS_CRASH          : natural := 2;  -- Bit position of the Crash in status register.       !!!!!! Position not fixed yet !!!!!!!
------------------------------------------
-- Signals                            --
------------------------------------------  
  signal clk                      : std_logic:= '0';
  -- signal en_comet                 : std_logic:= '0';
  signal timerInt                 : std_logic_vector(63 downto 0);
  signal mtime                    : std_logic_vector(63 downto 0);
  signal en_timerInt              : std_logic:= '0';
  signal mhartid                  : std_logic_vector (3 downto 0) := (others => '0');
  signal ir_timer                 : std_logic:='0';
  signal ipi                      : std_logic:='0';
  signal rst                      : std_logic:='1';
  signal globalStall              : std_logic;
  signal crashFlag_rsc_dat        : std_logic;
  signal imData_rsc_adr           : std_logic_vector (23 downto 0);
  signal imData_rsc_adr_15        : std_logic_vector (14 downto 0);
  signal imData_rsc_q             : std_logic_vector (31 downto 0);
  signal imData_rsc_d             : std_logic_vector (31 downto 0);
  signal imData_rsc_we            : std_logic;
  signal imData_rsc_en            : std_logic;
  signal dmData_rsc_adr           : std_logic_vector (23 downto 0);
  signal dmData_rsc_adr_15        : std_logic_vector (14 downto 0);
  signal dmData_rsc_q             : std_logic_vector (31 downto 0);
  signal dmData_rsc_d             : std_logic_vector (31 downto 0);
  signal dmData_rsc_we            : std_logic;
  signal dmData_rsc_en            : std_logic;
  
  signal ARM_instruction_we       : std_logic;
  signal ARM_instruction_addr     : std_logic_vector(14 downto 0);
  signal ARM_instruction_din      : std_logic_vector(31 downto 0);
  
  signal ARM_data_we              : std_logic;
  signal ARM_data_addr            : std_logic_vector(14 downto 0);
  signal ARM_data_din             : std_logic_vector(31 downto 0);  
  
  type status is (OK, TOO_LOW, TOO_HIGH);
  signal add_inst_status          : status;
  signal add_data_status          : status;
  
  signal end_of_test              : std_logic;
  signal hang                     : std_logic;
  signal crash                    : std_logic;
  signal char_out                 : std_logic_vector(31 downto 0); 
  signal we_data_mem              : std_logic;
  signal header                   : std_logic := '0';
  
  signal counter                  : std_logic_vector(31 downto 0) := (others => '0');

------------------------------------------
-- Procedure to load memories           --
------------------------------------------   
procedure load_mem (
    variable addr_32b    : in    std_logic_vector(31 downto 0);
    variable data2write  : in    std_logic_vector(31 downto 0);
    signal wdata         : out   std_logic_vector(31 downto 0);
    signal we            : out   std_logic;
    signal add_15b       : out   std_logic_vector(14 downto 0)
    )
  is
  begin
    
    wait until clk'event and clk='0';
    add_15b <= addr_32b(16 downto 2);
    we      <= '1';
    wdata   <= data2write;
    
    wait until clk'event and clk='0';
    we      <= '0';
end procedure;

-- function
function "+"(L: STD_LOGIC_VECTOR; R: INTEGER) return STD_LOGIC_VECTOR is
    -- pragma label_applies_to plus
    variable result  : unsigned (L'range);
begin
    result  := UNSIGNED(L) + to_unsigned(R, result'length);-- pragma label plus
    return   std_logic_vector(result);
end;

 

begin
  
------------------------------------------
-- Instance                             --
------------------------------------------
-- Comet 
  comet : entity work.doCore
  port map(
    clk             => clk,
    --en              => en_comet,
    mhartid_rsc_dat => mhartid,
    ir_timer_rsc_dat=> '0',
    ipi_rsc_dat     => '0',
    rst             => rst,
    globalStall     => globalStall,
    imData_rsc_adr  => imData_rsc_adr,
    imData_rsc_q    => imData_rsc_q,
    imData_rsc_d    => imData_rsc_d,
    imData_rsc_we   => imData_rsc_we,
    imData_rsc_en   => imData_rsc_en,
    dmData_rsc_adr  => dmData_rsc_adr,
    dmData_rsc_q    => dmData_rsc_q,
    dmData_rsc_d    => dmData_rsc_d,
    dmData_rsc_we   => dmData_rsc_we,
    dmData_rsc_en   => dmData_rsc_en 
  );
  
-- clint
  clint : entity work.clint
  port map(
    clk_i      => clk,
    reset_i    => rst,
    hartid_i   => mhartid,
    timerInt_i => timerInt,
    en_i       => en_timerInt,
    ir_timer_o => ir_timer,
    ipi_o      => ipi,
    mtime_o    => mtime
  );


-- instruction memory
  memory_inst : entity work.memory
  port map(
    clka      => clk,
    rsta      => rst,
    ena       => '1',
    wea(0)    => ARM_instruction_we,
    addra     => ARM_instruction_addr,
    dina      => ARM_instruction_din,
    douta     => open,
    clkb      => clk,
    rstb      => rst,
    enb       => '1',
    web(0)    => imData_rsc_we,
    addrb     => imData_rsc_adr_15,
    dinb      => imData_rsc_d,
    doutb     => imData_rsc_q,
    rsta_busy => open,
    rstb_busy => open
  );
  imData_rsc_adr_15 <= '0' & imData_rsc_adr(13 downto 0);

-- data memory
  memory_data : entity work.memory
  port map(
    clka      => clk,
    rsta      => rst,
    ena       => '1',
    wea(0)    => ARM_data_we,
    addra     => ARM_data_addr,
    dina      => ARM_data_din,
    douta     => open,
    clkb      => clk,
    rstb      => rst,
    enb       => '1',
    web(0)    => we_data_mem,
    addrb     => dmData_rsc_adr_15,
    dinb      => dmData_rsc_d,
    doutb     => dmData_rsc_q,
    rsta_busy => open,
    rstb_busy => open
  );
  dmData_rsc_adr_15 <= '0' & dmData_rsc_adr(13 downto 0);
--
----------------------------------------------------------------------------
-- Combi
----------------------------------------------------------------------------
-- wrong address detection
  add_inst_status <= TOO_LOW  when imData_rsc_adr < ADD_MIN_INSTRUCTION else
                     TOO_HIGH when imData_rsc_adr > ADD_MAX_INSTRUCTION else
                     OK;
                     
  add_data_status <= TOO_LOW  when dmData_rsc_adr < ADD_MIN_DATA and dmData_rsc_adr/= x"000000" else
                     TOO_HIGH when dmData_rsc_adr > ADD_MAX_DATA and dmData_rsc_adr/= x"0C0000" and  dmData_rsc_adr/= x"0C0001" else
                     OK;


--
  
  globalStall     <= '0'; -- because RAM has 1 cycle of latency
  we_data_mem     <= dmData_rsc_we              when dmData_rsc_adr/= x"0C0000" and dmData_rsc_adr/= x"0C0001" else '0'; -- To not write in data memory when we update a status or write character out
  end_of_test     <= dmData_rsc_d(BIT_POS_EOT)  when dmData_rsc_adr= x"0C0001"  and dmData_rsc_we = '1' else '0';                                    -- create end of test
  hang            <= dmData_rsc_d(BIT_POS_HANG) when dmData_rsc_adr= x"0C0001"  and dmData_rsc_we = '1' else '0'; 
  crash           <= dmData_rsc_d(BIT_POS_CRASH)when dmData_rsc_adr= x"0C0001"  and dmData_rsc_we = '1' else '0'; 
  char_out        <= dmData_rsc_d               when dmData_rsc_adr= x"0C0000"  and dmData_rsc_we = '1' else (others => '0');
----------------------------------------------------------------------------
-- Stimuli 
----------------------------------------------------------------------------

  -- generate clocks
  clk          <= not clk    after CLK_PERIOD/2;
  
  Stimuli : process
  variable line_num0 : line;
  variable dataread  : std_logic_vector(31 downto 0) := (others => '0');    
  variable vaddress  : std_logic_vector(31 downto 0) := (others => '0');    
  variable vdata     : std_logic_vector(31 downto 0) := (others => '0');
  file file_stimuli : text;
      
  begin
    --en_comet <= '0';
    rst      <= '1';
    globalStall <= '1';
    wait for RESET_DURATION;
    globalStall <= '0';
    rst      <= '0';
    wait for 1 us;
    
    -- Fill data memory 
    file_open(file_stimuli,"../../../../../dijkstra_data.bin",READ_MODE); -- zephyr bin
    while not endfile(file_stimuli) loop 
      readline(file_stimuli, line_num0);
      if character(line_num0(1)) /= ';' then --jump commented lines
        hread(line_num0,vdata);
        hread(line_num0,vaddress);
        wait for 1 ns;
        load_mem (vaddress, vdata, ARM_data_din, ARM_data_we, ARM_data_addr);
      end if;
      wait for 1 ns;
    end loop;
    file_close(file_stimuli);	
    
    -- Fill instruction memory 
    file_open(file_stimuli,"../../../../../dijkstra_instr.bin",READ_MODE);
    while not endfile(file_stimuli) loop 
      readline(file_stimuli, line_num0);
      if character(line_num0(1)) /= ';' then --jump commented lines
        hread(line_num0,vdata);
        hread(line_num0,vaddress);
        wait for 1 ns;
        load_mem (vaddress, vdata, ARM_instruction_din, ARM_instruction_we, ARM_instruction_addr);
      end if;
      wait for 1 ns;
    end loop;
    file_close(file_stimuli);
    --en_comet <= '1';
    -- **********************
    wait until end_of_test = '1';
    wait for 10 ns;
    report "end of test bench" severity failure;
  end process;
  

-- Write the result (all char_out)
Write_result_proc : process(clk)
variable line_num1 : line;
file file_result : text open write_mode is "../../../../../results.txt";
begin
  if rising_edge(clk) then
    if dmData_rsc_adr= x"0C0000" then -- mettre UART_RTX
      hwrite(line_num1,char_out);
      writeline(file_result,line_num1);
    elsif end_of_test = '1' then
      STD.TEXTIO.write(line_num1,string'("     Status : 0xC0001"     ));
      STD.TEXTIO.write(line_num1,string'("End of test : "));
      write(line_num1,end_of_test);
      writeline(file_result,line_num1);
      STD.TEXTIO.write(line_num1,string'("Hang : "));
      write(line_num1,hang);
      writeline(file_result,line_num1);
      STD.TEXTIO.write(line_num1,string'("CRASH : "));
      write(line_num1,crash);
      writeline(file_result,line_num1);
      file_close(file_result);
    end if;
  end if;
end process;


-- Write all data written by comet with an outrange address
data_writen_proc : process(clk)
variable line_num2 : line;
file file_data : text open write_mode is "../../../../../data_w_Comet.csv";
begin
  if rising_edge(clk) then
    if header = '0' then
    STD.TEXTIO.write(line_num2,string'("Time ; Address (hexa) ; Data (hexa)"));
    writeline(file_data,line_num2);
    header <= '1';
    elsif end_of_test = '1' then
      file_close(file_data);
    elsif dmData_rsc_we = '1' and add_data_status /= OK then 
      write(line_num2,time'image(now));
      STD.TEXTIO.write(line_num2,string'(" ; "));
      hwrite(line_num2,dmData_rsc_adr);
      STD.TEXTIO.write(line_num2,string'(" ; "));
      hwrite(line_num2,dmData_rsc_d);
      writeline(file_data,line_num2);
    end if;
  end if;
end process;

dump_proc : process(clk)
variable line_num_d : line;
variable line_num_i : line;
file file_dump_d : text open write_mode is "../../../../../dump_add_data.txt";
file file_dump_i : text open write_mode is "../../../../../dump_add_inst.txt";
begin
  if rising_edge(clk) and dmData_rsc_en = '1' then
    counter <= counter + 1;
    hwrite(line_num_d,dmData_rsc_adr);
    STD.TEXTIO.write(line_num_d,string'(" "));
    hwrite(line_num_i,imData_rsc_adr);
    STD.TEXTIO.write(line_num_i,string'(" "));
    if counter(2 downto 0) = "111" then
      writeline(file_dump_d,line_num_d);
      writeline(file_dump_i,line_num_i);
    end if;
  end if; 
end process;
  
end;
