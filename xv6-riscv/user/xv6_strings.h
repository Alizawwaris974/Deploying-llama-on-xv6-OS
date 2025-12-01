#ifndef XV6_STRINGS_H
#define XV6_STRINGS_H

#include <stdarg.h>
#include <stddef.h>

void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
int strcmp(const char *a, const char *b);
size_t strlen(const char *s);
char *strcpy(char *dst, const char *src);

/* snprintf/vsnprintf and sprintf (bounded) */
int xv6_vsnprintf(char *out, size_t n, const char *fmt, va_list ap);
int xv6_snprintf(char *out, size_t n, const char *fmt, ...);
int sprintf(char *out, const char *fmt, ...); /* override libc name intentionally if needed */

/* Simple sscanf supporting %d %f %s %c */
/* Simple sscanf supporting %d %f %s %c */
int xv6_vsscanf(const char *s, const char *fmt, va_list ap);
int xv6_sscanf(const char *s, const char *fmt, ...);
int sscanf(const char *s, const char *fmt, ...);

/* character classification */
int isprint(int c);
int isspace(int c);

#endif /* XV6_STRINGS_H */
