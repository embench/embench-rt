# Copyright(C) 2019 Hex Five Security, Inc.
# 10-MAR-2019 Cesare Garlati

#############################################################
# GNU Toolchain definitions
#############################################################

ifndef RISCV
$(error RISCV not set)
endif

export CROSS_COMPILE := $(abspath $(RISCV))/bin/riscv64-unknown-elf-
export CC      := $(CROSS_COMPILE)gcc
export OBJDUMP := $(CROSS_COMPILE)objdump
export OBJCOPY := $(CROSS_COMPILE)objcopy
export GDB     := $(CROSS_COMPILE)gdb
export AR      := $(CROSS_COMPILE)ar


#############################################################
# Platform definitions
#############################################################

BOARD ?= X300
ifeq ($(BOARD),E31)
	RISCV_ARCH := rv32imac
	RISCV_ABI := ilp32
else ifeq ($(BOARD),N25)
	RISCV_ARCH := rv32imac
	RISCV_ABI := ilp32
else ifeq ($(BOARD),X300)
	RISCV_ARCH := rv32imac
	RISCV_ABI := ilp32
else
	$(error Unsupported board $(BOARD))
endif

BSP_BASE := ../bsp
BSP_DIR := $(BSP_BASE)/$(BOARD)

#############################################################
# Arguments/variables available to all submakes
#############################################################

export RISCV_ARCH
export RISCV_ABI
export BSP_BASE
export BSP_DIR

#############################################################
# Rules for building all benchmarks
#############################################################

.PHONY: all 
all: 
	$(MAKE) -C ctx_switch

.PHONY: clean
clean: 
	$(MAKE) -C ctx_switch clean


#############################################################
# Load benchmark
#############################################################

TEST ?= ctx_switch

ifndef OPENOCD
$(error OPENOCD not set)
endif

OPENOCD := $(abspath $(OPENOCD))/bin/openocd

OPENOCDCFG ?= bsp/$(BOARD)/openocd.cfg
OPENOCDARGS += -f $(OPENOCDCFG)

GDB_PORT ?= 3333
GDB_LOAD_ARGS ?= --batch
GDB_LOAD_CMDS += -ex "set mem inaccessible-by-default off"
GDB_LOAD_CMDS += -ex "set remotetimeout 240"
GDB_LOAD_CMDS += -ex "target extended-remote localhost:$(GDB_PORT)"
GDB_LOAD_CMDS += -ex "monitor reset halt"
GDB_LOAD_CMDS += -ex "monitor flash protect 0 64 last off"
GDB_LOAD_CMDS += -ex "load"
GDB_LOAD_CMDS += -ex "monitor resume"
GDB_LOAD_CMDS += -ex "monitor shutdown"
GDB_LOAD_CMDS += -ex "quit"

.PHONY: load
load:
	$(OPENOCD) $(OPENOCDARGS) & \
	$(GDB) $(TEST)/$(TEST).hex $(GDB_LOAD_ARGS) $(GDB_LOAD_CMDS)

#############################################################
# Run benchmark
#############################################################

GDB_RUN_ARGS ?= --batch
GDB_RUN_CMDS += -ex "target extended-remote localhost:$(GDB_PORT)"
GDB_RUN_CMDS += -ex "shell clear"
GDB_RUN_CMDS += -ex 'printf "> emBench - running ...\n" '
GDB_RUN_CMDS += -ex "jump start"
GDB_RUN_CMDS += -ex 'printf "> emBench - complete\n" '
GDB_RUN_CMDS += -ex 'printf "> emBench - result : " '
GDB_RUN_CMDS += -ex 'info registers $$mhpmcounter4'
GDB_RUN_CMDS += -ex "monitor shutdown"
GDB_RUN_CMDS += -ex "quit"

.PHONY: run
run:
	$(OPENOCD) $(OPENOCDARGS) 2>/dev/null & \
	$(GDB) $(TEST)/$(TEST).elf $(GDB_RUN_ARGS) $(GDB_RUN_CMDS)
	
