ifeq ($(strip $(DEVKITARM)),)
$(error "DEVKITARM is not set")
endif
include $(DEVKITARM)/3ds_rules

TARGET  := MissingGames
CTRULIB := $(DEVKITPRO)/libctru
CC      := arm-none-eabi-gcc
ARCH    := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS  := -g -Wall -O2 -mword-relocations -ffunction-sections $(ARCH) -I$(CTRULIB)/include -D__3DS__ -std=gnu11
LDFLAGS := -specs=3dsx.specs -g $(ARCH) -L$(CTRULIB)/lib
LIBS    := -lctru -lm
OBJS    := $(patsubst %.c,%.o,$(wildcard source/*.c))

all: $(TARGET).3dsx

$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o $@

$(TARGET).smdh:
	smdhtool --create "Missing Games" "Find uninstalled 3DS games" "dd" $(CTRULIB)/default_icon.png $@

$(TARGET).3dsx: $(TARGET).elf $(TARGET).smdh
	3dsxtool $(TARGET).elf $@ --smdh=$(TARGET).smdh

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f source/*.o *.elf *.smdh *.3dsx
