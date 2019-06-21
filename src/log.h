#pragma once

#include <stdarg.h>
#include <stdio.h>

#ifdef MII_RELEASE
#define mii_debug(x, ...)
#else
#define mii_debug(x, ...)  fprintf(stderr, "[mii] \033[1;35mDEBUG %s:%d\033[1;37m : " x "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif

#define MII_LOG_COLOR_NEVER   0
#define MII_LOG_COLOR_AUTO    1
#define MII_LOG_COLOR_FORCE   2
#define MII_LOG_COLOR_DEFAULT MII_LOG_COLOR_AUTO

#define MII_LOG_VERBOSITY_DEFAULT 1

/*
 * verbosity levels
 *
 * 0: errors
 * 1: info
 * 2: warnings
 */

/* base logging function */
void mii_log(int level, const char* color, const char* tag, const char* fmt, va_list args);

/* logging shorthand */
void mii_info(const char* fmt, ...);
void mii_warn(const char* fmt, ...);
void mii_error(const char* fmt, ...);

/* set the logging verbosity */
void mii_log_set_verbosity(int verbosity);

/* set the color mode */
void mii_log_set_color(int mode);
