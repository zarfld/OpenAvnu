#!/bin/bash
# Simple script to run gptp

if [ "$#" -eq "0" ]; then 
    echo "please enter network interface name as parameter. For example:"
    echo "sudo ./run_gptp.sh eth1"
    exit -1
fi

# Check if gPTP daemon exists in the new location
if [ -f "thirdparty/gptp/linux/build/obj/daemon_cl" ]; then
    echo "Starting gPTP daemon from thirdparty/gptp..."
    groupadd -f ptp 2>/dev/null || true
    thirdparty/gptp/linux/build/obj/daemon_cl $1 $2 $3 $4 $5 $6 $7 $8
elif [ -f "build/thirdparty/gptp/linux/daemon_cl" ]; then
    echo "Starting gPTP daemon from CMake build directory..."
    groupadd -f ptp 2>/dev/null || true
    build/thirdparty/gptp/linux/daemon_cl $1 $2 $3 $4 $5 $6 $7 $8
elif [ -f "daemons/gptp/linux/build/obj/daemon_cl" ]; then
    echo "Starting gPTP daemon from legacy location..."
    groupadd -f ptp 2>/dev/null || true
    daemons/gptp/linux/build/obj/daemon_cl $1 $2 $3 $4 $5 $6 $7 $8
else
    echo "Error: gPTP daemon not found. Please build the project first:"
    echo "  mkdir build && cd build"
    echo "  cmake .."
    echo "  make"
    exit -1
fi
