#include "common.h"


#ifndef PARSE_H
#define PARSE_H


int parse_employee(char *employee_str, employee *e);
int parse_employee_vals(char *employee_str, char **name, char **address, uint32_t *hours);
int update_employee(char *employee_name, char *shours, employee *employees, size_t employees_size);
int delete_employee(char *employee_name, employee *employees, size_t employees_size);


#endif
