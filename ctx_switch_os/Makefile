
TARGET := ctx_switch_os.elf
LINKER_SCRIPT := $(BSP_DIR)/link.lds

.PHONY: all
all: $(TARGET)

ASM_SRCS += $(BSP_DIR)/startup.S
ASM_SRCS += source/context-switch-latency-rv.S
C_SRCS += source/context-switch-latency.c

# for new bsp add the following: 
# source/bsp-<bsp-name>.c - bsp interface as required by irq_latency documentation
# source/psp-int-<core-name>.S - non riscv core implementing interrupts
# -D<core-define> - core define isa name
ifeq ($(BOARD),EH1)
   CDEFINES += -DD_RISCV
#else ifeq ($(BOARD),<board-name>)
#   C_SRCS += source/bsp-<bsp-name>.c
#   ASM_SRCS += source/psp-int-<core-name>.S
#   CDEFINES += -DD_CORE_HAS_TRAP -D<core-define>
else
	$(error Unsupported board $(BOARD))
endif

INCLUDES =

ASM_OBJS := $(ASM_SRCS:.S=.o)
C_OBJS := $(C_SRCS:.c=.o)

# D_CYCLES - measure cpu cycles; if not defined, use instructions counter
CDEFINES += -DD_CYCLES

CFLAGS += -march=$(RISCV_ARCH) -mabi=$(RISCV_ABI) -mcmodel=medlow -Os -g3 -ffunction-sections -fdata-sections -Wall

LDFLAGS += -T $(LINKER_SCRIPT) -nostartfiles
LINK_OBJS += $(ASM_OBJS) $(C_OBJS)
LINK_DEPS += $(LINKER_SCRIPT)
CLEAN_OBJS += $(TARGET) $(LINK_OBJS)

HEX = $(subst .elf,.hex,$(TARGET))
LST = $(subst .elf,.lst,$(TARGET))
CLEAN_OBJS += $(HEX)
CLEAN_OBJS += $(LST) 

$(TARGET): $(LINK_OBJS) $(LINK_DEPS)
	$(CC) $(CDEFINES) $(CFLAGS) $(INCLUDES) $(LINK_OBJS) -o $@ $(LDFLAGS)
	$(OBJDUMP) --all-headers --demangle --disassemble --file-headers --wide -DS $(TARGET) > $(LST)

$(ASM_OBJS): %.o: %.S $(HEADERS)
	$(CC) $(CDEFINES) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(C_OBJS): %.o: %.c $(HEADERS)
	$(CC) $(CDEFINES) $(CFLAGS) $(INCLUDES) -c -o $@ $<

.PHONY: clean
clean:
	rm -f $(CLEAN_OBJS) 
