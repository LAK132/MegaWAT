CC=			gcc
COPT=		-Wall -g -std=gnu99

SRCDIR=		src
BINDIR=		bin
OUTDIR=		out
MKDIRS=		$(SRCDIR) $(BINDIR) $(OUTDIR)

DISK=		$(OUTDIR)/DISK.D81
PROGRAM=	$(OUTDIR)/megawat+fonts.prg

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
			autoboot.c65 \
			c64-m65.cfg

SOURCES=	$(SRCDIR)/main.c \
			$(SRCDIR)/screen.c \
			$(SRCDIR)/editor.c \
			$(SRCDIR)/videomodes.c \
			$(SRCDIR)/memory.c \
			$(SRCDIR)/f65.c \
			$(SRCDIR)/globals.c \
			$(SRCDIR)/charset.s \
			$(SRCDIR)/font.s \
			$(SRCDIR)/romprotection.s

ASSFILES=	$(BINDIR)/main.s \
			$(BINDIR)/screen.s \
			$(BINDIR)/editor.s \
			$(BINDIR)/videomodes.s \
			$(BINDIR)/memory.s \
			$(BINDIR)/f65.s \
			$(BINDIR)/globals.s \
			$(BINDIR)/charset.s \
			$(BINDIR)/font.s \
			$(BINDIR)/romprotection.s

HEADERS=	Makefile \
			$(SRCDIR)/main.h \
			$(SRCDIR)/screen.h \
			$(SRCDIR)/videomodes.h \
			$(SRCDIR)/globals.h \
			$(SRCDIR)/memory.h \
			$(SRCDIR)/f65.h \
			$(SRCDIR)/megaint.h \
			$(SRCDIR)/ascii.h

DATAFILES=	ascii8x8.bin \
			example.f65

CC65=		$(CC65DIR)/bin/cc65
CL65=		$(CC65DIR)/bin/cl65
CA65=		$(CC65DIR)/bin/ca65 --cpu 4510
LD65=		$(CC65DIR)/bin/ld65 -t none

CBMCONVERT=	$(CBMCONVDIR)/cbmconvert

ROMDIR=		$(XEMUDIR)/rom
XC65=		$(XEMUDIR)/build/bin/xc65.native
C65SYSROM=	$(ROMDIR)/c65-system.rom

MONLOAD=	$(COREDIR)/src/tools/monitor_load
KICKUP=		$(COREDIR)/bin/KICKUP.M65
BITSTRM=	$(COREDIR)/bin/nexys4ddr.bit
CHARROM=	$(COREDIR)/charrom.bin

TTFTOF65=	$(FONTRSTDIR)/ttftof65

# FUNCTIONS

all:		$(FILES) | $(MKDIRS)

clean: clean-bin clean-out clean-font

clean-bin:
	if [ -d $(BINDIR) ]; then cd $(BINDIR) && rm -f *; fi

clean-out:
	if [ -d $(OUTDIR) ]; then cd $(OUTDIR) && rm -f *; fi

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

.PRECIOUS: $(BINDIR)/%.s %.f65

%.f65:	%.ttf Makefile $(TTFTOF65)
	$(TTFTOF65) -A -P 8 -T $< -o $@

$(BINDIR)/%.s:		$(SOURCES) $(HEADERS) $(DATAFILES) $(CC65) | $(BINDIR)
	if [ -f $(SRCDIR)/$*.c ]; then $(CC65) $(C65OPTS) -o $@ $(SRCDIR)/$*.c; fi
	if [ -f $(SRCDIR)/$*.s ]; then cp $(SRCDIR)/$*.s $@; fi

c65.rom:
	echo "You must have a 128KB C65-style ROM file called c65.rom"
	echo "for some targets."

fontpack.bin:	assets/	makefonts.sh $(TTFTOF65) /bin/csh
	./makefonts.sh

$(OUTDIR)/megawat+fonts.prg:	$(OUTDIR)/megawat.prg c65.rom fontpack.bin Makefile
	# Assemble MegaWat!? program with C65 ROM and fonts for simple
	# in-memory testing, while we work in implementing loading fonts from disk.
	# We need to add 12KB of padding to end of program, then 2nd 64KB RAM, then the ROM, then
	# the fonts with the $0801 prefix stripped
	dd if=$(OUTDIR)/megawat.prg of=$@
	dd if=/dev/zero of=$@ oflag=append conv=notrunc bs=1024 count=12
	dd if=/dev/zero of=$@ oflag=append conv=notrunc bs=1024 count=64
	dd if=c65.rom of=$@ oflag=append conv=notrunc bs=1024 count=128
	# Now the fonts
	dd if=fontpack.bin of=$@ oflag=append conv=notrunc bs=1024 count=128

$(OUTDIR)/%.prg:	$(ASSFILES) c64-m65.cfg | $(OUTDIR)
	$(CL65) $(C65OPTS) $(L65OPTS) -vm -m $@.map -o $@ $(ASSFILES)

$(OUTDIR)/%.D81:	$(CBMCONVERT) $(FILES) | $(OUTDIR)
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
