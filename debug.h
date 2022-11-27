#pragma once

#define debug_print(...) \
        do { fprintf(stderr, "%s:%d:%s(): ",__FILE__, __LINE__, __func__);\
             fprintf(stderr, __VA_ARGS__); } while (0)
