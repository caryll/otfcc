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
	build/table-OS_2.o 
EXTOBJS = build/parson.o

OBJECTS = $(OBJTABLES) $(OBJOTFCCMAIN) $(EXTOBJS)

$(OBJTABLES) : build/table-%.o : tables/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJOTFCCMAIN) : build/%.o : %.c $(OBJTABLES) | build
	$(CC) $(CFLAGS) -c $< -o $@

$(EXTOBJS) : build/%.o : extern/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@
	
build/test-head.o : test-head.c | build
	$(CC) $(CFLAGS) -c $^ -o $@

build/test-head$(SUFFIX) : build/test-head.o $(OBJECTS)
	$(LINK) $^ -o $@

objects: build/test-head$(SUFFIX)

debug:
	make VERSION=debug
release:
	make VERSION=release
