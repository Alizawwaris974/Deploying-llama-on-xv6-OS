#ifndef XV6_STDLIB_H
#define XV6_STDLIB_H

void* xv6_calloc(int num, int size);
void* xv6_bsearch(const void* key, const void* base, int num, int size, int (*compare)(const void*, const void*));
void xv6_qsort(void* base, int num, int size, int (*compare)(const void*, const void*));
int xv6_atoi(const char* str);
float xv6_atof(const char* str);

// for llama2.c compatibility
#define calloc xv6_calloc
#define bsearch xv6_bsearch
#define qsort xv6_qsort
#define atoi xv6_atoi
#define atof xv6_atof

#endif