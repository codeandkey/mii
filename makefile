PREFIX 	  ?= /usr
REALPREFIX = $(realpath $(PREFIX))

ifndef $(REALPREFIX)
REALPREFIX = $(PREFIX)
endif

BUILD      = $(shell date "%D:%M:%Y")

CC         = g++
CFLAGS     = -std=c++11 -Wall -Werror -Wno-format-security -pedantic -O3 -DMII_DEBUG -DMII_PREFIX="\"$(REALPREFIX)\"" -DMII_BUILD_TIME="\"$(BUILD)\""
LDFLAGS    = -llua

C_SOURCES  = $(wildcard src/*.cpp) src/xxhash/xxhash.cpp
C_OBJECTS  = $(C_SOURCES:.cpp=.o)
C_OUTPUT   = mii

LUA_SOURCES = src/lua/utils.lua src/lua/sandbox.lua
LUA_OUTPUT  = sandbox.luac
LUAC        = luac

OUTPUTS    = $(C_OUTPUT) $(LUA_OUTPUT)

all: $(OUTPUTS)

$(C_OUTPUT): $(C_OBJECTS)
	$(CC) $(C_OBJECTS) $(LDFLAGS) -o $(C_OUTPUT)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

$(LUA_OUTPUT): $(LUA_SOURCES)
	$(LUAC) -o $@ $^

clean:
	rm -f $(C_OUTPUT) $(LUA_OUTPUT) $(C_OBJECTS) $(C_JSON_OBJECTS)

install: $(OUTPUTS)
	@echo "Installing mii to $(PREFIX)"
	mkdir -p $(PREFIX)/bin
	mkdir -p $(PREFIX)/share/mii
	cp $(C_OUTPUT) $(PREFIX)/bin
	cp -r init  $(PREFIX)/share/mii
ifeq ($(MII_ENABLE_LUA), yes)
	mkdir -p $(PREFIX)/share/mii/lua
	cp $(LUA_OUTPUT) $(PREFIX)/share/mii/lua
endif
