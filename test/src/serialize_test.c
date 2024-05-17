#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/uio.h>
#include <fcntl.h>

#include "common.h"
#include "serialize.h"

int test_serialize_deserialize_employee(employee *e)
{
    char *employee_init_name = e->name;
    char *employee_init_address = e->address;
    uint32_t employee_init_hours = e->hours;

    printf("Employee prior to serialization:\n");
    printf("\tname: %s\n", employee_init_name);
    printf("\taddress: %s\n", employee_init_address);
    printf("\thours: %u\n", employee_init_hours);

    printf("Serializing employee...");
    unsigned char *buf;
    size_t buf_len;

    if (serialize_employee(e, &buf, &buf_len) == STATUS_ERROR)
    {
        fprintf(stderr, "serialize_employee() failed\n");
        return STATUS_ERROR;
    }
    printf("completed.\n");

    printf("Deserializing employee...");
    employee new_e;
    if (deserialize_employee(&new_e, buf, &buf_len) == STATUS_ERROR)
    {
        fprintf(stderr, "deserialize_employee() failed\n");
        return STATUS_ERROR;
    }
    printf("completed.\n");

    if (strcmp(new_e.name, employee_init_name))
    {
        fprintf(stderr, "employees name did not deserialize properly. Init: %s, Final: %s\n", employee_init_name, new_e.name);
        return STATUS_ERROR;
    }

    if (strcmp(new_e.address, employee_init_address))
    {
        fprintf(stderr, "employees address did not deserialize properly. Init: %s, Final: %s\n", employee_init_address, new_e.address);
        return STATUS_ERROR;
    }

    if (new_e.hours != employee_init_hours)
    {
        fprintf(stderr, "employee hours did not deserialize properly. Init: %u, Final: %u\n", employee_init_hours, new_e.hours);
        return STATUS_ERROR;
    }

    printf("Employee post derialization:\n");
    printf("\tname: %s\n", new_e.name);
    printf("\taddress: %s\n", new_e.address);
    printf("\thours: %u\n", new_e.hours);

    return STATUS_SUCCESS;
}

int test_fserialize_and_fdeserialize(void)
{
    employee e1;
    e1.name = "Joe Sample";
    e1.address = "123 Wallaby Way, Sydney";
    e1.hours = 120;
    
    employee e2;
    e2.name = "Sally Sample";
    e2.address = "456 easy st. Mobile";
    e2.hours = 100;

    // Test file
    int test_fd = open("test_employe_file.bin", O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (test_fd == -1)
    {
        fprintf(stderr, "%s:%s:%d error opening file 'test_employee_file.bin': (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
        return STATUS_ERROR;
    } 
    
    // Serialize employees to file
    if (fserialize_employee(test_fd, &e1) == STATUS_ERROR)
    {
        return STATUS_ERROR;
    }
    if (fserialize_employee(test_fd, &e2) == STATUS_ERROR)
    {
        return STATUS_ERROR;
    }
    
    //  Reset file cursor
    lseek(test_fd, 0, SEEK_SET);

    employee read_e1;

    if (fdeserialize_employee(test_fd, &read_e1) == STATUS_ERROR)
    {
        return STATUS_ERROR;
    }
    
    if (strcmp(read_e1.name, e1.name))
    {
        fprintf(stderr, "%s:%s:%d error deserializing employee name: %s should be %s\n", __FILE__, __FUNCTION__, __LINE__, read_e1.name, e1.name);
        return STATUS_ERROR;
    } 
    if (strcmp(read_e1.address, e1.address))
    {
        fprintf(stderr, "%s:%s:%d error deserializing employee address: %s should be %s\n", __FILE__, __FUNCTION__, __LINE__, read_e1.address, e1.address);
        return STATUS_ERROR;
    } 
    if (read_e1.hours != e1.hours)
    {
        fprintf(stderr, "%s:%s:%d error deserializing employee hours: %u should be %u\n", __FILE__, __FUNCTION__, __LINE__, read_e1.hours, e1.hours);
        return STATUS_ERROR;
    } 
    employee read_e2;

    if (fdeserialize_employee(test_fd, &read_e2) == STATUS_ERROR)
    {
        return STATUS_ERROR;
    }
    
    if (strcmp(read_e2.name, e2.name))
    {
        fprintf(stderr, "%s:%s:%d error deserializing employee name: %s should be %s\n", __FILE__, __FUNCTION__, __LINE__, read_e2.name, e2.name);
        return STATUS_ERROR;
    } 
    if (strcmp(read_e2.address, e2.address))
    {
        fprintf(stderr, "%s:%s:%d error deserializing employee address: %s should be %s\n", __FILE__, __FUNCTION__, __LINE__, read_e2.address, e2.address);
        return STATUS_ERROR;
    } 
    if (read_e2.hours != e2.hours)
    {
        fprintf(stderr, "%s:%s:%d error deserializing employee hours: %u should be %u\n", __FILE__, __FUNCTION__, __LINE__, read_e2.hours, e2.hours);
        return STATUS_ERROR;
    } 

    return STATUS_SUCCESS;
}



int test_write_new_file_header()
{
    // Create a new file for reading and writing a database header to and from
    int test_fd = open("test_header_file.bin", O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (write_new_file_hdr(test_fd) == STATUS_ERROR)
    {
        fprintf(stderr, "error writing header to new database file\n");
        return STATUS_ERROR;
    }   

    lseek(test_fd, 0, SEEK_SET);
 
    db_header dbhdr;
    if (read(test_fd, &dbhdr, sizeof(db_header)) == STATUS_ERROR)
    {
        fprintf(stderr, "error reading header from new database file\n");
        return STATUS_ERROR;
    }
    // convert back to host format
    dbhdr.fsize = ntohl(dbhdr.fsize);
    dbhdr.employee_count = ntohl(dbhdr.employee_count); 
    
    if (dbhdr.fsize != sizeof(dbhdr))
    {
        fprintf(stderr, "incorrect file size in database header: %u should be %zu\n", dbhdr.fsize, sizeof(dbhdr));
        return STATUS_ERROR;
    }

    if (dbhdr.employee_count != 0)
    {
        fprintf(stderr, "incorrect employee count: %u should be %d\n", dbhdr.employee_count, 0);
        return STATUS_ERROR;
    }

    return STATUS_SUCCESS;
}

int main(void)
{
    printf("test_serialize_deserialize_employee()...");
    employee e1;
    e1.name = "John Doe";
    e1.address = "123 abc st. Sydney Australia";
    e1.hours = 120;
    if (test_serialize_deserialize_employee(&e1))
    {
        printf("failed\n");
        return STATUS_ERROR;
    }
        
    printf("------------------------------------------\n\n");

    employee e2;
    e2.name = "Saly Sample";
    e2.address = "456 easy st. Sydney Australia";
    e2.hours = 120;
    if (test_serialize_deserialize_employee(&e2) == STATUS_ERROR)
    {
        printf("failed\n");
        return STATUS_ERROR;
    }
    
    printf("passed\n");

    printf("test_write_new_file_header()...");
    if (test_write_new_file_header() == STATUS_ERROR) 
    {
        printf("failed\n");
        return STATUS_ERROR;
    }
    printf("passed\n");
    
    printf("test_fserialize_and_fdeserialize()...");
    if (test_fserialize_and_fdeserialize() == STATUS_ERROR)
    {
        printf("failed\n");
        return STATUS_ERROR;
    }
    printf("passed\n");

    
    
    return STATUS_SUCCESS;
}


