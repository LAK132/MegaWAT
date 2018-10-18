CXX = /usr/bin/g++-8	#bat CXX = vcvarsall.bat
CPPVER = c++17

APP = megawat			#bat APP = megawat.exe
OUTDIR = out
BINDIR = bin
LIBDIR = lib
LIBS =					#bat LIBS = SDL2main.lib SDL2.lib

COMPOPT = -ldl -lSDL2 -pthread -lstdc++fs	#bat COMPOPT = /nologo /EHa /MD /bigobj
LINKOPT =									#bat LINKOPT = /nologo

DBGCOMPOPT =			#bat DBGCOMPOPT = /Zi
DBGLINKOPT =			#bat DBGLINKOPT = /DEBUG

RELCOMPOPT = -DNDEBUG	#bat RELCOMPOPT = /DNDEBUG
RELLINKOPT =			#bat RELLINKOPT =

SOURCES = src opengl lak_types lak_utils lak_runtime

opengl_SRC = ./include/GL
opengl_OBJ = gl3w.c
opengl_HDR = gl3w.h glcorearb.h
opengl_INC = include /usr/include/SDL2				#bat lib_INC = include include/SDL

lak_utils_SRC = ./include/lak/utils
lak_utils_OBJ = stream.cpp
lak_utils_HDR = crc32_hash.h ldebug.h obj.h pnm.h stream.h type.h
lak_utils_INC = include/lak

lak_types_SRC = ./include/lak/types
lak_types_OBJ = shader.cpp mesh.cpp json.cpp queue.cpp stride_vector.cpp
lak_types_HDR = shader.h mesh.h json.h queue.h stride_vector.h texture.h type_list.h color.h image.h
lak_types_DEP = lak_utils
lak_types_INC = include include/lak

lak_runtime_SRC = ./include/lak/runtime
lak_runtime_OBJ = mainloop.cpp
lak_runtime_HDR = mainloop.h
lak_runtime_DEP = lak_types opengl
lak_runtime_INC = include include/lak

src_SRC = .
src_OBJ = main.cpp
src_HDR = main.h
src_DEP = opengl lak_types lak_utils lak_runtime
src_INC = include /usr/include/SDL2				#bat src_INC = include include/SDL

# BUILD SCRIPT

ALL_OBJ = $(foreach src,$(SOURCES),$(foreach obj,$($(src)_OBJ),$(src)$(obj).o))
ALL_HDR = $(foreach src,$(SOURCES),$(foreach header,$($(src)_HDR),$($(src)_SRC)/$(header)))

.PHONY: all
all: release

debug: $(BINDIR) $(OUTDIR) $(foreach obj,$(ALL_OBJ),$(BINDIR)/dbg$(obj))
	$(CXX) -std=$(CPPVER) $(foreach obj,$(ALL_OBJ),$(BINDIR)/dbg$(obj)) $(foreach libdir,$(LIBDIR),-L$(libdir)) $(foreach lib,$(LIBS),-l$(lib)) $(COMPOPT) $(LINKOPT) $(DBGLINKOPT) -o $(OUTDIR)/$(APP)

release: $(BINDIR) $(OUTDIR) $(foreach obj,$(ALL_OBJ),$(BINDIR)/rel$(obj))
	$(CXX) -std=$(CPPVER) $(foreach obj,$(ALL_OBJ),$(BINDIR)/rel$(obj)) $(foreach libdir,$(LIBDIR),-L$(libdir)) $(foreach lib,$(LIBS),-l$(lib)) $(COMPOPT) $(LINKOPT) $(RELLINKOPT) -o $(OUTDIR)/$(APP)

clean: $(BINDIR)
	cd $(BINDIR) && rm -f *.o

$(OUTDIR):
	if [ ! -d $@ ]; then mkdir $@; fi
$(BINDIR):
	if [ ! -d $@ ]; then mkdir $@; fi

.DEFAULT:
	$(error Cannot find target $@)

define COMPILE_TEMPLATE =
$(BINDIR)/dbg$(1)$(2).o: $(3)/$(2) $(4)
	$(CXX) -std=$(CPPVER) -c $(5) $(COMPOPT) $(DBGCOMPOPT) -o $$@ $$<
$(BINDIR)/rel$(1)$(2).o: $(3)/$(2) $(4)
	$(CXX) -std=$(CPPVER) -c $(5) $(COMPOPT) $(RELCOMPOPT) -o $$@ $$<
endef

$(foreach src,$(SOURCES),$(foreach obj,$($(src)_OBJ),$(eval $(call COMPILE_TEMPLATE,$(src),$(obj),$($(src)_SRC),$(foreach header,$($(src)_HDR), $($(src)_SRC)/$(header))$(foreach dep,$($(src)_DEP),$(foreach depobj,$($(dep)_OBJ), $($(dep)_SRC)/$(depobj))$(foreach dephdr,$($(dep)_HDR), $($(dep)_SRC)/$(dephdr))),$(foreach inc,$($(src)_INC), -I$(inc))$(foreach dep,$($(src)_DEP), -I$($(dep)_SRC)$(foreach depinc,$($(dep)_INC), -I$(depinc)))))))