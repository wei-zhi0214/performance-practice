# Makefile for qsort-race (MIT 6.172 HW4)
# Requires OpenCilk: https://opencilk.org
#
# Usage:
#   make             -- normal build (race may not manifest every run)
#   make CILKSAN=1   -- build with race detector (detects race reliably)

CC     := /opt/opencilk-2.1.0-x86_64-linux-gnu-ubuntu-22.04/bin/clang
TARGET := qsort-race

CFLAGS  := -Wall -O2 -fopencilk
LDFLAGS := -fopencilk -lm

ifeq ($(DEBUG),1)
  CFLAGS := -Wall -O0 -g -fopencilk
endif

ifeq ($(CILKSAN),1)
  CFLAGS += -fsanitize=cilk -g
endif

all: $(TARGET)

$(TARGET): qsort-race.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGET) *.o
