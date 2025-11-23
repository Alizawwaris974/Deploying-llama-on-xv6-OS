#include "kernel/types.h"
#include "user/user.h"
#include "udp_client.h"
#include "sha256.h"

// Byte order conversion helpers
uint32 htonl(uint32 hostlong) { return hostlong; }
uint32 ntohl(uint32 netlong) { return netlong; }
uint16 htons(uint16 hostshort) { return hostshort; }
uint16 ntohs(uint16 netshort) { return netshort; }

// Simple timeout function
void simple_timeout(int iterations) {
    for(int i = 0; i < iterations; i++) {
        // Busy wait
    }
}

// Send packet with retry logic
int send_with_retry(int sock, uint32 dst_ip, uint16 dst_port, void *data, int len) {
    int retries = 0;
    
    while(retries < 3) {
        if(send(sock, dst_ip, dst_port, (char*)data, len) >= 0) {
            return 0; // Success
        }
        retries++;
        simple_timeout(100000);
    }
    
    return -1; // All retries failed
}

// Receive packet with timeout
int recv_with_timeout(int sock, uint32 *src_ip, uint16 *src_port, void *buffer, int max_len) {
    int iterations = 0;
    int max_iterations = 1000000; // Adjust timeout as needed
    
    while(iterations < max_iterations) {
        int result = recv(sock, src_ip, src_port, (char*)buffer, max_len);
        if(result >= 0) {
            return result; // Packet received
        }
        iterations++;
        simple_timeout(100);
    }
    
    return -1; // Timeout
}

