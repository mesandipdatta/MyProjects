#!/bin/sh
bindir=$(pwd)
cd /Users/yusun/Repositories/MyProjects/OpenGL Tutorial/OpenGL-tutorial_v0014_21/tutorial17_rotations/
export DYLD_LIBRARY_PATH=:$DYLD_LIBRARY_PATH

if test "x$1" = "x--debugger"; then
	shift
	if test "x" = "xYES"; then
		echo "r  " > $bindir/gdbscript
		echo "bt" >> $bindir/gdbscript
		 -batch -command=$bindir/gdbscript  /Users/yusun/Repositories/MyProjects/OpenGL\ Tutorial/OpenGLTutorials_bin_XCode/RelWithDebInfo/tutorial17_rotations 
	else
		"/Users/yusun/Repositories/MyProjects/OpenGL\ Tutorial/OpenGLTutorials_bin_XCode/RelWithDebInfo/tutorial17_rotations"  
	fi
else
	"/Users/yusun/Repositories/MyProjects/OpenGL\ Tutorial/OpenGLTutorials_bin_XCode/RelWithDebInfo/tutorial17_rotations"  
fi
