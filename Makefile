#			Projet PC2R
#		Makefile pour le partie serveur "Ricochet Robot"

CC =gcc
LFLAGS =-lpthread
CFLAGS =-g -W -Wall -Iinclude

DIR=.
BIN=$(DIR)/bin/
OBJ=$(DIR)/obj/
INCLUDE=$(DIR)/include/
LIB=$(DIR)/lib/
SRC=$(DIR)/src/

.SUFFIXES:

HC=
O=

.PHONY: all clean test test-ftp_server test-ftp_client
all: $(BIN)serveur

test: $(BIN)serveur

test-serveur: $(BIN)serveur
	-$$PWD/bin/serveur 2000 &

clean: 
	rm -rf $(OBJ)*.o $(BIN)*

$(BIN)%: $(OBJ)serveur.o  $(OBJ)tools.o $(OBJ)resolution.o
	@if [ -d $(BIN) ]; then : ; else mkdir $(BIN); fi
	$(CC) $(LFLAGS) -o $@ $^

$(OBJ)%.o: $(SRC)%.c $(HC)
	@if [ -d $(OBJ) ]; then : ; else mkdir $(OBJ); fi
	$(CC) $(CFLAGS) -o $@ -c $<

$(INCLUDE)%.h:
	@if [ -d $(INCLUDE) ]; then : ; else mkdir $(INCLUDE); fi

