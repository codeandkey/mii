PREFIX ?= /usr
CC      = gcc
CFLAGS  = -std=c99 -Wall -Werror -pedantic -pg -O3 -D_POSIX_C_SOURCE -DMII_RELEASE -DMII_PREFIX="\"$(PREFIX)\""
LDFLAGS = -pg
OUTPUT  = mii

SOURCES = $(wildcard src/*.c) src/xxhash/xxhash.c
OBJECTS = $(SOURCES:.c=.o)

all: $(OUTPUT)

$(OUTPUT): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(OUTPUT)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OUTPUT) $(OBJECTS)

install:
	@echo "Installing mii to $(PREFIX)"
	mkdir -p $(PREFIX)/bin
	mkdir -p $(PREFIX)/share/mii
	cp mii $(PREFIX)/bin
	cp -r init $(PREFIX)/share/mii
