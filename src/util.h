#pragma once

/* generic utils */

#define mii_min(x, y) ((x < y) ? (x) : (y))

char* mii_strdup(const char* str);
char* mii_join_path(const char* a, const char* b);
int mii_levenshtein_distance(const char* a, const char* b);
