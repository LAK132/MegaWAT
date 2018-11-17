CC=			gcc
COPT=		-Wall -g -std=gnu99

SRCDIR=		src
OBJDIR=		obj
BINDIR=		bin
ROMDIR=		rom
MKDIRS=		$(SRCDIR) $(OBJDIR) $(BINDIR) $(ROMDIR)

DISK=		$(BINDIR)/DISK.D81
PROGRAM=	$(BINDIR)/megawat+fonts.prg

WGET=		wget
CC65DIR=	cc65
CBMCONVDIR=	cbmconvert
FONTRSTDIR=	c65gs-font-rasteriser
XEMUDIR=	../xemu
COREDIR=	../mega65-core
# CC65DIR=	$(COREDIR)/cc65
# CBMCONVDIR=	$(COREDIR)/cbmconvert
# FONTRSTDIR=	../c65gs-font-rasteriser

C65OPTS=	-t c64 -O -Or -Oi -Os --cpu 65c02 -I$(CC65DIR)/include
L65OPTS=	-C c64-m65.cfg --asm-include-dir $(CC65DIR)/asminc --lib-path $(CC65DIR)/lib

FILES=		$(PROGRAM) \
		$(BINDIR)/loader.prg \
			autoboot.c65 \
			c64-m65.cfg

SOURCES=	$(SRCDIR)/main.c \
			$(SRCDIR)/screen.c \
			$(SRCDIR)/editor.c \
			$(SRCDIR)/videomodes.c \
			$(SRCDIR)/memory.c \
			$(SRCDIR)/fast_memory.s \
			$(SRCDIR)/f65.c \
			$(SRCDIR)/globals.c \
			$(SRCDIR)/serial.c \
			$(SRCDIR)/charset.s \
			$(SRCDIR)/romprotection.s

ASSFILES=	$(OBJDIR)/main.s \
			$(OBJDIR)/screen.s \
			$(OBJDIR)/editor.s \
			$(OBJDIR)/videomodes.s \
			$(OBJDIR)/memory.s \
			$(OBJDIR)/fast_memory.s \
			$(OBJDIR)/f65.s \
			$(OBJDIR)/globals.s \
			$(OBJDIR)/serial.s \
			$(OBJDIR)/charset.s \
			$(OBJDIR)/romprotection.s

HEADERS=	Makefile \
			$(SRCDIR)/main.h \
			$(SRCDIR)/screen.h \
			$(SRCDIR)/videomodes.h \
			$(SRCDIR)/globals.h \
			$(SRCDIR)/memory.h \
			$(SRCDIR)/f65.h \
			$(SRCDIR)/megaint.h \
			$(SRCDIR)/serial.h \
			$(SRCDIR)/ascii.h

DATAFILES=	ascii8x8.bin


CC65=		$(CC65DIR)/bin/cc65
CL65=		$(CC65DIR)/bin/cl65
CA65=		$(CC65DIR)/bin/ca65 --cpu 4510
LD65=		$(CC65DIR)/bin/ld65 -t none

CBMCONVERT=	$(CBMCONVDIR)/cbmconvert

XC65=		$(XEMUDIR)/build/bin/xc65.native

# But use local ROM file if present
F1_EXISTS=$(shell [ -e c65.rom ] && echo 1 || echo 0 )
ifeq ($(F1_EXISTS), 1)
C65SYSROM=	c65.rom
else
C65SYSROM=	$(ROMDIR)/910111-390488-01.bin.rom
endif

MONLOAD=	$(COREDIR)/src/tools/monitor_load
KICKUP=		$(COREDIR)/bin/KICKUP.M65
BITSTRM=	$(COREDIR)/bin/nexys4ddr.bit
CHARROM=	$(COREDIR)/charrom.bin

TTFTOF65=	$(FONTRSTDIR)/ttftof65

# FUNCTIONS

all:		$(FILES) | $(MKDIRS)

clean: clean-bin clean-out clean-font

clean-bin:
	if [ -d $(OBJDIR) ]; then cd $(OBJDIR) && rm -f *; fi

