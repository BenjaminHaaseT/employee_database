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
#include <stdbool.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>

#include "common.h"
#include "serialize.h"
#include "parse.h"
#include "models.h"
#include "proto.h"

#define ALPHA 0.5L
#define MAX_SERV_LEN 100


void print_usage(char **argv);
int get_listener_socket(char *address, char *port);
int add_fd(struct pollfd **pfds, size_t *fd_count, nfds_t *fd_size, int fd);
void remove_fd(struct pollfd *pfds, size_t *fd_count, connection_map *m, size_t conn_idx);
int receive_from_client(int client_fd, client_connection *conn);
int send_handshake_response(int client_fd, unsigned char flag);
int send_invalid_request_response(int client_fd);


int main(int argc, char *argv[])
{
    if (argc < 5)
    {
        print_usage(argv);
        exit(0);
    }

    // process command line arguments to configure server
    char *address = NULL;
    char *port = NULL;
    char *protocol_version_str = NULL;
    char *fname = NULL;
    bool new_file_flag = false;
    int c;

    while ((c = getopt(argc, argv, ":f:a:p:v:n")) != -1)
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
            case 'f':
                fname = optarg;
                break;
            case 'n':
                new_file_flag = optarg;
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

    // verify required command line arguments have non null values
    if (!address || !port || !protocol_version_str || !fname)
    {
        print_usage(argv);
        exit(1);
    }

    // open database file
    int fd;
    if (new_file_flag)
    {
        if ((fd = open(fname, O_RDWR | O_CREAT | O_EXCL, 0666)) == -1)
        {
            fprintf(stderr, "%s:%s:%d - error creating new file '%s' : (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, fname, errno, strerror(errno));
            exit(1);
        }
        if (write_new_file_hdr(fd) == STATUS_ERROR) 
        {
            exit(1);
        }
		// reset file cursor to beginning
		if (lseek(fd, 0, SEEK_SET) == -1)
		{
			fprintf(stderr, "%s:%s:%d - unable to reset file cursor: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
			exit(1);
		}
    }
    else
    {
        if ((fd = open(fname, O_RDWR, 0666)) == -1)
        {
            fprintf(stderr, "%s:%s:%d - unable to open file %s: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, fname, errno, strerror(errno));
            exit(1);
        }
    }
    
    // Read database file header and stats from file
    db_header dbhdr;
    if(read_dbhdr(fd, &dbhdr) == STATUS_ERROR)
    {
        exit(1);
    }

    // Read employees from data base
    size_t employees_size = dbhdr.employee_count;      
    employee *employees = (employee *) malloc(sizeof(employee) * employees_size);
    if (read_employees(fd, &employees, employees_size) == STATUS_ERROR)
    {
        exit(1);
    }

    // convert/validate protocol version
    char *end = NULL;
    long parsed_protocol_version = strtol(protocol_version_str, &end, 10);
    if (!end || *end != '\0' || errno == ERANGE || parsed_protocol_version < 0 || parsed_protocol_version > UINT16_MAX)
    {
        fprintf(stderr, "invalid protocol version %ld\n", parsed_protocol_version);
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

    if (add_fd(&pfds, &fd_count, &fd_size, listener) == STATUS_ERROR)
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
                        fprintf(stderr, "client file descriptor not found in map\n");
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

                    if (nbytes_read == 0)
                    {
                        // client's connection has terminated
                        printf("client disconnected\n");
                        size_t conn_idx = conn->conn_idx;
                        free_client_connection(conn);
                        remove_fd(pfds, &fd_count, conn_idx);
                        connection_map_remove(&client_connections, pfds[i].fd);
                        continue;
                    }
                    else if (conn->state == UNINITIALIZED && nbytes_read == sizeof(proto_msg) + sizeof(uint16_t))
                    {
                        // check message type
                        proto_msg msg_type = *(proto_msg *)(conn->header);
                        if (msg_type != HANDSHAKE_REQUEST)
                        {
                            fprintf(stderr, "illegal message type received from client\n");
                            // send error response
                           if (send_invalid_request_response(pfds[i].fd)  == STATUS_ERROR)
                           {
                               fprintf(stderr, "send_invalid_request_response() failed\n");
                               exit(1);
                           }

                           //reset header cursor for client connection
                           conn->header_cursor = conn->header;
                        }
                        else
                        {
                            uint16_t client_protocol_version = ntohs(*(uint16_t *)(conn->header + sizeof(proto_msg)));
                            int flag;
                            if (client_protocol_version != parsed_protocol_version)
                            {
                                flag = 1;
                            }
                            else
                            {
                                // set flag to 0 to signal no errors 
                                flag = 0;
                                // transistion state of client to initialized
                                conn->state = INITIALIZED;
                                // reset cursor for request header
                                conn->header_cursor = conn->header;
                            }

                           if (send_handshake_response(pfds[i].fd, flag) == STATUS_ERROR)
                           {
                               fprintf(stderr, "send_handshake_response() failed\n");
                               exit(1);
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
                            if (send_invalid_request_response(pfds[i].fd)  == STATUS_ERROR)
                            {
                                fprintf(stderr, "send_invalid_request_response() failed\n");
                                exit(1);
                            }
                            // reset header cursor for client connection
                            conn->header_cursor = conn->header;
                            continue;
                        }

                        // parse data length and allocate buffer for request data
                        uint32_t data_len = ntohl(*(uint32_t *)(conn->header + sizeof(proto_msg)));
                        conn->buf = malloc(data_len);
                        conn->buf_cursor = conn->buf;
                        conn->buf_size = (size_t) data_len;

                        // transition state of connection
                        conn->state = REQUEST;

                        // attempt to read any remaining bytes from client's socket just in case more were able to get through
                        // reset 'nbytes_read' to zero to keep track of bytes read from the request
                        nbytes_read = 0;
                        if ((nbytes_read = receive_from_client(pfds[i].fd, conn)) == STATUS_ERROR)
                        {
                            fprintf(stderr, "receive_from_client() failed\n");
                            exit(1);
                        }
                    }

                    // Check if connection has been transistioned/or is in, request state and all bytes of request have been read successfully
                    if (conn->state == REQUEST && nbytes_read == conn->buf_size)
                    {
                        // once this state is reached process request and reset state of connection
                        // allocate buffer for response to client
                        size_t response_buf_size = sizeof(proto_msg) + sizeof(uint32_t) + 1;
                        unsigned char *response_buf = malloc(response_buf_size);
                        *(proto_msg *)(response_buf) = DB_ACCESS_RESPONSE;

                        // process request and write to response buffer depending on options requested
                        if (deserialize_request_options(fd, &employees, &dbhdr, &response_buf, conn) == STATUS_ERROR)
                        {
                            fprintf(stderr, "deserialize_request_options() failed\n");
                            exit(1);
                        }

                        // send response back to client
                        if (send_all(pfds[i].fd, response_buf, response_buf_size, 0) == STATUS_ERROR)
                        {
                            fprintf(stderr, "send_all() failed\n");
                            exit(1);
                        }

                        // reset client, reset header and buf cursor's and state to initialized
                        conn->state = INITIALIZED;
                        conn->header_cursor = conn->header;
                        free(conn->buf);
                        conn->buf = NULL;
                        conn->buf_cursor = NULL;
                    }
                }
            }   // check for pollin flag being set
        } // for loop checking sockets to poll
    } // end while loop

    return 0;
}


void print_usage(char **argv)
{
    printf("usage: %s -f <FILE> -a <ADDRESS> -p <PORT> -v <VERSION>\n", argv[0]);
    printf("-f <FILE>: (REQUIRED) the file of the database\n");
    printf("-a <ADDRESS>: (REQUIRED) the address of the server\n");
    printf("-p <PORT>:  (REQUIRED) the port of the server\n");
    printf("-v <VERSION>: (REQUIRED) the protocol version\n");
    printf("-n : (OPTIONAL) flag to create a new file\n");
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
        fprintf(stderr, "%s:%s:%d unable to bind socket: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
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
            fprintf(stderr, "%s:%s:%d unable to reallocate file descriptor set\n", __FILE__, __FUNCTION__, __LINE__);
            return STATUS_ERROR;
        }
        *pfds = new_pfds;
    }
    (*pfds)[*fd_count].fd = fd;
    (*pfds)[*fd_count].events = POLLIN;
    (*fd_count)++;
    return STATUS_SUCCESS;
}

