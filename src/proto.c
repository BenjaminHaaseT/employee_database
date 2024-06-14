#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <stdint.h>
#include <arpa/inet.h>

#include "proto.h"
//#include "common.h"
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

int resize_buffer(unsigned char **buf, unsigned char **cursor, size_t *capacity, size_t data_len)
{
    size_t delta;
    if ((delta = *capacity - ((*cursor) - (*buf))) < data_len)
    {
        size_t placeholder = ((*cursor) - (*buf)); 
        *capacity += data_len - delta; 
        unsigned char *new_buf = realloc(*buf, *capacity);
        if (!new_buf)
        {
            fprintf(stderr, "%s:%s:%d error reallocating request buffer: (%d) %s failed\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
            return STATUS_ERROR;
        }
        *buf = new_buf;
        (*cursor) = (*buf) + placeholder;
    }

    return STATUS_SUCCESS;
}

int serialize_add_employee_option(unsigned char **buf, unsigned char **cursor, size_t *capacity, char *add_employee_str)
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
    if (resize_buffer(buf, cursor, capacity, data_len) == STATUS_ERROR)
    {
        return STATUS_ERROR;
    }

    // serialize the data
    // write option type
    *(*cursor)++ = 'a';
    
    // write name length to buffer
    uint16_t name_len_n = (uint16_t)htons((uint16_t)name_len);
    *((uint16_t *)(*cursor)) = name_len_n;

    (*cursor) += sizeof(uint16_t);

    // write name to buffer
    strncpy((char*)(*cursor), name, name_len);
    (*cursor) += name_len;

    // write address length to buffer
    *((uint16_t*)(*cursor)) = (uint16_t)htons((uint16_t)address_len);
    (*cursor) += sizeof(uint16_t);

    // write address to buffer
    strncpy((char*)(*cursor), address, address_len);
    (*cursor) += address_len;

    // write hours
    *((uint32_t*)(*cursor)) = (uint32_t)htonl(hours);
    (*cursor) += sizeof(uint32_t);

    return STATUS_SUCCESS;
}

int deserialize_add_employee_option(unsigned char **cursor, employee *e)
{
    // unpack length of name
    uint16_t name_len = ntohs(*((uint16_t*)(*cursor)));
    (*cursor) += sizeof(uint16_t);

    // allocate name of employee and write to it 
    e->name = malloc(name_len + 1);
    strncpy(e->name, (char*)(*cursor), name_len);
    e->name[name_len] = '\0';
    (*cursor) += name_len;

    // unpack length of address
    uint16_t address_len = ntohs(*((uint16_t *)(*cursor)));
    (*cursor) += sizeof(uint16_t);

    // allocate address of employee and write to it
    e->address = malloc(address_len + 1);
    strncpy(e->address, (char*)(*cursor), address_len);
    e->address[address_len] = '\0';
    (*cursor) += address_len;

    // unpack hours
    uint32_t hours = ntohl(*((uint32_t*)(*cursor)));
    e->hours = hours;
    (*cursor) += sizeof(uint32_t);
    
    return STATUS_SUCCESS;
}



int serialize_update_employee_option(unsigned char **buf, unsigned char **cursor, size_t *capacity, char *update_employee_name, char *shours)
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
    if (resize_buffer(buf, cursor, capacity, data_len) == STATUS_ERROR)
    {
        return STATUS_ERROR;
    }

    // write type of request to buffer
    *(*cursor)++ = 'u';

    // write length of name to buffer
    *((uint16_t*)(*cursor)) = htons((uint16_t)name_len);
    (*cursor) += sizeof(uint16_t);

    // write name to buffer
    strncpy((char*)(*cursor), update_employee_name, name_len);
    (*cursor) += name_len;

    // write hours to buffer
    *((uint32_t*)(*cursor)) = htonl(hours);
    (*cursor) += sizeof(uint32_t);
    
    return STATUS_SUCCESS;
}


