SHELL = /bin/bash
CC = gcc
CFLAGS = -Wall -O2
LIB = ./util.c
SRC = ./web_server.c
EXE = ./web_server


web_server: $(SRC) $(LIB)
	$(CC) $(CFLAGS) $(LIB) $(SRC)  -o $(EXE)

.PHONY: clean
clean: $(EXE)
	rm $(EXE)
