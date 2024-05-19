#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "common.h"
#include "serialize.h"

int write_all(int fd, void *buf, size_t buf_size)
{
    size_t total = 0;
    while (total < buf_size)
    {
        int n_bytes = write(fd, buf + total, buf_size - total);
        if (n_bytes == -1)
        {
            fprintf(stderr, "%s:%s:%d error writing bytes to file descriptor: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
            return STATUS_ERROR;
        }
        total += n_bytes;
    }
    return total;
}

int serialize_employee(employee *e, unsigned char **buf, size_t *buf_len)
{
    uint16_t name_len = strlen(e->name) + 1;
    uint16_t address_len = strlen(e->address) + 1;

    // allocate buffer with enough space for length of name and address, name and address and hours worked
    *buf_len = name_len + address_len + 2 * sizeof(uint16_t) + sizeof(uint32_t);
    *buf = malloc(*buf_len);
    if (!*buf)
    {
        fprintf(stderr, "%s:%s:%d - error allocating buffer for serializing employee: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
        return STATUS_ERROR;
    }

    // for traversing buffer
    unsigned char *p = *buf;
    
    // write name length and name string to buffer
    *((uint16_t *)p) = (uint16_t) htons(name_len);
    p += sizeof(uint16_t);
    strncpy((char*)p, e->name, name_len);
    p += name_len;
    *p++ = '\0';

    // write address length and address string to buffer
    *((uint16_t*)p) = (uint16_t) htons(address_len);
    p += sizeof(uint16_t);
    strncpy((char*)p, e->address, address_len);
    p += address_len;
    *p++ = '\0';

    // write hours to buffer
    uint32_t *hours_ptr = (uint32_t*)p;
    *hours_ptr = (uint32_t) htonl(e->hours);
    return STATUS_SUCCESS;
}

int deserialize_employee(employee *e, unsigned char *buf, size_t *buf_len)
{
    // for traversing the buffer
    unsigned char *p = buf;

    // unpack name length and name
    uint16_t name_len = ntohs(*(uint16_t *)p);
    p += sizeof(uint16_t);
    char *name = (char*) p;

    // move p to location of address length
    p += name_len + 1;

    // unpack address and length of address
    uint16_t address_len = ntohs(*(uint16_t*)p);

    p += sizeof(uint16_t);
    char *address = (char*)p;

    // move p to location of hours
    p += address_len + 1;

    uint32_t hours = ntohl(*(uint32_t*)p);

    e->name = name;
    e->address = address;
    e->hours = hours;

    return STATUS_SUCCESS;
}


int fserialize_employee(int fd, employee *e)
{
    // add 1 to record the length of the null terminator
    uint16_t name_len = strlen(e->name) + 1;
    uint16_t serialized_name_len = htons(name_len);

    // keep track of total bytes written for current employee
    int total_bytes = 0;
    int nbytes = 0;

    // write serialized name length to file
    if ((nbytes = write_all(fd, &serialized_name_len, sizeof(uint16_t))) == STATUS_ERROR)
    {
        fprintf(stderr, "%s:%s:%d - error writing name length to database file: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
        return STATUS_ERROR;
    }
    total_bytes += nbytes;

    // write name to file
    if ((nbytes = write_all(fd, e->name, name_len)) == STATUS_ERROR)
    {
        fprintf(stderr, "%s:%s:%d - error writing name to database file: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
        return STATUS_ERROR;
    }
    total_bytes += nbytes;

    uint16_t address_len = strlen(e->address) + 1;
    uint16_t serialized_address_len = htons(address_len);

    // write serialized address length to file
    if ((nbytes = write_all(fd, &serialized_address_len, sizeof(uint16_t))) == STATUS_ERROR)
    {
        fprintf(stderr, "%s:%s:%d - error writing address length to database file: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
        return STATUS_ERROR;
    }
    total_bytes += nbytes;


    // write address to file
    if ((nbytes = write_all(fd, e->address, address_len)) == STATUS_ERROR)
    {
        fprintf(stderr, "%s:%s:%d - error writing address to database file: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
        return STATUS_ERROR;
    }
    total_bytes += nbytes;

    uint32_t hours = htonl(e->hours);

    // write serialized hours to database file
    if ((nbytes = write_all(fd, &hours, sizeof(uint32_t))) == STATUS_ERROR)
    {
        fprintf(stderr, "%s:%s:%d - error writing hours to database file: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
        return STATUS_ERROR;
    }
    total_bytes += nbytes;

    return total_bytes;
}


int fdeserialize_employee(int fd, employee *e)
{
    // deserialize name
    uint16_t name_len;
    if (read(fd, &name_len, sizeof(uint16_t)) != sizeof(uint16_t))
    {
        fprintf(stderr, "%s:%s:%d - error reading name length from database file: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
        return STATUS_ERROR;
    }
    
    // convert back to host endianess
    name_len = ntohs(name_len);

    // read name from file
    char *name = malloc(name_len);
    if (read(fd, name, name_len) != name_len)
    {
        fprintf(stderr, "%s:%s:%d - error reading name from database file: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
        return STATUS_ERROR;
    }

    // deserialize address
    uint16_t address_len;
    if (read(fd, &address_len, sizeof(uint16_t)) != sizeof(uint16_t))
    {
        fprintf(stderr, "%s:%s:%d - error reading address length from database file: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
        return STATUS_ERROR;
    }

    // convert back to host endianess
    address_len = ntohs(address_len);

    // read address from file
    char *address = malloc(address_len); 
    if (read(fd, address, address_len) != address_len)
    {
        fprintf(stderr, "%s:%s:%d - error reading address from database file: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
        return STATUS_ERROR;

    }
    
    // deserialize hours
    uint32_t hours;
    if (read(fd, &hours, sizeof(uint32_t)) != sizeof(uint32_t))
    {
        fprintf(stderr, "%s:%s:%d - error reading hours from database file: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
        return STATUS_ERROR;
    }

    hours = ntohl(hours);

    // construct employee
    e->name = name;
    e->address = address;
    e->hours = hours;
    return STATUS_SUCCESS;
}

int write_new_file_hdr(int fd)
{
    db_header hdr = (db_header) { .fsize=htonl(sizeof(db_header)), .employee_count=htonl(0) };
    int status;
    if ((status = write(fd, (void*)&hdr, sizeof(db_header))) == -1)
    {
        fprintf(stderr, "%s:%s:%d - unable to write new database header to file: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
        return STATUS_ERROR;
    }

    return STATUS_SUCCESS;
}

int write_employees(int fd, employee *employees, size_t employees_size)
{
    size_t total_bytes = 0;
    int nbytes = 0;
    for (size_t i = 0; i < employees_size; i++)
    {
        if ((nbytes = fserialize_employee(fd, employees + i)) == STATUS_ERROR)
        {
            fprintf(stderr, "%s:%s:%d unable to write employee to database file: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
            return STATUS_ERROR;
        }
        total_bytes += nbytes;
    }
    return total_bytes;
}

int read_employees(int fd, employee **employees, size_t employees_size)
{
    for (size_t i = 0; i < employees_size; i++)
    {
        if (fdeserialize_employee(fd, *employees + i) == STATUS_ERROR)  
            return STATUS_ERROR;
    }
    
    return STATUS_SUCCESS;
}

int read_dbhdr(int fd, db_header *dbhdr)
{

    // Read database file header and stats from file
    if (read(fd, dbhdr, sizeof(db_header)) == -1)
    {
        fprintf(stderr, "%s:%s:%d - unable to read database header from file: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__,  errno, strerror(errno));
        return STATUS_ERROR;
    }

    // Read stats from file, check file sizes in bytes match
    dbhdr->fsize = ntohl(dbhdr->fsize);
    dbhdr->employee_count = ntohl(dbhdr->employee_count);
    struct stat s;
    if (fstat(fd, &s) == -1)
    {
        fprintf(stderr, "%s:%s:%d - unable to read stats for file: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
        return STATUS_ERROR;
    }

    if (s.st_size != dbhdr->fsize)
    {
        fprintf(stderr, "%s:%s:%d - corrupted data, header file size does not match stat size: header file size = %u, stats file size = %zu\n", __FILE__, __FUNCTION__, __LINE__, dbhdr->fsize, s.st_size);
        return STATUS_ERROR;
    }
    return STATUS_SUCCESS;
}

