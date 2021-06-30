PREFIX 	  ?= /usr
REALPREFIX = $(realpath $(PREFIX))
CC         = gcc
CFLAGS     = -std=c99 -Wall -Werror -Wno-format-security -pedantic -O3 -DMII_RELEASE -DMII_PREFIX="\"$(REALPREFIX)\"" -DMII_BUILD_TIME="\"$(shell date)\""
LDFLAGS    = -llua
OUTPUT     = mii

SOURCES = $(wildcard src/*.c) src/xxhash/xxhash.c
OBJECTS = $(SOURCES:.c=.o)

all: $(OUTPUT)

$(OUTPUT): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(OUTPUT)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OUTPUT) $(OBJECTS)

install: $(OUTPUT)
	@echo "Installing mii to $(PREFIX)"
	mkdir -p $(PREFIX)/bin
	mkdir -p $(PREFIX)/share/mii
	cp $(OUTPUT) $(PREFIX)/bin
	cp -r init src/lua $(PREFIX)/share/mii
