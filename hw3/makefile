SHELL = /bin/bash
CC = gcc
LIB = -lncurses -lpcap
CFLAGS = -O2
S = ./smol-shell/shell.c
LIST = ./smol-shell/list.c
SRC = $(filter-out $(LIB), $(wildcard *.c))
EXE = $(patsubst %.c, %, $(SRC))

pcap_reader: pcap_reader.c $(SHELL) $(LIST) makefile
	$(CC) $(CFLAGS) $(S) $(LIST) pcap_reader.c -o pcap_reader $(LIB)


.PHONY: clean
clean: $(EXE)
	rm $(EXE)
