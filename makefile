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

ifdef SystemRoot # win32 + tdm-gcc
SUFFIX = .exe
endif

MAIN_OBJECTS_1 = $(TARGETDIR)/caryll-font.o $(TARGETDIR)/caryll-sfnt.o $(TARGETDIR)/caryll-sfnt-builder.o
MAIN_OBJECTS = $(MAIN_OBJECTS_1) $(TARGETDIR)/otfccdump.o $(TARGETDIR)/otfccbuild.o
TABLE_OBJECTS = $(TARGETDIR)/table-head.o $(TARGETDIR)/table-hhea.o $(TARGETDIR)/table-maxp.o \
	$(TARGETDIR)/table-hmtx.o $(TARGETDIR)/table-post.o $(TARGETDIR)/table-hdmx.o \
	$(TARGETDIR)/table-PCLT.o $(TARGETDIR)/table-LTSH.o $(TARGETDIR)/table-vhea.o \
	$(TARGETDIR)/table-OS_2.o $(TARGETDIR)/table-glyf.o $(TARGETDIR)/table-cmap.o \
	$(TARGETDIR)/table-name.o $(TARGETDIR)/table-fpgm-prep.o $(TARGETDIR)/table-gasp.o \
	$(TARGETDIR)/table-vmtx.o \
	$(TARGETDIR)/table-otl.o $(TARGETDIR)/table-otl-extend.o $(TARGETDIR)/table-otl-gsub-single.o \
	$(TARGETDIR)/table-otl-gsub-multi.o \
	$(TARGETDIR)/table-otl-chaining.o $(TARGETDIR)/table-otl-gpos-common.o $(TARGETDIR)/table-otl-gpos-mark-to-single.o
FONTOP_OBJECTS = $(TARGETDIR)/fontop-unconsolidate.o $(TARGETDIR)/fontop-consolidate.o $(TARGETDIR)/fontop-stat.o
EXTERN_OBJECTS = $(TARGETDIR)/extern-sds.o $(TARGETDIR)/extern-json.o $(TARGETDIR)/extern-json-builder.o
SUPPORT_OBJECTS = $(TARGETDIR)/support-glyphorder.o $(TARGETDIR)/support-aglfn.o \
              $(TARGETDIR)/support-stopwatch.o $(TARGETDIR)/support-unicodeconv.o \
			  $(TARGETDIR)/support-buffer.o
EXECUTABLES = $(TARGETDIR)/otfccdump$(SUFFIX) $(TARGETDIR)/otfccbuild$(SUFFIX)

OBJECTS = $(TABLE_OBJECTS) $(MAIN_OBJECTS_1) $(EXTERN_OBJECTS) $(SUPPORT_OBJECTS) $(FONTOP_OBJECTS)

$(EXTERN_OBJECTS) : $(TARGETDIR)/extern-%.o : extern/%.c | $(DIRS)
	@echo CC "->" $@
	@$(CC) $(CFLAGS_EXTERN) -c $< -o $@

SUPPORT_H = $(subst .o,.h,$(subst $(TARGETDIR)/support-,support/,$(SUPPORT_OBJECTS))) support/util.h
$(SUPPORT_OBJECTS) : $(TARGETDIR)/support-%.o : support/%.c | $(DIRS)
	@echo CC "->" $@
	@$(CC) $(CFLAGS) -c $< -o $@

TABLES_H = $(subst .o,.h,$(subst $(TARGETDIR)/table-,tables/,$(TABLE_OBJECTS)))
$(TABLE_OBJECTS) : $(TARGETDIR)/table-%.o : tables/%.c tables/%.h $(SUPPORT_H) $(TABLES_H) | $(DIRS)
	@echo CC "->" $@
	@$(CC) $(CFLAGS) -c $< -o $@
	
FONTOPS_H = $(subst .o,.h,$(subst $(TARGETDIR)/fontop-,fontops/,$(FONTOP_OBJECTS)))
$(FONTOP_OBJECTS) : $(TARGETDIR)/fontop-%.o : fontops/%.c fontops/%.h $(SUPPORT_H) $(TABLES_H) | $(DIRS)
	@echo CC "->" $@
	@$(CC) $(CFLAGS) -c $< -o $@

MAIN_H = $(subst .o,.h,$(subst $(TARGETDIR)/,,$(MAIN_OBJECTS_1)))
$(MAIN_OBJECTS) : $(TARGETDIR)/%.o : %.c $(MAIN_H) $(SUPPORT_H) $(TABLES_H) $(FONTOPS_H) | $(DIRS)
	@echo CC "->" $@
	@$(CC) $(CFLAGS) -c $< -o $@

$(EXECUTABLES): $(TARGETDIR)/%$(SUFFIX) : $(TARGETDIR)/%.o $(OBJECTS)
	@echo CC "->" $@
	@$(LINK) $(LINKFLAGS) $^ -o $@

objects: $(EXECUTABLES)

TESTFILES = $(TARGETDIR)/test-payload-1$(SUFFIX) $(TARGETDIR)/test-buffer$(SUFFIX)
$(TARGETDIR)/%.o : tests/%.c | $(DIRS)
	@echo CC "->" $@
	@$(CC) $(CFLAGS) -c $^ -o $@
$(TARGETDIR)/%$(SUFFIX) : $(TARGETDIR)/%.o $(OBJECTS)
	@echo CC "->" $@
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
