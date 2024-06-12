#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <string.h>

#include "common.h"
#include "proto.h"
#include "models.h"
//#include "parse.h"


int test_serialize_options(void)
{
    size_t header_size = sizeof(proto_msg) + sizeof(uint32_t);
    size_t capacity = header_size;
    
    unsigned char *req_buf = malloc(capacity);
    unsigned char *cursor = req_buf + capacity;

    // test serializing options into buffer
    char add_employee_str[] = "John Doe,123 easy st. New York,120";
    if (serialize_add_employee_option(&req_buf, &cursor, &capacity, add_employee_str) == STATUS_ERROR)
    {
        fprintf(stderr, "%s:%s:%d serialize_add_employee_option() failed\n", __FILE__, __FUNCTION__, __LINE__);
        return STATUS_ERROR;
    }

    char *update_employee_str = "Sally Sample";
    char *update_hours_str = "180";
    if (serialize_update_employee_option(&req_buf, &cursor, &capacity, update_employee_str, update_hours_str) == STATUS_ERROR)
    {
        fprintf(stderr, "%s:%s:%d serialize_update_employee_option() failed\n", __FILE__, __FUNCTION__, __LINE__);
        return STATUS_ERROR;
    }

    char *delete_employee_str = "Suzy Mediocare";
    if (serialize_delete_employee_option(&req_buf, &cursor, &capacity, delete_employee_str) == STATUS_ERROR)
    {
        fprintf(stderr, "%s:%s:%d serialize_delete_employee_option() failed\n", __FILE__, __FUNCTION__, __LINE__);
        return STATUS_ERROR;
    }

    if (serialize_list_option(&req_buf, &cursor, &capacity) == STATUS_ERROR)
    {
        fprintf(stderr, "%s:%s:%d serialize_list_option() failed\n", __FILE__, __FUNCTION__, __LINE__);
        return STATUS_ERROR;
    }

    // simulate writing final length to request buffer
    size_t total_len = (size_t)(cursor - req_buf) - header_size; 
    cursor = req_buf + sizeof(proto_msg);
    *((uint32_t*)cursor) = (uint32_t)htonl((uint32_t) total_len);

    free(req_buf);
    return STATUS_SUCCESS;
}

