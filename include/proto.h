#ifndef PROTO_H
#define PROTO_H

typedef enum {
    HANDSHAK_RESPONSE,  /* Response to client connecting to server */
    DB_ACCESS_REQUEST,  /* Request from client to access the database */
    DB_ACCESS_RESPONSE, /* Response from server to a client's db access request */
} proto_msg;


#endif