void remove_fd(struct pollfd *pfds, size_t *fd_count, connection_map *m, size_t conn_idx)
{
    size_t replacement_idx = --(*fd_count);
    if (replacement_idx != conn_idx)
    {
        // we need to update the conn_idx for the connection that is the replacement
        pfds[conn_idx] = pfds[replacement_idx];
        client_connection *replacement_conn = connection_map_get(m, pfds[replacement_idx].fd);
        if (!replacement_conn)
        {
            fprintf(stderr, "%s:%s:%d replacement connection not present in map\n", __FILE__, __FUNCTION__, __LINE__);
            return STATUS_ERROR;
        }

        replacement_conn->conn_idx = conn_idx;
        return STATUS_SUCCESS
    }

    return STATUS_SUCCESS;
}


int receive_from_client(int client_fd, client_connection *conn)
{
    if (conn->state == UNINITIALIZED)
    {
        // compute number of byte remaining when in uninitialized state
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
        // compute bytes left to be read
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
            fprintf(stderr, "%s:%s:%d unable to receive bytes from client's socket: (%d) %s\n", __FILE__, __FUNCTION__, __LINE__, errno, strerror(errno));
            return STATUS_ERROR;
        }

        // client has ended connection early
        if (nbytes_read == 0) return 0;

        // update buffer cursor
        conn->buf_cursor += nbytes_read;
        return (int)(conn->buf_cursor - conn->buf);
    }
}

            
int send_handshake_response(int client_fd, unsigned char flag)
{
    // allocate buffer and serialize response data
    unsigned char *response_buffer = malloc(sizeof(proto_msg) + 1);
    *(proto_msg*)(response_buffer) = HANDSHAKE_RESPONSE;
    *(response_buffer + sizeof(proto_msg)) = flag;

    // send response to client
    if (send_all(client_fd, response_buffer, sizeof(proto_msg) + 1, 0) == STATUS_ERROR)
    {
        fprintf(stderr, "%s:%s:%d unable to send handshake response to client\n", __FILE__, __FUNCTION__, __LINE__);
        return STATUS_ERROR;
    }

    free(response_buffer);

    return STATUS_SUCCESS;
}

    
int send_invalid_request_response(int client_fd)
{
    // allocate buffer and serialize response data
    unsigned char *response_buffer = malloc(sizeof(proto_msg) + sizeof(uint32_t) + 1);
    *(proto_msg *)response_buffer = INVALID_REQUEST;
    
    // send response down the wire
    if (send_all(client_fd, response_buffer, sizeof(proto_msg), 0) == STATUS_ERROR)
    {
        fprintf(stderr, "%s:%s:%d unable to send error response to client\n", __FILE__, __FUNCTION__, __LINE__);
        return STATUS_ERROR;
    }

    free(response_buffer);
    return STATUS_SUCCESS;
}

