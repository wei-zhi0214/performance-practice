# Makefile for transpose (MIT 6.172 HW4)
# Requires OpenCilk: https://opencilk.org

CC     := /opt/opencilk-2.1.0-x86_64-linux-gnu-ubuntu-22.04/bin/clang
TARGET := transpose

CFLAGS  := -Wall -O2 -fopencilk
LDFLAGS := -fopencilk -lm

ifeq ($(DEBUG),1)
  CFLAGS := -Wall -O0 -g -fopencilk
endif

ifeq ($(CILKSAN),1)
  CFLAGS += -fsanitize=cilk -g
endif

all: $(TARGET)

$(TARGET): transpose.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGET) *.o
