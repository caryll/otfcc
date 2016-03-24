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

OBJOTFCCMAIN = build/caryll-font.o build/caryll-io.o build/caryll-sfnt.o
OBJTABLES = build/table-head.o build/table-hhea.o build/table-maxp.o \
	build/table-hmtx.o build/table-post.o build/table-hdmx.o \
	build/table-PCLT.o build/table-LTSH.o build/table-vhea.o \
	build/table-OS_2.o build/table-glyf.o build/table-cmap.o \
	build/table-name.o
EXTOBJS = build/extern-sds.o build/extern-json.o build/extern-json-builder.o
SUPPORTOBJS = build/support-glyphorder.o build/support-aglfn.o \
              build/support-stopwatch.o build/support-unicodeconv.o 

OBJECTS = $(OBJTABLES) $(OBJOTFCCMAIN) $(EXTOBJS) $(SUPPORTOBJS)

$(OBJTABLES) : build/table-%.o : tables/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJOTFCCMAIN) : build/%.o : %.c $(OBJTABLES) | build
	$(CC) $(CFLAGS) -c $< -o $@

$(EXTOBJS) : build/extern-%.o : extern/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

$(SUPPORTOBJS) : build/support-%.o : support/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@
	
build/otfccdump.o : otfccdump.c | build
	$(CC) $(CFLAGS) -c $^ -o $@

build/otfccdump$(SUFFIX) : build/otfccdump.o $(OBJECTS)
	$(LINK) $^ -o $@

build/otfccbuild.o : otfccbuild.c | build
	$(CC) $(CFLAGS) -c $^ -o $@

build/otfccbuild$(SUFFIX) : build/otfccbuild.o $(OBJECTS)
	$(LINK) $^ -o $@

objects:build/otfccdump$(SUFFIX) build/otfccbuild$(SUFFIX)

TESTFILES = build/test-payload-1$(SUFFIX)
build/%.o : tests/%.c | build
	$(CC) $(CFLAGS) -c $^ -o $@
build/%$(SUFFIX) : build/%.o $(OBJECTS)
	$(LINK) $^ -o $@

test: $(TESTFILES)
	@echo "====== Start Test ======"
	@build/test-payload-1$(SUFFIX) tests/payload/test-out.ttf

debug:
	make VERSION=debug
release:
	make VERSION=release
