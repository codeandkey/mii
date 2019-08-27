#include "module.h"

#include <stdlib.h>

mii_module* mii_module_alloc(char* path, char* code, int type, time_t last_updated) {
    mii_module* out = malloc(sizeof *out);
    mii_module_init(out, path, code, type, last_updated);
    return out;
}

void mii_module_init(mii_module* p, char* path, char* code, int type, time_t last_updated) {
    p->path = path;
    p->code = code;
    p->type = type;
    p->last_updated = last_updated;
    p->analyzed = 0;

    mii_strlist_init(&p->bins);
    mii_strlist_init(&p->modulepaths);
}

void mii_module_free(mii_module* p) {
    free(p->path);
    free(p->code);

    mii_strlist_free(&p->bins);
    mii_strlist_free(&p->modulepaths);

    free(p);
}
