#ifndef PROTO_H
#define PROTO_H

typedef enum {
    HANDSHAKE_REQUEST,  /* Request from a client to connect to the server, includes protocol version */
    HANDSHAK_RESPONSE,  /* Response to client connecting to server, validates protocol version being used by client*/
    DB_ACCESS_REQUEST,  /* Request from client to access the database */
    DB_ACCESS_RESPONSE, /* Response from server to a client's db access request */
} proto_msg;

int send_all(int socket, const void *buf, size_t buf_size, int flags);


#endif
