#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <stdint.h>
#include <arpa/inet.h>

#include "proto.h"
#include "common.h"
//#include "parse.h"


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

int resize_request_buffer(char **buf, char *cursor, size_t *capacity, size_t data_len)
{
    size_t delta;
    if ((delta = *capacity - (cursor - (*buf))) < data_len)
    {
        size_t placeholder = (cursor - (*buf)); 
        *capacity += data_len - delta; 
        char *new_buf = realloc(*buf, *capacity);
        if (!new_buf)
        {
            fprintf(stderr, "%s:%s:%d error reallocating request buffer: (%d) %s failed\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
            return STATUS_ERROR;
        }
        *buf = new_buf;
        cursor = (*buf) + placeholder;
    }

    return STATUS_SUCCESS;
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
    if (name_len > UINT16_MAX)
    {
        fprintf(stderr, "%s:%s:%d size of name exceeds allowed maximum", __FILE__, __FUNCTION__, __LINE__);
        return STATUS_ERROR;
    }

    size_t address_len = strlen(address);
    // ensure address does not exceed allowed maximum size
    if (address_len > UINT16_MAX)
    {
        fprintf(stderr, "%s:%s:%d size of address exceeds allowed maximum", __FILE__, __FUNCTION__, __LINE__);
        return STATUS_ERROR;
    }


    // compute length of data to be serialized
    size_t data_len = name_len + address_len + 2 * sizeof(uint16_t) + sizeof(uint32_t) + 1;

    // ensure we have enough space in the buffer to serialize all the data 
    if (resize_request_buffer(buf, cursor, capacity, data_len) == STATUS_ERROR)
    {
        return STATUS_ERROR;
    }
//    size_t delta;
//    if ((delta = capacity - (cursor - (*buf))) < data_len)
//    {
//        size_t placeholder = (cursor - (*buf)); 
//        *capacity += data_len - delta; 
//        char *new_buf = realloc(*buf, *capacity);
//        if (!new_buf)
//        {
//            fprintf(stderr, "%s:%s:%d error reallocating request buffer: (%d) %s failed\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
//            return STATUS_ERROR;
//        }
//        *buf = new_buf;
//        cursor = (*buf) + placeholder;
//    }

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
    strncpy(cursor, address, address_len);
    cursor += address_len;

    // write hours
    *((uint32_t*)cursor) = (uint32_t)htonl(hours);
    cursor += sizeof(uint32_t);
    
    return STATUS_SUCCESS;
}


int serialize_update_employee_request(char **buf, char *cursor, size_t *capacity, char *update_employee_name, char *shours)
{
    // parse shours into a uint32_t
    uint32_t hours;
    if (parse_employee_hours(shours, &hours) == STATUS_ERROR)
    {
        return STATUS_ERROR;
    }

    // serialize name of employee, validate length does not exceed maximum allowed length
    size_t name_len = strlen(update_employee_name);
    if (name_len > UINT16_MAX)
    {
        fprintf(stderr, "%s:%s:%d size of name exceeds allowed maximum", __FILE__, __FUNCTION__, __LINE__);
        return STATUS_ERROR;
    }


    // compute size of data to be written to buffer
    size_t data_len = sizeof(uint16_t) + sizeof(uint32_t) + name_len + 1;

    // check if there is enough space in the buffer
    if (resize_request_buffer(buf, cursor, capacity, data_len) == STATUS_ERROR)
    {
        return STATUS_ERROR;
    }

    //size_t delta;
    //if ((delta = *capacity - (cursor - (*buf))) < data_len)
    //{
    //    size_t placeholder = (cursor - (*buf));
    //    *capacity += data_len - delta;
    //    char *new_buf = realloc(*buf, *capacity);
    //    if (!new_buf)
    //    {
    //        fprintf(stderr, "%s:%s:%d error reallocating request buffer: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
    //        return STATUS_ERROR;
    //    }
    //    *buf = new_buf;
    //    cursor = (*buf) + placeholder;
    //}

    // write type of request to buffer
    *cursor++ = 'u';

    // write length of name to buffer
    *((uint16_t*)cursor) = htons((uint16_t)name_len);
    cursor += sizeof(uint16_t);

    // write name to buffer
    strncpy(cursor, update_employee_name, name_len);
    cursor += name_len;

    // write hours to buffer
    *((uint32_t*)cursor) = (uint32_t)htonl(hours);
    cursor += sizeof(uint32_t);
    
    return STATUS_SUCCESS;
}


int serialize_delete_employee_request(char **buf, char *cursor, size_t *capacity, char *delete_employee_name)
{
    // compute length of employee name, and validate it does not exceed maximum
    size_t name_len = strlen(delete_employee_name);
    if (name_len > UINT16_MAX)
    {
        fprintf(stderr, "%s:%s:%d size of name exceeds allowed maximum", __FILE__, __FUNCTION__, __LINE__);
        return STATUS_ERROR;
    }

    size_t data_len = sizeof(uint16_t) + name_len + 1;
    // ensure buffer has capacity
    if (resize_request_buffer(buf, cursor, capacity, data_len) == STATUS_ERROR)
    {
        return STATUS_ERROR;
    }
    // size_t delta;
    // if ((delta = *capacity - (cursor - (*buf))) < data_len)
    // {
    //     size_t placeholder = (cursor - (*buf));
    //     *capacity += data_len - delta;
    //     char *new_buf = realloc(*buf, capacity);
    //     if (!new_buf)
    //     {
    //         fprintf(stderr, "%s:%s:%d error reallocating request buffer: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
    //         return STATUS_ERROR;
    //     }

    //     *buf = new_buf;
    //     cursor = (*buf) + placeholder;
    // }

    // write option type to buffer
    *cursor++ = 'd';
    
    // write name length to buffer
    *((uint16_t*)cursor) = (uint16_t)htons((uint16_t)name_len);
    cursor += sizeof(uint16_t);

    // write name to buffer
    strncpy(cursor, delete_employee_name, name_len);
    cursor += name_len;

    return STATUS_SUCCESS;
}

int serialize_list_request(char **buf, char *cursor, size_t *capacity)
{
    // check that we have enough space to add a single character
    if (resize_request_buffer(buf, cursor, capacity, 1) == STATUS_ERROR)
    {
        return STATUS_ERROR;
    }
    // if (*capacity - (cursor - (*buf)) < 1)
    // {
    //     size_t placeholder = (cursor - (*buf));
    //     (*capacity)++;
    //     char *new_buf = realloc(*buf, capacity);
    //     if (!new_buf)
    //     {
    //         fprintf(stderr, "%s:%s:%d error reallocating request buffer: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
    //         return STATUS_ERROR;
    //     }

    //     *buf = new_buf;
    //     cursor = (*buf) + placeholder;
    // }

    *cursor++ = 'l';
    return STATUS_SUCCESS;
}

        


    

    

 

    


