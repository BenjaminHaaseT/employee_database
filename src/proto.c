#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <stdint.h>

#include "proto.h"
#include "common.h"
#include "parse.h"


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

int serialize_add_employee_request(char **buf, char *cursor, size_t *capacity, char *add_employee_str)
{
    char *name, *address;
    uint32_t hours;
    if (parse_employee_vals(add_employee_str, &name, &address, &hours) == STATUS_ERROR)
    {
        fprintf(stderr, "%s:%s:%d parse_employee_vals() failed\n", __FILE__, __FUNCTION__, __LINE__);
        return STATUS_ERROR;
    }

    size_t name_len = strlen(name);
    // ensure name does not exceed allowed maximum
    if (name_len - UINT16_MAX > 0)
    {
        fprintf(stderr, "%s:%s:%d size of name exceeds allowed maximum", __FILE__, __FUNCTION__, __LINE__);
        return STATUS_ERROR;
    }

    size_t address_len = strlen(address);
    // ensure address does not exceed allowed maximum size
    if (address_len - UINT16_MAX > 0)
    {
        fprintf(stderr, "%s:%s:%d size of address exceeds allowed maximum", __FILE__, __FUNCTION__, __LINE__);
        return STATUS_ERROR;
    }


    // compute length of data to be serialized
    size_t data_len = name_len + address_len + 2 * sizeof(uint16_t) + sizeof(uint32_t) + 1;

    // ensure we have enough space in the buffer to serialize all the data 
    size_t delta;
    if ((delta = capacity - (cursor - (*buf))) < data_len)
    {
        size_t placeholder = (cursor - (*buf)); 
        *capacity += data_len - delta; 
        char *new_buf = realloc(*buf, *capacity);
        if (!new_buf)
        {
            fprintf(stderr, "%s:%s:%d reallocating request buffer failed\n", __FILE__, __FUNCTION__, __LINE__);
            return STATUS_ERROR;
        }
        *buf = new_buf;
        cursor = (*buf) + placeholder;
    }

    // serialize the data
    // write option type
    *cursor++ = 'a';
    
    // write name length to buffer
    *((uint16_t*)cursor) = (uint16_t)htons((uint16_t)name_len);
    cursor += sizeof(uint16_t);

    // write name to buffer
    strncpy(cursor, name, name_len);
    cursor += name_len;

    // write address length to buffer
    *((uint16_t*)cursor) = (uint16_t)htons((uint16_t)address_len);
    cursor += sizeof(uint16_t);

    // write address to buffer
    strncpy(cusor, address, address_len);
    cursor += address_len;

    // write hours
    *((uint32_t*)cursor) = htonl(hours);
    cursor += sizeof(uint32_t);
    
    return STATUS_SUCCESS;
}


 

    


