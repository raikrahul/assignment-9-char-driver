CC ?= gcc
CFLAGS ?= -g -Wall -Werror -O2
TARGET ?= aesdsocket
LDFLAGS ?= 

CROSS_COMPILE ?=

all: $(TARGET)
default: $(TARGET)

$(TARGET): $(TARGET).c
	$(CROSS_COMPILE)$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(TARGET).c

clean:
	rm -f $(TARGET) *.o

.PHONY: all default clean
