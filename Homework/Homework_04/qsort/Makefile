# Makefile for qsort (MIT 6.172 HW4)
# Requires OpenCilk: https://opencilk.org
#
# Usage:
#   make               -- normal parallel build
#   make CILKSAN=1     -- race detection build
#   make CILKSCALE=1   -- scalability analysis build

CC     := /opt/opencilk-2.1.0-x86_64-linux-gnu-ubuntu-22.04/bin/clang
TARGET := qsort

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

$(TARGET): qsort.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGET) *.o
