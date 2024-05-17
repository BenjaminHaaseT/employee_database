#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

//#include "common.h"
#include "parse.h"
//#include "serialize.h"
//
//
//int test_write_new_file_header()
//{
//    // Create a new file for reading and writing a database header to and from
//    int test_fd = open("test_header_file.bin", O_RDWR | O_CREAT | O_TRUNC, 0666);
//    if (write_new_file_hdr(test_fd) == STATUS_ERROR)
//    {
//        fprintf(stderr, "error writing header to new database file\n");
//        return STATUS_ERROR;
//    }   
//
//    
//    db_header dbhdr;
//    if (read(test_fd, &dbhdr, sizeof(db_header)) == STATUS_ERROR)
//    {
//        fprintf(stderr, "error reading header from new database file\n");
//        return STATUS_ERROR;
//    }
//
//    dbhdr.fsize = ntohl(dbhdr.fsize);
//    dbhdr.employee_count = ntohl(dbhdr.employee_count);
//    printf("fsize: %zu\n", dbhdr.fsize);
//    printf("employee count: %zu", dbhdr.employee_count);
//
//    if (dbhdr.fsize != sizeof(dbhdr))
//    {
//        fprintf(stderr, "incorrect file size in database header: %zu should be %zu\n", dbhdr.fsize, sizeof(dbhdr));
//        return STATUS_ERROR;
//    }
//
//    if (dbhdr.employee_count != 0)
//    {
//        fprintf(stderr, "incorrect employee count: %zu should be %zu\n", dbhdr.employee_count, 0);
//        return STATUS_ERROR;
//    }
//
//    return STATUS_SUCCESS;
//}
//
int main(void)
{
    return 0;
}
