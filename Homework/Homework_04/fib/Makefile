# Makefile for fib (MIT 6.172 HW4)
# Requires OpenCilk: https://opencilk.org
#
# Usage:
#   make             -- normal build
#   make CILKSAN=1   -- build with race detector
#   make DEBUG=1     -- debug build (-O0 -g)

CC     := /opt/opencilk-2.1.0-x86_64-linux-gnu-ubuntu-22.04/bin/clang
TARGET := fib

CFLAGS  := -Wall -O2 -fopencilk
LDFLAGS := -fopencilk -lm

ifeq ($(DEBUG),1)
  CFLAGS := -Wall -O0 -g -fopencilk
endif

ifeq ($(CILKSAN),1)
  CFLAGS  += -fsanitize=cilk -g
endif

all: $(TARGET)

$(TARGET): fib.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGET) *.o
