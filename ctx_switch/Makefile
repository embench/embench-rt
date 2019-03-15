# Copyright(C) 2019 Hex Five Security, Inc.
# 10-MAR-2019 Cesare Garlati

TARGET := ctx_switch.elf
LINKER_SCRIPT := $(BSP_DIR)/flash.lds

.PHONY: all
all: $(TARGET)

ASM_SRCS += ctx_switch.S
INCLUDES += -I$(BSP_DIR)

ASM_OBJS := $(ASM_SRCS:.S=.o)

CFLAGS += -march=$(RISCV_ARCH)
CFLAGS += -mabi=$(RISCV_ABI)
CFLAGS += -mcmodel=medany
CFLAGS += -O0
CFLAGS += -g

LDFLAGS += -T $(LINKER_SCRIPT) -nostartfiles
LINK_OBJS += $(ASM_OBJS)
LINK_DEPS += $(LINKER_SCRIPT)
CLEAN_OBJS += $(TARGET) $(LINK_OBJS)

HEX = $(subst .elf,.hex,$(TARGET))
LST = $(subst .elf,.lst,$(TARGET))
CLEAN_OBJS += $(HEX)
CLEAN_OBJS += $(LST) 

$(TARGET): $(LINK_OBJS) $(LINK_DEPS)
	$(CC) $(CFLAGS) $(INCLUDES) $(LINK_OBJS) -o $@ $(LDFLAGS)
	$(OBJCOPY) -O ihex $(TARGET) $(HEX)
	$(OBJDUMP) --all-headers --demangle --disassemble --file-headers --wide -D $(TARGET) > $(LST)

$(ASM_OBJS): %.o: %.S $(HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(C_OBJS): %.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

.PHONY: clean
clean:
	rm -f $(CLEAN_OBJS) 



