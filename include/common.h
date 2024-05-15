#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

#define STATUS_SUCCESS 0
#define STATUS_ERROR -1

typedef struct {
    char *name;
    char *address;
    uint32_t hours;
} employee;

typedef struct {
    uint32_t fsize;                /* Size of file in bytes */
    uint32_t employee_count;       /* count of employees in file */
} db_header;


#endif