int deserialize_update_employee_option(unsigned char **cursor, char **employee_name, uint32_t *hours)
{
    // unpack name length from cursor
    uint16_t name_len = ntohs(*((uint16_t*)(*cursor)));
    (*cursor) += sizeof(uint16_t);

    // write to employee's name
    *employee_name = malloc(name_len + 1);
    strncpy(*employee_name, (char*)(*cursor), name_len);
    (*employee_name)[name_len] = '\0';
    (*cursor) += name_len;

    // unpack hours for employee
    *hours = ntohl(*((uint32_t*)(*cursor)));
    (*cursor) += sizeof(uint32_t);
    return STATUS_SUCCESS;
}


int serialize_delete_employee_option(unsigned char **buf, unsigned char **cursor, size_t *capacity, char *delete_employee_name)
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
    if (resize_buffer(buf, cursor, capacity, data_len) == STATUS_ERROR)
    {
        return STATUS_ERROR;
    }

    // write option type to buffer
    *(*cursor)++ = 'd';
    
    // write name length to buffer
    *((uint16_t*)(*cursor)) = htons((uint16_t)name_len);
    (*cursor) += sizeof(uint16_t);

    // write name to buffer
    strncpy((char*)(*cursor), delete_employee_name, name_len);
    (*cursor) += name_len;

    return STATUS_SUCCESS;
}


int deserialize_delete_employee_option(unsigned char **cursor, char **employee_name)
{
    // unpack name length
    uint16_t name_len = ntohs(*((uint16_t*)(*cursor)));
    (*cursor) += sizeof(uint16_t);

    // allocate and write to employee name
    *employee_name = malloc(name_len + 1);
    strncpy(*employee_name, (char*)(*cursor), name_len);
    (*employee_name)[name_len] = '\0';
    (*cursor) += name_len;
    return STATUS_SUCCESS;
}


int serialize_list_option(unsigned char **buf, unsigned char **cursor, size_t *capacity)
{
    // check that we have enough space to add a single character
    if (resize_buffer(buf, cursor, capacity, 1) == STATUS_ERROR)
    {
        return STATUS_ERROR;
    }

    *(*cursor)++ = 'l';
    return STATUS_SUCCESS;
}

    
int serialize_list_employee_response(unsigned char **buf, unsigned char *cursor, uint32_t *buf_len, employee *employees, size_t employees_size)
{
    // serialize each employee one by one into the response buffer
    for (size_t i = 0; i < employees_size; i++)
    {
        // add one to automatically copy null terminating character whenever 'strncpy' is used
        uint16_t name_len = strlen(employees[i].name) + 1;
        uint16_t address_len = strlen(employees[i].address) + 1;

        // allocate buffer with enough space for length of name and address, name and address and hours worked
        size_t employee_len = name_len + address_len + 2 * sizeof(uint16_t) + sizeof(uint32_t);

        // resize response buffer if necessary
        if (resize_buffer(buf, &cursor, (size_t*)buf_len, employee_len) == STATUS_ERROR)
        {
            fprintf(stderr, "%s:%s:%d resize_buffer() failed\n", __FILE__, __FUNCTION__, __LINE__);
            return STATUS_ERROR;
        }
        
        // write name length and name string to buffer
        *((uint16_t *)cursor) = htons(name_len);
        cursor += sizeof(uint16_t);
        strncpy((char*)cursor, employees[i].name, name_len);
        cursor += name_len;

        // write address length and address string to buffer
        *((uint16_t*)cursor) = (uint16_t) htons(address_len);
        cursor += sizeof(uint16_t);
        strncpy((char*)cursor, employees[i].address, address_len);
        cursor += address_len;

        // write hours to buffer
        uint32_t *hours_ptr = (uint32_t*)cursor;
        *hours_ptr = (uint32_t) htonl(employees[i].hours);
        cursor += sizeof(uint32_t);
    }
    return STATUS_SUCCESS;
}

