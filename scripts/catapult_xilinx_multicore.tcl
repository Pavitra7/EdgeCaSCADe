set WORKING_DIR $::env(PWD)

solution new -state initial
solution options defaults


solution options set /ComponentLibs/TechLibSearchPath /opt/DesignKit/cmos28fdsoi_29/C28SOI_SC_12_CORE_LL/5.1-05/libs
solution options set /ComponentLibs/SearchPath /opt/Catapult-10.0b/Mgc_home/pkgs/siflibs/designcompiler -append
solution options set /Input/CompilerFlags {-D __CATAPULT__ -D __HLS__ -D MEMORY_INTERFACE=IncompleteMemory}
solution options set /Input/SearchPath $WORKING_DIR/../include
solution options set /Output/GenerateCycleNetlist false
solution file add $WORKING_DIR/../src/core.cpp -type C++

go new
directive set -DESIGN_HIERARCHY doCore
go compile
solution library add mgc_Xilinx-ZYNQ-uplus-2L_beh -- -rtlsyntool Vivado -manufacturer Xilinx -family ZYNQ-uplus -speed -2L -part xczu11eg-ffvb1517-2L-e
solution library add Xilinx_RAMS
solution library add Xilinx_ROMS
solution library add amba
solution library add Xilinx_FIFO
go libraries
directive set -CLOCKS {clk {-CLOCK_PERIOD 10.0 -CLOCK_EDGE rising -CLOCK_UNCERTAINTY 0.0 -CLOCK_HIGH_TIME 5.0 -RESET_SYNC_NAME rst -RESET_ASYNC_NAME arst_n -RESET_KIND sync -RESET_SYNC_ACTIVE high -RESET_ASYNC_ACTIVE low -ENABLE_NAME en -ENABLE_ACTIVE high}}
go assembly
directive set /doCore/core/imCache.cacheMemory:rsc -MAP_TO_MODULE Xilinx_RAMS.BLOCK_SPRAM_RBW
directive set /doCore/core/imCache.age:rsc -MAP_TO_MODULE Xilinx_RAMS.BLOCK_SPRAM_RBW
directive set /doCore/core/imCache.multicoreCacheState:rsc -MAP_TO_MODULE Xilinx_RAMS.BLOCK_SPRAM_RBW

directive set /doCore/core/dmCache.cacheMemory:rsc -MAP_TO_MODULE Xilinx_RAMS.BLOCK_SPRAM_RBW
directive set /doCore/core/dmCache.age:rsc -MAP_TO_MODULE Xilinx_RAMS.BLOCK_SPRAM_RBW
directive set /doCore/core/dmCache.multicoreCacheState:rsc -MAP_TO_MODULE Xilinx_RAMS.BLOCK_SPRAM_RBW

directive set /doCore/core/imCache.cacheMemory:rsc -INTERLEAVE 4
directive set /doCore/core/imCache.age:rsc -INTERLEAVE 4
directive set /doCore/core/imCache.multicoreCacheState:rsc -INTERLEAVE 4

directive set /doCore/core/dmCache.cacheMemory:rsc -INTERLEAVE 4
directive set /doCore/core/dmCache.age:rsc -INTERLEAVE 4
directive set /doCore/core/dmCache.multicoreCacheState:rsc -INTERLEAVE 4


directive set /doCore/bufferInIM.addr:rsc -MAP_TO_MODULE ccs_ioport.ccs_in
directive set /doCore/bufferInIM.interfaceRequest:rsc -MAP_TO_MODULE ccs_ioport.ccs_in

directive set /doCore/bufferInIM.buffer:rsc -MAP_TO_MODULE ccs_ioport.ccs_in
directive set /doCore/bufferInIM.protocolMiss:rsc -MAP_TO_MODULE ccs_ioport.ccs_in

directive set /doCore/bufferInDM.addr:rsc -MAP_TO_MODULE ccs_ioport.ccs_in
directive set /doCore/bufferInDM.interfaceRequest:rsc -MAP_TO_MODULE ccs_ioport.ccs_in

directive set /doCore/bufferInDM.buffer:rsc -MAP_TO_MODULE ccs_ioport.ccs_in
directive set /doCore/bufferInDM.protocolMiss:rsc -MAP_TO_MODULE ccs_ioport.ccs_in

directive set /doCore/bufferOutDM.releaseAck:rsc -MAP_TO_MODULE {[DirectInput]}
directive set /doCore/bufferOutIM.releaseAck:rsc -MAP_TO_MODULE {[DirectInput]}
directive set /doCore/bufferInDM.interfaceAck:rsc -MAP_TO_MODULE ccs_ioport.ccs_in_buf_wait
directive set /doCore/bufferInIM.interfaceAck:rsc -MAP_TO_MODULE ccs_ioport.ccs_in_buf_wait

directive set /doCore/bufferOutWBDM.releaseAck:rsc -MAP_TO_MODULE {[DirectInput]}
directive set /doCore/bufferOutWBIM.releaseAck:rsc -MAP_TO_MODULE {[DirectInput]}

directive set /doCore/globalStall:rsc -MAP_TO_MODULE {[DirectInput]}

directive set /doCore/core/core.regFile:rsc -MAP_TO_MODULE {[Register]}
directive set /doCore/core/main -PIPELINE_INIT_INTERVAL 1
directive set /doCore/core/core.multiplicationUnit.process:if:else:for -UNROLL yes
directive set -GEN_EXTERNAL_ENABLE true
go architect
go extract