// Base protocol function - fetch file by ID (matches server protocol)
char* fetch_file_by_id(uint8 file_id, int *size_out) {
    const char* file_names[] = {"model", "tokenizer"};
    printf("UDP Client: Starting file fetch for %s (ID: %d)\n", file_names[file_id], file_id);
    
    // Create UDP socket
    int sock = socket(0, 0, 0);
    if(sock < 0) {
        printf("UDP Client: Failed to create socket\n");
        return 0;
    }
    
    // Bind to a client port
    uint16 client_port = 30000 + (getpid() % 1000);
    if(bind(client_port) < 0) {
        printf("UDP Client: Failed to bind to port %d\n", client_port);
        close(sock);
        return 0;
    }
    
    printf("UDP Client: Bound to port %d\n", client_port);
    
    // Send metadata request (matches server protocol)
    // Format: 1 byte msg_type, 1 byte file_id
    char metadata_request[2];
    metadata_request[0] = MSG_METADATA_REQUEST;
    metadata_request[1] = file_id;
    
    printf("UDP Client: Sending metadata request for file ID %d\n", file_id);
    
    if(send_with_retry(sock, SERVER_IP, SERVER_PORT, metadata_request, sizeof(metadata_request)) < 0) {
        printf("UDP Client: Failed to send metadata request\n");
        unbind(client_port);
        close(sock);
        return 0;
    }
    
    // Receive metadata response
    char recv_buffer[4096];
    uint32 src_ip;
    uint16 src_port;
    
    printf("UDP Client: Waiting for metadata response...\n");
    
    int recv_len = recv_with_timeout(sock, &src_ip, &src_port, recv_buffer, sizeof(recv_buffer));
    if(recv_len < 42) { // Minimum size for metadata response
        printf("UDP Client: Timeout or invalid metadata response (got %d bytes)\n", recv_len);
        unbind(client_port);
        close(sock);
        return 0;
    }
    
    // Parse metadata response (matches server protocol)
    // Format: 1 byte msg_type, 1 byte file_id, 4 bytes file_size, 4 bytes num_chunks, 32 bytes SHA-256
    uint8 response_type = recv_buffer[0];
    uint8 response_file_id = recv_buffer[1];
    
    if(response_type != MSG_METADATA_RESPONSE) {
        printf("UDP Client: Unexpected response type: %d (expected %d)\n", response_type, MSG_METADATA_RESPONSE);
        unbind(client_port);
        close(sock);
        return 0;
    }
    
    if(response_file_id != file_id) {
        printf("UDP Client: Response file ID mismatch: %d vs %d\n", response_file_id, file_id);
        unbind(client_port);
        close(sock);
        return 0;
    }
    
    uint32 file_size, num_chunks;
    unsigned char expected_hash[32];
    
    // Extract file size and chunk count (big-endian)
    file_size = (recv_buffer[2] << 24) | (recv_buffer[3] << 16) | (recv_buffer[4] << 8) | recv_buffer[5];
    num_chunks = (recv_buffer[6] << 24) | (recv_buffer[7] << 16) | (recv_buffer[8] << 8) | recv_buffer[9];
    
    // Extract SHA-256 hash
    for(int i = 0; i < 32; i++) {
        expected_hash[i] = recv_buffer[10 + i];
    }
    
    printf("UDP Client: File metadata - size: %d bytes, chunks: %d\n", file_size, num_chunks);
    
    if(file_size <= 0 || num_chunks <= 0) {
        printf("UDP Client: Invalid file metadata\n");
        unbind(client_port);
        close(sock);
        return 0;
    }
    
    // Allocate memory for the file
    char *file_data = malloc(file_size);
    if(!file_data) {
        printf("UDP Client: Failed to allocate %d bytes for file\n", file_size);
        unbind(client_port);
        close(sock);
        return 0;
    }
    
    // Request and receive all data chunks
    printf("UDP Client: Starting data transfer (%d chunks)...\n", num_chunks);
    
    for(uint32 chunk_index = 0; chunk_index < num_chunks; chunk_index++) {
        // Send data request (matches server protocol)
        // Format: 1 byte msg_type, 1 byte file_id, 4 bytes chunk_index
        char data_request[6];
        data_request[0] = MSG_DATA_REQUEST;
        data_request[1] = file_id;
        data_request[2] = (chunk_index >> 24) & 0xFF;  // Big-endian
        data_request[3] = (chunk_index >> 16) & 0xFF;
        data_request[4] = (chunk_index >> 8) & 0xFF;
        data_request[5] = chunk_index & 0xFF;
        
        if(send_with_retry(sock, SERVER_IP, SERVER_PORT, data_request, sizeof(data_request)) < 0) {
            printf("UDP Client: Failed to send data request for chunk %d\n", chunk_index);
            free(file_data);
            unbind(client_port);
            close(sock);
            return 0;
        }
        
        // Receive data response
        recv_len = recv_with_timeout(sock, &src_ip, &src_port, recv_buffer, sizeof(recv_buffer));
        if(recv_len < 8) { // Minimum size for data response
            printf("UDP Client: Timeout or invalid data response for chunk %d\n", chunk_index);
            free(file_data);
            unbind(client_port);
            close(sock);
            return 0;
        }
        
        // Parse data response (matches server protocol)
        // Format: 1 byte msg_type, 1 byte file_id, 4 bytes chunk_index, 2 bytes chunk_size, N bytes data
        response_type = recv_buffer[0];
        response_file_id = recv_buffer[1];
        uint32 response_chunk_index = (recv_buffer[2] << 24) | (recv_buffer[3] << 16) | (recv_buffer[4] << 8) | recv_buffer[5];
        uint16 chunk_size = (recv_buffer[6] << 8) | recv_buffer[7];
        
        if(response_type != MSG_DATA_RESPONSE) {
            printf("UDP Client: Unexpected data response type: %d\n", response_type);
            free(file_data);
            unbind(client_port);
            close(sock);
            return 0;
        }
        
        if(response_chunk_index != chunk_index) {
            printf("UDP Client: Chunk index mismatch: %d vs %d\n", response_chunk_index, chunk_index);
            free(file_data);
            unbind(client_port);
            close(sock);
            return 0;
        }
        
        // Calculate offset and copy data
        uint32 offset = chunk_index * CHUNK_SIZE;
        if(offset + chunk_size > file_size) {
            chunk_size = file_size - offset; // Handle last chunk
        }
        
        if(recv_len < 8 + chunk_size) {
            printf("UDP Client: Incomplete data for chunk %d\n", chunk_index);
            free(file_data);
            unbind(client_port);
            close(sock);
            return 0;
        }
        
        memcpy(file_data + offset, recv_buffer + 8, chunk_size);
        
        // Print progress every 10 chunks
        if(chunk_index % 10 == 0 || chunk_index == num_chunks - 1) {
            printf("UDP Client: Received chunk %d/%d (%d bytes)\n", chunk_index + 1, num_chunks, chunk_size);
        }
    }
    
    printf("UDP Client: All chunks received, verifying checksum...\n");
    
    // Verify SHA-256 checksum
    unsigned char computed_hash[32];
    sha256_hash((unsigned char*)file_data, file_size, computed_hash);
    
    if(memcmp(computed_hash, expected_hash, 32) != 0) {
        printf("UDP Client: SHA-256 checksum verification FAILED\n");
        printf("Expected: ");
        for(int i = 0; i < 32; i++) printf("%02x", expected_hash[i]);
        printf("\nComputed: ");
        for(int i = 0; i < 32; i++) printf("%02x", computed_hash[i]);
        printf("\n");
        
        free(file_data);
        unbind(client_port);
        close(sock);
        return 0;
    }
    
    printf("UDP Client: SHA-256 checksum verification PASSED\n");
    
    unbind(client_port);
    close(sock);
    
    *size_out = file_size;
    printf("UDP Client: Successfully fetched file ID %d (%d bytes)\n", file_id, file_size);
    return file_data;
}

