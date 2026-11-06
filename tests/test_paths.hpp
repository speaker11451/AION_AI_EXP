#pragma once
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>

#ifndef AION_ARTIFACTS_ROOT
#define AION_ARTIFACTS_ROOT "/data/data/com.termux/files/home/aion_m1/artifacts"
#endif

inline FILE* open_artifact(const char* subpath, const char* mode) {
    char full[1024];
    snprintf(full, sizeof(full), "%s/%s", AION_ARTIFACTS_ROOT, subpath);
    char tmp[1024];
    strncpy(tmp, full, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = 0;
    for (char* p = tmp + 1; *p; ++p) {
        if (*p == '/') { *p = 0; mkdir(tmp, 0755); *p = '/'; }
    }
    FILE* f = fopen(full, mode);
    if (!f) fprintf(stderr, "[test_paths] fopen failed: %s\n", full);
    return f;
}
