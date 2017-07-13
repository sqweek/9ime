#!/bin/sh
CC=gcc
${CC} genkbd.c -o genkbd  &&
./genkbd <keyboard >kbd.h  &&
${CC} -mwindows hook.c -o 9ime
