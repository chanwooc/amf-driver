MY_PATH=$(dirname $0)
cd $MY_PATH

source $XILINX_HOME/Vivado/$XILINX_VERSION/settings64.sh
vivado -nolog -nojournal -mode batch -source vcu108-echo.tcl 
pciescanportal
