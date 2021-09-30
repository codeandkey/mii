# Mii
Mii is a shell extension designed to make HPC modules easier to use.
## concept
Mii works around an existing Lmod or TCL modules installation, efficiently searching and loading modules on-demand for users.

Once Mii is installed into a user's shell, modules will be quietly autoloaded for any unambiguous commands. Ambiguous commands will interactively ask for modules to load:

```
$ blastn
[mii] Please select a module to run blastn:
    1  ncbi-rmblastn/2.2.28-t52d4vp
    2  ncbi-rmblastn/2.6.0-2kyyml7
    3  blast-plus/2.7.1-py2-vvbzyor
Make a selection (1-3, q aborts) [1]:
```

## features
- Support for [Lmod](https://lmod.readthedocs.io/en/latest/) and [Environment Modules](http://modules.sourceforge.net/)
- `bash` and `zsh` shell integration
- Module listing / individual information (via `mii list`, `mii show`)
- Searching for exact commands
- Searching for similar commands
- Optional JSON export format

## dependencies
Mii has no dependencies. The source is fully POSIX-compliant and will build on almost any UNIX system.
`make` is required to execute the makefile, but it is there for convienence.

## installation
To install Mii to your computer under `/usr`:
```
$ git clone https://github.com/codeandkey/mii
$ cd mii
# make install
```

To integrate Mii with your shell, execute the interactive installer with `mii install`.

##  advanced lua analysis

Mii uses a basic regex by default to keep the index as fast as possible. If your Lua modules contain advanced logic, build Mii with advanced Lua support:

```
# MII_ENABLE_LUA=yes make install
```

## hierarchical MODULEPATHs

If you have modules which themselves modify the MODULEPATH, Mii will not detect the embedded modules. To enable hierarchical MODULEPATH support (Lmod only):

```
# MII_ENABLE_SPIDER=yes make install
```

## synchronizing
The index is synchronized on every login. If no modules have changed this will finish quite quickly.

To manually synchronize the index, execute `mii sync`.

To force rebuild the index, execute `mii build`.

## methods

### storage
Mii originally used an SQLite3-based database to store the module index. While this worked well, performance was not optimal and the database was often corrupted.
This version uses an in-house binary format to store the module tables.
At runtime the index lives in a large hashmap using the high-performance [xxHash](https://github.com/Cyan4973/xxHash) non-cryptographic hash function.

### synchronizing
Mii uses timestamp-based updating to keep the index up-to-date.
When the index is built, each module file is stored along with the date the file was last modified.
This allows the sync to load already analyzed modules from the existing index when updating, saving much time.

### searching
Mii's exact search is a basic linear search which iterates through the module table looking for matches.
The fuzzy searching uses a [Damerau–Levenshtein distance](https://en.wikipedia.org/wiki/Damerau%E2%80%93Levenshtein_distance) metric to determine query relevance.
