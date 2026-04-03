CC     = gcc
CFLAGS = -Wall -O2 -g
PARAMS ?=

TARGET = mdriver

SRCS_COMMON = mdriver.c memlib.c
OBJS_COMMON  = $(SRCS_COMMON:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS_COMMON) simple_allocator.o wrapped_allocator.o \
           packed_allocator.o fixed_aligned_allocator.o smart_allocator.o
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(PARAMS) -c -o $@ $<

simple_allocator.o:        simple_allocator.c        allocator_interface.h memlib.h
wrapped_allocator.o:       wrapped_allocator.c       allocator_interface.h memlib.h
packed_allocator.o:        packed_allocator.c        allocator_interface.h memlib.h
fixed_aligned_allocator.o: fixed_aligned_allocator.c allocator_interface.h memlib.h
smart_allocator.o:         smart_allocator.c         allocator_interface.h memlib.h
mdriver.o:                 mdriver.c                 allocator_interface.h memlib.h
memlib.o:                  memlib.c                  memlib.h

autotune:
	@echo "OpenTuner not available in OCW version"

clean:
	rm -f $(TARGET) *.o

.PHONY: all clean autotune
