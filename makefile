CC = clang
LINK = clang
CCOPTIONS = -O3
all : objects

ifdef SystemRoot # win32 + tdm-gcc
SUFFIX = .exe
endif

build : 
	@- mkdir $@

OBJFILES = build/caryll-font.o build/caryll-io.o build/caryll-sfnt.o
EXTOBJS = build/parson.o

$(OBJFILES) : build/%.o : %.c | build
	$(CC) $(CCOPTIONS) -c $^ -o $@

$(EXTOBJS) : build/%.o : extern/%.c | build
	$(CC) $(CCOPTIONS) -c $^ -o $@
	
build/test-head.o : test-head.c | build
	$(CC) $(CCOPTIONS) -c $^ -o $@

build/test-head$(SUFFIX) : build/test-head.o $(OBJFILES) $(EXTOBJS)
	$(LINK) $(CCOPTIONS) $^ -o $@

objects: $(OBJFILES) $(EXTOBJS) build/test-head$(SUFFIX)
