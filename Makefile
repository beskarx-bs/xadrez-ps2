
EE_BIN = xadrez.elf
EE_OBJS = src/main.o

EE_CFLAGS += -O2 -Wall

all: $(EE_BIN)

clean:
	rm -f $(EE_BIN) $(EE_OBJS)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
