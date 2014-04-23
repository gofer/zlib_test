#!/bin/sh

CC=gcc
CCFLAGS="-std=c89"
SRCFILES=(zlib_test.c)
TARGET=zlib_test.out
LDFLAGS=-lz

${CC} ${CCFLAGS} ${SRCFILES[@]} -o ${TARGET} ${LDFLAGS}
