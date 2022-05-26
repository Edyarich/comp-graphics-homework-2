#!/bin/sh
bindir=$(pwd)
cd /home/eduard/CLionProjects/Shooter/
export 

if test "x$1" = "x--debugger"; then
	shift
	if test "xYES" = "xYES"; then
		echo "r  " > $bindir/gdbscript
		echo "bt" >> $bindir/gdbscript
		/usr/bin/gdb -batch -command=$bindir/gdbscript --return-child-result /home/eduard/CLionProjects/Shooter/cmake-build-debug/shooter 
	else
		"/home/eduard/CLionProjects/Shooter/cmake-build-debug/shooter"  
	fi
else
	"/home/eduard/CLionProjects/Shooter/cmake-build-debug/shooter"  
fi
