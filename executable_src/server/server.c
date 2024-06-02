#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "common.h"
#include "models.h"





void print_usage(char **argv)
{
    printf("usage: %s -a <ADDRESS> -p <PORT> -v <VERSION>\n", argv[0]);
    printf("-a <ADDRESS>: the address of the server\n");
    printf("-p <PORT>: the port of the server\n");
    printf("-v <VERSION>: the protocol version\n");
}


int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        print_usage(argv);
        exit(0);
    }

    // process command line arguments to configure server
    char *address = NULL;
    char *port = NULL;
    char *protocol_version_str = NULL;
    int c;

    while ((c = getopt(argc, argv, ":a:p:v:")) != -1)
    {
        switch (c)
        {
            case 'a':
                address = optarg;
                break;
            case 'p':
                port = optarg;
                break;
            case 'v':
                protocol_version_str = optarg;
                break;
            case ':':
                fprintf(stderr, "missing argument value\n");
                print_usage(argv);
                exit(1);
                break;
            case '?':
                fprintf(stderr, "unknown argument\n");
                print_usage(argv);
                exit(1);
                break;
            default:
                abort();
        }
    }

    // verify command line arguments have non null values
    if (!address || !port || !protocol_version_str)
    {
        print_usage(argv);
        exit(1);
    }

    // convert/validate protocol version
    char *end = NULL;
    long parsed_protocol_version = strtol(protocol_version_str, &end, 10);
    if (!end || *end != '\0' || errno == ERANGE || parsed_protocol_version < 0 || parsed_protocol_version > UINT16_MAX)
    {
        fprintf(stderr, "invalid protocol version %l\n", parsed_protocol_version);
        exit(1);
    }


    return 0;
}