clean-out:
	if [ -d $(BINDIR) ]; then cd $(BINDIR) && rm -f *; fi

clean-rom:
	if [ -d $(ROMDIR) ]; then cd $(ROMDIR) && rm -f *; fi

clean-font:
	rm -f *.f65

# Run the program in XEMU
test:		$(XC65) $(C65SYSROM) $(DISK)
	$(XC65) -rom $(C65SYSROM) -8 $(DISK)

run:		$(MONLOAD) $(C65SYSROM)
	$(MONLOAD) -b $(BITSTRM) -R $(C65SYSROM) -k $(KICKUP) -C $(CHARROM)

# Load the program onto the MEGA65
load:		$(MONLOAD) $(C65SYSROM) $(PROGRAM)
	$(MONLOAD) -b $(BITSTRM) -R $(C65SYSROM) -k $(KICKUP) -C $(CHARROM) -4 -r $(PROGRAM)

# TEMPLATES

.PRECIOUS: $(OBJDIR)/%.s %.f65 $(ROMDIR)/%.rom

%.f65:	%.ttf Makefile $(TTFTOF65)
	$(TTFTOF65) -A -P 8 -T $< -o $@

$(OBJDIR)/%.s:		$(SOURCES) $(HEADERS) $(DATAFILES) $(CC65) | $(OBJDIR)
	if [ -f $(SRCDIR)/$*.c ]; then $(CC65) $(C65OPTS) -o $@ $(SRCDIR)/$*.c; fi
	if [ -f $(SRCDIR)/$*.s ]; then cp $(SRCDIR)/$*.s $@; fi

$(ROMDIR)/%.rom:	| $(ROMDIR)
	$(WGET) -O $@ http://www.zimmers.net/anonftp/pub/cbm/firmware/computers/c65/$* || { rm -f $@ ; false; }

$(OBJDIR)/%.fpk:	Makefile assets/*.ttf assets/*.otf $(TTFTOF65) | assets $(OBJDIR)
	./makefonts.sh $@

$(BINDIR)/%.prg:	$(ASSFILES) c64-m65.cfg | $(BINDIR)
	$(CL65) $(C65OPTS) $(L65OPTS) -vm -m $@.map -o $@ $(ASSFILES)

$(BINDIR)/loader.prg:	src/fast_memory.s src/splash.s src/loader.c src/memory.c src/memory.h Makefile assets/megawat-splash.m65
	$(CL65) $(C65OPTS) $(L65OPTS) -vm -m $@.map -o $@ src/splash.s src/loader.c src/memory.c src/fast_memory.s

$(BINDIR)/megawat+fonts.prg:	Makefile $(OBJDIR)/fontpack.fpk $(BINDIR)/megawat.prg $(C65SYSROM)
	#	Generate single binary with fonts and ROM in place
	dd if=$(BINDIR)/megawat.prg of=$@
	dd if=/dev/zero bs=1024 count=76 of=$@ oflag=append conv=notrunc
	dd if=$(C65SYSROM) bs=1024 count=128 of=$@ oflag=append conv=notrunc
	dd if=$(OBJDIR)/fontpack.fpk bs=1024 count=128 of=$@ oflag=append conv=notrunc

$(BINDIR)/%.D81:	$(CBMCONVERT) $(FILES) | $(BINDIR)
	if [ -f $@ ]; then rm -f $@; fi
	$(CBMCONVERT) -v2 -D8o $@ $(FILES)

define DIR_TEMPLATE =
$(1):
	mkdir $(1)
endef

$(foreach dir,$(MKDIRS),$(eval $(call DIR_TEMPLATE,$(dir))))

# TOOLS

cbmconvert/cbmconvert:
	git submodule init
	git submodule update
	( cd cbmconvert && make -f Makefile.unix )

cc65/bin/cc65:
	git submodule init
	git submodule update
	( cd cc65 && make -j 8 )

$(MONLOAD):
	( cd $(COREDIR) && make src/tools/monitor_load )

$(TTFTOF65):
	( cd $(FONTRSTDIR) ; make )

.DEFAULT:
	$(error Could not find file $@)
