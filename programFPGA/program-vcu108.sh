#VIVADO_PATH=/opt/Xilinx/Vivado/2018.1/bin

MY_PATH=$(dirname $0)
cd $MY_PATH

# If multiple boards, use -tcl args flag:
#  -tclargs 210203861260

## Select MLC or SLC for flash drives
if [ -n "$SLC" ]
then
	vivado -nolog -nojournal -mode batch -source vcu108-slc.tcl 
else
	vivado -nolog -nojournal -mode batch -source vcu108-mlc.tcl 
fi

pciescanportal
