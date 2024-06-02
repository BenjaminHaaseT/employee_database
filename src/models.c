#include <stdlib.h>
#include <stdio.h>

#include "models.h"

void client_connection_init(client_connection *conn)
{
    conn->state = UNINITIALIZED;
}

void free_client_connection(client_connetcion *conn)
{
    free(conn->buf);
    free(con->header);
    free(conn);
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

void connection_map_resize(connection_map *m)
{
    size_t new_capacity = 2 * m->capacity;
    struct map_node **new_table = malloc(new_capacity * sizeof(map_node*));
    for (size_t i = 0; i < new_capacity; i++)
        new_table[i] = NULL;

    for (size_t i = 0; i < m->capacity; i++)
    {
        map_node *cur = m->table[i];
        while (cur)
        {
            struct map_node *temp = cur->next;
            uint64_t new_hash = hash_key(cur->key);
            size_t new_idx = new_hash % new_capacity;
            cur->next = new_table[new_idx];
            new_table[new_idx] = cur;
            cur = temp;
        }
        m->table[i] = NULL;
    }

    free(m->table);
    m->table = new_table;
    m->capacity = new_capacity;
}


void connection_map_insert(connection_map *m, int key, client_connection *conn)
{
    uint64_t hash = hash_key(key);
    size_t idx = hash % m->capacity;
    struct map_node *cur = m->table[idx];

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
    struct map_node *neo = malloc(sizeof(map_node));
    neo->key = key;
    neo->conn = conn;
    neo->next = m->table[idx];
    m->table[idx] = neo;

    // check if map needs to be resized
    if (((double)m->entry_count / (double)m->capacity) > m->alpha)
        connection_map_resize(m);
}
            
int connection_map_contains(connection_map *m, int key)
{
    uint64_t hash = hash_key(key);
    size_t idx = hash % m->capacity;
    struct map_node *cur = m->table[idx];
    while (cur)
    {
        if (cur->key == key) return 1;
        cur = cur->next;
    }
    return 0;
}


client_connection *connection_map_get(connection_map *m, int key)
{
    uint64_t hash = hash_key(key);
    size_t idx = hash % m->capacity;
    struct map_node *cur = m->table[idx];
    while (cur)
    {
        if (cur->key == key) return cur->conn;
        cur = cur->next;
    }
    return NULL;
}


void connection_map_remove(connection_map *m, int key)
{
    uint64_t hash = hash_key(key);
    size_t idx = hash % m->capacity;
    struct map_node *cur = m->table[idx];
    struct map_node *prev = NULL;
    while (cur)
    {
        if (cur->key == key) break;
        prev = cur;
        cur = cur->next;
    }
    if (cur && prev)
    {
        prev = cur->next;
        free_client_connection(cur->conn);
        free(cur);
    }
    else if (cur && !prev)
    {
        m->table[idx]->next = cur->next;
        free_client_connection(cur->conn);
        free(cur);
    }
}

void free_connection_map(connection_map *m)
{
    for (size_t i = 0; i < m->capacity; i++)
    {
        struct map_node *cur = m->table[i];
        while (cur)
        {
            struct map_node *temp = cur->next;
            free_client_connection(cur->conn);
            free(cur);
            cur = temp;
        }
        m->table[i] = NULL;
    }
    free(m->table);
}
