CC = gcc
LINK = gcc
CCOPTIONS = -O3
all : objects

ifdef SystemRoot # win32 + tdm-gcc
SUFFIX = .exe
endif

build : 
	@- mkdir $@

OBJFILES = build/caryll-font.o build/caryll-io.o build/caryll-sfnt.o build/parson.o

$(OBJFILES) : build/%.o : %.c | build
	$(CC) $(CCOPTIONS) -c $^ -o $@
	
build/test-head.o : test-head.c | build
	$(CC) $(CCOPTIONS) -c $^ -o $@

build/test-head$(SUFFIX) : build/test-head.o $(OBJFILES)
	$(LINK) $^ -o $@

objects: $(OBJFILES) build/test-head$(SUFFIX)