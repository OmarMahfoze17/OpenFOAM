#!/bin/bash

CPP	= mpic++
CC	= mpicc
CPPFLAGS	= -std=c++11 -O3 -g
CFLAGS	= 
CPP_HEADER_PATH	= -I/media/omar/bb5bfc6d-a551-4b3e-9f47-2b7d74410f28/WORK/codes/MUI/src
#CPP_HEADER_PATH = -I/home/omar/WORK/MUI/MUI_1.2
C_HEADER_PATH	=
CPP_LAB_PATH = 
C_LAB_PATH = 
CPP_LDLIBS = 
C_LDLIBS = -lstdc++ -lmpi_cxx -lmpi -lm
CPP_SCR = $(wildcard *.cpp)
C_SCR = $(wildcard *.c)
CPP_EXE = $(CPP_SCR:.cpp=)
C_EXE = $(C_SCR:.c=)


all: $(CPP_EXE) $(C_EXE)

% : %.cpp
	$(CPP) $(CPPFLAGS) $< -o $@ $(CPP_HEADER_PATH) $(CPP_LAB_PATH) $(CPP_LDLIBS)

% : %.c
	$(CC) $(CFLAGS) $< -o $@ $(C_HEADER_PATH) $(C_LAB_PATH) $(C_LDLIBS)


.PHONY: clean
	
clean:
	-rm -f $(CPP_EXE) $(C_EXE)
