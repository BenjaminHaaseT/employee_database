#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>

#include "common.h"
#include "parse.h"




void print_usage(char *argv[])
{
    printf("Usage: %s -f <FILE> [OPTIONS]\n", argv[0]);
    printf("\t-n : Flag to create a new database file from -f argument\n");
    printf("\t-a <EMPLOYEE> : Add a new employee to the database file\n");
    printf("\t-d <EMPLOYEE NAME> : Delete an employee from the database file\n");
    printf("\t-u <EMPLOYEE NAME> <EMPLOYEE HOURS> : Updates the hours for the employee with name <EMPLOYEE NAME>\n");
    printf("\t-l : Flag to list all employees in the database\n");
}


int main(int argc, char *argv[])
{
    char *fname = NULL;
    bool new_file_flag = false;
    char *add_employee_str = NULL;
    char *update_employee_str = NULL;
    char *delete_employee_str = NULL;
    bool list_flag = false;
    int c;

    while ((c = getopt(argc, argv, "f:na:d:u:l")) != -1)
    {
        switch (c)
        {
            case 'f':
                fname = optarg;
                break;
            case 'n':
                new_file_flag = true;
                break;
            case 'a':
                add_employee_str = optarg;
                break;
            case 'd':
                delete_employee_str = optarg;
                break;
            case 'u':
                update_employee_str = optarg;
                break;
            case 'l':
                list_flag = true;
                break;
            case '?':
                print_usage(argv);
                exit(0);
            default:
                fprintf(stderr, "%s:%s:%d - unknown error occurred when parsing command line arguments\n", __FILE__, __FUNCTION__, __LINE__);
                exit(1);
        } 
    }

    if (!fname)
    {
        print_usage(argv);
        exit(0);
    }

    int fd;
    if (new_file_flag)
    {
        if ((fd = open(fname, O_RDWR | O_CREAT | O_EXCL, 0666)) == -1)
        {
            fprintf(stderr, "%s:%s:%d - file %s already exists: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, fname, errno, strerror(errno));
            exit(1);
        }
        if (write_new_file_hdr(fd) == STATUS_ERROR) 
        {
            exit(1);
        }
    }
    else
    {
        if ((fd = open(fname, O_RDWR, 0666)) == -1)
        {
            fprintf(stderr, "%s:%s:%d - unable to open file %s: (%s) %s\n", __FILE__, __FUNCTION__, __LINE__, fname, errno, strerror(errno));
            exit(1);
        }
    }
    
    // Read database file header and stats from file
    db_header dbhdr;
    if (read(fd, &dbhrd, sizeof(db_header)) == -1)
    {
        fprintf(stderr, "%s:%s:%d - unable to read database header from file %s: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, fname, errno, strerror(errno));
        exit(1);
    }
    // Read stats from file, check file sizes in bytes match
    dbhdr.fsize = ntohl(dbhdr.fsize);
    dbhdr.employee_count = ntohl(dbhdr.employee_count);
    struct stat;
    if (fstat(fd, &stat) == -1)
    {
        fprintf(stderr, "%s:%s:%d - unable to read stats for file %s: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, fname, errno, strerror(errno));
        exit(1);
    }

    if (stat.st_size != dbhdr.fsize)
    {
        fprintf(stderr, "%s:%s:%d - corrupted data, header file size does not match stat size: header file size = %zu, stats file size = %zu\n", __FILE__, __FUNCTION__, __LINE__, dbhdr.fsize, stat.st_size);
        exit(1);
    }

    // Read employees from data base
    employee *employees;
    size_t employees_size = dbhdr.employee_count;      
 
    


}