int test_serialize_deserialize_options(void)
{
    size_t header_size = sizeof(proto_msg) + sizeof(uint32_t);
    size_t capacity = header_size;
    unsigned char *req_buf = malloc(capacity);
    *((proto_msg*)req_buf) = DB_ACCESS_REQUEST;
    unsigned char *cursor = req_buf + capacity;

    // test serializing options into buffer
    char add_employee_str[] = "John Doe,123 easy st. New York,120";
    if (serialize_add_employee_option(&req_buf, &cursor, &capacity, add_employee_str) == STATUS_ERROR)
    {
        fprintf(stderr, "%s:%s:%d serialize_add_employee_option() failed\n", __FILE__, __FUNCTION__, __LINE__);
        return STATUS_ERROR;
    }

    char *update_employee_str = "Sally Sample";
    char *update_hours_str = "180";
    if (serialize_update_employee_option(&req_buf, &cursor, &capacity, update_employee_str, update_hours_str) == STATUS_ERROR)
    {
        fprintf(stderr, "%s:%s:%d serialize_update_employee_option() failed\n", __FILE__, __FUNCTION__, __LINE__);
        return STATUS_ERROR;
    }

    char *delete_employee_str = "Suzy Mediocare";
    if (serialize_delete_employee_option(&req_buf, &cursor, &capacity, delete_employee_str) == STATUS_ERROR)
    {
        fprintf(stderr, "%s:%s:%d serialize_delete_employee_option() failed\n", __FILE__, __FUNCTION__, __LINE__);
        return STATUS_ERROR;
    }

    if (serialize_list_option(&req_buf, &cursor, &capacity) == STATUS_ERROR)
    {
        fprintf(stderr, "%s:%s:%d serialize_list_option() failed\n", __FILE__, __FUNCTION__, __LINE__);
        return STATUS_ERROR;
    }

    // simulate writing final length to request buffer
    size_t total_len = (cursor - req_buf) - header_size;
    cursor = req_buf + sizeof(proto_msg);
    *((uint32_t*)cursor) = (uint32_t)htonl((uint32_t) total_len);

    // now unpack values and ensure they are all correct
    // unpack header and size
    cursor = req_buf;
    if (*(proto_msg*)cursor != DB_ACCESS_REQUEST)
    {
        fprintf(stderr, "%s:%s:%d failed to read type from request buffer\n", __FILE__, __FUNCTION__, __LINE__);
        return STATUS_ERROR;
    }

    cursor += sizeof(proto_msg);
    // deserialize total length of request data
    size_t deserialized_total_len = (uint32_t)ntohl(*(uint32_t*)cursor);
    if (deserialized_total_len != total_len)
    {
        fprintf(stderr, "%s:%s:%d deserializing length failed: '%zu' should be '%zu'\n", __FILE__, __FUNCTION__, __LINE__, deserialized_total_len, total_len);
        return STATUS_ERROR;
    }

    cursor += sizeof(uint32_t);
    // simulate reading the request from the buffer
    unsigned char *deserialized_req_buf = malloc(deserialized_total_len);
    memcpy(deserialized_req_buf, cursor, deserialized_total_len);

    // attempt to read from deserialized_req_buf
    cursor = deserialized_req_buf;
    if (*cursor == 'a')
    {
        cursor++;
        employee e;
        if (deserialize_add_employee_option(&cursor, &e) == STATUS_ERROR)
        {
            fprintf(stderr, "%s:%s:%d deserializing add employee option failed\n", __FILE__, __FUNCTION__, __LINE__);
            return STATUS_ERROR;
        }
        // ensure employee values were deserialized correctly
        if (strcmp("John Doe", e.name))
        {
            fprintf(stderr, "%s:%s:%d deserializing employee name failed: '%s' should be 'John Doe'\n", __FILE__, __FUNCTION__, __LINE__, e.name);
            return STATUS_ERROR;
        }
        if (strcmp("123 easy st. New York", e.address))
        {
            fprintf(stderr, "%s:%s:%d deserializing employee address failed: '%s' should be '123 easy st. New York'\n", __FILE__, __FUNCTION__, __LINE__, e.address);
            return STATUS_ERROR;
        }
        if (120 != e.hours)
        {
            fprintf(stderr, "%s:%s:%d deserializing employee hours failed: '%u' should be '%u'\n", __FILE__, __FUNCTION__, __LINE__, e.hours, 120U);
            return STATUS_ERROR;
        }
    }
    else
    {
        fprintf(stderr, "%s:%s:%d deserializing add option flag failed: %c\n", __FILE__, __FUNCTION__, __LINE__, (char)*cursor);
        return STATUS_ERROR;
    }

    // deserialize update option
    if (*cursor == 'u')
    {
        cursor++;
        char *employee_name;
        uint32_t hours;
        if (deserialize_update_employee_option(&cursor, &employee_name, &hours) == STATUS_ERROR)
        {
            fprintf(stderr, "%s:%s:%d deserializing update employee option failed\n", __FILE__, __FUNCTION__, __LINE__);
            return STATUS_ERROR;
        }

        // ensure deserialized values are correct
        if (strcmp(employee_name, "Sally Sample"))
        {
            fprintf(stderr, "%s:%s:%d deserializing employee name failed: '%s' should be 'Sally Sample'\n", __FILE__, __FUNCTION__, __LINE__, employee_name);
            return STATUS_ERROR;
        }
        if (hours != 180)
        {
            fprintf(stderr, "%s:%s:%d deserializing employee hours failed: '%u' should be '%u'\n", __FILE__, __FUNCTION__, __LINE__, hours, 180U);
            return STATUS_ERROR;
        }
    }
    else
    {
        fprintf(stderr, "%s:%s:%d deserializing update option failed\n", __FILE__, __FUNCTION__, __LINE__);
        return STATUS_ERROR;
    }

    // deserialize delete option
    if (*cursor == 'd')
    {
        cursor++;
        char *employee_name;
        if (deserialize_delete_employee_option(&cursor, &employee_name) == STATUS_ERROR)
        {
            fprintf(stderr, "%s:%s:%d deserializing delete employee option failed\n", __FILE__, __FUNCTION__, __LINE__);
            return STATUS_ERROR;
        }

        // ensure deserialized values are correct
        if (strcmp(employee_name, "Suzy Mediocare"))
        {
            fprintf(stderr, "%s:%s:%d deserializing employee name failed: '%s' should be 'Suzy Mediocare'\n", __FILE__, __FUNCTION__, __LINE__, employee_name);
            return STATUS_ERROR;
        }
    }
    else
    {
        fprintf(stderr, "%s:%s:%d deserializing delete option failed\n", __FILE__, __FUNCTION__, __LINE__);
        return STATUS_ERROR;
    }

    if (*cursor != 'l')
    {
        fprintf(stderr, "%s:%s:%d deserializing list option failed\n", __FILE__, __FUNCTION__, __LINE__);
        return STATUS_ERROR;
    }

    return STATUS_SUCCESS;
}

int test_serialize_list_employee_response(void)
{
    // employees to be serialized
    employee *employees = malloc(3 * sizeof(employee));
    employees[0].name = "John Doe";
    employees[0].address = "123 Wallaby Way, Sydney";
    employees[0].hours = 120;

    employees[1].name = "Sally Sample";
    employees[1].address = "123 easy st, Sydney";
    employees[1].hours = 180;

    employees[2].name = "Suzy Mediocare";
    employees[2].address = "666 Sunny Ln, New York";
    employees[2].hours = 220;

    // for serializing employees
    size_t header_len = sizeof(proto_msg) + sizeof(uint32_t) + 1;
    size_t buf_len = header_len;
    unsigned char *buf = malloc(buf_len);
    *((proto_msg*)buf) = DB_ACCESS_RESPONSE;
    *(buf + sizeof(proto_msg)) = 0;
    
    // cursor for the buffer
    unsigned char *cursor = buf + header_len;

    // test serializing the employees
    if (serialize_list_employee_response(&buf, cursor, (uint32_t*)&buf_len, employees, 3) == STATUS_ERROR)
    {
        fprintf(stderr, "%s:%s:%d serialize_list_employees() failed\n", __FILE__, __FUNCTION__, __LINE__);
        return STATUS_ERROR;
    }

    size_t data_len = buf_len - header_len;
    printf("data_len: %zu\n", data_len);
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
    printf("passed\n\n");

    printf("test_serialize_deserialize_options()...\n");
    if (test_serialize_deserialize_options() == STATUS_ERROR)
    {
        printf("failed\n");
        return STATUS_ERROR;
    }
    printf("passed\n\n");

    printf("test_serialize_list_employee_response()...\n");
    if (test_serialize_list_employee_response() == STATUS_ERROR)
    {
        printf("failed\n");
        return STATUS_ERROR;
    }
    printf("passed\n\n");

    return STATUS_SUCCESS;
}

