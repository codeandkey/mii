#include "log.h"

#include <time.h>
#include <unistd.h>

static char _mii_log_datestr[64];

static int _mii_log_verbosity = MII_LOG_VERBOSITY_DEFAULT;
static int _mii_log_colors    = MII_LOG_COLOR_DEFAULT;

void mii_log(int level, const char* color, const char* tag, const char* fmt, va_list args) {
    if (level > _mii_log_verbosity) return;

    /* get current time */
    time_t time_point = time(NULL);

    /* print time prefix */
    strftime(_mii_log_datestr, sizeof _mii_log_datestr, "%H:%M:%S", localtime(&time_point));
    fprintf(stderr, "[%s] ", _mii_log_datestr);

    /* determine if we need colors */
    int write_colors = 0;

    switch (_mii_log_colors) {
    case MII_LOG_COLOR_AUTO:
        if (!isatty(fileno(stderr))) break;
    case MII_LOG_COLOR_FORCE:
        write_colors = 1;
    case MII_LOG_COLOR_NEVER:
        break;
    }

    if (write_colors) {
        fprintf(stderr, color);
    }

    fprintf(stderr, "%s ", tag);

    /* reset color if we messed with it */
    if (write_colors) {
        fprintf(stderr, "\033[0;39m");
    }

    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
}

void mii_info(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    mii_log(1, "\033[0;36m", "INFO ", fmt, args);

    va_end(args);
}

void mii_warn(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    mii_log(2, "\033[1;33m", "WARN ", fmt, args);

    va_end(args);
}

void mii_error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    mii_log(0, "\033[1;31m", "ERROR", fmt, args);

    va_end(args);
}

void mii_log_set_verbosity(int verbosity) {
    _mii_log_verbosity = verbosity;
}

void mii_log_set_color(int mode) {
    _mii_log_colors = mode;
}
