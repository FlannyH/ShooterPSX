PROJECT = flandocs

CC = gcc
CFLAGS = -O3 -Wall -std=c11

SRCS = 	flandocs.c

TARGET_DIR = bin

all: $(TARGET_DIR)/$(PROJECT)

$(TARGET_DIR):
	mkdir -p $(TARGET_DIR)

$(TARGET_DIR)/$(PROJECT): $(SRCS) $(HEADERS) | $(TARGET_DIR)
	$(CC) $(CFLAGS) -o $(TARGET_DIR)/$(PROJECT) $(SRCS)

clean:
	rm -rf $(TARGET_DIR)/$(PROJECT)

.PHONY: all clean
