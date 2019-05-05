vpath %.s hal/rda/device/TOOLCHAIN_GCC_ARM
vpath %.c main hal/rda hal/rda/device/rda5981 bootloader bootloader/hw/rda5981
vpath %.h hal/rda hal/rda/device/rda5981 bootloader bootloader/hw bootloader/hw/rda5981 RTE

DEFS += -DUNO_81C_U04

INCS += -Imain -Ihal/rda -Ihal/rda/device/rda5981 -Ibootloader -Ibootloader/hw -Ibootloader/hw/rda5981 -IRTE

OBJS += main/main.o main/retarget.o
OBJS += hal/rda/board.o hal/rda/drv_uart.o hal/rda/rda_api.o
OBJS += hal/rda/device/rda5981/system_rda5981.o
OBJS += hal/rda/device/TOOLCHAIN_GCC_ARM/startup_rda5981.o 
OBJS += bootloader/bootloader.o bootloader/bootloader_copy.o bootloader/bootloader_ping_pong.o 
OBJS += bootloader/bootloader_triple_area.o bootloader/crc32.o bootloader/firmware.o bootloader/image.o
OBJS += bootloader/hw/rda5981/flash.o bootloader/hw/rda5981/flash_layout.o 

CFLAGS += -mcpu=cortex-m4 -mthumb -Wall
CFLAGS += -Os
CFLAGS += -ffunction-sections -fdata-sections

LFLAGS += -mcpu=cortex-m4 -mthumb
LFLAGS += -specs=nano.specs -specs=nosys.specs
LFLAGS += -Wl,--gc-sections 

all:bootloader.bin

clean:
	@rm -f $(OBJS) bootloader.bin bootloader.elf bootloader.asm
	
bootloader.bin:bootloader.elf
	@arm-none-eabi-objcopy -O binary -S $< $@
	@arm-none-eabi-objdump -d $< > bootloader.asm

bootloader.elf:$(OBJS)
	@arm-none-eabi-gcc $(LFLAGS) $^ -Thal/rda/device/TOOLCHAIN_GCC_ARM/rda5981.ld -o $@
	@arm-none-eabi-size $@
	
%.o:%.s
	@arm-none-eabi-gcc $(CFLAGS) -c $< -o $@

%.o:%.c
	@arm-none-eabi-gcc $(CFLAGS) $(DEFS) $(INCS) -c $< -o $@
