#!/usr/bin/env bash

if [[ $# -ne 1 ]]
then
	echo './program-vcu108.sh [0|1]'
	echo '  0: SLC (default) or 1: MLC'
fi

if [[ -z ${XILINX_HOME} ]] || [[ -z ${XILINX_VERSION} ]]
then
	echo 'set $XILINX_HOME and $XILINX_VERSION'
	exit 1
fi

MY_PATH=$(dirname $0)
cd ${MY_PATH}

XILINX_DIR=${XILINX_HOME}/Vivado/${XILINX_VERSION}
source ${XILINX_DIR}/settings64.sh

if [[ $# -ge 1 ]] && [[ $1 -eq 1 ]]
then
	# If multiple boards, use -tcl args flag: -tclargs
	vivado -nolog -nojournal -mode batch -source vcu108-mlc.tcl 
else
	vivado -nolog -nojournal -mode batch -source vcu108-slc.tcl 
fi

pciescanportal