int deserialize_list_employee_response(unsigned char *buf, size_t buf_size, employee **employees, size_t *employees_size)
{
    // for keeping track of the number of bytes we have read from buf
    size_t total_bytes_read = 0;
    unsigned char *cursor = buf;

    // allocate for deserializing employees
    *employees = malloc(32 * sizeof(employee));
    *employees_size = 32;
    size_t employees_len = 0;

    while (total_bytes_read < buf_size)
    {
        // parse name length of employee
        uint16_t name_len = ntohs(*(uint16_t*)cursor);
        cursor += sizeof(uint16_t);
        total_bytes_read += sizeof(uint16_t);

        // copy employee name
        char *name = malloc(name_len);
        strncpy(name, (char*)cursor, name_len);
        cursor += name_len;
        total_bytes_read += name_len;
        
        // parse length of address
        uint16_t address_len = ntohs(*(uint16_t*)cursor);
        cursor += sizeof(uint16_t);
        total_bytes_read += sizeof(uint16_t);

        // copy employee address
        char *address = malloc(address_len);
        strncpy(address, (char*)cursor, address_len);
        cursor += address_len;
        total_bytes_read += address_len;

        // parse hours
        uint32_t hours = ntohl(*(uint32_t*)cursor);
        cursor +=  sizeof(uint32_t);
        total_bytes_read += sizeof(uint32_t);

        // add employee to employees
        if (employees_len == *employees_size)
        {
            *employees_size *= 2;
            employee *new_employees = malloc(*employees_size * sizeof(employee));
            if (!new_employees)
            {
                fprintf(stderr, "%s:%s:%d error reallocating employee buffer: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
                return STATUS_ERROR;
            }

            *employees = new_employees;
        }

        ((*employees) + employees_len)->name = name;
        ((*employees) + employees_len)->address = address;
        ((*employees) + employees_len)->hours = hours;
        employees_len++;
    }

    // resize employee buffer
    employee *resized_employees = realloc(*employees, employees_len * sizeof(employee));
    if (!resized_employees)
    {
        fprintf(stderr, "%s:%s:%d error reallocating employee buffer: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
        return STATUS_ERROR;
    }

    *employees = resized_employees;
    *employees_size = employees_len; 
    return STATUS_SUCCESS;
}


int deserialize_request_options(int fd, employee **employees, db_header *dbhdr, unsigned char **response_buf, client_connection *conn)
{
    // set cursor to beginning of request buffer
    conn->buf_cursor = conn->buf;

    // check for add employee option
    if ((size_t)(conn->buf_cursor - conn->buf) < conn->buf_size && *conn->buf_cursor == 'a')
    {
        // we need to deserialize an add employee request
        // move cursor past option character
        conn->buf_cursor++;

        // add space for new employee
        dbhdr->employee_count++;
        employee *new_employees = realloc(*employees, dbhdr->employee_count);
        if (!new_employees)
        {
            fprintf(stderr, "%s:%s:%d error reallocating employee buffer\n", __FILE__, __FUNCTION__, __LINE__);
            return STATUS_ERROR;
        }

        (*employees) = new_employees;

        // attempt to deserialize add employee request
        if (deserialize_add_employee_option(&conn->buf_cursor, (*employees) + (dbhdr->employee_count - 1)) == STATUS_ERROR)
        {
            fprintf(stderr, "%s:%s:%d deserialize_add_employee_option() failed\n", __FILE__, __FUNCTION__, __LINE__);
            return STATUS_ERROR;
        }

        // write to file
        if (write_db(fd, dbhdr, *employees) == STATUS_ERROR)
        {
            fprintf(stderr, "%s:%s:%d write_db() failed\n", __FILE__, __FUNCTION__, __LINE__);
            return STATUS_ERROR;
        }
    }

    // check for update employee option
    if ((size_t)(conn->buf_cursor - conn->buf) < conn->buf_size && *conn->buf_cursor == 'u')
    {
        // attempt to deserialize update option from request
        conn->buf_cursor++;
        uint32_t hours;
        char *employee_name;

        if (deserialize_update_employee_option(&conn->buf_cursor, &employee_name, &hours) == STATUS_ERROR)
        {
            fprintf(stderr, "%s:%s:%d deserialize_update_employee_option() failed\n", __FILE__, __FUNCTION__, __LINE__);
            return STATUS_ERROR;
        }

        // search for employee
        bool found = false;
        for (size_t i = 0; i < dbhdr->employee_count; i++)
        {
            if (!strcmp((*employees)[i].name, employee_name))
            {
                found = true;
                (*employees)[i].hours = hours;
                break;
            }
        }

        free(employee_name);

        if (!found)
        {
            // write error code 1 to response buffer
            *((*response_buf) + sizeof(proto_msg)) = 1;
            return STATUS_SUCCESS;
        }

        // write to file
        if (write_db(fd, dbhdr, *employees) == STATUS_ERROR)
        {
            fprintf(stderr, "%s:%s:%d write_db() failed\n", __FILE__, __FUNCTION__, __LINE__);
            return STATUS_ERROR;
        }
    }

    // check for delete employee option
    if ((size_t)(conn->buf_cursor - conn->buf) < conn->buf_size && *conn->buf_cursor == 'd')
    {
        // deserialize a delete employee option from request
        // adjust buffer's cursor
        conn->buf_cursor++;

        char *employee_name;
        if (deserialize_delete_employee_option(&conn->buf_cursor, &employee_name) == STATUS_ERROR)
        {
            fprintf(stderr, "%s:%s:%d deserialize_delete_employee_option() failed\n", __FILE__, __FUNCTION__, __LINE__);
            return STATUS_ERROR;
        }

        // search for employee
        bool found = false;
        for (size_t i = 0; i < dbhdr->employee_count; i++)
        {
            if (!strcmp((*employees)[i].name, employee_name))
            {
                found = true;
                (*employees)[i] = (*employees)[--dbhdr->employee_count];
                break;
            }
        }

        free(employee_name);

        if (!found)
        {
            // write error code 1 to response buffer
            *((*response_buf) + sizeof(proto_msg)) = 1;
            return STATUS_SUCCESS;
        }

        // write to file
        if (write_db(fd, dbhdr, *employees) == STATUS_ERROR)
        {
            fprintf(stderr, "%s:%s:%d write_db() failed\n", __FILE__, __FUNCTION__, __LINE__);
            return STATUS_ERROR;
        }
    }

    // check for list option
    if ((size_t)(conn->buf_cursor - conn->buf) < conn->buf_size && *conn->buf_cursor == 'l')
    {
        // we need to serialize all employees into the response buffer
        uint32_t response_header_size = sizeof(proto_msg) + sizeof(uint32_t) + 1;
        uint32_t response_size = response_header_size;
        unsigned char *response_cursor = *response_buf + response_size;
        if (serialize_list_employee_response(response_buf, response_cursor, &response_size, *employees, dbhdr->employee_count) == STATUS_ERROR)
        {
            fprintf(stderr, "%s:%s:%d serialize_list_employee_response() failed\n", __FILE__, __FUNCTION__, __LINE__);
            return STATUS_ERROR;
        }

        // write success flag to response buffer
        *((*response_buf) + sizeof(proto_msg)) = 0;

        // compute length of data
        uint32_t data_len = response_size - response_header_size;

        // write data length to response buffer
        *(uint32_t *)((*response_buf) + sizeof(proto_msg) + 1) = htonl(data_len);
        return STATUS_SUCCESS;
    }

    // write succes flag to response buffer
    *(*response_buf + sizeof(proto_msg)) = 0;
    *(uint32_t*)((*response_buf) + sizeof(proto_msg) + 1) = 0;
    return STATUS_SUCCESS;
}

            
    

 

    


