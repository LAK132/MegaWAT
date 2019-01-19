CC=			gcc
COPT=		-Wall -g -std=gnu99

SRCDIR=		src
OBJDIR=		obj
BINDIR=		bin
ROMDIR=		rom
ASTDIR=		assets
MKDIRS=		$(SRCDIR) $(OBJDIR) $(BINDIR) $(ROMDIR)

DISK=		$(BINDIR)/MEGAWAT.D81
PROGRAM=	$(BINDIR)/megawat.prg
SERIAL=		/dev/ttyUSB1

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
			$(SRCDIR)/megastring.c \
			$(SRCDIR)/editor.c \
			$(SRCDIR)/videomodes.c \
			$(SRCDIR)/memory.c \
			$(SRCDIR)/fast_memory.s \
			$(SRCDIR)/f65.c \
			$(SRCDIR)/globals.c \
			$(SRCDIR)/fileio.c \
			$(SRCDIR)/font_load.s \
			$(SRCDIR)/charset.s \
			$(SRCDIR)/romprotection.s

ASSFILES=	$(OBJDIR)/main.s \
			$(OBJDIR)/megastring.s \
			$(OBJDIR)/editor.s \
			$(OBJDIR)/videomodes.s \
			$(OBJDIR)/memory.s \
			$(OBJDIR)/fast_memory.s \
			$(OBJDIR)/f65.s \
			$(OBJDIR)/globals.s \
			$(OBJDIR)/fileio.s \
			$(OBJDIR)/font_load.s \
			$(OBJDIR)/charset.s \
			$(OBJDIR)/romprotection.s

HEADERS=	Makefile \
			c64-m65.cfg \
			$(SRCDIR)/main.h \
			$(SRCDIR)/megastring.h \
			$(SRCDIR)/editor.h \
			$(SRCDIR)/screen.h \
			$(SRCDIR)/videomodes.h \
			$(SRCDIR)/globals.h \
			$(SRCDIR)/memory.h \
			$(SRCDIR)/f65.h \
			$(SRCDIR)/megaint.h \
			$(SRCDIR)/serial.h \
			$(SRCDIR)/fileio.h \
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
C65SYSROM=	$(ROMDIR)/911001.bin.rom
endif

MONLOAD=	$(COREDIR)/src/tools/monitor_load
KICKUP=		$(COREDIR)/bin/KICKUP.M65
BITSTRM=	$(COREDIR)/bin/nexys4ddr-widget-20190116.16-newpix-b686673+DIRTY.bit
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
	$(MONLOAD) -b $(BITSTRM) -l $(SERIAL)
	# $(MONLOAD) -b $(BITSTRM) -R $(C65SYSROM) -k $(KICKUP) -C $(CHARROM) -l $(SERIAL)

# Load the program onto the MEGA65
load:		$(MONLOAD) $(C65SYSROM) $(PROGRAM)
	$(MONLOAD) -b $(BITSTRM) -4 -r $(PROGRAM) -l $(SERIAL)
	# $(MONLOAD) -b $(BITSTRM) -R $(C65SYSROM) -k $(KICKUP) -C $(CHARROM) -4 -r $(PROGRAM) -l $(SERIAL)

# TEMPLATES

.PRECIOUS: $(OBJDIR)/%.s $(OBJDIR)/%.FPK %.f65 $(ROMDIR)/%.rom

%.f65:	%.ttf Makefile $(TTFTOF65)
	$(TTFTOF65) -A -P 8 -T $< -o $@

$(OBJDIR)/%.s:		$(SOURCES) $(HEADERS) $(DATAFILES) $(CC65) | $(OBJDIR)
	if [ -f $(SRCDIR)/$*.c ]; then $(CC65) $(C65OPTS) -o $@ $(SRCDIR)/$*.c; fi
	if [ -f $(SRCDIR)/$*.s ]; then cp $(SRCDIR)/$*.s $@; fi

$(ROMDIR)/%.rom:	| $(ROMDIR)
	$(WGET) -O $@ http://www.zimmers.net/anonftp/pub/cbm/firmware/computers/c65/$* || { rm -f $@ ; false; }

$(OBJDIR)/%.FPK:	Makefile $(ASTDIR)/*.ttf $(ASTDIR)/*.otf $(TTFTOF65) | $(ASTDIR) $(OBJDIR)
	./makefonts.sh $@ > /dev/null

$(BINDIR)/%.prg:	$(ASSFILES) c64-m65.cfg | $(BINDIR)
	$(CL65) $(C65OPTS) $(L65OPTS) -vm -m $@.map -o $@ $(ASSFILES)

$(BINDIR)/loader.prg:	src/fast_memory.s src/splash.s src/loader.c src/memory.c src/memory.h Makefile $(ASTDIR)/megawat-splash.m65
	$(CL65) $(C65OPTS) $(L65OPTS) -vm -m $@.map -o $@ src/splash.s src/loader.c src/memory.c src/fast_memory.s

$(BINDIR)/%.fprg:	Makefile $(OBJDIR)/%.FPK $(BINDIR)/%.prg $(C65SYSROM)
	#	Generate single binary with fonts and ROM in place
	dd if=$(BINDIR)/$*.prg of=$@
	dd if=/dev/zero bs=1024 count=12 of=$@ oflag=append conv=notrunc
	dd if=$(ASTDIR)/dosram.bin bs=1024 count=8 of=$@ oflag=append conv=notrunc
	dd if=/dev/zero bs=1024 count=56 of=$@ oflag=append conv=notrunc
	dd if=$(C65SYSROM) bs=1024 count=128 of=$@ oflag=append conv=notrunc
	dd if=$(OBJDIR)/$*.FPK bs=1024 count=128 of=$@ oflag=append conv=notrunc

$(BINDIR)/MEGAWAT.D81:	assets/PART1 bin/loader.prg bin/megawat.prg assets/*.mwt*
	rm -fr $(BINDIR)/MEGAWAT.D81 tmp
	mkdir tmp
	cp assets/*.mwt* tmp/
	cp assets/PART1 tmp/part1
	cp bin/loader.prg tmp/megawat
	cp bin/megawat.prg tmp/part2
	(cd tmp ; cbmconvert -D8 ../$(BINDIR)/MEGAWAT.D81 * )

#$(BINDIR)/%.D81:	$(CBMCONVERT) $(FILES) | $(BINDIR)
#	if [ -f $@ ]; then rm -f $@; fi
#	$(CBMCONVERT) -v2 -D8o $@ $(FILES)

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
