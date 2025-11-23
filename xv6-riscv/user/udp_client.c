#include "kernel/types.h"
#include "user/user.h"
#include "user/udp_client.h"
#include "user/sha256.h"

// --- Helper Functions for Memory (xv6 lacks string.h) ---

void *my_memset(void *dst, int c, uint n) {
    char *cdst = (char *)dst;
    for(int i = 0; i < n; i++){
        cdst[i] = c;
    }
    return dst;
}

void *my_memcpy(void *dst, const void *src, uint n) {
    const char *s = src;
    char *d = dst;
    for(int i = 0; i < n; i++)
        d[i] = s[i];
    return dst;
}

int my_memcmp(const void *v1, const void *v2, uint n) {
    const uchar *s1, *s2;
    s1 = v1;
    s2 = v2;
    while(n-- > 0){
        if(*s1 != *s2)
            return *s1 - *s2;
        s1++, s2++;
    }
    return 0;
}

// --- Busy Wait Helper (replaces sleep) ---
void busy_wait(int iterations) {
    volatile int i;
    for(i = 0; i < iterations; i++);
}

// --- Core Logic ---

char* fetch_file_by_id(uint8 file_id, int *size_out) {
    const char* file_names[] = {"model", "tokenizer"};
    printf("UDP Client: Starting file fetch for %s (ID: %d)\n", file_names[file_id], file_id);
    
    // 1. Bind to a random client port (30000-30099)
    uint16 client_port = 30000 + (getpid() % 100);
    if(bind(client_port) < 0) {
        printf("UDP Client: Failed to bind port %d\n", client_port);
        return 0;
    }
    
    // 2. Metadata Handshake (with Retry)
    char meta_req[2];
    meta_req[0] = MSG_METADATA_REQUEST;
    meta_req[1] = file_id;
    
    char recv_buf[1024 + 16]; // Large enough for headers + chunk
    uint32 file_size = 0, num_chunks = 0;
    unsigned char expected_hash[32];
    int metadata_received = 0;
    int tries = 0;

    printf("UDP Client: Requesting Metadata...\n");

    while(!metadata_received && tries < 10) {
        // API: send(short sport, int dst, short dport, char *buf, int len)
        send(client_port, SERVER_IP, SERVER_PORT, meta_req, 2);
        
        // Wait for response loop
        int wait_iters = 0;
        // Try receiving for a short period (simple timeout simulation)
        // If recv blocks forever in your kernel, this loop runs once per packet received.
        while(wait_iters < 100) {
             uint32 src_ip = 0;
             uint16 src_port = 0;
             
             // API: recv(short dport, int *src, short *sport, char *buf, int maxlen)
             // Note: In strict xv6 lab, recv might block. If it blocks, we rely on the packet arriving.
             // If your recv is non-blocking or returns -1, this loop handles timing.
             int recv_len = recv(client_port, &src_ip, &src_port, recv_buf, sizeof(recv_buf));
             
             if(recv_len > 0) {
                 // Check if it's the right packet
                 if(recv_buf[0] == MSG_METADATA_RESPONSE && recv_buf[1] == file_id) {
                     metadata_received = 1;
                     break; 
                 }
             }
             
             // Small delay to allow packets to arrive or prevent tight CPU spin
             busy_wait(10000); 
             wait_iters++;
        }
        if (metadata_received) break;
        tries++;
        printf("UDP Client: Retry metadata...\n");
    }

    if(!metadata_received) {
        printf("UDP Client: Failed to receive metadata after retries.\n");
        unbind(client_port);
        return 0;
    }

    // Parse Metadata (Big Endian decode)
    // Format: [Type:1][ID:1][Size:4][Chunks:4][Hash:32]
    file_size = ((uchar)recv_buf[2] << 24) | ((uchar)recv_buf[3] << 16) | ((uchar)recv_buf[4] << 8) | (uchar)recv_buf[5];
    num_chunks = ((uchar)recv_buf[6] << 24) | ((uchar)recv_buf[7] << 16) | ((uchar)recv_buf[8] << 8) | (uchar)recv_buf[9];
    my_memcpy(expected_hash, &recv_buf[10], 32);

    printf("UDP Client: Metadata: Size=%d, Chunks=%d\n", file_size, num_chunks);

    // 3. Allocate Memory
    char *file_data = malloc(file_size);
    if(!file_data) {
        printf("UDP Client: Malloc failed for %d bytes\n", file_size);
        unbind(client_port);
        return 0;
    }
    
    // 4. Data Transfer Loop (Stop-and-Wait ARQ)
    for(uint32 i = 0; i < num_chunks; i++) {
        int chunk_done = 0;
        int chunk_retries = 0;
        
        // Construct Request
        char data_req[6];
        data_req[0] = MSG_DATA_REQUEST;
        data_req[1] = file_id;
        data_req[2] = (i >> 24) & 0xFF;
        data_req[3] = (i >> 16) & 0xFF;
        data_req[4] = (i >> 8) & 0xFF;
        data_req[5] = i & 0xFF;

        while(!chunk_done) {
            if(chunk_retries > 50) {
                printf("UDP Client: Max retries exceeded for chunk %d\n", i);
                free(file_data);
                unbind(client_port);
                return 0;
            }

            // Send Request
            send(client_port, SERVER_IP, SERVER_PORT, data_req, 6);

            // Wait for response
            int wait_iters = 0;
            while(wait_iters < 200) { 
                uint32 src_ip = 0;
                uint16 src_port = 0;
                
                int len = recv(client_port, &src_ip, &src_port, recv_buf, sizeof(recv_buf));
                
                if(len > 0) {
                    // Check Header: [Type:1][ID:1][Idx:4][Size:2]
                    if(recv_buf[0] == MSG_DATA_RESPONSE && recv_buf[1] == file_id) {
                        uint32 recv_idx = ((uchar)recv_buf[2] << 24) | ((uchar)recv_buf[3] << 16) | 
                                          ((uchar)recv_buf[4] << 8) | (uchar)recv_buf[5];
                        
                        if(recv_idx == i) {
                            uint16 d_size = ((uchar)recv_buf[6] << 8) | (uchar)recv_buf[7];
                            
                            // Calculate offset and copy
                            uint32 offset = i * CHUNK_SIZE;
                            // Safety check
                            if(offset + d_size <= file_size) {
                                my_memcpy(file_data + offset, recv_buf + 8, d_size);
                            }
                            
                            chunk_done = 1;
                            break; 
                        }
                    }
                }
                busy_wait(1000); // Short delay
                wait_iters++;
            }
            if(!chunk_done) chunk_retries++;
        }
        
        if(i % 100 == 0 || i == num_chunks - 1)
            printf("UDP Client: Got Chunk %d/%d\n", i+1, num_chunks);
    }

    // 5. Verify Integrity
    printf("UDP Client: Verifying SHA-256...\n");
    unsigned char computed_hash[32];
    sha256_hash((unsigned char*)file_data, file_size, computed_hash);

    if(my_memcmp(computed_hash, expected_hash, 32) != 0) {
        printf("UDP Client: Hash Mismatch!\n");
        printf("Exp: "); for(int k=0;k<4;k++) printf("%x", expected_hash[k]); printf("...\n");
        printf("Got: "); for(int k=0;k<4;k++) printf("%x", computed_hash[k]); printf("...\n");
        free(file_data);
        unbind(client_port);
        return 0;
    }

    printf("UDP Client: Transfer Complete & Verified.\n");
    unbind(client_port);
    
    *size_out = file_size;
    return file_data;
}

char* fetch_model_weights(int *size_out) {
    return fetch_file_by_id(FILE_MODEL, size_out);
}

char* fetch_tokenizer(int *size_out) {
    return fetch_file_by_id(FILE_TOKENIZER, size_out);
}

int udp_client_test(void) {
    int s;
    char *d;
    
    printf("\n--- Test 1: Fetch Tokenizer ---\n");
    d = fetch_tokenizer(&s);
    if(d) {
        printf("TEST: Tokenizer Success (%d bytes)\n", s);
        free(d);
    } else {
        printf("TEST: Tokenizer Failed\n");
    }

    // Uncomment this for the full model download (might take time)
    printf("\n--- Test 2: Fetch Model ---\n");
    d = fetch_model_weights(&s);
    if(d) {
        printf("TEST: Model Success (%d bytes)\n", s);
        free(d);
    } else {
        printf("TEST: Model Failed\n");
    }
    return 0;
}

int main(int argc, char *argv[]) {
    printf("UDP Client Starting...\n");
    udp_client_test();
    printf("UDP Client Exiting...\n");
    exit(0);
}
