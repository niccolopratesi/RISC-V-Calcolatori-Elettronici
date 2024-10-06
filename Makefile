K=kernel
U=user
I=include
IO=io
ODIR=objs
B=build

compile: $B/kernel $B/user.strip $B/io.strip

_OBJS = $(patsubst $(K)/%.c,%.o,$(wildcard $(K)/*.c)) $(patsubst $(K)/%.s,%.o,$(wildcard $(K)/*.s)) $(patsubst $(K)/%.cpp,%.o,$(wildcard $(K)/*.cpp))
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))

_USEROBJS = $(patsubst $(U)/%.c,%.o,$(wildcard $(U)/*.c)) $(patsubst $(U)/%.s,%.o,$(wildcard $(U)/*.s)) $(patsubst $(U)/%.cpp,%.o,$(wildcard $(U)/*.cpp))
USEROBJS = $(patsubst %,$(ODIR)/%,$(_USEROBJS))

_IO_OBJS = $(patsubst $(IO)/%.c,%.o,$(wildcard $(IO)/*.c)) $(patsubst $(IO)/%.s,%.o,$(wildcard $(IO)/*.s)) $(patsubst $(IO)/%.cpp,%.o,$(wildcard $(IO)/*.cpp))
IO_OBJS = $(patsubst %,$(ODIR)/%,$(_IO_OBJS))

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
# TODO: controllare l'indirizzo di caricamento dell'initrd. In UBUNTU è diverso da Debian.
# UBUNTU modifica USER_MOD_START in costanti.h
ifdef UBUNTU
CFLAGS   += -DUBUNTU
CXXFLAGS += -DUBUNTU
endif

ASFLAGS = 		\
	-g 			\
	-fpic		\

LDFLAGS = 		\
	-nostdlib	\
	-L$(B)		\

LDLIBS = -lce

RUNFLAGS = -machine virt,aia=aplic-imsic
RUNFLAGS += -bios none
RUNFLAGS += -gdb tcp::1234
RUNFLAGS += -m 128M
RUNFLAGS += -device VGA
RUNFLAGS += -device virtio-keyboard
RUNFLAGS += -serial stdio
RUNFLAGS += -smp 1


DEBUGFLAGS = -machine virt,aia=aplic-imsic
DEBUGFLAGS += -bios none
DEBUGFLAGS += -gdb tcp::1234
DEBUGFLAGS += -m 128M
DEBUGFLAGS += -device VGA
DEBUGFLAGS += -device virtio-keyboard
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

$(ODIR)/io_c.o: $(IO)/io.c
	$(CC) -c $(CFLAGS) $< -o $@

$(ODIR)/io_cpp.o: $(IO)/io.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(ODIR)/io_s.o: $(IO)/io.s
	$(AS) $(ASFLAGS) $< -o $@

$(ODIR)/%.o: libCE/%.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(ODIR)/%.o: libCE/as/%.s
	$(AS) $(ASFLAGS) $< -o $@
##################


STARTUSER = 0xffff800000001000
#regione numero 2(partendo da 0) di livello massimo riservata 
#ad inidirizzi virtuali per il sistema
#inizio spazio di memoria condivisa I/O
#lasciamo libera la prima pagina per eventuali header
START_IO = 0x0000010000001000

$(B)/libce.a: $(LIBCE_OBJECTS) $(HEADERS)
	$(TOOLPREFIX)ar rcs $@ $(LIBCE_OBJECTS)

 $B/kernel: $(OBJS) $(HEADERS) $B/libce.a $K/kernel.ld
	$(LD) $(LDFLAGS) -T $K/kernel.ld -o $@ $(OBJS) $(LDLIBS)

 $B/user: $(USEROBJS) $(HEADERS) $B/libce.a
	$(LD) $(LDFLAGS) -Ttext $(STARTUSER) -o $@ $(USEROBJS) $(LDLIBS)

 $B/user.strip: $B/user
	$(STRIP) -s $< -o $@

 $(B)/io: objs/io_s.o objs/io_cpp.o $(HEADERS) $B/libce.a
	$(LD) $(LDFLAGS) -Ttext $(START_IO) -o $@ objs/io_s.o objs/io_cpp.o $(LDLIBS)

 $(B)/io.strip: $B/io
	$(STRIP) -s $< -o $@

$B/moduli.strip: $B/io.strip $B/user.strip
	(printf "%d\n%d\n" $$(stat -c%s $B/io.strip) $$(stat -c%s $B/user.strip); cat $B/io.strip $B/user.strip) > $@

run: build $B/kernel $B/moduli.strip
	$(RUN) -kernel $B/kernel -initrd $B/moduli.strip

debug: $B/kernel $B/moduli.strip
	$(DEBUG) -kernel $B/kernel -initrd $B/moduli.strip

libce: $B/libce.a

build:
	mkdir -p $@

#keeps make from doing something with a file named clean
.PHONY: clean

clean:
	rm -f $(ODIR)/*.o $B/kernel $B/user $B/user.strip $B/io $B/io.strip $B/lib*.a $B/moduli.strip
