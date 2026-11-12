# Switch File Hub - Nintendo Switch Homebrew
# Requires devkitPro, devkitA64 and libnx.

TARGET      := SwitchFileHub
BUILD       := build
SOURCES     := source
DATA        :=
INCLUDES    :=

APP_TITLE   := Switch File Hub
APP_AUTHOR  := iikawalucas-13
APP_VERSION := 0.1.0

CFLAGS      := -g -Wall -Wextra -O2 -ffunction-sections -fdata-sections
CFLAGS      += $(INCLUDE) -D__SWITCH__
CXXFLAGS    := $(CFLAGS) -fno-rtti -fno-exceptions
ASFLAGS     := -g $(INCLUDE)
LDFLAGS     := -specs=$(DEVKITPRO)/libnx/switch.specs -Wl,-Map,$(notdir $@).map
LIBS        := -lnx

include $(DEVKITPRO)/libnx/switch_rules
