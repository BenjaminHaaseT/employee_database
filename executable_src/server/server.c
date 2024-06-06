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

#define ALPHA 0.5L
#define MAX_SERV_LEN 100


void print_usage(char **argv);
int get_listener_socket(char *address, char *port);
int add_fd(struct pollfd *pfds, size_t *fd_count, nfds_t *fd_size);
int receive_from_client(int client_fd, client_connection *conn);
int send_handshake_response(int client_fd, unsigned char flag);


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

    if (add_fd(&pfds, &fd_count, &fd_size) == STATUS_ERROR)
    {
        fprintf(stderr, "unable to add listener to file descriptor set\n");
        exit(1);
    }

    // for mapping client file descriptors to client connection states
    connection_map client_connections;
    connection_map_init(&client_connections, ALPHA);

    // accpet loop
    while (1)
    {
        int poll_count = poll(pfds, fd_count, -1);
        if (poll_count == -1)
        {
            fprintf(stderr, "error polling sockets: (%d) %s\n", errno, strerror(errno));
            exit(1);

        }

        for (size_t i = 0; i < fd_count; i++)
        {
            // check if socket is ready to be read from
            if (pfds[i].revents & POLLIN)
            {
                if (pfds[i].fd == listener)
                {
                    // We are accepting a new client
                    struct sockaddr_storage client_addr;
                    socklen_t client_addrlen = sizeof(client_addr);
                    int client_fd = accept(listener, (struct sockaddr *)&client_addr, &client_addrlen);

                    // check if accepting failed
                    if (client_fd == -1)
                    {
                        fprintf(stderr, "accepting client failed: (%d) %s\n", errno, strerror(errno));
                        continue;
                    }
                    else
                    {
                        char client_addr_buf[INET6_ADDRSTRLEN];
                        char client_serv_buf[MAX_SERV_LEN];
                        int status = getnameinfo(
                                (struct sockaddr *)&client_addr, client_addrlen, 
                                client_addr_buf, INET6_ADDRSTRLEN, 
                                client_serv_buf, MAX_SERV_LEN, NI_NUMERICHOST | NI_NUMERICSERV);

                        if (status)
                        {
                            fprintf(stderr, "unable to get name info for client: %s\n", gai_strerror(status));
                        }
                        else
                        {
                            printf("accepting new connection from %s:%s\n", client_addr_buf, client_serv_buf);
                        }

                        // add new client to pfds and map client socket to new client connection

                        size_t conn_idx = fd_count;
                        if (add_fd(&pfds, &fd_count, &fd_size, client_fd) == STATUS_ERROR)
                        {
                            fprintf(stderr, "unable to add client file descriptor\n");
                            exit(1);
                        }

                        // create new client connection for mapping
                        client_connection *client_conn = malloc(sizeof(client_connection));
                        client_connection_init(client_conn, conn_idx);
                        connection_map_insert(&client_connections, client_fd, client_conn);
                    }
                }
                else
                {
                    // get client connection from map
                    if (!connection_map_contains(&client_connections, pfds[i].fd))
                    {
                        fprintf(stderr, "invalid client file descriptor\n");
                        exit(1);
                    }

                    client_connection *conn = connection_map_get(&client_connections, pfds[i].fd);

                    // read into clients buffer
                    int nbytes_read = 0;
                    if ((nbytes_read = receive_from_client(pfds[i].fd, conn)) == STATUS_ERROR)
                    {
                        fprintf(stderr, "receive_from_client() failed\n");
                        exit(1);
                    }
                    else if (nbytes_read == 0)
                    {
                        // client's connection has terminated
                        printf("client disconnected\n");
                        size_t conn_idx = conn->conn_idx;
                        free_client_connection(conn);
                        remove_fd(pfds, &fd_count conn_idx);
                        connection_map_remove(&client_connections, pfds[i].fd);
                    }
                    else if (conn->state == UNINITIALIZED && nbytes_read == sizeof(proto_msg) + sizeof(uint16_t))
                    {
                        // check message type
                        proto_msg msg_type = *(proto_msg *)(conn->header);
                        if (msg_type != HANDSHAKE_REQUEST)
                        {
                            fprintf(stderr, "illegal message type received from client\n");
                            // send error
                           if (send_handshake_response(pfds[i].fd, 0) == STATUS_ERROR)
                           {
                               fprintf("send_handshake_response() failed\n");
                           }
                        }
                        else
                        {
                            uint16_t client_protocol_version = ntohs(*(uint16_t *)(conn->header + sizeof(proto_msg)));
                            int flag;
                            if (client_protocol_version != parsed_protocol_version)
                            {
                                flag = 0;
                            }
                            else
                            {
                                flag = 1;
                                conn->state = INITIALIZED;
                                conn->header_cursor = conn->header;
                            }

                           if (send_handshake_response(pfds[i].fd, flag) == STATUS_ERROR)
                           {
                               fprintf("send_handshake_response() failed\n");
                           }
                        }
                    }
                    else if (conn->state == INITIALIZED && nbytes_read == sizeof(proto_msg) + sizeof(uint32_t))
                    {
                        // parse message type 
                        proto_msg msg_type = *(proto_msg *)(conn->header);
                        if (msg_type != DB_ACCESS_REQUEST)
                        {
                            fprintf(stderr, "illegal message type received from client\n");
                            // TODO: send error
                        }


                        // parse data length and allocate buffer for connection

                        // transition state of connection

                        // attempt to read anymore bytes into data buffer from clients socket


                    }
                    else if (conn->state == REQUEST && nbytes_read == 

                        










                      





                

                    

    



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
    return STATUS_SUCCESS;
}

void remove_fd(struct pollfd *pfds, size_t *fd_count, size_t conn_idx)
{
    pfds[conn_idx] = pfds[--(*fd_count)];
}


int receive_from_client(int client_fd, client_connection *conn)
{
    if (conn->state == UNINITIALIZED)
    {
        size_t bytes_rem = sizeof(proto_msg) + sizeof(uint16_t) - (size_t)(conn->header_cursor - conn->header);
        int nbytes_read = 0;
        if ((nbytes_read = recv(client_fd, conn->header_cursor, bytes_rem, 0)) == -1)
        {
            fprintf(stderr, "%s:%s:%d unable to receive header bytes from client's socket: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
            return STATUS_ERROR;
        }
        
        if (nbytes_read == 0) return 0;     // client has terminated their connection

        // adjust cursor and return total bytes that have been read so far
        conn->header_cursor += nbytes_read;
        return (int)(conn->header_cursor - conn->header);
    }
    else if (conn->state == INITIALIZED)
    {
        size_t header_bytes_rem = sizeof(proto_msg) + sizeof(uint32_t) - (size_t)(conn->header_cursor - conn->header);
        int nbytes_read = 0;
        if ((nbytes_read = recv(client_fd, conn->header_cursor, header_bytes_rem, 0)) == -1)
        {
            fprintf(stderr, "%s:%s:%d unable to receive header bytes from client's socket: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
            return STATUS_ERROR;
        }

        if (nbytes_read == 0) return 0;

        // adjust header cursor and check if header has been completely read
        conn->header_cursor += nbytes_read;
        return (int)(conn->header_cursor - conn->header);
    }
    else
    {
        // for keeping track of how many bytes to read and how many bytes were read
        size_t buf_bytes_rem = conn->buf_size - (size_t)(conn->buf_cursor - conn->buf); 
        int nbytes_read = 0;

        if ((nbytes_read = recv(client_fd, conn->buf_cursor, buf_bytes_rem, 0)) == -1)
        {
            fprintf(stderr, "%s:%s:%d unable to receive data bytes from client's socket: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__);
            return STATUS_ERROR;
        }

        // client has ended connection early
        if (nbytes_read == 0) return 0;

        return (int)(conn->buf_cursor - conn->buf);
    }
        
        

        //if (conn->header_cursor - conn->header == sizeof(proto_msg) + sizeof(uint32_t))
        //{
        //    // ensure tag contains correct type
        //    proto_msg msg_type = *(proto_msg*)(conn->header);
        //    if (msg_type != DB_ACCESS_REQUEST)
        //    {
        //        fprintf(stderr, "%s:%s:%d illegal message type\n", __FILE__, __FUNCTION__, __LINE__);
        //        return STATUS_ERROR;
        //    }

        //    // transistion state for conn, client is now ready to read data from socket
        //    conn->state = REQUEST;

        //    // parse length of data and allocate buffer for data
        //    uint32_t data_len = ntohl(*(uint32_t*)(conn->header + sizeof(proto_msg)));
        //    conn->buf = malloc(data_len);
        //    conn->cursor = conn->buf;



            








}

            
int send_handshake_response(int client_fd, unsigned char flag)
{
    // allocate buffer and serialize response data
    unsigned char *response_buffer = malloc(sizeof(proto_msg) + 1);
    *(proto_msg*)(response_buffer) = HANDSHAKE_RESPONSE;
    *(response_buffer + sizeof(proto_msg)) = flag;

    // send response to client
    if (send_all(client_fd, response_buffer, sizeof(proto_msg) + 1, 0) == STATUS_ERRRO)
    {
        fprintf(stderr, "%s:%s:%d unable to send handshake response to client\n", __FILE__, __FUNCTION__, __LINE__);
        return STATUS_ERROR;
    }

    free(response_buffer);

    return STATUS_SUCCESS;
}

    





        
        







    
