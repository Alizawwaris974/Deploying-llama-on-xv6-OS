#include "kernel/types.h"
#include "user/user.h"
#include "user/xv6_math.h"
#include "user/test_milestone2.h"

// Include the generated test vectors
#include "math_test_vectors.c"

#define EPSILON 1e-4f
#define fabsf_local(x) ((x) < 0 ? -(x) : (x))

static float relative_error(float a, float b) {
    float diff = fabsf_local(a - b);
    float mag = fabsf_local(b);
    if (mag < 1.0f) return diff;
    return diff / mag;
}

// Helper to check for NaN
static int is_nan(float x) {
    return x != x;
}

// Helper to print float since printf doesn't support %f
static void print_float(float f) {
    if (f != f) { printf("NaN"); return; }
    if (f > 1e30f) { printf("Inf"); return; }
    if (f < -1e30f) { printf("-Inf"); return; }
    
    if (f < 0) {
        printf("-");
        f = -f;
    }
    int int_part = (int)f;
    int frac_part = (int)((f - int_part) * 100000000);
    if (frac_part < 0) frac_part = 0;
    
    printf("%d.", int_part);
    // Manual padding for 8 digits
    if (frac_part < 10000000) printf("0");
    if (frac_part < 1000000) printf("0");
    if (frac_part < 100000) printf("0");
    if (frac_part < 10000) printf("0");
    if (frac_part < 1000) printf("0");
    if (frac_part < 100) printf("0");
    if (frac_part < 10) printf("0");
    printf("%d", frac_part);
}

static void test_results(const char *name, float max_err, float avg_err, int pass, int total) {
    printf("%s: max_error=", name);
    print_float(max_err);
    printf(" avg_error=");
    print_float(avg_err);
    printf(" pass=%d/%d\n", pass, total);
}

void test_math(void) {
    printf("=== xv6 Math Function Tests ===\n");

    // sqrtf test
    {
        float max_err = 0, avg_err = 0;
        int pass = 0;
        for (int i=0; i<NUM_SQRT; i++) {
            float out = xv6_sqrtf(sqrt_inputs[i]);
            float exp = sqrt_expected[i];
            float err;
            if (is_nan(exp) && is_nan(out)) err = 0;
            else if (is_nan(exp) || is_nan(out)) err = 1.0f; // Mismatch in NaN status
            else if (exp > 1e30f && out > 1e30f) err = 0; // Handle Inf
            else err = relative_error(out, exp);
            
            avg_err += err;
            if (err < EPSILON || (is_nan(exp) && is_nan(out))) pass++;
            if (err > max_err) max_err = err;
        }
        avg_err /= NUM_SQRT;
        test_results("sqrtf", max_err, avg_err, pass, NUM_SQRT);
    }

    // expf test
    {
        float max_err = 0, avg_err = 0;
        int pass = 0;
        for (int i=0; i<NUM_EXP; i++) {
            float out = xv6_expf(exp_inputs[i]);
            float exp = exp_expected[i];
            float err;
            // Handle infinity matches
            if (exp > 1e30f && out > 1e30f) err = 0;
            else if (exp < -1e30f && out < -1e30f) err = 0;
            else if (is_nan(exp) && is_nan(out)) err = 0;
            else if (is_nan(exp) || is_nan(out)) err = 1.0f;
            else err = relative_error(out, exp);

            avg_err += err;
            if (err < EPSILON || (is_nan(exp) && is_nan(out)) || (exp == out)) pass++;
            else {
                printf("expf fail: in="); print_float(exp_inputs[i]);
                printf(" exp="); print_float(exp);
                printf(" out="); print_float(out);
                printf("\n");
            }
            if (err > max_err && err < 1e30f) max_err = err; // Ignore inf diffs for max_err stats
        }
        avg_err /= NUM_EXP;
        test_results("expf", max_err, avg_err, pass, NUM_EXP);
    }

    // powf test
    {
        float max_err = 0, avg_err = 0;
        int pass = 0;
        for (int i=0; i<NUM_POW; i++) {
            float out = xv6_powf(pow_x[i], pow_y[i]);
            float exp = pow_expected[i];
            float err;
             if (exp > 1e30f && out > 1e30f) err = 0;
            else if (exp < -1e30f && out < -1e30f) err = 0;
            else if (is_nan(exp) && is_nan(out)) err = 0;
            else if (is_nan(exp) || is_nan(out)) err = 1.0f;
            else err = relative_error(out, exp);

            avg_err += err;
            if (err < EPSILON || (is_nan(exp) && is_nan(out)) || (exp == out)) pass++;
            if (err > max_err && err < 1e30f) max_err = err;
        }
        avg_err /= NUM_POW;
        test_results("powf", max_err, avg_err, pass, NUM_POW);
    }

    // sinf test
    {
        float max_err = 0, avg_err = 0;
        int pass = 0;
        for (int i=0; i<NUM_TRIG; i++) {
            float out = xv6_sinf(sin_inputs[i]);
            float exp = sin_expected[i];
            float err;
            if (is_nan(exp) && is_nan(out)) err = 0;
            else if (is_nan(exp) || is_nan(out)) err = 1.0f;
            else if (exp > 1e30f && out > 1e30f) err = 0; // Handle Inf
            else if (exp < -1e30f && out < -1e30f) err = 0; // Handle -Inf
            else err = relative_error(out, exp);

            avg_err += err;
            if (err < EPSILON || (is_nan(exp) && is_nan(out))) pass++;
            else {
                printf("sinf fail: in="); print_float(sin_inputs[i]);
                printf(" exp="); print_float(exp);
                printf(" out="); print_float(out);
                printf("\n");
            }
            if (err > max_err) max_err = err;
        }
        avg_err /= NUM_TRIG;
        test_results("sinf", max_err, avg_err, pass, NUM_TRIG);
    }

    // cosf test
    {
        float max_err = 0, avg_err = 0;
        int pass = 0;
        for (int i=0; i<NUM_TRIG; i++) {
            float out = xv6_cosf(cos_inputs[i]);
            float exp = cos_expected[i];
            float err;
            if (is_nan(exp) && is_nan(out)) err = 0;
            else if (is_nan(exp) || is_nan(out)) err = 1.0f;
            else if (exp > 1e30f && out > 1e30f) err = 0; // Handle Inf
            else if (exp < -1e30f && out < -1e30f) err = 0; // Handle -Inf
            else err = relative_error(out, exp);

            avg_err += err;
            if (err < EPSILON || (is_nan(exp) && is_nan(out))) pass++;
            else {
                printf("cosf fail: in="); print_float(cos_inputs[i]);
                printf(" exp="); print_float(exp);
                printf(" out="); print_float(out);
                printf("\n");
            }
            if (err > max_err) max_err = err;
        }
        avg_err /= NUM_TRIG;
        test_results("cosf", max_err, avg_err, pass, NUM_TRIG);
    }

    // fabsf test
    {
    }
}
