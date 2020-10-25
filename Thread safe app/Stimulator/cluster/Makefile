NAME ?= cluster
NAME_L := $(shell echo $(NAME) | tr '[:upper:]' '[:lower:]')

MODES := SERIAL MPI_SERIAL MPI_OPENCL
MODES_L := $(shell echo $(MODES) | tr '[:upper:]' '[:lower:]')

MODE ?= SERIAL # or MPI_SERIAL or MPI_OPENCL

BIN_DIR := ./bin
SRC_DIR := ./src

CFLAGS =

ifeq ($(MODE), MPI_OPENCL)
	CFLAGS += -lOpenCL
endif

all: clean build

all-modes:
	make clean

	for m in $(MODES) ; do \
		make build NAME=$(NAME)-$$m MODE=$$m ; \
	done

	ls -l $(BIN_DIR)/$(NAME)*

build:
	mkdir -p $(BIN_DIR)

	cp $(SRC_DIR)/*.cl $(BIN_DIR)

	mpic++ \
		$(shell find $(SRC_DIR) -name *.cpp) \
		-o $(BIN_DIR)/$(NAME_L) \
		-DCLUSTER_MODE_$(MODE) \
		$(CFLAGS) \
		-Wall -Wpedantic -Wextra

clean:
	rm -rf $(BIN_DIR)
