#include "user_types.h"
#include "user/user.h"
#include "xv6_stdlib.h"

// CALLOC 
void* xv6_calloc(int num, int size) {
    if (num <= 0 || size <= 0)
        return 0;

    unsigned long total = (unsigned long)num * (unsigned long)size;
    if (total == 0 || total > 0x7fffffff)
        return 0;

    void* ptr = malloc((int)total);
    if (!ptr) return 0;

    unsigned char* p = (unsigned char*)ptr;
    for (unsigned long i = 0; i < total; i++)
        p[i] = 0;

    return ptr;
}

// BSEARCH 
void* xv6_bsearch (const void* key , const void* base ,int nmemb , int size ,
    int (* compar)(const void*, const void*)) {
    const char* low = base;
    const char* high = low + nmemb * size;

    while (low < high) {
    const char* mid = low + (( high - low) / size / 2) * size;
    int cmp = compar(key , mid);

    if (cmp == 0) return (void*) mid;
    if (cmp < 0) high = mid;
    else low = mid + size;
    }
    return 0;
    }

// QSORT (Implemented as iterative Shell Sort to avoid stack overflow)
void xv6_qsort (void* base , int nmemb , int size ,
    int (* compar)(const void*, const void*)) {
    
    if (nmemb <= 1) return;
    
    char* arr = (char*)base;
    char* temp = malloc(size);
    if (!temp) return; // Allocation failed
    
    // Shell sort with geometric gap sequence
    for (int gap = nmemb / 2; gap > 0; gap /= 2) {
        for (int i = gap; i < nmemb; i++) {
            // temp = arr[i]
            for (int k = 0; k < size; k++) temp[k] = arr[i * size + k];
            
            int j;
            for (j = i; j >= gap; j -= gap) {
                // if arr[j - gap] > temp
                if (compar(arr + (j - gap) * size, temp) > 0) {
                    // arr[j] = arr[j - gap]
                    for (int k = 0; k < size; k++) 
                        arr[j * size + k] = arr[(j - gap) * size + k];
                } else {
                    break;
                }
            }
            // arr[j] = temp
            for (int k = 0; k < size; k++) arr[j * size + k] = temp[k];
        }
    }
    free(temp);
}
    

// ATOI 
int xv6_atoi(const char* str) {
    if (!str) return 0;
    int i = 0, sign = 1, result = 0;
    while (str[i] == ' ' || str[i] == '\t' || str[i] == '\n') i++;
    if (str[i] == '-') { sign = -1; i++; }
    else if (str[i] == '+') { i++; }
    while (str[i] >= '0' && str[i] <= '9') {
        result = result * 10 + (str[i] - '0');
        i++;
    }
    return sign * result;
}

static int xv6_isspace(char c) {
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v');
}



// ATOF
float xv6_atof(const char* str) {
    // Skip whitespace
    while ( xv6_isspace (* str)) str ++;

    // Handle sign
    float sign = 1.0f;
    if (* str == '+') str ++;
    else if (* str == '-') {
    sign = -1.0f;
    str ++;
    }

    // Parse integer part
    float result = 0.0f;
    while (* str >= '0' && *str <= '9') {
    result = result * 10.0f + (* str - '0');
    str ++;
    }

    // Parse fractional part
    if (* str == '.') {
    str ++;
    float fraction = 1.0f;
    while (* str >= '0' && *str <= '9') {
    fraction *= 0.1f;
    result += (* str - '0') * fraction;
    str ++;
    }
    }

    // Parse exponent
    if (* str == 'e' || *str == 'E') {
    str ++;
    int exp_sign = 1;
    if (* str == '+') str ++;
    else if (* str == '-') {
    exp_sign = -1;
    str ++;
    }

    int exponent = 0;
    while (* str >= '0' && *str <= '9') {
    exponent = exponent * 10 + (* str - '0');
    str ++;
    }

    // Apply exponent
    while (exponent -- > 0) {

    result = (exp_sign == 1) ? result * 10.0f : result *

    0.1f;

    }
    }

    return sign * result;
    }

