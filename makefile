CC = clang
LINK = clang
CFLAGS = -O3 -Wall -Wno-multichar -D_CARYLL_USE_PRE_SERIALIZED
CFLAGS_EXTERN = -O3 -D_CARYLL_USE_PRE_SERIALIZED

TARGETDIR = $(if $(TARGET),build/$(TARGET),build)
DIRS = $(if $(TARGET),build $(TARGETDIR),$(TARGETDIR))

ifeq ($(TARGET), debug)
CFLAGS = -g -Ddebug
CFLAGS_EXTERN = -g -Ddebug
LINKFLAGS = -g -Ddebug
endif

ifeq ($(TARGET), mingw-w32)
CC = gcc
LINK = gcc
CFLAGS = -m32 -O3 -flto -Wall -Wno-multichar -D_CARYLL_USE_PRE_SERIALIZED
CFLAGS_EXTERN = -m32 -O3 -flto -D_CARYLL_USE_PRE_SERIALIZED
LINKFLAGS = -m32
endif

ifeq ($(TARGET), mingw-w64)
CC = gcc
LINK = gcc
CFLAGS = -m64 -O3 -flto -Wall -Wno-multichar -D_CARYLL_USE_PRE_SERIALIZED
CFLAGS_EXTERN = -m64 -O3 -flto -D_CARYLL_USE_PRE_SERIALIZED
LINKFLAGS = -m64
endif

ifeq ($(TARGET), mingw-clang-w64)
CFLAGS = -m64 -O3 -Wall -Wno-multichar -D_CARYLL_USE_PRE_SERIALIZED
CFLAGS_EXTERN = -m64 -O3 -D_CARYLL_USE_PRE_SERIALIZED
LINKFLAGS = -m64
endif

all : objects

ifdef SystemRoot # win32
SUFFIX = .exe
endif

MAIN_OBJECTS_1 = $(TARGETDIR)/caryll-font.o $(TARGETDIR)/caryll-sfnt.o $(TARGETDIR)/caryll-sfnt-builder.o
MAIN_OBJECTS = $(MAIN_OBJECTS_1) $(TARGETDIR)/otfccdump.o $(TARGETDIR)/otfccbuild.o
TABLE_OBJECTS = $(TARGETDIR)/table-head.o $(TARGETDIR)/table-hhea.o $(TARGETDIR)/table-maxp.o \
	$(TARGETDIR)/table-hmtx.o $(TARGETDIR)/table-post.o $(TARGETDIR)/table-hdmx.o \
	$(TARGETDIR)/table-PCLT.o $(TARGETDIR)/table-LTSH.o $(TARGETDIR)/table-vhea.o \
	$(TARGETDIR)/table-OS_2.o $(TARGETDIR)/table-glyf.o $(TARGETDIR)/table-cmap.o \
	$(TARGETDIR)/table-name.o $(TARGETDIR)/table-fpgm-prep.o $(TARGETDIR)/table-gasp.o \
	$(TARGETDIR)/table-vmtx.o

OTL_OBJECTS = $(TARGETDIR)/otl-otl.o $(TARGETDIR)/otl-coverage.o \
	$(TARGETDIR)/otl-classdef.o $(TARGETDIR)/otl-extend.o \
	$(TARGETDIR)/otl-gsub-single.o $(TARGETDIR)/otl-gsub-multi.o \
	$(TARGETDIR)/otl-gsub-ligature.o $(TARGETDIR)/otl-chaining.o \
	$(TARGETDIR)/otl-gpos-common.o $(TARGETDIR)/otl-gpos-single.o \
	$(TARGETDIR)/otl-gpos-pair.o $(TARGETDIR)/otl-gpos-cursive.o \
	$(TARGETDIR)/otl-gpos-mark-to-single.o $(TARGETDIR)/otl-gpos-mark-to-ligature.o \
	$(TARGETDIR)/otl-gsub-reverse.o $(TARGETDIR)/otl-GDEF.o

FONTOP_OBJECTS = $(TARGETDIR)/fontop-unconsolidate.o $(TARGETDIR)/fontop-consolidate.o \
	$(TARGETDIR)/fontop-stat.o

FONTOP_OTL_OBJECTS = $(TARGETDIR)/fopotl-common.o \
	$(TARGETDIR)/fopotl-gsub-single.o $(TARGETDIR)/fopotl-gsub-multi.o \
	$(TARGETDIR)/fopotl-gsub-ligature.o $(TARGETDIR)/fopotl-chaining.o \
	$(TARGETDIR)/fopotl-gpos-single.o $(TARGETDIR)/fopotl-gpos-pair.o \
	$(TARGETDIR)/fopotl-gpos-cursive.o $(TARGETDIR)/fopotl-mark.o \
	$(TARGETDIR)/fopotl-gsub-reverse.o

