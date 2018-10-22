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

DBGCOMPOPT =			#bat /Zi
DBGLINKOPT =			#bat /DEBUG

RELCOMPOPT = -DNDEBUG	#bat /DNDEBUG
RELLINKOPT =

SOURCES = lak cql opengl sdl lakutils laktypes lakruntime megawat src glm stb

lak_SRC = ./include/lak

glm_SRC = $(lak_SRC)/glm

stb_SRC = $(lak_SRC)/stb

cql_SRC = $(lak_SRC)/cql
cql_HDR = cql.h
cql_DEP = lak

opengl_SRC = $(lak_SRC)/GL
opengl_OBJ = gl3w.c
opengl_HDR = gl3w.h glcorearb.h
opengl_DEP = lak

sdl_SRC = /usr/include/SDL2			#bat sdl_SRC = ./include/lak/SDL
# sdl_LIB = $(sdl_SRC)				#bat sdl_LIB = ./include/lak/SDL

lakutils_SRC = $(lak_SRC)/utils		#bat lak_utils_SRC = ./include/lak/utils
lakutils_OBJ = stream.cpp
lakutils_HDR = crc32_hash.h ldebug.h obj.h pnm.h stream.h type.h
lakutils_INC = $(lak_SRC)
lakutils_DEP = lak

laktypes_SRC = $(lak_SRC)/types		#bat lak_types_SRC = ./include/lak/types
laktypes_OBJ = shader.cpp mesh.cpp json.cpp queue.cpp stride_vector.cpp
laktypes_HDR = shader.h mesh.h json.h queue.h stride_vector.h texture.h type_list.h color.h image.h
lakutils_INC = $(lak_SRC)
laktypes_DEP = lak lakutils sdl glm opengl

lakruntime_SRC = $(lak_SRC)/runtime	#bat lak_runtime_SRC = ./include/lak/runtime
lakruntime_OBJ = mainloop.cpp
lakruntime_HDR = mainloop.h
lakruntime_INC = $(lak_SRC)
lakruntime_DEP = lak laktypes lakutils glm sdl opengl

megawat_SRC = ..
megawat_OBJ = editor.c f65.c globals.c main.c screen.c videomodes.c
megawat_HDR = editor.h f65.h globals.h main.h screen.h videomodes.h memory.h memorymap.h #ascii.h

src_SRC = .
src_OBJ = source.cpp memory.c font.s #bat source.cpp memory.c font.cpp
src_HDR = source.h
src_DEP = cql lak laktypes lakutils glm sdl stb opengl megawat
src_INC = include

# BUILD SCRIPT

ALL_DBG = $(foreach src,$(SOURCES),$(foreach obj,$($(src)_OBJ),$(if $(findstring $(suffix $(obj)),.s),nasm,dbg)$(src)$(obj).o))
ALL_REL = $(foreach src,$(SOURCES),$(foreach obj,$($(src)_OBJ),$(if $(findstring $(suffix $(obj)),.s),nasm,rel)$(src)$(obj).o))
ALL_HDR = $(foreach src,$(SOURCES),$(foreach hdr,$($(src)_HDR),$(strip $($(src)_SRC))/$(strip $(hdr))))

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