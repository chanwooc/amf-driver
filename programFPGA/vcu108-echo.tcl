
open_hw
connect_hw_server

open_hw_target 

set vcu108fpga [lindex [get_hw_devices xcvu095_0] 0]

set file2 ./clearVCU108.bit
set file4 ./echo.bit

set_property PROGRAM.FILE $file2 $vcu108fpga
puts "fpga is $vcu108fpga, bit file size is [exec ls -sh $file2], CLEAR BEGIN"
program_hw_devices $vcu108fpga

set_property PROGRAM.FILE $file4 $vcu108fpga
puts "fpga is $vcu108fpga, bit file size is [exec ls -sh $file4], PROGRAM BEGIN"
program_hw_devices $vcu108fpga

refresh_hw_device $vcu108fpga

close_hw_target

disconnect_hw_server
