// user/sha256.c
// SHA-256 implementation for xv6 (adapted for limited C environment)

#include "kernel/types.h"
#include "user/user.h"
#include "user/sha256.h"

// SHA-256 constants (first 32 bits of fractional parts of cube roots of first 64 primes)
static const unsigned int K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

// Helper functions
#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define SHR(x, n) ((x) >> (n))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define EP1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SIG0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ SHR(x, 3))
#define SIG1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ SHR(x, 10))

// SHA-256 context structure
typedef struct {
    unsigned char data[64];
    unsigned int datalen;
    unsigned long long bitlen;
    unsigned int state[8];
} SHA256_CTX;

// Initialize SHA-256 context
static void sha256_init(SHA256_CTX *ctx) {
    ctx->datalen = 0;
    ctx->bitlen = 0;
    // Initial hash values (first 32 bits of fractional parts of square roots of first 8 primes)
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;
}

// Process a single 512-bit block
static void sha256_transform(SHA256_CTX *ctx, const unsigned char data[]) {
    unsigned int a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];

    // Prepare message schedule
    for (i = 0, j = 0; i < 16; ++i, j += 4)
        m[i] = ((unsigned int)data[j] << 24) | ((unsigned int)data[j + 1] << 16) |
               ((unsigned int)data[j + 2] << 8) | ((unsigned int)data[j + 3]);
    for (; i < 64; ++i)
        m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];

    // Initialize working variables
    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    f = ctx->state[5];
    g = ctx->state[6];
    h = ctx->state[7];

    // Main loop
    for (i = 0; i < 64; ++i) {
        t1 = h + EP1(e) + CH(e, f, g) + K[i] + m[i];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    // Add compressed chunk to current hash value
    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

// Update hash with new data
static void sha256_update(SHA256_CTX *ctx, const unsigned char data[], unsigned int len) {
    unsigned int i;

    for (i = 0; i < len; ++i) {
        ctx->data[ctx->datalen] = data[i];
        ctx->datalen++;
        if (ctx->datalen == 64) {
            sha256_transform(ctx, ctx->data);
            ctx->bitlen += 512;
            ctx->datalen = 0;
        }
    }
}

// Finalize hash computation
static void sha256_final(SHA256_CTX *ctx, unsigned char hash[]) {
    unsigned int i;

    i = ctx->datalen;

    // Pad with 0x80 followed by zeros
    if (ctx->datalen < 56) {
        ctx->data[i++] = 0x80;
        while (i < 56)
            ctx->data[i++] = 0x00;
    } else {
        ctx->data[i++] = 0x80;
        while (i < 64)
            ctx->data[i++] = 0x00;
        sha256_transform(ctx, ctx->data);
        for (i = 0; i < 56; ++i)
            ctx->data[i] = 0x00;
    }

    // Append length in bits (big-endian)
    ctx->bitlen += ctx->datalen * 8;
    ctx->data[63] = ctx->bitlen;
    ctx->data[62] = ctx->bitlen >> 8;
    ctx->data[61] = ctx->bitlen >> 16;
    ctx->data[60] = ctx->bitlen >> 24;
    ctx->data[59] = ctx->bitlen >> 32;
    ctx->data[58] = ctx->bitlen >> 40;
    ctx->data[57] = ctx->bitlen >> 48;
    ctx->data[56] = ctx->bitlen >> 56;
    sha256_transform(ctx, ctx->data);

    // Produce final hash value (big-endian)
    for (i = 0; i < 4; ++i) {
        hash[i]      = (ctx->state[0] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 4]  = (ctx->state[1] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 8]  = (ctx->state[2] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 12] = (ctx->state[3] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 16] = (ctx->state[4] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 20] = (ctx->state[5] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 24] = (ctx->state[6] >> (24 - i * 8)) & 0x000000ff;
        hash[i + 28] = (ctx->state[7] >> (24 - i * 8)) & 0x000000ff;
    }
}

// Public API: Compute SHA-256 hash
void sha256_hash(const unsigned char *data, unsigned int len, unsigned char hash[32]) {
    SHA256_CTX ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, data, len);
    sha256_final(&ctx, hash);
}

