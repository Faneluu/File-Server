CC=@gcc
BIN_DIR=bin
SRC_DIR=src
ROOT_DIR=root
FILES_DIR=files

.PHONY: all clean

all: final

final: $(BIN_DIR)/server.o $(BIN_DIR)/operations.o $(BIN_DIR)/utils.o $(SRC_DIR)/client.c
	$(CC) $(BIN_DIR)/server.o $(BIN_DIR)/operations.o $(BIN_DIR)/utils.o -lpthread -o $(BIN_DIR)/server
	$(CC) $(SRC_DIR)/client.c -o $(BIN_DIR)/client

$(BIN_DIR)/operations.o: $(SRC_DIR)/operations.c
	$(CC) -c $(SRC_DIR)/operations.c -o $(BIN_DIR)/operations.o

$(BIN_DIR)/utils.o: $(SRC_DIR)/utils.c
	$(CC) -c $(SRC_DIR)/utils.c -o $(BIN_DIR)/utils.o

$(BIN_DIR)/server.o: $(SRC_DIR)/server.c
	$(CC) -c $(SRC_DIR)/server.c -o $(BIN_DIR)/server.o

clean: 
	@rm -rf $(ROOT_DIR)/* $(BIN_DIR)/* $(FILES_DIR)/*
