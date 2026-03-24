# Makefile for queens (MIT 6.172 HW4)
# Requires OpenCilk: https://opencilk.org
#
# Usage:
#   make               -- serial build (no Cilk yet)
#   make CILKSAN=1     -- race detector (comment out the I-loop in main first)
#   make CILKSCALE=1   -- scalability analysis

CC     := /opt/opencilk-2.1.0-x86_64-linux-gnu-ubuntu-22.04/bin/clang
TARGET := queens

CFLAGS  := -Wall -O2 -fopencilk
LDFLAGS := -fopencilk -lm

ifeq ($(DEBUG),1)
  CFLAGS := -Wall -O0 -g -fopencilk
endif

ifeq ($(CILKSAN),1)
  CFLAGS += -fsanitize=cilk -g
endif

ifeq ($(CILKSCALE),1)
  CFLAGS  += -fcilktool=cilkscale
  LDFLAGS += -lcilkscale
endif

all: $(TARGET)

$(TARGET): queens.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGET) *.o
