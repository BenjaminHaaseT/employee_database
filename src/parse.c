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