#ifndef SHA256_LIB
// Helper function to print hash in hex format
static void print_hash(const char *label, const unsigned char hash[32]) {
    printf("%s: ", label);
    for (int i = 0; i < 32; i++) {
        printf("%x%x", (hash[i] >> 4) & 0xf, hash[i] & 0xf);
    }
    printf("\n");
}

// Helper function to compare two hashes
static int compare_hash(const unsigned char hash1[32], const unsigned char hash2[32]) {
    for (int i = 0; i < 32; i++) {
        if (hash1[i] != hash2[i])
            return 0;
    }
    return 1;
}

// Helper to convert hex string to bytes
static void hex_to_bytes(const char *hex, unsigned char *bytes) {
    for (int i = 0; i < 32; i++) {
        unsigned char high = hex[i * 2];
        unsigned char low = hex[i * 2 + 1];
        
        // Convert hex digit to value
        high = (high >= 'a') ? (high - 'a' + 10) : (high - '0');
        low = (low >= 'a') ? (low - 'a' + 10) : (low - '0');
        
        bytes[i] = (high << 4) | low;
    }
}

// Test function
static void run_tests(void) {
    unsigned char hash[32];
    unsigned char expected[32];
    int passed = 0;
    int total = 5;

    printf("=== SHA-256 Test Suite ===\n\n");

    // Test 1: Empty string
    printf("Test 1: Empty string\n");
    sha256_hash((unsigned char *)"", 0, hash);
    hex_to_bytes("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", expected);
    print_hash("Expected ", expected);
    print_hash("Computed ", hash);
    if (compare_hash(hash, expected)) {
        printf("Result: PASS\n\n");
        passed++;
    } else {
        printf("Result: FAIL\n\n");
    }

    // Test 2: Single character "a"
    printf("Test 2: Single character 'a'\n");
    sha256_hash((unsigned char *)"a", 1, hash);
    hex_to_bytes("ca978112ca1bbdcafac231b39a23dc4da786eff8147c4e72b9807785afee48bb", expected);
    print_hash("Expected ", expected);
    print_hash("Computed ", hash);
    if (compare_hash(hash, expected)) {
        printf("Result: PASS\n\n");
        passed++;
    } else {
        printf("Result: FAIL\n\n");
    }

    // Test 3: Short string "hello world"
    printf("Test 3: Short string 'hello world'\n");
    sha256_hash((unsigned char *)"hello world", 11, hash);
    hex_to_bytes("b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9", expected);
    print_hash("Expected ", expected);
    print_hash("Computed ", hash);
    if (compare_hash(hash, expected)) {
        printf("Result: PASS\n\n");
        passed++;
    } else {
        printf("Result: FAIL\n\n");
    }

    // Test 4: Block boundary string (63 bytes)
    printf("Test 4: Block boundary string (63 bytes)\n");
    const char *test4 = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456";
    sha256_hash((unsigned char *)test4, 63, hash);
    hex_to_bytes("03a1f702a13d743806a091b46396152a98aaaf0f169d941a68da9f1f59e031c8", expected);
    print_hash("Expected ", expected);
    print_hash("Computed ", hash);
    if (compare_hash(hash, expected)) {
        printf("Result: PASS\n\n");
        passed++;
    } else {
        printf("Result: FAIL\n\n");
    }

    // Test 5: Multi-block string
    printf("Test 5: Multi-block string\n");
    const char *test5 = "The quick brown fox jumps over the lazy dog. This is a longer test string that spans multiple blocks.";
    sha256_hash((unsigned char *)test5, 103, hash);
    hex_to_bytes("ab9c22bbd49a6ba20396b430803e9cec460a2eb18537a6c73a345ceb55ffe23e", expected);
    print_hash("Expected ", expected);
    print_hash("Computed ", hash);
    if (compare_hash(hash, expected)) {
        printf("Result: PASS\n\n");
        passed++;
    } else {
        printf("Result: FAIL\n\n");
    }

    // Summary
    printf("=========================\n");
    printf("Test Results: %d/%d passed\n", passed, total);
    if (passed == total) {
        printf("All tests PASSED!\n");
    } else {
        printf("Some tests FAILED!\n");
    }
}

// Main function for testing
int main(int argc, char *argv[]) {
    run_tests();
    exit(0);
}
#endif
