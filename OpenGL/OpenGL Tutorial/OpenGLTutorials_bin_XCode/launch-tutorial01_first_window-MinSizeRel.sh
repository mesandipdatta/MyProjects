#!/bin/sh
bindir=$(pwd)
cd /Users/yusun/Repositories/MyProjects/OpenGL Tutorial/OpenGL-tutorial_v0014_21/tutorial01_first_window/
export DYLD_LIBRARY_PATH=:$DYLD_LIBRARY_PATH

if test "x$1" = "x--debugger"; then
	shift
	if test "x" = "xYES"; then
		echo "r  " > $bindir/gdbscript
		echo "bt" >> $bindir/gdbscript
		 -batch -command=$bindir/gdbscript  /Users/yusun/Repositories/MyProjects/OpenGL\ Tutorial/OpenGLTutorials_bin_XCode/MinSizeRel/tutorial01_first_window 
	else
		"/Users/yusun/Repositories/MyProjects/OpenGL\ Tutorial/OpenGLTutorials_bin_XCode/MinSizeRel/tutorial01_first_window"  
	fi
else
	"/Users/yusun/Repositories/MyProjects/OpenGL\ Tutorial/OpenGLTutorials_bin_XCode/MinSizeRel/tutorial01_first_window"  
fi
