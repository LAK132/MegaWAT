CXX = /usr/bin/g++-8	#bat vcvarsall.bat
CPPVER = c++17
NASM = nasm -felf64

APP = megawat			#bat megawat.exe
OUTDIR = out
BINDIR = bin
LIBDIR = lib
LIBS =					#bat SDL2main.lib SDL2.lib

COMPOPT = -ldl -lSDL2 -pthread -fpermissive	#bat COMPOPT = /nologo /EHa /MD /bigobj
LINKOPT =									#bat LINKOPT = /nologo

DBGCOMPOPT = -g			#bat /Zi
DBGLINKOPT =			#bat /DEBUG

RELCOMPOPT = -DNDEBUG	#bat /DNDEBUG
RELLINKOPT =

SUBMODULES = lak
export SOURCES = megawat src

export lak_SRC = ./include/lak

megawat_SRC = ..
megawat_OBJ = editor.c f65.c globals.c main.c screen.c videomodes.c
megawat_HDR = editor.h f65.h globals.h main.h screen.h videomodes.h memory.h memorymap.h #ascii.h

src_SRC = .
src_OBJ = source.cpp memory.c font.s #bat source.cpp memory.c font.cpp
src_HDR = source.h
src_DEP = cql lak lak_types lak_utils glm sdl stb opengl megawat
src_INC = include

# BUILD SCRIPT

define SUBMODULE_TEMPLATE =
$(1)_MKF = $$($(1)_SRC)/Makefile
$$($(1)_SRC):
	( if [ ! -d $$@ ]; then $$(error Submodule $$@ not found, use `git submodule update --init --recursive` to initialise submodules); fi )
$$($(1)_MKF): | $$($(1)_SRC)
	( if [ ! -f $$@ ]; then $$(error Submodule $$($(1)_SRC) not initialised, use `git submodule update --init --recursive` to initialise submodules) ; fi )
include $$($(1)_MKF)
endef

$(foreach sub,$(SUBMODULES),$(eval $(call SUBMODULE_TEMPLATE,$(sub))))

ALL_DBG = $(foreach src,$(SOURCES),$(foreach obj,$($(src)_OBJ),$(if $(findstring $(suffix $(obj)),.s),nasm,dbg)$(src)$(obj).o))
ALL_REL = $(foreach src,$(SOURCES),$(foreach obj,$($(src)_OBJ),$(if $(findstring $(suffix $(obj)),.s),nasm,rel)$(src)$(obj).o))
ALL_HDR = $(foreach src,$(SOURCES),$(foreach hdr,$($(src)_HDR),$(strip $($(src)_SRC))/$(strip $(hdr))))

$(info $(foreach obj,$(ALL_OBJ),$(obj)))

.PHONY: all
all: release

debug: $(foreach obj,$(ALL_DBG),$(BINDIR)/$(obj)) | $(BINDIR) $(OUTDIR)
	$(CXX) -std=$(CPPVER) $^ $(foreach libdir,$(LIBDIR),-L$(libdir)) $(foreach lib,$(LIBS),-l$(lib)) $(COMPOPT) $(LINKOPT) $(DBGLINKOPT) -o $(OUTDIR)/$(APP)

release: $(foreach obj,$(ALL_REL),$(BINDIR)/$(obj)) | $(BINDIR) $(OUTDIR)
	$(CXX) -std=$(CPPVER) $^ $(foreach libdir,$(LIBDIR),-L$(libdir)) $(foreach lib,$(LIBS),-l$(lib)) $(COMPOPT) $(LINKOPT) $(RELLINKOPT) -o $(OUTDIR)/$(APP)

clean: | $(BINDIR)
	cd $(BINDIR) && rm -f *.o

$(OUTDIR):
	if [ ! -d $@ ]; then mkdir $@; fi
$(BINDIR):
	if [ ! -d $@ ]; then mkdir $@; fi

.DEFAULT:
	$(error Cannot find target $@)

define COMPILE_TEMPLATE =
$(BINDIR)/nasm$(1)$(3).o: $(2)/$(3) $(4)
	$(NASM) -o $$@ $$<
$(BINDIR)/dbg$(1)$(3).o: $(2)/$(3) $(4)
	$(CXX) -std=$(CPPVER) -c $(5) $(DBGCOMPOPT) $(COMPOPT) -o $$@ $$<
$(BINDIR)/rel$(1)$(3).o: $(2)/$(3) $(4)
	$(CXX) -std=$(CPPVER) -c $(5) $(RELCOMPOPT) $(COMPOPT) -o $$@ $$<
endef

$(foreach src,$(SOURCES),\
	$(foreach obj,$($(src)_OBJ),\
		$(eval $(call COMPILE_TEMPLATE,$(src),$(strip $($(src)_SRC)),$(obj),\
			$(foreach header,$(strip $($(src)_HDR)), $(strip $($(src)_SRC))/$(header))\
			$(foreach dep,$(strip $($(src)_DEP)),\
				$(foreach depobj,$(strip $($(dep)_OBJ)), $(strip $($(dep)_SRC))/$(depobj))\
				$(foreach dephdr,$(strip $($(dep)_HDR)), $(strip $($(dep)_SRC))/$(dephdr))\
			),\
			$(foreach inc,$(strip $($(src)_INC)), -I$(strip $(inc)))\
			$(foreach dep,$(strip $($(src)_DEP)), \
				$(if $(strip $($(dep)_SRC)),-I$(strip $($(dep)_SRC)),$(info bad dep $(dep) $($(dep)_SRC)))\
				$(foreach depinc,$(strip $($(dep)_INC)), -I$(strip $(depinc)))\
			)\
		))\
	)\
)