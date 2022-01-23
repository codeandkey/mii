#define _POSIX_C_SOURCE 200809L

#include "util.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>

using namespace mii;
using namespace std;

void util::debug(const char* fmt, ...) 
{
#ifndef MII_DEBUG
    return;
#endif
    va_list args;
    va_start(args, fmt);

    vfprintf(stderr, fmt, args);

    va_end(args);
}