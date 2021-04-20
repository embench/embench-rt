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
else ifeq ($(BOARD),EH1)
	RISCV_ARCH := rv32imc
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
# Rules for building single benchmark
#############################################################
.PHONY: ctx_switch 
ctx_switch: 
	$(MAKE) -C ctx_switch

.PHONY: irq_latency 
irq_latency: 
	$(MAKE) -C irq_latency

.PHONY: ctx_switch_os
ctx_switch_os: 
	$(MAKE) -C ctx_switch_os

#############################################################
# Rules for building all benchmarks
#############################################################

.PHONY: all 
all: 
	$(MAKE) -C ctx_switch
	$(MAKE) -C irq_latency
	$(MAKE) -C ctx_switch_os

.PHONY: clean
clean: 
	$(MAKE) -C ctx_switch clean
	$(MAKE) -C irq_latency clean
	$(MAKE) -C ctx_switch_os clean


#############################################################
# Load benchmark
#############################################################

TEST ?= ctx_switch

ifndef OPENOCD
$(error OPENOCD not set)
endif

OPENOCD := $(abspath $(OPENOCD))/openocd

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
# GDB args: ctx_switch benchmark
#############################################################

GDB_RUN_ARGS_ctx_switch ?= --batch
GDB_RUN_CMDS_ctx_switch += -ex "target extended-remote localhost:$(GDB_PORT)"
GDB_RUN_CMDS_ctx_switch += -ex "shell clear"
GDB_RUN_CMDS_ctx_switch += -ex 'printf "> emBench - running ...\n" '
GDB_RUN_CMDS_ctx_switch += -ex "jump start"
GDB_RUN_CMDS_ctx_switch += -ex 'printf "> emBench - complete\n" '
GDB_RUN_CMDS_ctx_switch += -ex 'printf "> emBench - result : " '
GDB_RUN_CMDS_ctx_switch += -ex 'info registers $$mhpmcounter4'
GDB_RUN_CMDS_ctx_switch += -ex "monitor shutdown"
GDB_RUN_CMDS_ctx_switch += -ex "quit"

#############################################################
# GDB args: ctx_switch_os benchmark
#############################################################

GDB_RUN_ARGS_ctx_switch_os ?= 
GDB_RUN_CMDS_ctx_switch_os += -ex "target remote localhost:$(GDB_PORT)"
GDB_RUN_CMDS_ctx_switch_os += -ex "set mem inaccessible-by-default off"
GDB_RUN_CMDS_ctx_switch_os += -ex "set remotetimeout 250"
GDB_RUN_CMDS_ctx_switch_os += -ex "set arch riscv:rv32"
GDB_RUN_CMDS_ctx_switch_os += -ex "load"
# OpenOCD will execute Fence + Fence.i when resuming
# the processor from the debug mode. This is needed for proper operation
# of SW breakpoints with ICACHE
GDB_RUN_CMDS_ctx_switch_os += -ex "si"
GDB_RUN_CMDS_ctx_switch_os += -ex "c"
GDB_RUN_CMDS_ctx_switch_os += -ex 'printf "\n" '
GDB_RUN_CMDS_ctx_switch_os += -ex 'printf "\n" '
GDB_RUN_CMDS_ctx_switch_os += -ex 'printf "> ctx_switch_os: event_set cycles ...\n" '
GDB_RUN_CMDS_ctx_switch_os += -ex "p g_num_of_cycles_event_set_end"
GDB_RUN_CMDS_ctx_switch_os += -ex 'printf "> ctx_switch_os: semaphore_give cycles  ...\n" '
GDB_RUN_CMDS_ctx_switch_os += -ex "p g_num_of_cycles_semaphore_give_end"
GDB_RUN_CMDS_ctx_switch_os += -ex 'printf "> ctx_switch_os: queue_send cycles ...\n" '
GDB_RUN_CMDS_ctx_switch_os += -ex "p g_num_of_cycles_queue_send_end"
GDB_RUN_CMDS_ctx_switch_os += -ex 'printf "> ctx_switch_os: yield cycles ...\n" '
GDB_RUN_CMDS_ctx_switch_os += -ex "p g_num_of_cycles_task_yield_end"
GDB_RUN_CMDS_ctx_switch_os += -ex 'printf "> ctx_switch_os: Done ...\n" '
GDB_RUN_CMDS_ctx_switch_os += -ex "monitor shutdown"
GDB_RUN_CMDS_ctx_switch_os += -ex "quit"

#############################################################
# GDB args: irq_latency benchmark
#############################################################

GDB_RUN_ARGS_irq_latency ?= 
GDB_RUN_CMDS_irq_latency += -ex "target remote localhost:$(GDB_PORT)"
GDB_RUN_CMDS_irq_latency += -ex "set mem inaccessible-by-default off"
GDB_RUN_CMDS_irq_latency += -ex "set remotetimeout 250"
GDB_RUN_CMDS_irq_latency += -ex "set arch riscv:rv32"
GDB_RUN_CMDS_irq_latency += -ex "load"
# OpenOCD will execute Fence + Fence.i when resuming
# the processor from the debug mode. This is needed for proper operation
# of SW breakpoints with ICACHE
GDB_RUN_CMDS_irq_latency += -ex "si"
GDB_RUN_CMDS_irq_latency += -ex "c"
GDB_RUN_CMDS_irq_latency += -ex 'printf "\n" '
GDB_RUN_CMDS_irq_latency += -ex 'printf "\n" '
GDB_RUN_CMDS_irq_latency += -ex 'printf "> irq_latency: cycles from interrupt -> vect entry ...\n" '
GDB_RUN_CMDS_irq_latency += -ex "p cycles_to_vect_entry"
GDB_RUN_CMDS_irq_latency += -ex 'printf "> irq_latency: cycles from interrupt -> trap entry ...\n" '
GDB_RUN_CMDS_irq_latency += -ex "p cycles_to_trap_entry"
GDB_RUN_CMDS_irq_latency += -ex 'printf "> irq_latency: cycles from interrupt -> isr entry (vector mode) ...\n" '
GDB_RUN_CMDS_irq_latency += -ex "p cycles_to_isr_vect_mode"
GDB_RUN_CMDS_irq_latency += -ex 'printf "> irq_latency: cycles from interrupt -> isr entry (trap mode)  ...\n" '
GDB_RUN_CMDS_irq_latency += -ex "p cycles_to_isr_trap_mode"
GDB_RUN_CMDS_irq_latency += -ex 'printf "> irq_latency: Done ...\n" '
GDB_RUN_CMDS_irq_latency += -ex "monitor shutdown"
GDB_RUN_CMDS_irq_latency += -ex "quit"

#############################################################
# Run benchmark
#############################################################
#	$(OPENOCD) $(OPENOCDARGS) 2>/dev/null & \

.PHONY: run
run:
	$(OPENOCD) $(OPENOCDARGS) & \
	$(GDB) $(TEST)/$(TEST).elf $(GDB_RUN_ARGS_$(TEST)) $(GDB_RUN_CMDS_$(TEST))
	
