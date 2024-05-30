#ifndef PROTO_H
#define PROTO_H

#include "parse.h"
#include "common.h"

typedef enum {
    HANDSHAKE_REQUEST,  /* Request from a client to connect to the server, includes protocol version */
    HANDSHAK_RESPONSE,  /* Response to client connecting to server, validates protocol version being used by client*/
    DB_ACCESS_REQUEST,  /* Request from client to access the database */
    DB_ACCESS_RESPONSE, /* Response from server to a client's db access request */
} proto_msg;

#define HANDSHAKE_REQ_SIZE sizeof(proto_msg) + sizeof(uint16_t)
#define HANDSHAKE_RESP_SIZE sizeof(proto_msg) + 1

int send_all(int socket, const void *buf, size_t buf_size, int flags);
int receive_all(int socket, void *buf, size_t buf_size, int flags);
int serialize_add_employee_option(char **buf, char *cursor, size_t *capacity, char *add_employee_str);
int serialize_update_employee_option(char **buf, char *cursor, size_t *capacity, char *update_employee_name, char *shours);
int serialize_delete_employee_option(char **buf, char *cursor, size_t *capacity, char *delete_employee_name);
int serialize_list_option(char **buf, char *cusor, size_t *capacity);
int deserialize_add_employee_option(char **cursor, employee *e);
int deserialize_update_employee_option(char **cursor, char **employee_name, uint32_t *hours);
int deserialize_delete_employee_option(char **cursor, char **employee_name);



#endif
