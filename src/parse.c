#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "parse.h"
#include "common.h"


int parse_employee(char *employee_str, employee *e)
{

    char *name = strtok(employee_str, ",");
    char *address = strtok(NULL, ",");
    char *shours = strtok(NULL, ",");
    if (!name || !address || !shours)
    {
        fprintf(stderr, "%s:%s:%d - bad argument, <EMPLOYEE>: 'name,address,hours'\n", __FILE__, __FUNCTION__, __LINE__);
        return STATUS_ERROR;
    }

    // convert shours to unsigned integer
    char *end_ptr = NULL;
    uint32_t hours = (uint32_t) strtol(shours, &end_ptr, 10);
    if (end_ptr == shours || *end_ptr != '\0')
    {
        fprintf(stderr, "bad argument: (%d) %s\n", errno, strerror(errno));
        return STATUS_ERROR;
    }

    e->name = name;
    e->address = address;
    e->hours = hours;
    return STATUS_SUCCESS;
}


int update_employee(char *employee_name, char *shours, employee *employees, size_t employees_size)
{
    // parse employee hours
    char *end = NULL;
    uint32_t hours = strtol(shours, &end, 10);
    if (!end || end == shours || *end != '\0')
    {
        fprintf(stderr, "unable to parse hours correctly\n");
        return STATUS_ERROR;
    }

    // search for the employee to update
    for (size_t i = 0; i < employees_size; i++)
    {
        if (!strcmp(employees[i].name, employee_name))
        {
            employees[i].hours = hours;
            return STATUS_SUCCESS;
        }
    }

    fprintf(stderr, "employee '%s' not present in database\n", employee_name);
    return STATUS_ERROR;
}

int delete_employee(char *employee_name, employee *employees, size_t *employees_size)
{
    // search for employee to be deleted
    for (size_t i = 0; i < employees_size; i++)
    {
        if (!strcmp(employee_name, employees[i].name))
        {
            employee temp = employees[*employees_size - 1];
            employees[*employees_size - 1] = employees[i];
            employees[i] = temp;
            free(employees[*employees_size - 1].name);
            free(employees[*employees_size - 1].address);
            *employees_size -= 1;
            return STATUS_SUCCESS;
        }
    }

    fprintf(stderr, "'%s' not present in database\n", employee_name);
    return STATUS_ERROR;
}

        


