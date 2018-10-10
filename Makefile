CC=			gcc
COPT=		-Wall -g -std=gnu99

SRCDIR=		src
BINDIR=		bin
OUTDIR=		out

DISK=		$(OUTDIR)/DISK.D81
PROGRAM=	$(OUTDIR)/megawat.prg

# CC65DIR=	cc65
# CBMCONVDIR=	cbmconvert
XEMUDIR=	../xemu
COREDIR=	../mega65-core
CC65DIR=	$(COREDIR)/cc65
CBMCONVDIR=	$(COREDIR)/cbmconvert

C65OPTS=	-t c64 -O -Or -Oi -Os --cpu 65c02 -I$(CC65DIR)/include
L65OPTS=	-C c64-m65.cfg --asm-include-dir $(CC65DIR)/asminc --lib-path $(CC65DIR)/lib

FILES=		$(PROGRAM) \
			autoboot.c65 \
			c64-m65.cfg

SOURCES=	$(SRCDIR)/main.c \
			$(SRCDIR)/screen.c \
			$(SRCDIR)/memory.c

ASSFILES=	$(BINDIR)/charset.s \
			$(BINDIR)/font.s \
			$(BINDIR)/main.s \
			$(BINDIR)/memory.s \
			$(BINDIR)/screen.s

HEADERS=	Makefile \
			$(SRCDIR)/main.h \
			$(SRCDIR)/screen.h \
			$(SRCDIR)/memory.h \
			$(SRCDIR)/ascii.h

DATAFILES=	ascii8x8.bin \
			font.f65

CC65=		$(CC65DIR)/bin/cc65
CL65=		$(CC65DIR)/bin/cl65
CA65=		$(CC65DIR)/bin/ca65 --cpu 4510
LD65=		$(CC65DIR)/bin/ld65 -t none

CBMCONVERT=	$(CBMCONVDIR)/cbmconvert

XC65=		$(XEMUDIR)/build/bin/xc65.native
C65SYSROM=	$(XEMUDIR)/rom/c65-system.rom

MONLOAD=	$(COREDIR)/src/tools/monitor_load
KICKUP=		$(COREDIR)/bin/KICKUP.M65
BITSTRM=	$(COREDIR)/bin/nexys4ddr.bit
CHARROM=	$(COREDIR)/charrom.bin

# FUNCTIONS

all:		dir-$(OUTDIR) dir-$(BINDIR) dir-$(SRCDIR) $(FILES)

clean:
	if [ -d $(BINDIR) ]; then cd $(BINDIR) && rm -f *; fi

clean-out:
	if [ -d $(OUTDIR) ]; then cd $(OUTDIR) && rm -f *; fi

# Run the program in XEMU
test:		$(XC65) $(C65SYSROM) $(DISK)
	$(XC65) -rom $(C65SYSROM) -8 $(DISK)

# Load the program onto the MEGA65
load:		$(MONLOAD) $(C65SYSROM) $(PROGRAM)
	$(MONLOAD) -b $(BITSTRM) -R $(C65SYSROM) -k $(KICKUP) -C $(CHARROM) -4 -r $(PROGRAM)

# TEMPLATES

dir-%:
	if [ ! -d $* ]; then mkdir $*; fi

.PRECIOUS: $(BINDIR)/%.s

$(BINDIR)/%.s:		$(HEADERS) $(DATAFILES) $(CC65) dir-$(BINDIR)
	if [ -f $(SRCDIR)/$*.c ]; then $(CC65) $(C65OPTS) -o $@ $(SRCDIR)/$*.c; fi
	if [ -f $(SRCDIR)/$*.s ]; then cp $(SRCDIR)/$*.s $@; fi

$(OUTDIR)/%.prg:	$(ASSFILES) c64-m65.cfg dir-$(OUTDIR)
	$(CL65) $(C65OPTS) $(L65OPTS) -vm -m $@.map -o $@ $(ASSFILES)

$(OUTDIR)/%.D81:	$(CBMCONVERT) $(FILES) dir-$(OUTDIR)
	if [ -f $@ ]; then rm -f $@; fi
	$(CBMCONVERT) -v2 -D8o $@ $(FILES)

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
	cd $(COREDIR) && make src/tools/monitor_load