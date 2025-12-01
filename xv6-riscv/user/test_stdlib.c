#include "kernel/types.h"
#include "user/user.h"
#include "user/xv6_stdlib.h"
#include "user/test_milestone2.h"

static int test_calloc() {
    int *p = (int*)calloc(5, sizeof(int));
    if (!p) return 0;
    for (int i=0; i<5; i++) {
        if (p[i] != 0) return 0;
    }
    free(p);
    return 1;
}

static int int_cmp(const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}

static int test_qsort() {
    int arr[] = {5, 2, 9, 1, 5, 6};
    qsort(arr, 6, sizeof(int), int_cmp);
    for (int i=0; i<5; i++) {
        if (arr[i] > arr[i+1]) return 0;
    }
    return 1;
}

static int test_bsearch() {
    int arr[] = {1, 2, 5, 6, 9};
    int key = 5;
    int *res = (int*)bsearch(&key, arr, 5, sizeof(int), int_cmp);
    if (!res || *res != 5) return 0;

    key = 3;
    res = (int*)bsearch(&key, arr, 5, sizeof(int), int_cmp);
    if (res) return 0;

    return 1;
}

static int test_atoi() {
    if (atoi("123") != 123) return 0;
    if (atoi("-123") != -123) return 0;
    if (atoi("  42") != 42) return 0;
    return 1;
}

static int test_atof() {
    float f = atof("3.14");
    if (f < 3.13 || f > 3.15) return 0;
    
    f = atof("-0.5");
    if (f < -0.51 || f > -0.49) return 0;

    f = atof("1.5e2");
    if (f < 149.0 || f > 151.0) return 0;

    return 1;
}

void test_stdlib(void) {
    printf("=== xv6 Stdlib Function Tests ===\n");
    
    int pass = 0;
    int total = 0;

    total++; if(test_calloc()) pass++; else printf("test_calloc failed\n");
    total++; if(test_qsort()) pass++; else printf("test_qsort failed\n");
    total++; if(test_bsearch()) pass++; else printf("test_bsearch failed\n");
    total++; if(test_atoi()) pass++; else printf("test_atoi failed\n");
    total++; if(test_atof()) pass++; else printf("test_atof failed\n");

    printf("Stdlib: pass=%d/%d\n", pass, total);
}