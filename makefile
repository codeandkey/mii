PREFIX 	  ?= /usr
REALPREFIX = $(realpath $(PREFIX))

ifndef $(REALPREFIX)
REALPREFIX = $(PREFIX)
endif

CC         = gcc
CFLAGS     = -std=c99 -Wall -Werror -Wno-format-security -pedantic -O3 -DMII_RELEASE -DMII_PREFIX="\"$(REALPREFIX)\"" -DMII_BUILD_TIME="\"$(shell date)\""
LDFLAGS    =
C_OUTPUT   = mii
OUTPUTS    = $(C_OUTPUT)

C_SOURCES = $(wildcard src/*.c) src/xxhash/xxhash.c
C_OBJECTS = $(C_SOURCES:.c=.o)

LUA_SOURCES = src/lua/utils.lua src/lua/sandbox.lua
LUA_OUTPUT  = sandbox.luac
LUAC        = luac

MII_ENABLE_LUA  ?= no
MII_LUA_LDFLAG  ?= -llua
MII_LUA_INCLUDE ?=

ifeq ($(MII_ENABLE_LUA), yes)
CFLAGS  += -DMII_ENABLE_LUA $(MII_LUA_INCLUDE)
LDFLAGS += $(MII_LUA_LDFLAG)
OUTPUTS += $(LUA_OUTPUT)
endif

all: $(OUTPUTS)

$(C_OUTPUT): $(C_OBJECTS)
	$(CC) $(C_OBJECTS) $(LDFLAGS) -o $(C_OUTPUT)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(LUA_OUTPUT): $(LUA_SOURCES)
	$(LUAC) -o $@ $^

clean:
	rm -f $(C_OUTPUT) $(LUA_OUTPUT) $(C_OBJECTS)

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
