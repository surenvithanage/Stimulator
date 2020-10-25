NAME ?= parallax
NAME_L := $(shell echo $(NAME) | tr '[:upper:]' '[:lower:]')

BIN_DIR := ./bin
SRC_DIR := ./src

CFLAGS = -lcurl -fopenmp

all: clean build

build:
	mkdir -p $(BIN_DIR)

	g++ \
		$(shell find ../libparallax/src -name *.cpp) \
		$(shell find $(SRC_DIR) -name *.cpp) \
		-o $(BIN_DIR)/$(NAME_L) \
		$(CFLAGS) \
		-Wall -Wpedantic -Wextra

clean:
	rm -rf $(BIN_DIR)
