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
 
  constant ADD_MAX_INSTRUCTION    : std_logic_vector(23 downto 0) := x"080000";
  constant ADD_MIN_INSTRUCTION    : std_logic_vector(23 downto 0) := x"030000";
  
  constant ADD_MAX_DATA           : std_logic_vector(23 downto 0) := x"080000";
  constant ADD_MIN_DATA           : std_logic_vector(23 downto 0) := x"030000";
  
------------------------------------------
-- Signals                            --
------------------------------------------  
  signal clk                      : std_logic:='0';
  signal rst                      : std_logic:='1';
  signal globalStall              : std_logic:='1';
  signal en_comet                 : std_logic:='1';
-- CLINT
  signal timer_interrupt          : std_logic_vector(63 downto 0):= (others=>'0');
  signal mtime                    : std_logic_vector(63 downto 0):= (others=>'0');
  signal en_timer_interrupt       : std_logic:='0';
  signal mhartid                  : std_logic_vector (3 downto 0) := (others => '0');
  signal ir_timer                 : std_logic:='0';
  signal ipi                      : std_logic:='0';
-- MEMORY
  signal boot_address             : std_logic_vector (31 downto 0) := x"00030000";
  signal memory_rsc_adr           : std_logic_vector (18 DOWNTO 0);
  signal memory_rsc_d             : std_logic_vector (31 DOWNTO 0);
  signal memory_rsc_we            : std_logic:='0';
  signal memory_rsc_q             : std_logic_vector (31 DOWNTO 0);
  signal memory_rsc_en            : std_logic:='0';
  --signal memory_rsc_adr_19        : std_logic_vector (18 downto 0);

  signal imData_rsc_adr           : std_logic_vector (18 downto 0);
  --signal imData_rsc_adr_19        : std_logic_vector (18 downto 0);
  signal imData_rsc_q             : std_logic_vector (31 downto 0);
  signal imData_rsc_d             : std_logic_vector (31 downto 0);
  signal imData_rsc_we            : std_logic:='0';
  signal imData_rsc_en            : std_logic:='0';
  signal dmData_rsc_adr           : std_logic_vector (18 downto 0);
  signal dmData_rsc_q             : std_logic_vector (31 downto 0);
  signal dmData_rsc_d             : std_logic_vector (31 downto 0);
  signal dmData_rsc_we            : std_logic:='0';
  signal dmData_rsc_en            : std_logic:='0';
  
  signal ARM_we                   : std_logic:= '0';
  signal ARM_addr                 : std_logic_vector(23 downto 0);
  signal ARM_din                  : std_logic_vector(31 downto 0):= (others=>'0');
  signal ARM_en                   : std_logic:='0';

  type status is (OK, TOO_LOW, TOO_HIGH);
  signal add_inst_status          : status;
  signal add_data_status          : status;

  signal char_out                 : std_logic_vector(31 downto 0);
  signal char_out_q               : std_logic_vector(31 downto 0);
  signal counter                  : std_logic_vector(31 downto 0) := (others => '0');

------------------------------------------
-- Procedure to load memories           --
------------------------------------------   
procedure load_mem (
    variable addr_32b    : in    std_logic_vector(31 downto 0);
    variable data2write  : in    std_logic_vector(31 downto 0);
    signal wdata         : out   std_logic_vector(31 downto 0);
    signal we            : out   std_logic;
    signal add_15b       : out   std_logic_vector(23 downto 0)
    )
  is
  begin
    
    wait until clk'event and clk='0';
    add_15b <= addr_32b(25 downto 2);
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
    en              => en_comet,
    mhartid_rsc_dat => mhartid,
    ir_timer        => ir_timer,
    ipi             => '0',
    rst             => rst,
    boot_address_rsc_dat => boot_address,
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

-- directory
  directory: entity work.directory
  port map(
   clk_i     => clk,
   reset_i   => rst,

   -- cache core one
   dmData_rsc_adr_i => dmData_rsc_adr,
   dmData_rsc_d_i   => dmData_rsc_d,
   dmData_rsc_we_i  => dmData_rsc_we,
   dmData_rsc_en_i  => dmData_rsc_en,
   dmData_rsc_q_o   => dmData_rsc_q,
   
   ARM_din_i  => ARM_din,
   ARM_we_i   => ARM_we,
   ARM_addr_i => ARM_addr,
   ARM_en_i   => ARM_en,
   -- memory interface
   memory_rsc_adr_o => memory_rsc_adr, 
   memory_rsc_d_o   => memory_rsc_d,
   memory_rsc_we_o  => memory_rsc_we,
   memory_rsc_q_i   => memory_rsc_q,
   memory_rsc_en_o  => memory_rsc_en,
   
   -- clint
   mtime_i           => mtime,
   timer_interrupt_o => timer_interrupt,
   en_timer_o        => en_timer_interrupt
  );

