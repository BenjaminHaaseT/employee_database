#include <stdlib.h>
#include <stdio.h>

#include "models.h"

void client_connection_init(client_connection *conn)
{
    conn->state = UNINITIALIZED;
}


uint64_t hash_key(int key)
{
    uint64_t hash = FNV_OFFSET;
    for (size_t i = 0; i < sizeof(int); i++)
    {
        hash ^= (uint64_t)((key >> (i * 8)) & 0xff);
        hash *= FNV_PRIME;
    }
    return hash;
}

void connection_map_init(connection_map *m, double alpha)
{
    m->table = malloc(101 * sizeof(client_connection *));
    for (size_t i = 0; i < 101; i++)
        m->table[i] = NULL;

    m->capacity = 101;
    m->entry_count = 0;
    m->alpha = alpha;
}


void connection_map_insert(connection_map *m, int key, client_connection *conn)
{
    uint64_t hash = hash_key(key);
    size_t idx = hash % m->capacity;
    map_node *cur = m->table[idx];

    // check if there already exists a value for key
    while (cur)
    {
        if (cur->key == key)
        {
            free_client_connection(cur->conn);
            cur->conn = conn;
            return;
        }
        cur = cur->next;
    }
    
    // instantiate a new node to put into hash map
    map_node *neo = malloc(sizeof(map_node));
    neo->key = key;
    neo->conn = conn;
    neo->next = m->table[idx];
    m->table[idx] = neo;

    // check if map needs to be resized
    if (((double)m->entry_count / (double)m->capacity) > m->alpha)
        connection_map_resize(m);
}
            








int connection_map_contains(connection_map *m, int key);
client_connection *connection_map_get(connection_map *m, int key);
void connection_map_remove(connection_map *m, int key);
void free_connection_map(connection_map *m);

#endif
