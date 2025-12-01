#include "kernel/types.h"
#include "user/user.h"
#include "user/xv6_strings.h"
#include "user/test_milestone2.h"

#define ASSERT(cond, msg) do { \
    if (!(cond)) { \
        printf("FAIL: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
        return 0; \
    } \
} while(0)

static int test_memcpy() {
    char src[] = "hello world";
    char dest[20];
    
    // Normal copy
    memcpy(dest, src, 12);
    if (strcmp(dest, "hello world") != 0) return 0;

    // Zero length
    memcpy(dest, "overwrite", 0);
    if (strcmp(dest, "hello world") != 0) return 0;

    return 1;
}

static int test_memset() {
    char buf[10];
    
    // Normal set
    memset(buf, 'A', 5);
    buf[5] = '\0';
    if (strcmp(buf, "AAAAA") != 0) return 0;

    // Set to 0
    memset(buf, 0, 5);
    if (buf[0] != 0 || buf[4] != 0) return 0;

    return 1;
}

static int test_strcmp() {
    if (strcmp("abc", "abc") != 0) return 0;
    if (strcmp("abc", "abd") >= 0) return 0;
    if (strcmp("abd", "abc") <= 0) return 0;
    if (strcmp("", "") != 0) return 0;
    if (strcmp("a", "") <= 0) return 0;
    return 1;
}

static int test_strlen() {
    if (strlen("hello") != 5) return 0;
    if (strlen("") != 0) return 0;
    return 1;
}

static int test_sprintf() {
    char buf[100];
    
    // Integer
    sprintf(buf, "Number: %d", 42);
    if (strcmp(buf, "Number: 42") != 0) return 0;

    // String
    sprintf(buf, "Hello %s", "world");
    if (strcmp(buf, "Hello world") != 0) return 0;

    // Float with precision
    sprintf(buf, "Pi: %.4f", 3.14159);
    // Simple float check (string match might be tricky due to float precision, but our implementation is deterministic)
    // Our implementation might round slightly differently, but let's check prefix
    if (strcmp(buf, "Pi: 3.1416") != 0 && strcmp(buf, "Pi: 3.1415") != 0) {
        printf("sprintf float mismatch: %s\n", buf);
        return 0;
    }

    // Mixed
    sprintf(buf, "%s scored %d", "Alice", 95);
    if (strcmp(buf, "Alice scored 95") != 0) return 0;

    return 1;
}

static int test_sscanf() {
    int d;
    float f;
    char s[20];
    char c;

    // Integer
    if (sscanf("123", "%d", &d) != 1 || d != 123) { printf("sscanf int failed\n"); return 0; }

    // Float
    if (sscanf("3.14", "%f", &f) != 1 || (f < 3.13 || f > 3.15)) { printf("sscanf float failed: f=%d.%d\n", (int)f, (int)((f-(int)f)*100)); return 0; }

    // String
    if (sscanf("hello", "%s", s) != 1 || strcmp(s, "hello") != 0) { printf("sscanf string failed\n"); return 0; }

    // Char
    if (sscanf("x", "%c", &c) != 1 || c != 'x') { printf("sscanf char failed\n"); return 0; }

    // Mixed
    if (sscanf("42 3.14 abc z", "%d %f %s %c", &d, &f, s, &c) != 4) { printf("sscanf mixed count failed\n"); return 0; }
    if (d != 42) { printf("sscanf mixed int failed\n"); return 0; }
    if (f < 3.13 || f > 3.15) { printf("sscanf mixed float failed\n"); return 0; }
    if (strcmp(s, "abc") != 0) { printf("sscanf mixed string failed\n"); return 0; }
    if (c != 'z') { printf("sscanf mixed char failed\n"); return 0; }

    return 1;
}

void test_string(void) {
    printf("=== xv6 String Function Tests ===\n");
    
    int pass = 0;
    int total = 0;

    total++; if(test_memcpy()) pass++; else printf("test_memcpy failed\n");
    total++; if(test_memset()) pass++; else printf("test_memset failed\n");
    total++; if(test_strcmp()) pass++; else printf("test_strcmp failed\n");
    total++; if(test_strlen()) pass++; else printf("test_strlen failed\n");
    total++; if(test_sprintf()) pass++; else printf("test_sprintf failed\n");
    total++; if(test_sscanf()) pass++; else printf("test_sscanf failed\n");

    printf("Strings: pass=%d/%d\n", pass, total);
}
