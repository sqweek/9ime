#!/bin/sh
if ! [ -e ./keyboard ]; then
	echo >&2 "./keyboard does not exist; use builtin lib/keyboard?"
	echo >&2 "(governed by Lucent Public License, see lib/LICENSE.plan9)"
	read >&2 -p "[y/n] " -n 1
	echo
	case "$REPLY" in
		y*|Y*) cp lib/keyboard keyboard;;
		*) echo >&2 aborting; exit  1;;
	esac
fi
CC=gcc
set -x
${CC} genkbd.c -o genkbd  &&
./genkbd <keyboard >kbd.h  &&
${CC} -mwindows hook.c -o 9ime
