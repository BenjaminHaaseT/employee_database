#include <stdint.h>

#ifndef COMMON_H
#define COMMON_H


#define STATUS_SUCCESS 0
#define STATUS_ERROR -1

typedef struct {
    char *name;
    char *address;
    uint32_t hours;
} employee;

typedef struct {
    size_t fsize;                /* Size of file in bytes */
    size_t employee_count;       /* count of employees in file */
} db_header;

#endif