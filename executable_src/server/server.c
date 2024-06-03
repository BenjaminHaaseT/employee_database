#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>

#include "common.h"
#include "models.h"
#include "proto.h"


void print_usage(char **argv);
int get_listener_socket(char *address, char *port);
int add_fd(struct pollfd *pfds, size_t *fd_count, nfds_t *fd_size);


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

    // get listener for accepting new connections
    int listener = get_listener_socket(address, port);
    if (listener == STATUS_ERROR)
    {
        fprintf(stderr, "getting listener socket failed\n");
        exit(1);
    }

    // for polling sockets
    nfds_t fd_count = 0;
    size_t fd_size = 16;
    struct pollfd *pfds = malloc(fd_size * sizeof(struct pollfd));

    if (add_fd(pfds, &fd_count, &fd_size) == STATUS_ERROR)
    {
        fprintf(stderr, "unable to add listener to file descriptor set\n");
        exit(1);
    }

    // accpet loop
    while (1)
    {
    



    }

    




    return 0;
}


void print_usage(char **argv)
{
    printf("usage: %s -a <ADDRESS> -p <PORT> -v <VERSION>\n", argv[0]);
    printf("-a <ADDRESS>: the address of the server\n");
    printf("-p <PORT>: the port of the server\n");
    printf("-v <VERSION>: the protocol version\n");
}


int get_listener_socket(char *address, char *port)
{
    // for getting socket info
    int status;
    struct addrinfo hints, *ai, *p;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    // read address info
    if ((status = getaddrinfo(address, port, &hints, &ai)))
    {
        fprintf(stderr, "%s:%s:%d getaddrinfo() failed: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, status, gai_strerror(status));
        return STATUS_ERROR;
    }

    // attempt to create/bind a socket
    int listener;
    int yes = 1;
    for (p = ai; p; p = p->ai_next)
    {
        // get socket
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener == -1)
            continue;

        // to prevent 'address already in use errors'
        if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            fprintf(stderr, "%s:%s:%d failed to set socket options: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
            return STATUS_ERROR;
        }

        if (bind(listener, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(listener);
            continue;
        }
    }

    // validate we have a listener
    freeaddrinfo(ai);
    if (!p)
    {
        fprintf(stderr, "%s:%s:%d unable to bind socket\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
        return STATUS_ERROR;
    }

    if (listen(listener, 10) == -1)
    {
        fprintf(stderr, "%s:%s:%d listen() failed: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
        return STATUS_ERROR;
    }

    return listener;
}
        

int add_fd(struct pollfd **pfds, size_t *fd_count, nfds_t *fd_size, int fd)
{
    if (*fd_count == *fd_size)
    {
        (*fd_size) *= 2;
        struct pollfd *new_pfds = realloc(*pfds, *fd_size * sizeof(struct pollfd));
        if (!new_pfds)
        {
            fprintf("%s:%s:%d unable to reallocate file descriptor set\n", __FILE__, __FUNCTION__, __LINE__);
            return STATUS_ERROR;
        }
        *pfds = new_pfds;
    }
    (*pfds)[*fd_count].fd = fd;
    (*pfds)[*fd_count].events = POLLIN;
    (*fd_count)++;
}





    

