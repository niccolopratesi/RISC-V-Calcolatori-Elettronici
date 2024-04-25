K=kernel
U=user
T=test
ODIR=objs

_OBJS = \
  entry.o \
  start.o \
  machine_interrupts.o \
  vga.o \
  pci.o \
  ctors.o \
  uart.o \
  plic.o\

OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))

_PRODOBJS = \
  boot_main.o \

PRODOBJS = $(patsubst %,$(ODIR)/%,$(_PRODOBJS))
  
_TESTOBJS = \
  test_stato_c.o \
  test_stato_asm.o \
  test_main.o \
  test_ctors_asm.o \
  test_keyboard_c.o \
  test_traps_asm.o \
  test_traps_c.o \
  test_paginazione_c.o \

TESTOBJS = $(patsubst %,$(ODIR)/%,$(_TESTOBJS))

_USEROBJS = \
  user_c.o \
  user_asm.o \

USEROBJS = $(patsubst %,$(ODIR)/%,$(_USEROBJS))

LIBCE_CXX_SOURCES:=$(wildcard libCE/*.cpp)
LIBCE_AS_SOURCES:=$(wildcard libCE/as/*.s)
_LIBCE_OBJECTS:=$(notdir $(addsuffix .o, $(basename $(LIBCE_CXX_SOURCES)))) $(notdir $(addsuffix .o, $(basename $(LIBCE_AS_SOURCES))))
LIBCE_OBJECTS = $(patsubst %,$(ODIR)/%,$(_LIBCE_OBJECTS))

HEADERS:=$(wildcard include/*.h)

ifndef TOOLPREFIX
TOOLPREFIX := $(shell if riscv64-unknown-elf-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-unknown-elf-'; \
	elif riscv64-linux-gnu-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-linux-gnu-'; \
	elif riscv64-unknown-linux-gnu-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-unknown-linux-gnu-'; \
	else echo "***" 1>&2; \
	echo "*** Error: Couldn't find a riscv64 version of GCC/binutils." 1>&2; \
	echo "*** To turn off this error, run 'gmake TOOLPREFIX= ...'." 1>&2; \
	echo "***" 1>&2; exit 1; fi)
endif

QEMU = qemu-system-riscv64

CC = $(TOOLPREFIX)gcc
CXX = $(TOOLPREFIX)g++
AS = $(TOOLPREFIX)as
LD = $(TOOLPREFIX)ld

CFLAGS = -Wall
CFLAGS += -O0
CFLAGS += -ggdb
CFLAGS += -mcmodel=medany
CFLAGS += -Iinclude

CXXFLAGS = -Wall
CXXFLAGS += -O0
CXXFLAGS += -ggdb
CXXFLAGS += -fpic
CXXFLAGS += -mcmodel=medany
CXXFLAGS += -Iinclude
CXXFLAGS += -fno-exceptions
CXXFLAGS += -fno-rtti
CXXFLAGS += -fno-stack-protector
CXXFLAGS += -fcf-protection=none
CXXFLAGS += -fno-omit-frame-pointer
CXXFLAGS += -std=c++17
CXXFLAGS += -ffreestanding
CXXFLAGS += -nostdlib

ASFLAGS:= -ggdb

LDFLAGS = -nostdlib

RUNFLAGS = -machine virt
RUNFLAGS += -bios none
RUNFLAGS += -gdb tcp::1234
RUNFLAGS += -m 128M
RUNFLAGS += -device VGA
RUNFLAGS += -vga cirrus
RUNFLAGS += -serial stdio
RUNFLAGS += -smp 1


DEBUGFLAGS = -machine virt
DEBUGFLAGS += -bios none
DEBUGFLAGS += -gdb tcp::1234
DEBUGFLAGS += -m 128M
DEBUGFLAGS += -device VGA
DEBUGFLAGS += -vga cirrus
DEBUGFLAGS += -serial stdio
DEBUGFLAGS += -smp 1
DEBUGFLAGS += -S

RUN = qemu-system-riscv64 $(RUNFLAGS)
DEBUG = qemu-system-riscv64 $(DEBUGFLAGS)

###############
# Implicit build rules to gather all .o files in the objs directory
$(ODIR)/%.o: $K/%.c
	$(CC) -c $(CFLAGS) $< -o $@

$(ODIR)/%.o: $K/%.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(ODIR)/%.o: $K/%.s
	$(AS) $(ASFLAGS) $< -o $@

$(ODIR)/%.o: $T/%.c
	$(CC) -c $(CFLAGS) $< -o $@

$(ODIR)/%.o: $T/%.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(ODIR)/%.o: $T/%.s
	$(AS) $(ASFLAGS) $< -o $@

$(ODIR)/%.o: $U/%.c
	$(CC) -c $(CFLAGS) $< -o $@

$(ODIR)/%.o: $U/%.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(ODIR)/%.o: $U/%.s
	$(AS) $(ASFLAGS) $< -o $@

$(ODIR)/%.o: libCE/%.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(ODIR)/%.o: libCE/as/%.s
	$(AS) $(ASFLAGS) $< -o $@
##################



$K/kernel: $(OBJS) $(PRODOBJS) $(LIBCE_OBJECTS) $(HEADERS) $K/kernel.ld
	$(LD) $(LDFLAGS) -T $K/kernel.ld -o $K/kernel $(OBJS) $(PRODOBJS) $(LIBCE_OBJECTS)

$T/kernel_test: $(OBJS) $(TESTOBJS) $(LIBCE_OBJECTS) $(HEADERS) $K/kernel.ld
	$(LD) $(LDFLAGS) -T $K/kernel.ld -o $T/kernel_test $(OBJS) $(TESTOBJS) $(LIBCE_OBJECTS)

$U/user_test: $(OBJS) $(TESTOBJS) $(USEROBJS) $(LIBCE_OBJECTS) $(HEADERS) $K/kernel.ld
	$(LD) $(LDFLAGS) -T $K/kernel.ld -o $U/user_test $(OBJS) $(TESTOBJS) $(USEROBJS) $(LIBCE_OBJECTS)

compile: $K/kernel

run: $K/kernel
	$(RUN) -kernel $K/kernel

debug: $K/kernel
	$(DEBUG) -kernel $K/kernel

test: $T/kernel_test
	$(RUN) -kernel $T/kernel_test

test_debug: $T/kernel_test
	$(DEBUG) -kernel $T/kernel_test

test_user: $U/user_test
	$(RUN) -kernel $U/user_test

test_user_debug: $U/user_test
	$(DEBUG) -kernel $U/user_test

clean:
	rm $(ODIR)/*.o $K/kernel $T/kernel_test $U/user_test
