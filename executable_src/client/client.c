#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

void print_usage(char **argv)
{
    printf("usage: %s -h -p [OPTIONS]\n", argv[0]);
    printf("\t-h <HOST> : address of host\n");
    printf("\t-p <PORT> : port of host\n");
    printf("\t-a <EMPLOYEE> : add an employee to the database, <EMPLOYEE> should be a comma seperated list of values\n");
    printf("\t-u <EMPLOYEE NAME> : name of an employee whose hours are to be updated, -n argument is also required to specify number of hours\n");
    printf("\t-n <HOURS> : the number of hours to update a given employee\n");
    printf("\t-d <EMPLOYEE NAME> : deletes an the employee with <EMPLOYEE NAME> from the databaes\n");
    printf("\t-l : list all employees in the database\n");

}

int main(int argc, char *argv[])
{
	// validate command line arguments
    if (argc < 3)
    {
        print_usage(argv);
        return 0;
    }

	// Parse command line arguments
    char *host = NULL;
    char *port = NULL;
    char *add_employee_str = NULL;
    char *update_employee_str = NULL;
    char *update_hours_str = NULL;
    char *delete_employee_str = NULL;
    bool list_flag = false;

    int c;
    while ((c = getopt(argc, argv, "h:p:a:u:n:d:l")) != -1)
    {
        switch (c)
        {
            case 'h':
                host = optarg;
                break;
            case 'p':
                port = optarg;
                break;
            case 'a':
                add_employee_str  = optarg;
                break;
            case 'u':
                update_employee_str = optarg;
                break;
            case 'n':
                update_hours_str = optarg;
                break;
            case 'd':
                delete_employee_str = optarg;
                break;
            case 'l':
                list_flag = true;
                break;
            case '?':
                print_usage(argv);
                exit(1);
            default:
                abort();
        }
    }

	// ensure host and port are both valid
    if (!host || !port)
    {
        print_usage(argv);
        exit(1);
    }



	// get info for host/port

	// create socket for host/port

	// connect to socket and ensure connection has been established

	// serialize request

	// send request

	// de-serialize and parse response

}
