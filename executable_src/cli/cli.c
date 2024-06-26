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
#include "serialize.h"




void print_usage(char *argv[])
{
    printf("Usage: %s -f <FILE> [OPTIONS]\n", argv[0]);
    printf("\t-n : Flag to create a new database file using the -f argument\n");
    printf("\t-a <EMPLOYEE> : Add a new employee to the database file\n");
    printf("\t-d <EMPLOYEE NAME> : Delete an employee from the database file\n");
    printf("\t-u <EMPLOYEE NAME> : Speicifies the name of the employee to be updated, requires that -h is also provided\n");
    printf("\t-h <EMPLOYEE HOURS> : The hours that will update the employee specified by -u\n");
    printf("\t-l : Flag to list all employees in the database\n");
}


int main(int argc, char *argv[])
{
    char *fname = NULL;
    bool new_file_flag = false;
    char *add_employee_str = NULL;
    char *update_employee_str = NULL;
    char *update_employee_hours_str = NULL;
    char *delete_employee_str = NULL;
    bool list_flag = false;
    int c;

    while ((c = getopt(argc, argv, "f:na:d:u:h:l")) != -1)
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
            case 'h':
                update_employee_hours_str = optarg;
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
            fprintf(stderr, "%s:%s:%d - error creating new file '%s' : (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, fname, errno, strerror(errno));
            exit(1);
        }
        if (write_new_file_hdr(fd) == STATUS_ERROR) 
        {
            exit(1);
        }
		// reset file cursor to beginning
		if (lseek(fd, 0, SEEK_SET) == -1)
		{
			fprintf(stderr, "%s:%s:%d - unable to reset file cursor: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
			exit(1);
		}
    }
    else
    {
        if ((fd = open(fname, O_RDWR, 0666)) == -1)
        {
            fprintf(stderr, "%s:%s:%d - unable to open file %s: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, fname, errno, strerror(errno));
            exit(1);
        }
    }
    
    // Read database file header and stats from file
    db_header dbhdr;
    if(read_dbhdr(fd, &dbhdr) == STATUS_ERROR)
    {
        exit(1);
    }

    // Read employees from data base
    size_t employees_size = dbhdr.employee_count;      
    employee *employees = (employee *) malloc(sizeof(employee) * employees_size);
    if (read_employees(fd, &employees, employees_size) == STATUS_ERROR)
    {
        exit(1);
    }
    
    // process command line arguments
    if (add_employee_str)
    {
        // Reallocate space for new employee
        employees_size++;
        employee *new_employees = realloc(employees, sizeof(employee) * employees_size);
        if (!new_employees)
        {
            fprintf(stderr, "%s:%s:%d error reallocating employees: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
            free(employees);
            exit(1);
        }

        dbhdr.employee_count++;
        employees = new_employees;
        
        if (parse_employee(add_employee_str, employees + (employees_size - 1)) == STATUS_ERROR)
        {
            fprintf(stderr, "%s:%s:%d parse_employee() failed\n", __FILE__, __FUNCTION__, __LINE__);
            free(employees);
            exit(1);
        }
    }

    // validate arugments for updating an employee
    if (update_employee_str && update_employee_hours_str)
    {
        if (update_employee(update_employee_str, update_employee_hours_str, employees, employees_size) == STATUS_ERROR)
        {
            fprintf(stderr, "%s:%s:%d update_employee() failed\n", __FILE__, __FUNCTION__, __LINE__);
            free(employees);
            exit(1);
        }
    }
    else if (update_employee_str && !update_employee_hours_str)
    {
        fprintf(stderr, "required: -h\n");
        print_usage(argv);
        free(employees);
        exit(1);
    }
    else if (!update_employee_str && update_employee_hours_str)
    {
        fprintf(stderr, "required: -u\n");
        print_usage(argv);
        free(employees);
        exit(1);
    }

    if (delete_employee_str)
    {
        if (delete_employee(delete_employee_str, employees, employees_size) == STATUS_ERROR)
        {
            fprintf(stderr, "%s:%s:%d delete_employee() failed\n", __FILE__, __FUNCTION__, __LINE__);
            free(employees);
            exit(1);
        }

		employees_size--;
        dbhdr.employee_count--;
    }

    if (list_flag)
    {
        for (size_t i = 0; i < employees_size; i++)
        {
            printf("%s %s %u\n", employees[i].name, employees[i].address, employees[i].hours);
        }
    }

	// Write output to file
	if (write_db(fd, &dbhdr, employees) == STATUS_ERROR)
	{
		fprintf(stderr, "%s:%s:%d write_db failed()\n", __FILE__, __FUNCTION__, __LINE__);
		free(employees);
		exit(1);
	}

    free(employees);
    return 0;
}
