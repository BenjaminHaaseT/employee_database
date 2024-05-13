#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "common.h"
#include "proto.h"
#include "parse.h"


int write_new_file_hdr(int fd)
{
    db_header hdr = (db_header) { .fsize=ntohl(sizeof(db_header)), .employee_count=ntohl(0) };
    int status;
    if ((status = write(fd, (void*)&hdr, sizeof(db_header))) == -1)
    {
        fprintf(stderr, "%s:%s:%d - unable to write new database header to file: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
        return STATUS_ERROR;
    }

    return STATUS_SUCCESS;
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


 
    
