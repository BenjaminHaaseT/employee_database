#ifndef SERIALIZE_H
#define SERIALIZE_H
#include "common.h"

int serialize_employee(employee *e, unsigned char **buf, size_t *buf_len);
int deserialize_employee(employee *e, unsigned char *buf, size_t *buf_len);
#endif