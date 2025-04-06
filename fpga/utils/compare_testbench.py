#Python program to compare the testbench from the emulation and the testbench from the RTL simulation

# Using readlines()
file1 = open('debug.txt', 'r')
file2 = open('dump_add_inst.txt', 'r')

LinesDebug = file1.readlines()
LinesRTL   = file2.readlines()

diff = 0
count = 0
count_line = 0
# Strips the newline character
for line in LinesRTL:
    count_line += 1
    # Convert hex string to hex with RTL debug
    line = "0x"+line
    line_integer = int(line, 16)
    line_integer = line_integer<<2
    hex_value = hex(line_integer)
    # Parse debug file
    can_be_compared = -1
    while(can_be_compared < 0):
      can_be_compared = LinesDebug[count].find("[0]") + LinesDebug[count].find("[1]")
      if can_be_compared < 0:
        count += 1
    line_debug_to_compare = LinesDebug[count][LinesDebug[count].find(":")+1:LinesDebug[count].find("[")]
    line_debug_to_compare = "0x"+ line_debug_to_compare.replace(" ", "")
    line_debug_to_compare = int(line_debug_to_compare, 16)
    hex_value_debug = hex(line_debug_to_compare)
    if(hex_value != hex_value_debug):
      diff += 1
      print("* Line{}: (rtl){} vs (debug){}".format(count_line, hex_value, hex_value_debug))
    else:
      print("= Line{}: (rtl){} vs (debug){}".format(count_line, hex_value, hex_value_debug))
      count += 1
      if diff != 0:
        print(diff)
      diff = 0 
    if diff > 10:
     break;


file1.close()
file2.close()
