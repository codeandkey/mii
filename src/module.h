#ifndef MII_MODULE_H
#define MII_MODULE_H

/*
 * mii_module represents a single module file and stores
 * its attributes such as module type and eventually
 * analyzed module information.
 */

#include "strlist.h"

#include <time.h>

#define MII_MODULE_TYPE_LMOD 0
#define MII_MODULE_TYPE_TCL  1

typedef struct _mii_module {
    char* path, *code;
    int type, analyzed;
    time_t last_updated;
    mii_strlist bins, modulepaths;
} mii_module;

/* strings are MOVED */
mii_module* mii_module_alloc(char* path, char* code, int type, time_t last_updated);
void mii_module_free(mii_module* p);

#endif /* MII_MODULE_H */
