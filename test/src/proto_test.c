#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>

#include "common.h"
#include "proto.h"
//#include "parse.h"


int test_serialize_options(void)
{
    size_t capacity = sizeof(proto_msg) + sizeof(uint32_t);
    char *req_buf = malloc(capacity);
    char *cursor = req_buf + capacity;

    // test serializing options into buffer
    char add_employee_str[] = "John Doe,123 easy st. New York,120";
    if (serialize_add_employee_request(&req_buf, cursor, &capacity, add_employee_str) == STATUS_ERROR)
    {
        fprintf(stderr, "%s:%s:%d serialize_add_employee_request() failed\n", __FILE__, __FUNCTION__, __LINE__);
        return STATUS_ERROR;
    }

    char *update_employee_str = "Sally Sample";
    char *update_hours_str = "180";
    if (serialize_update_employee_request(&req_buf, cursor, &capacity, update_employee_str, update_hours_str) == STATUS_ERROR)
    {
        fprintf(stderr, "%s:%s:%d serialize_update_employee_request() failed\n", __FILE__, __FUNCTION__, __LINE__);
        return STATUS_ERROR;
    }

    char *delete_employee_str = "Suzy Mediocare";
    if (serialize_delete_employee_request(&req_buf, cursor, &capacity, delete_employee_str) == STATUS_ERROR)
    {
        fprintf(stderr, "%s:%s:%d serialize_delete_employee_request() failed\n", __FILE__, __FUNCTION__, __LINE__);
        return STATUS_ERROR;
    }

    if (serialize_list_request(&req_buf, cursor, &capacity) == STATUS_ERROR)
    {
        fprintf(stderr, "%s:%s:%d serialize_list_request() failed\n", __FILE__, __FUNCTION__, __LINE__);
        return STATUS_ERROR;
    }

    // simulate writing final length to request buffer
    size_t total_len = (cursor - req_buf) - capacity;
    cursor = req_buf + sizeof(proto_msg);
    *((uint32_t*)cursor) = (uint32_t)htonl((uint32_t) total_len);

    free(req_buf);
    return STATUS_SUCCESS;
}

int main(void)
{
    printf("test_serialize_options()...\n");
    if (test_serialize_options() == STATUS_ERROR)
    {
        printf("failed\n");
        return STATUS_ERROR;
    }
    printf("passed\n");

    return STATUS_SUCCESS;
}

