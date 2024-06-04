#ifndef MODELS_H
#define MODELS_H

#include <stdint.h>
#include "proto.h"

typedef enum {
    UNINITIALIZED,
    INITIALIZED
} client_state;

typedef struct {
    unsigned char *header;
    unsigned char *buf;
    unsigned char *cursor;
    size_t buf_size;
    client_state state;
} client_connection;

void client_connection_set_handshake_header(client_connection *conn);
void client_connection_init(client_connection *conn);
void free_client_connection(client_connetcion *conn);

// for defining hashmap
#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

uint64_t hash_key(int key);

struct map_node {
    int key;
    client_connection *conn;
    struct map_node *next
}; 

typedef struct {
    struct mape_node **table;
    size_t capacity;
    size_t entry_count;
    double alpha;
} connection_map;

void connection_map_init(connection_map *m, double alpha);
void connection_map_insert(connection_map *m, int key, client_connection *conn);
void connection_map_resize(connection_map *m);
int connection_map_contains(connection_map *m, int key);
client_connection *connection_map_get(connection_map *m, int key);
void connection_map_remove(connection_map *m, int key);
void free_connection_map(connection_map *m);

#endif
