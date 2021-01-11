
TARGET := irq_latency.elf
LINKER_SCRIPT := $(BSP_DIR)/link.lds

.PHONY: all
all: $(TARGET)

ASM_SRCS += $(BSP_DIR)/startup.S
ASM_SRCS += source/psp-int-rv.S
C_SRCS += source/bsp-rv-swerv-olof-eh1.c
C_SRCS += source/int-latency.c

INCLUDES =

ASM_OBJS := $(ASM_SRCS:.S=.o)
C_OBJS := $(C_SRCS:.c=.o)

CDEFINES += -DD_CYCLES -DD_64_BIT_CYCLES

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



