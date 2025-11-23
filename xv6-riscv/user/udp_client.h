#ifndef _UDP_CLIENT_H_
#define _UDP_CLIENT_H_

#include "kernel/types.h"

// Protocol constants
#define SERVER_IP 0x0A000202  // 10.0.2.2 (QEMU Host)
#define SERVER_PORT 9999
#define CHUNK_SIZE 1024

// File identifiers
#define FILE_MODEL 0
#define FILE_TOKENIZER 1

// Protocol message types
#define MSG_METADATA_REQUEST 1
#define MSG_METADATA_RESPONSE 2
#define MSG_DATA_REQUEST 3
#define MSG_DATA_RESPONSE 4
#define MSG_ERROR 5

// Base protocol functions
char* fetch_file_by_id(uint8 file_id, int *size_out);

// LLM-specific wrapper functions
char* fetch_model_weights(int *size_out);
char* fetch_tokenizer(int *size_out);

// Test function
int udp_client_test(void);

#endif