// Fetch model weights file (stories15M.bin)
char* fetch_model_weights(int *size_out) {
    printf("UDP Client: Fetching model weights (file ID %d)...\n", FILE_MODEL);
    return fetch_file_by_id(FILE_MODEL, size_out);
}

// Fetch tokenizer file (tokenizer.bin)
char* fetch_tokenizer(int *size_out) {
    printf("UDP Client: Fetching tokenizer (file ID %d)...\n", FILE_TOKENIZER);
    return fetch_file_by_id(FILE_TOKENIZER, size_out);
}

// Test function
int udp_client_test(void) {
    int weights_size, tokenizer_size;
    
    printf("\n=== UDP Client Test Suite ===\n");
    
    // Test 1: Fetch model weights
    printf("\n--- Test 1: Model Weights ---\n");
    char *weights = fetch_model_weights(&weights_size);
    if(weights) {
        printf("SUCCESS: Model weights fetched: %d bytes\n", weights_size);
        
        // Verify with SHA-256
        unsigned char hash[32];
        sha256_hash((unsigned char*)weights, weights_size, hash);
        printf("SHA-256: ");
        for(int i = 0; i < 32; i++) {
            printf("%02x", hash[i]);
        }
        printf("\n");
        
        free(weights);
    } else {
        printf("FAILED: Could not fetch model weights\n");
    }
    
    // Test 2: Fetch tokenizer
    printf("\n--- Test 2: Tokenizer ---\n");
    char *tokenizer = fetch_tokenizer(&tokenizer_size);
    if(tokenizer) {
        printf("SUCCESS: Tokenizer fetched: %d bytes\n", tokenizer_size);
        
        // Verify with SHA-256
        unsigned char hash[32];
        sha256_hash((unsigned char*)tokenizer, tokenizer_size, hash);
        printf("SHA-256: ");
        for(int i = 0; i < 32; i++) {
            printf("%02x", hash[i]);
        }
        printf("\n");
        
        free(tokenizer);
    } else {
        printf("FAILED: Could not fetch tokenizer\n");
    }
    
    printf("\n=== Test Complete ===\n");
    return 0;
}

int main(int argc, char *argv[]) {
    printf("UDP Client Starting...\n");
    
    // Run the test suite
    udp_client_test();
    
    printf("UDP Client Exiting...\n");
    exit(0);
}