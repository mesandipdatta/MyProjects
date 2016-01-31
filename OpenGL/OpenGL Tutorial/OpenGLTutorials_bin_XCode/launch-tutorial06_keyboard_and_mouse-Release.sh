#!/bin/sh
bindir=$(pwd)
cd /Users/yusun/Repositories/MyProjects/OpenGL Tutorial/OpenGL-tutorial_v0014_21/tutorial06_keyboard_and_mouse/
export DYLD_LIBRARY_PATH=:$DYLD_LIBRARY_PATH

if test "x$1" = "x--debugger"; then
	shift
	if test "x" = "xYES"; then
		echo "r  " > $bindir/gdbscript
		echo "bt" >> $bindir/gdbscript
		 -batch -command=$bindir/gdbscript  /Users/yusun/Repositories/MyProjects/OpenGL\ Tutorial/OpenGLTutorials_bin_XCode/Release/tutorial06_keyboard_and_mouse 
	else
		"/Users/yusun/Repositories/MyProjects/OpenGL\ Tutorial/OpenGLTutorials_bin_XCode/Release/tutorial06_keyboard_and_mouse"  
	fi
else
	"/Users/yusun/Repositories/MyProjects/OpenGL\ Tutorial/OpenGLTutorials_bin_XCode/Release/tutorial06_keyboard_and_mouse"  
fi