EXTERN_OBJECTS = $(TARGETDIR)/extern-sds.o $(TARGETDIR)/extern-json.o $(TARGETDIR)/extern-json-builder.o
SUPPORT_OBJECTS = $(TARGETDIR)/support-glyphorder.o $(TARGETDIR)/support-aglfn.o \
              $(TARGETDIR)/support-stopwatch.o $(TARGETDIR)/support-unicodeconv.o \
			  $(TARGETDIR)/support-buffer.o
EXECUTABLES = $(TARGETDIR)/otfccdump$(SUFFIX) $(TARGETDIR)/otfccbuild$(SUFFIX)

OBJECTS = $(EXTERN_OBJECTS) $(SUPPORT_OBJECTS) $(TABLE_OBJECTS) $(OTL_OBJECTS) \
	$(FONTOP_OBJECTS) $(FONTOP_OTL_OBJECTS) $(MAIN_OBJECTS_1) 

$(EXTERN_OBJECTS) : $(TARGETDIR)/extern-%.o : extern/%.c | $(DIRS)
	@echo CC "->" $@
	@$(CC) $(CFLAGS_EXTERN) -c $< -o $@

SUPPORT_H = $(subst .o,.h,$(subst $(TARGETDIR)/support-,src/support/,$(SUPPORT_OBJECTS))) src/support/util.h
$(SUPPORT_OBJECTS) : $(TARGETDIR)/support-%.o : src/support/%.c | $(DIRS)
	@echo CC "->" $@
	@$(CC) $(CFLAGS) -c $< -o $@

TABLES_H = $(subst .o,.h,$(subst $(TARGETDIR)/table-,src/tables/,$(TABLE_OBJECTS)))
$(TABLE_OBJECTS) : $(TARGETDIR)/table-%.o : src/tables/%.c src/tables/%.h $(SUPPORT_H) $(TABLES_H) | $(DIRS)
	@echo CC "->" $@
	@$(CC) $(CFLAGS) -c $< -o $@

OTL_H = $(subst .o,.h,$(subst $(TARGETDIR)/otl-,src/tables/otl/,$(OTL_OBJECTS)))
$(OTL_OBJECTS) : $(TARGETDIR)/otl-%.o : src/tables/otl/%.c src/tables/otl/%.h $(SUPPORT_H) $(OTL_H) | $(DIRS)
	@echo CC "->" $@
	@$(CC) $(CFLAGS) -c $< -o $@

FOPOTL_H = $(subst .o,.h,$(subst $(TARGETDIR)/fopotl-,src/fontops/otl/,$(FONTOP_OTL_OBJECTS)))
$(FONTOP_OTL_OBJECTS) : $(TARGETDIR)/fopotl-%.o : src/fontops/otl/%.c src/fontops/otl/%.h $(SUPPORT_H) $(TABLES_H) $(OTL_H) $(FOPOTL_H) | $(DIRS)
	@echo CC "->" $@
	@$(CC) $(CFLAGS) -c $< -o $@

FONTOPS_H = $(subst .o,.h,$(subst $(TARGETDIR)/fontop-,src/fontops/,$(FONTOP_OBJECTS)))
$(FONTOP_OBJECTS) : $(TARGETDIR)/fontop-%.o : src/fontops/%.c src/fontops/%.h $(SUPPORT_H) $(TABLES_H) $(OTL_H) $(FONTOPS_H) $(FOPOTL_H) | $(DIRS)
	@echo CC "->" $@
	@$(CC) $(CFLAGS) -c $< -o $@

MAIN_H = $(subst .o,.h,$(subst $(TARGETDIR)/,src/,$(MAIN_OBJECTS_1)))
$(MAIN_OBJECTS) : $(TARGETDIR)/%.o : src/%.c $(MAIN_H) $(SUPPORT_H) $(TABLES_H) $(OTL_H) $(FONTOPS_H) $(FOPOTL_H) | $(DIRS)
	@echo CC "->" $@
	@$(CC) $(CFLAGS) -c $< -o $@

$(EXECUTABLES): $(TARGETDIR)/%$(SUFFIX) : $(TARGETDIR)/%.o $(OBJECTS)
	@echo LD "->" $@
	@$(LINK) $(LINKFLAGS) $^ -o $@

objects: $(EXECUTABLES)

TESTFILES = $(TARGETDIR)/test-payload-1$(SUFFIX) $(TARGETDIR)/test-buffer$(SUFFIX)
$(TARGETDIR)/%.o : tests/%.c | $(DIRS)
	@echo CC "->" $@
	@$(CC) $(CFLAGS) -c $^ -o $@
$(TARGETDIR)/%$(SUFFIX) : $(TARGETDIR)/%.o $(OBJECTS)
	@echo LD "->" $@
	@$(LINK) $(LINKFLAGS) $^ -o $@

test: $(TESTFILES)
	@echo "====== Start Test ======"
	@$(TARGETDIR)/test-buffer$(SUFFIX)
	@$(TARGETDIR)/test-payload-1$(SUFFIX) tests/payload/test-out.ttf

debug :
	make TARGET=debug
release :
	make TARGET=release

# directories
build :
	@- mkdir $@

ifdef TARGET
$(TARGETDIR) : | build
	@- mkdir $@
endif
