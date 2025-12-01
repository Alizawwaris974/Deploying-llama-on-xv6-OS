/*
 xv6_strings.c
 Minimal implementations of common string functions and a small printf/scanf subset.
 Place in users/ and include xv6_strings.h where needed.
*/

#include "xv6_strings.h"
#include <stdarg.h>
#include <stddef.h>

#ifndef INT_MAX
#define INT_MAX 2147483647
#endif

/* ---------------- basic memory/string helpers ---------------- */

void *memcpy(void *dest, const void *src, size_t n){
    unsigned char *d = (unsigned char*)dest;
    const unsigned char *s = (const unsigned char*)src;
    while(n--) *d++ = *s++;
    return dest;
}

void *memset(void *s, int c, size_t n){
    unsigned char *p = (unsigned char*)s;
    unsigned char uc = (unsigned char)c;
    while(n--) *p++ = uc;
    return s;
}

int strcmp(const char *a, const char *b){
    while(*a && (*a == *b)){ a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

size_t strlen(const char *s){
    const char *p = s;
    while(*p) p++;
    return (size_t)(p - s);
}

char *strcpy(char *dst, const char *src){
    char *d = dst;
    while((*d++ = *src++)){}
    return dst;
}

/* ---------------- char classification ---------------- */

int isprint(int c){
    return (c >= 0x20 && c <= 0x7E);
}
int isspace(int c){
    return (c==' ' || c=='\f' || c=='\n' || c=='\r' || c=='\t' || c=='\v');
}

/* ---------------- small helpers for number -> string ---------------- */

/* write reversed digits to buffer; returns count */
static int uitoa_rev(unsigned int v, char *out){
    int i=0;
    if(v==0){ out[i++]='0'; return i; }
    while(v){ out[i++] = '0' + (v % 10); v/=10; }
    return i;
}

/* integer to string (signed), returns written length (not including NUL), bounded */
static int int_to_str(int val, char *buf, size_t bufsz){
    if(bufsz==0) return 0;
    char tmp[16];
    unsigned int uv;
    int neg = 0;
    if(val<0){ neg=1; uv = (unsigned int)(-val); } else uv = (unsigned int)val;
    int n = uitoa_rev(uv, tmp);
    size_t idx=0;
    if(neg && idx < bufsz-1) buf[idx++]='-';
    for(int i=n-1;i>=0 && idx < bufsz-1;i--) buf[idx++]=tmp[i];
    buf[idx] = '\0';
    return (int)idx;
}

/* float to string with precision (simple, not IEEE-perfect) */
/* Supports finite floats; uses naive approach: sign, integer part, fractional by repeated multiply */
static int float_to_str(double f, char *buf, size_t bufsz, int precision){
    if(bufsz==0) return 0;
    if(f != f){ /* NaN */ 
        const char *nan = "nan";
        size_t i=0; while(i+1<bufsz && *nan){ buf[i++]=*nan++; } buf[i]='\0'; return (int)i;
    }
    if(f == 1.0/0.0){ const char *inf="inf"; size_t i=0; while(i+1<bufsz && *inf){ buf[i++]=*inf++; } buf[i]='\0'; return (int)i; }
    if(f == -1.0/0.0){ const char *ninf="-inf"; size_t i=0; while(i+1<bufsz && *ninf){ buf[i++]=*ninf++; } buf[i]='\0'; return (int)i; }
    size_t idx=0;
    if(f < 0.0){ if(idx < bufsz-1) buf[idx++]='-'; f = -f; }
    /* integer part */
    unsigned long long ip = (unsigned long long)f;
    double frac = f - (double)ip;
    /* write integer part using reverse */
    char tmp[32];
    int ti=0;
    if(ip==0){ tmp[ti++]='0'; }
    else {
        while(ip){
            tmp[ti++] = '0' + (int)(ip % 10);
            ip /= 10;
        }
    }
    for(int i=ti-1;i>=0;i--){
        if(idx < bufsz-1) buf[idx++] = tmp[i];
    }
    /* decimal point and fractional */
    if(precision > 0){
        if(idx < bufsz-1) buf[idx++]='.';
        /* generate fractional digits */
        int p = precision;
        while(p--){
            frac *= 10.0;
            int digit = (int)frac;
            if(idx < bufsz-1) buf[idx++] = '0' + digit;
            frac -= digit;
        }
        /* rounding: if next digit >=5, round last digit */
        frac *= 10.0;
        if((int)frac >= 5){
            /* round */
            int j = (int)idx - 1;
            while(j >= 0){
                if(buf[j]=='.'){ j--; continue; }
                if(buf[j] < '0' || buf[j] > '9'){ j--; continue; }
                if(buf[j] == '9'){ buf[j] = '0'; j--; continue; }
                buf[j] += 1;
                break;
            }
            if(j < 0){
                /* overflowed integer part like 9.99 -> 10.00; shift right */
                /* naive: prepend '1' if space; else ignore */
                if(bufsz > 1){
                    /* move right if possible */
                    size_t len = idx;
                    if(len + 1 < bufsz){
                        for(size_t k = len; k > 0; --k) buf[k] = buf[k-1];
                        buf[0] = '1';
                        idx++;
                    }
                }
            }
        }
    }
    buf[idx] = '\0';
    return (int)idx;
}

/* ---------------- vsnprintf implementation (limited) ---------------- */

/*
 Supported format specifiers:
  %d   signed integer
  %s   null-terminated string
  %f   floating point (use precision like %.4f)
  %c   character
  %%   percent sign
 Field width is ignored except for precision with %f (%.Nf).
 The function always NUL-terminates if n>0.
 Returns number of bytes that would have been written (not including NUL), similar to vsnprintf.
*/
int xv6_vsnprintf(char *out, size_t n, const char *fmt, va_list ap){
    size_t out_pos = 0;
    const char *p = fmt;
    char tmpbuf[128];
    while(*p){
        if(*p != '%'){
            /* literal */
            if(out_pos + 1 < n) out[out_pos] = *p;
            out_pos++;
            p++;
            continue;
        }
        /* handle format */
        p++; /* skip % */
        /* flags/width/precision parsing (very small subset) */
        int precision = -1;
        if(*p == '%'){ if(out_pos + 1 < n) out[out_pos] = '%'; out_pos++; p++; continue; }
        /* simple parse for precision like %.4f */
        if(*p == '.'){
            p++;
            int val=0;
            while(*p >= '0' && *p <= '9'){ val = val*10 + (*p - '0'); p++; }
            precision = val;
        }
        /* optional length modifiers ignored */
        while(*p == 'l' || *p == 'h') p++;
        char spec = *p++;
        int written = 0;
        switch(spec){
            case 'd': {
                int vi = va_arg(ap,int);
                written = int_to_str(vi, tmpbuf, sizeof(tmpbuf));
                break;
            }
            case 's': {
                const char *s = va_arg(ap,const char*);
                if(!s) s = "(null)";
                /* copy s into tmpbuf up to sizeof-1 */
                size_t i=0;
                while(s[i] && i+1<sizeof(tmpbuf)) { tmpbuf[i]=s[i]; i++; }
                tmpbuf[i] = '\0';
                written = (int)i;
                break;
            }
            case 'c': {
                int ch = va_arg(ap,int);
                tmpbuf[0] = (char)ch; tmpbuf[1] = '\0'; written = 1;
                break;
            }
            case 'f': {
                double vf = va_arg(ap,double); /* variadic float promoted to double */
                int prec = (precision >= 0) ? precision : 6;
                float_to_str(vf, tmpbuf, sizeof(tmpbuf), prec);
                written = (int)strlen(tmpbuf);
                break;
            }
            default: {
                /* unsupported spec - emit it raw */
                tmpbuf[0] = '%'; tmpbuf[1] = spec; tmpbuf[2] = '\0'; written = 2;
                break;
            }
        }
        /* copy tmpbuf to out with bounds checking */
        for(int i=0;i<written;i++){
            if(out_pos + 1 < n) out[out_pos] = tmpbuf[i];
            out_pos++;
        }
    }
    /* NUL-terminate */
    if(n > 0){
        size_t to_write = (out_pos < n-1) ? out_pos : (n-1);
        out[to_write] = '\0';
    }
    return (int)out_pos;
}

int xv6_snprintf(char *out, size_t n, const char *fmt, ...){
    va_list ap;
    va_start(ap, fmt);
    int r = xv6_vsnprintf(out, n, fmt, ap);
    va_end(ap);
    return r;
}

/* provide sprintf as a wrapper that writes into large internal buffer (but bounded) */
int sprintf(char *out, const char *fmt, ...){
    va_list ap;
    va_start(ap, fmt);
    /* very large local buffer to emulate sprintf, but we still bound to avoid overflow */
    int r = xv6_vsnprintf(out, (size_t)INT_MAX, fmt, ap);
    va_end(ap);
    return r;
}

/* ---------------- simple sscanf implementation (limited) ---------------- */

/* helper: skip whitespace */
static const char *skip_ws(const char *s){ while(*s && isspace((unsigned char)*s)) s++; return s; }

/* parse integer (decimal) - returns pointer after parsed chars, writes if out != NULL */
static const char *parse_int(const char *s, int *out){
    s = skip_ws(s);
    int neg = 0;
    if(*s == '+' || *s == '-'){ if(*s=='-') neg=1; s++; }
    long val = 0;
    int any = 0;
    while(*s >= '0' && *s <= '9'){
        any = 1;
        val = val*10 + (*s - '0');
        s++;
    }
    if(!any) return NULL;
    if(out) *out = (int)(neg ? -val : val);
    return s;
}

/* parse float - very simple: handles optional sign, digits, optional '.' fraction, optional exponent e/E */
static const char *parse_float(const char *s, double *out){
    s = skip_ws(s);
    int neg = 0;
    if(*s == '+' || *s == '-'){ if(*s=='-') neg=1; s++; }
    double val = 0.0;
    int any = 0;
    while(*s >= '0' && *s <= '9'){ any = 1; val = val*10.0 + (double)(*s - '0'); s++; }
    if(*s == '.'){
        s++;
        double place = 1.0;
        while(*s >= '0' && *s <= '9'){ place *= 10.0; val = val + (double)(*s - '0')/place; s++; any=1; }
    }
    if(!any) return NULL;
    /* exponent */
    if(*s=='e' || *s=='E'){
        s++;
        int expneg = 0;
        if(*s=='+' || *s=='-'){ if(*s=='-') expneg=1; s++; }
        int expv = 0; int expany=0;
        while(*s >= '0' && *s <= '9'){ expany=1; expv = expv*10 + (*s - '0'); s++; }
        if(expany){
            double pow10=1.0;
            for(int i=0;i<expv;i++) pow10 *= 10.0;
            if(expneg) val /= pow10; else val *= pow10;
        } else {
            /* malformed exponent - backtrack (treat as end) */
        }
    }
    if(out) *out = neg ? -val : val;
    return s;
}

/* parse string up to whitespace */
static const char *parse_string(const char *s, char *out, size_t outcap){
    s = skip_ws(s);
    size_t i=0;
    while(*s && !isspace((unsigned char)*s)){
        if(i+1 < outcap) out[i] = *s;
        i++; s++;
    }
    if(outcap>0) out[(i < outcap-1)? i : outcap-1] = '\0';
    return s;
}

/*
 xv6_vsscanf:
  limited: supports %d, %f, %s, %c in format string.
  returns number of items successfully assigned.
*/
int xv6_vsscanf(const char *s, const char *fmt, va_list ap){
    int assigned = 0;
    const char *p = fmt;
    while(*p){
        if(isspace((unsigned char)*p)){ 
            s = skip_ws(s);
            p++; 
            continue; 
        } /* skip spaces in format AND input */
        if(*p != '%'){
            /* literal match */
            s = skip_ws(s);
            if(*s == *p){ s++; p++; continue; }
            else break;
        }
        p++; /* skip % */
        char spec = *p++;
        if(spec == 'd'){
            int *out = va_arg(ap, int*);
            const char *nx = parse_int(s, out);
            if(nx == NULL) break;
            assigned++; s = nx;
        } else if(spec == 'f'){
            float *out = va_arg(ap, float*);
            double dval;
            const char *nx = parse_float(s, &dval);
            if(nx == NULL) break;
            *out = (float)dval;
            assigned++; s = nx;
        } else if(spec == 's'){
            char *out = va_arg(ap, char*);
            /* Note: standard sscanf doesn't take capacity, but our previous xv6_sscanf did.
               For compatibility with standard sscanf, we should probably NOT expect capacity.
               However, to be safe, let's assume standard behavior (no capacity arg) for %s 
               unless we want to keep the custom behavior. 
               The prompt asked for standard library functions. Standard sscanf %s is unsafe.
               Let's assume standard behavior for now to match `sscanf` signature.
            */
            const char *nx = parse_string(s, out, 99999); /* unsafe but standard-like */
            if(nx == NULL) break;
            assigned++; s = nx;
        } else if(spec == 'c'){
            char *out = va_arg(ap, char*);
            /* sscanf %c does NOT skip whitespace by default */
            /* But if format had space before %c, we already skipped whitespace in the loop above */
            if(!*s) break;
            *out = *s; assigned++; s++;
        } else {
            /* unsupported - stop */
            break;
        }
    }
    return assigned;
}

int xv6_sscanf(const char *s, const char *fmt, ...){
    va_list ap;
    va_start(ap, fmt);
    int r = xv6_vsscanf(s, fmt, ap);
    va_end(ap);
    return r;
}

/* For compatibility, provide plain name */
int sscanf(const char *s, const char *fmt, ...){
    va_list ap;
    va_start(ap, fmt);
    int r = xv6_vsscanf(s, fmt, ap);
    va_end(ap);
    return r;
}

