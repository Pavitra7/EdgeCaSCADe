################################################################
# Define Variables
################################################################
set vivado_version_lock         2019.1
set ip_name                     "Comet"
set ip_version                  "1.0"
set part                        "xc7z045ffg676-2"
set project_name                "simu_$ip_name"


################################################################
# Clean
################################################################
file delete -force vivado


################################################################
# Check Vivado version
################################################################
set current_vivado_version [version -short]

if { [string first $vivado_version_lock $current_vivado_version] == -1 } {
   puts ""
   puts "ERROR: This script was generated using Vivado <$vivado_version_lock> and is being run in <$current_vivado_version> of Vivado."

   return 1
}


################################################################
# Creation of the Project
################################################################
create_project $project_name ./vivado/ -part $part -force
reset_project
#set_property board            $board  [current_project]
set_property target_language  VHDL    [current_project]


################################################################
# Add RTL Sources & Vivado IP
################################################################
 add_files -fileset sources_1 ./design_1_blk_mem_gen_1_0.xci
 add_files -fileset sources_1 ./clint.vhd
 add_files -fileset sources_1 ./directory.vhd
 add_files -fileset sources_1 ./amo.vhd
 add_files -fileset sources_1 ./comet.vhdl
 update_compile_order -fileset sources_1

#Add package from src

################################################################
# Add Simulation files
################################################################
add_files -fileset sim_1 ./tb_comet.vhd
update_compile_order -fileset sim_1

################################################################
# Launch Simu
################################################################
launch_simulation -simset sim_1 -mode behavioral
save_wave_config ./tb_comet_behav.wcfg
open_vcd tb_comet.vcd
log_vcd /tb_comet/clk
log_vcd /tb_comet/rst
log_vcd /tb_comet/globalStall
log_vcd /tb_comet/ARM_we
log_vcd /tb_comet/ARM_addr
log_vcd /tb_comet/ARM_din
log_vcd /tb_comet/mtime
log_vcd /tb_comet/imData_*
log_vcd /tb_comet/dmData_*
log_vcd /tb_comet/memory_*
log_vcd /tb_comet/char_out
# restart
run 300 ms
flush_vcd
close_vcd



