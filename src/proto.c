#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>

#include "proto.h"
#include "common.h"


int send_all(int socket, const void *buf, size_t buf_size, int flags)
{
    size_t total_bytes_sent = 0;
    while (total_bytes_sent < buf_size)
    {
        int nbytes_sent = send(socket, buf + total_bytes_sent, buf_size - total_bytes_sent, flags);
        if (nbytes_sent == -1)
        {
            fprintf(stderr, "%s:%s:%d failed to send bytes: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
            return STATUS_ERROR;
        }
        
        total_bytes_sent += nbytes_sent;
    }
    return total_bytes_sent;
}

int receive_all(int socket, void *buf, size_t buf_size, int flags)
{
    size_t total_bytes_recv = 0;
    while (total_bytes_recv < buf_size)
    {
        int nbytes_recv = recv(socket, buf + total_bytes_recv, buf_size - total_bytes_recv, flags);
        if (nbytes_recv == -1)
        {
            fprintf(stderr, "%s:%s:%d failed to receive bytes: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
            return STATUS_ERROR;
        }
        else if (nbytes_recv == 0)
        {
            return 0;
        }

        total_bytes_recv += nbytes_recv;
    }

    return total_bytes_recv;
}

