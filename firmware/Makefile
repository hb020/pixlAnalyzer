PROJECT_NAME     := secure_bootloader_ble_s112_pca10040
TARGETS          := PixlAnalyzerOLED PixlAnalyzerLCD
OUTPUT_DIRECTORY := build
SDK_ROOT := sdk

# Default variant for flashing (can be overridden via cmd line: make flash VARIANT=PixlAnalyzerLCD)
VARIANT ?= PixlAnalyzerOLED

# Apply the linker script to all defined targets
$(foreach target, $(TARGETS), $(eval $(target): LINKER_SCRIPT := loader.ld))

SRC_FILES += \
  $(SDK_ROOT)/modules/nrfx/mdk/gcc_startup_nrf52.S \
  $(SDK_ROOT)/modules/nrfx/mdk/system_nrf52.c \
  $(SDK_ROOT)/components/boards/boards.c \
  main.c \

INC_FOLDERS += \
  $(SDK_ROOT)/components/softdevice/s112/headers \
  $(SDK_ROOT)/components/libraries/util \
  $(SDK_ROOT)/modules/nrfx \
  $(SDK_ROOT)/integration/nrfx \
  $(SDK_ROOT)/components/libraries/delay \
  $(SDK_ROOT)/components/boards \
  . \
  $(SDK_ROOT)/modules/nrfx/hal \
  $(SDK_ROOT)/components/toolchain/cmsis/include \
  $(SDK_ROOT)/modules/nrfx/mdk \

OPT = -Os -g3

CFLAGS += $(OPT)
CFLAGS += -DCONFIG_GPIO_AS_PINRESET
CFLAGS += -DFLOAT_ABI_HARD
CFLAGS += -DNRF52
CFLAGS += -DNRF52832_XXAA
CFLAGS += -DNRF52_PAN_74
CFLAGS += -mcpu=cortex-m4
CFLAGS += -mthumb -mabi=aapcs
CFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
CFLAGS += -ffunction-sections -fdata-sections -fno-strict-aliasing
CFLAGS += -fno-builtin -fshort-enums
CFLAGS += -DBOARD_CUSTOM

CXXFLAGS += $(OPT)
ASMFLAGS += -g3
ASMFLAGS += -mcpu=cortex-m4
ASMFLAGS += -mthumb -mabi=aapcs
ASMFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
ASMFLAGS += -DCONFIG_GPIO_AS_PINRESET
ASMFLAGS += -DFLOAT_ABI_HARD
ASMFLAGS += -DNRF52
ASMFLAGS += -DNRF52832_XXAA
ASMFLAGS += -DNRF52_PAN_74
ASMFLAGS += -DBOARD_CUSTOM

LDFLAGS += $(OPT)
LDFLAGS += -mthumb -mabi=aapcs -L$(SDK_ROOT)/modules/nrfx/mdk -T$(LINKER_SCRIPT)
LDFLAGS += -mcpu=cortex-m4
LDFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
LDFLAGS += -Wl,--gc-sections
LDFLAGS += --specs=nano.specs

# Target Specific Flags
# ---------------------
$(TARGETS): ASMFLAGS += -D__HEAP_SIZE=0

# OLED Specifics
PixlAnalyzerOLED: CFLAGS += -DOLED_TYPE_SH1106

# LCD Specifics
PixlAnalyzerLCD: CFLAGS += -DLCD_TYPE_DEFAULT 

LIB_FILES += -lc -lnosys -lm

# -------------------------------------------------------------------------
# Default target
# 1. Builds all targets (OLED and LCD)
# 2. Generates OTA .zip files for all targets
# -------------------------------------------------------------------------
default: $(TARGETS)
	@echo "------------------------------------------------"
	@echo "Generating OTA Packages (.zip) for all targets..."
	@echo "------------------------------------------------"
	$(foreach target, $(TARGETS), \
		echo "Creating $(target).zip ..."; \
		nrfutil pkg generate --application $(OUTPUT_DIRECTORY)/$(target).hex --application-version 99 --hw-version 52 --sd-req 0x0103 --key-file priv.pem $(target).zip; \
	)
	@echo "Done."

TEMPLATE_PATH := $(SDK_ROOT)/components/toolchain/gcc
include $(TEMPLATE_PATH)/Makefile.common

$(foreach target, $(TARGETS), $(call define_target, $(target)))

.PHONY: flash

# Flash target
# Usage: 
#   make flash                        (Flashes OLED)
#   make flash VARIANT=PixlAnalyzerLCD (Flashes LCD)
flash: default
	@echo "------------------------------------------------"
	@echo "Flashing Target: $(VARIANT)"
	@echo "------------------------------------------------"
	@echo "device nrf52832_xxaa" > flash.jlink
	@echo "power on" >> flash.jlink
	@echo "if swd" >> flash.jlink
	@echo "speed 4000" >> flash.jlink
	@echo "loadfile $(OUTPUT_DIRECTORY)/$(VARIANT).hex" >> flash.jlink
	@echo "r" >> flash.jlink
	@echo "g" >> flash.jlink
	@echo "exit" >> flash.jlink
	"C:\Program Files\SEGGER\JLink_V888\JLink.exe" -CommandFile flash.jlink
	@rm flash.jlink
