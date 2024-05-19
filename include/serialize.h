#ifndef SERIALIZE_H
#define SERIALIZE_H
#include "common.h"

int serialize_employee(employee *e, unsigned char **buf, size_t *buf_len);
int deserialize_employee(employee *e, unsigned char *buf, size_t *buf_len);
int fserialize_employee(int fd, employee *e);
int fdeserialize_employee(int fd, employee *e);
int read_employees(int fd, employee **employees, size_t employees_size);
int write_new_file_hdr(int fd);
int read_dbhdr(int fd, db_header *dbhdr);
int write_employees(int fd, employee *employees, size_t employees_size);
int write_all(int fd, void *buf, size_t buf_size);


#endif
