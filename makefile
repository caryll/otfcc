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

OBJFILES = build/caryll-font.o build/caryll-io.o build/caryll-sfnt.o
EXTOBJS = build/parson.o

$(OBJFILES) : build/%.o : %.c | build
	$(CC) $(CFLAGS) -c $^ -o $@

$(EXTOBJS) : build/%.o : extern/%.c | build
	$(CC) $(CFLAGS) -c $^ -o $@
	
build/test-head.o : test-head.c | build
	$(CC) $(CFLAGS) -c $^ -o $@

build/test-head$(SUFFIX) : build/test-head.o $(OBJFILES) $(EXTOBJS)
	$(LINK) $^ -o $@

objects: $(OBJFILES) $(EXTOBJS) build/test-head$(SUFFIX)

debug:
	make VERSION=debug
release:
	make VERSION=release
