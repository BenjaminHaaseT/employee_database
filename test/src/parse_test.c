#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "common.h"
#include "parse.h"


int test_parse_employee(void)
{

    char employee_str[] = "John Sample,123 easy st.,120";
    employee e;
    if (parse_employee(employee_str, &e) == STATUS_ERROR)
    {
        fprintf(stderr, "%s:%s:%d parse_employee() failed\n", __FILE__, __FUNCTION__, __LINE__);
        return STATUS_ERROR;
    }
    
    if (strcmp(e.name, "John Sample"))
    {
        fprintf(stderr, "%s:%s:%d employee's name parsed incorrectly: '%s' should b 'John Sample'\n", __FILE__, __FUNCTION__, __LINE__, e.name);
        return STATUS_ERROR;
    }

    if (strcmp(e.address, "123 easy st."))
    {
        fprintf(stderr, "%s:%s:%d empoyee's address parsed incorrectly: '%s' should be '123 easy st.'\n", __FILE__, __FUNCTION__, __LINE__, e.address);
        return STATUS_ERROR;
    }

    if (e.hours != 120)
    {
        fprintf(stderr, "%s:%s:%d employee's hours parsed incorrectly: %u should be 120\n", __FILE__, __FUNCTION__, __LINE__, e.hours);
        return STATUS_ERROR;
    }

    return STATUS_SUCCESS;
}



int main(void)
{
    printf("test_parse_employee()...");
    if (test_parse_employee() == STATUS_ERROR)
    {
        printf("failed\n");
        return STATUS_ERROR;
    }
    printf("passed\n");

    return 0;
}
