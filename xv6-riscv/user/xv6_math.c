#include "xv6_math.h"
#include <string.h>
#include <math.h>

#define XV6_LN2 0.69314718056f
#define XV6_INV_LN2 1.44269504089f
#define XV6_PI 3.14159265358979323846f
#define XV6_2PI 6.28318530717958647692f
#define XV6_PI_2 1.57079632679489661923f

static inline unsigned int float_to_bits(float f){unsigned int u;memcpy(&u,&f,sizeof(u));return u;}
static inline float bits_to_float(unsigned int u){float f;memcpy(&f,&u,sizeof(f));return f;}

static float xv6_frexpf(float x, int *exp) {
    if (x == 0.0f) {
        *exp = 0;
        return 0.0f;
    }
    unsigned int i = float_to_bits(x);
    int e = (int)((i >> 23) & 0xFF) - 126;
    *exp = e;
    i &= 0x807FFFFF;
    i |= 0x3F000000;
    return bits_to_float(i);
}

static float xv6_ldexpf(float x, int exp) {
    if (x == 0.0f) return 0.0f;
    unsigned int i = float_to_bits(x);
    int e = (int)((i >> 23) & 0xFF);
    if (e == 0) { // Subnormal
        x *= 33554432.0f; // 2^25
        i = float_to_bits(x);
        e = (int)((i >> 23) & 0xFF) - 25;
    }
    e += exp;
    if (e <= 0) return 0.0f; // Underflow
    if (e >= 255) return (x < 0) ? -INFINITY : INFINITY; // Overflow
    i = (i & 0x807FFFFF) | (e << 23);
    return bits_to_float(i);
}

static float xv6_roundf(float x) {
    return (float)((int)(x + (x >= 0 ? 0.5f : -0.5f)));
}

float xv6_fabsf(float x){unsigned int u;memcpy(&u,&x,sizeof(u));u&=0x7FFFFFFFu;float r;memcpy(&r,&u,sizeof(r));return r;}

float xv6_sqrtf(float x){if(x<0.0f)return 0.0f/0.0f;if(x==0.0f||x==INFINITY)return x;unsigned int i;memcpy(&i,&x,sizeof(i));i=(i>>1)+0x1FC00000u;float y;memcpy(&y,&i,sizeof(y));y=0.5f*(y+x/y);y=0.5f*(y+x/y);return y;}

static float expf_poly(float r){const float c0=1.0f,c1=1.0f,c2=0.49999985f,c3=0.16666552f,c4=0.04165734f,c5=0.00830136f,c6=0.00132988f,c7=0.00014132f;return c0+r*(c1+r*(c2+r*(c3+r*(c4+r*(c5+r*(c6+r*c7))))));}

float xv6_expf(float x){if(x!=x)return x;if(x>88.0f)return INFINITY;if(x<-88.0f)return 0.0f;float kf=(float)((int)(x*XV6_INV_LN2+(x>=0?0.5f:-0.5f)));int k=(int)kf;float r=x-k*XV6_LN2;float y=expf_poly(r);if(k>127)return INFINITY;if(k<-126)return y*xv6_powf(2.0f,(float)k);return xv6_ldexpf(y,k);}

static float xv6_logf(float x){if(x!=x)return x;if(x<=0.0f)return -INFINITY;int ex;float m=xv6_frexpf(x,&ex);float z=(m-1)/(m+1);float z2=z*z;float ln_m=2.0f*(z+z2*z/3.0f+z2*z2*z/5.0f+z2*z2*z2*z/7.0f);return ex*(float)M_LN2+ln_m;}

float xv6_powf(float x,float y){if(y==0.0f)return 1.0f;if(x==1.0f)return 1.0f;if(x==0.0f){if(y>0.0f)return 0.0f;else return INFINITY;}if(x<0.0f){float yi=(float)(int)y;if(y==yi){float r=xv6_expf(y*xv6_logf(-x));if(((int)yi)&1)return -r;else return r;}else return 0.0f/0.0f;}float ln=xv6_logf(x);return xv6_expf(y*ln);}

static float sin_poly(float x){
    const float s3=-1.0f/6.0f,s5=1.0f/120.0f,s7=-1.0f/5040.0f,s9=1.0f/362880.0f;
    float x2=x*x;
    return x+x*x2*(s3+x2*(s5+x2*(s7+x2*s9)));
}
static float cos_poly(float x){
    const float c2=-1.0f/2.0f,c4=1.0f/24.0f,c6=-1.0f/720.0f,c8=1.0f/40320.0f;
    float x2=x*x;
    return 1.0f+x2*(c2+x2*(c4+x2*(c6+x2*c8)));
}

float xv6_sinf(float x){
    if(x!=x)return x;
    if(x==INFINITY || x==-INFINITY) return 0.0f/0.0f; // NaN
    float n=xv6_roundf(x/XV6_2PI);
    x-=n*XV6_2PI;
    int sign=1;
    if(x>XV6_PI)x-=XV6_2PI;
    if(x<-XV6_PI)x+=XV6_2PI;
    if(x>XV6_PI_2){x=XV6_PI-x;sign=1;}
    else if(x<-XV6_PI_2){x=-XV6_PI-x;sign=-1;}
    float r=sin_poly(x);
    return sign*r;
}

float xv6_cosf(float x){
    if(x!=x)return x;
    if(x==INFINITY || x==-INFINITY) return 0.0f/0.0f; // NaN
    float n=xv6_roundf(x/XV6_2PI);
    x-=n*XV6_2PI;
    if(x>XV6_PI)x-=XV6_2PI;
    if(x<-XV6_PI)x+=XV6_2PI;
    int sign=1;
    if(x>XV6_PI_2){x=XV6_PI-x;sign=-1;}
    else if(x<-XV6_PI_2){x=-XV6_PI-x;sign=-1;}
    float r=cos_poly(x);
    return sign*r;
}
