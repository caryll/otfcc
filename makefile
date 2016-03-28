CC = clang
LINK = clang

ifeq ($(VERSION), debug)
CFLAGS = -g -Ddebug
else
CFLAGS = -O3 -Wall
endif

all : objects

ifdef SystemRoot # win32 + tdm-gcc
SUFFIX = .exe
endif

build : 
	@- mkdir $@

MAIN_OBJECTS_1 = build/caryll-font.o build/caryll-sfnt.o build/caryll-sfnt-builder.o
MAIN_OBJECTS = $(MAIN_OBJECTS_1) build/otfccdump.o build/otfccbuild.o
TABLE_OBJECTS = build/table-head.o build/table-hhea.o build/table-maxp.o \
	build/table-hmtx.o build/table-post.o build/table-hdmx.o \
	build/table-PCLT.o build/table-LTSH.o build/table-vhea.o \
	build/table-OS_2.o build/table-glyf.o build/table-cmap.o \
	build/table-name.o build/table-fpgm-prep.o
EXTERN_OBJECTS = build/extern-sds.o build/extern-json.o build/extern-json-builder.o
SUPPORT_OBJECTS = build/support-glyphorder.o build/support-aglfn.o \
              build/support-stopwatch.o build/support-unicodeconv.o \
			  build/support-buffer.o
TARGETS = build/otfccdump$(SUFFIX) build/otfccbuild$(SUFFIX)

OBJECTS = $(TABLE_OBJECTS) $(MAIN_OBJECTS_1) $(EXTERN_OBJECTS) $(SUPPORT_OBJECTS)

$(EXTERN_OBJECTS) : build/extern-%.o : extern/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

SUPPORT_H = $(subst .o,.h,$(subst build/support-,support/,$(SUPPORT_OBJECTS))) support/util.h
$(SUPPORT_OBJECTS) : build/support-%.o : support/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

TABLES_H = $(subst .o,.h,$(subst build/table-,tables/,$(TABLE_OBJECTS)))
$(TABLE_OBJECTS) : build/table-%.o : tables/%.c tables/%.h $(SUPPORT_H) $(TABLES_H) | build
	$(CC) $(CFLAGS) -c $< -o $@

MAIN_H = $(subst .o,.h,$(subst build/,,$(MAIN_OBJECTS_1)))
$(MAIN_OBJECTS) : build/%.o : %.c $(MAIN_H) $(SUPPORT_H) $(TABLES_H) | build
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGETS): build/%$(SUFFIX) : build/%.o $(OBJECTS)
	$(LINK) $^ -o $@ -lm

objects: $(TARGETS)

TESTFILES = build/test-payload-1$(SUFFIX) build/test-buffer$(SUFFIX)
build/%.o : tests/%.c | build
	$(CC) $(CFLAGS) -c $^ -o $@
build/%$(SUFFIX) : build/%.o $(OBJECTS)
	$(LINK) $^ -o $@ -lm

test: $(TESTFILES)
	@echo "====== Start Test ======"
	@build/test-buffer$(SUFFIX)
	@build/test-payload-1$(SUFFIX) tests/payload/test-out.ttf

debug:
	make VERSION=debug
release:
	make VERSION=release
