PREFIX 	  ?= /usr
REALPREFIX = $(realpath $(PREFIX))
CC         = gcc
CFLAGS     = -std=c99 -Wall -Werror -Wno-format-security -pedantic -O3 -DMII_RELEASE -DMII_PREFIX="\"$(REALPREFIX)\"" -DMII_BUILD_TIME="\"$(shell date)\""
LDFLAGS    = -llua
OUTPUT     = mii

SOURCES = $(wildcard src/*.c) src/xxhash/xxhash.c
OBJECTS = $(SOURCES:.c=.o)

LUASOURCES = $(wildcard src/lua/*.lua)
LUAOUTPUT  = sandbox.luac
LUAC       = luac

all: $(OUTPUT) $(LUAOUTPUT)

$(OUTPUT): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(OUTPUT)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(LUAOUTPUT): $(LUASOURCES)
	$(LUAC) -o $@ $?

clean:
	rm -f $(OUTPUT) $(LUAOUTPUT) $(OBJECTS)

install: $(OUTPUT) $(LUAOUTPUT)
	@echo "Installing mii to $(PREFIX)"
	mkdir -p $(PREFIX)/bin
	mkdir -p $(PREFIX)/share/mii/lua
	cp $(OUTPUT) $(PREFIX)/bin
	cp $(LUAOUTPUT) $(PREFIX)/share/mii/lua
	cp -r init  $(PREFIX)/share/mii
