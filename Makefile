K=kernel
U=user
I=include
ODIR=objs

_OBJS = $(patsubst $(K)/%.c,%.o,$(wildcard $(K)/*.c)) $(patsubst $(K)/%.s,%.o,$(wildcard $(K)/*.s)) $(patsubst $(K)/%.cpp,%.o,$(wildcard $(K)/*.cpp))
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))

_USEROBJS = $(patsubst $(U)/%.c,%.o,$(wildcard $(U)/*.c)) $(patsubst $(U)/%.s,%.o,$(wildcard $(U)/*.s)) $(patsubst $(U)/%.cpp,%.o,$(wildcard $(U)/*.cpp))
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
STRIP = $(TOOLPREFIX)strip

CFLAGS = -Wall
CFLAGS += -O0
CFLAGS += -ggdb
CFLAGS += -mcmodel=medany
CFLAGS += -Iinclude

CXXFLAGS = -Wall
CXXFLAGS += -O0
CXXFLAGS += -ggdb
CXXFLAGS += -fpic
CXXFLAGS += -no-pie
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

ASFLAGS = 		\
	-ggdb 		\
	-fpic		\

LDFLAGS = 		\
	-nostdlib	\
	-L$(I)		\

LDLIBS = -lce

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


STARTUSER = 0xffff800000001000

$(I)/libce.a: $(LIBCE_OBJECTS) $(HEADERS)
	$(TOOLPREFIX)ar rcs $@ $(LIBCE_OBJECTS)

$K/kernel: $(OBJS) $(HEADERS) $(I)/libce.a $K/kernel.ld
	$(LD) $(LDFLAGS) -T $K/kernel.ld -o $K/kernel $(OBJS) $(LDLIBS)

$U/user: $(USEROBJS) $(HEADERS) $(I)/libce.a
	$(LD) $(LDFLAGS) -Ttext $(STARTUSER) -o $@ $(USEROBJS) $(LDLIBS)

$U/%.strip: $U/%
	$(STRIP) -s $< -o $@


compile: $K/kernel $U/user.strip

run: $K/kernel $U/user.strip
	$(RUN) -kernel $K/kernel -initrd $U/user.strip

debug: $K/kernel $U/user.strip
	$(DEBUG) -kernel $K/kernel -initrd $U/user.strip

clean:
	rm -f $(ODIR)/*.o $K/kernel $T/kernel_test $U/user_test $(I)/lib*.a