-- clint
  clint : entity work.clint
  port map(
    clk_i => clk,
    reset_i => rst,
    hartid_i => mhartid,
    timer_interrupt_i => timer_interrupt,
    en_i => en_timer_interrupt,
    ir_timer_o => ir_timer,
    ipi_o => ipi,
    mtime_o => mtime
  );

-- memory
  memory_data : entity work.design_1_blk_mem_gen_1_0
  port map(
    clka      => clk,
    ena       => imData_rsc_en,
    wea(0)    => imData_rsc_we,
    addra     => imData_rsc_adr,
    dina      => imData_rsc_d,
    douta     => imData_rsc_q,
    clkb      => clk,
    enb       => '1',
    web(0)    => memory_rsc_we,
    addrb     => memory_rsc_adr,
    dinb      => memory_rsc_d,
    doutb     => memory_rsc_q
  );

--
----------------------------------------------------------------------------
-- Combi
----------------------------------------------------------------------------
-- wrong address detection
  add_inst_status <= TOO_LOW  when ARM_addr < ADD_MIN_INSTRUCTION else
                     TOO_HIGH when ARM_addr > ADD_MAX_INSTRUCTION else
                     OK;
                     
  add_data_status <= TOO_LOW  when memory_rsc_adr < ADD_MIN_DATA and dmData_rsc_adr(15 downto 0) /= x"0000" else
                     TOO_HIGH when memory_rsc_adr > ADD_MAX_DATA else OK;



  char_out        <= memory_rsc_d when memory_rsc_adr(15 downto 0) = x"01E4" and memory_rsc_we = '1' and memory_rsc_d /= x"0000001B" else char_out_q;
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
    rst      <= '1';
    globalStall <= '1';
    en_comet <= '0';
    wait for 1 us;
    rst      <= '0';
    wait for 1 us;
    
    -- Fill memory 
    ARM_en <= '1';
    file_open(file_stimuli,"../../../../../zephyr_synch.bin", READ_MODE); -- zephyr bin
    while not endfile(file_stimuli) loop 
      readline(file_stimuli, line_num0);
      if character(line_num0(1)) /= ';' then --jump commented lines
        hread(line_num0,vdata);
        hread(line_num0,vaddress);
        wait for 1 ns;
        load_mem (vaddress, vdata, ARM_din, ARM_we, ARM_addr);
      end if;
      wait for 1 ns;
    end loop;
    file_close(file_stimuli);
    ARM_en <= '0';
    -- Launch Comet	
    en_comet <= '1';
    globalStall <= '0';
    -- **********************
    wait for 100 ms;
    report "end of test bench" severity failure;
  end process;

--Write the result (all char_out)
Write_result_proc : process(clk)
variable line_num1 : line;
file file_result : text open write_mode is "../../../../../results.txt";
begin
  if rising_edge(clk) then
    char_out_q <= char_out;
    if memory_rsc_adr(15 downto 0)= x"01E4" and memory_rsc_we = '1' then -- UART_RXTX
      hwrite(line_num1,char_out);
      writeline(file_result,line_num1);
    end if;
  end if;
end process;

dump_proc : process(clk)
variable line_num_d : line;
variable line_num_i : line;
file file_dump_d : text open write_mode is "../../../../../dump_add_data.txt";
file file_dump_i : text open write_mode is "../../../../../dump_add_inst.txt";
begin
  if rising_edge(clk) and imData_rsc_en = '1' and globalStall = '0' then
    counter <= counter + 1;
    hwrite(line_num_d,dmData_rsc_adr);
    STD.TEXTIO.write(line_num_d,string'(" "));
    hwrite(line_num_i,imData_rsc_adr);
    STD.TEXTIO.write(line_num_i,string'(" "));
    writeline(file_dump_d,line_num_d);
    writeline(file_dump_i,line_num_i);
  end if; 
end process;
  
end;
