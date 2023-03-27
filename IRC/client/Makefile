# Makefile for my simple server

INC_DIR := ./include
SRC_DIR := ./src
OBJ_DIR := ./obj

CC := gcc
CFLAGS := -Wall -I$(INC_DIR)

SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC))

TARGET := server
$(TARGET): $(OBJ)
	$(CC) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	rm -f $(OBJ_DIR)/*.o $(TARGET)
