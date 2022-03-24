#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "../include/global.h"
#include "../include/logger.h"

#define MESSAGE_SIZE 2048
#define STDIN 0
#define CMD_SIZE 100
#define BUFFER_SIZE 1024
#define TRUE 1

struct client
{
    int list_id;
    char *ip_addr;
    int client_file_descriptor;
    char *hostname;
    int isBlocked;
    int logged_in;
    int port_num;
    struct client *next;
};

int port_num[15];
char client_ip[15][150];
int server(struct client **first_server_reference, int port_num, int server_client_socket_number);
int client(struct client **first_client_reference, int port_num, int server_client_socket_number);
void getList(struct client *headref);
void send_to_client(int sock_index, char *send_to_ip, char *buffer, struct client *temp);
void getIpAddress(char *ip_string, int socket_number);
int bindSocket(int server_client_socket_number, struct sockaddr_in socket_listen_address, int port_number, int new_server_client_socket_number);
void transferPorts(int listening_port, int server_login_socket, int client_socket);
void connectIpPort(char str_cip[INET_ADDRSTRLEN], int client_socket);
int getIpPort(char str_cip[INET_ADDRSTRLEN], int client_socket, int res);
void assign_port(char *buffer, struct client *temp);
void toString(char str[], int num);
void send_client_list(struct client *headref, char client_list[]);
void generateList(struct client **first_client_reference, char *buffer);
void broadcast(struct client *first_client_reference, char *message, int srver_fd);
void createClient(struct client **first_server_reference, int file_descriptor_server_client_socket, struct sockaddr_in client_addr);
void sort(struct client *start);
int client_id = 1;

int main(int argc, char *argv[])
{

    /*Init. Logger*/
    cse4589_init_log(argv[2]);

    /* Clear LOGFILE*/
    fclose(fopen(LOGFILE, "w"));

    int server_client_socket_number = 0, new_server_client_socket_number;
    struct client *first_server_reference = NULL;
    struct client *first_client_reference = NULL;

    server_client_socket_number = socket(AF_INET, SOCK_STREAM, 0);
    if (server_client_socket_number == 0)
    {
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in socket_listen_address;
    new_server_client_socket_number = bindSocket(server_client_socket_number, socket_listen_address, atoi(argv[2]), new_server_client_socket_number);
    if (new_server_client_socket_number < 0)
    {
        exit(EXIT_FAILURE);
    }
    if (argc == 3)
    {
        if (*argv[1] == 's')
            server(&first_server_reference, atoi(argv[2]), server_client_socket_number);
        else
            client(&first_client_reference, atoi(argv[2]), server_client_socket_number);

        return 0;
    }
}

int server(struct client **first_server_reference, int port_num, int server_client_socket_number)
{
    // first_server_reference = (struct client*) malloc(sizeof(struct client));

    int server_head, selret, caddr_len = 0, file_descriptor_server_client_socket = 0;
    struct sockaddr_in client_addr;
    fd_set master_list, watch_list;

    /* Zero select FD sets */
    FD_ZERO(&master_list);
    FD_ZERO(&watch_list);
    /* Register the listening socket */
    FD_SET(server_client_socket_number, &master_list);
    /* Register STDIN */
    FD_SET(STDIN, &master_list);

    server_head = server_client_socket_number;

    while (TRUE)
    {
        memcpy(&watch_list, &master_list, sizeof(master_list));

        // printf("\n[PA1-Server@CSE489/589]$ ");
        // fflush(stdout);

        selret = select(server_head + 1, &watch_list, NULL, NULL, NULL);
        if (selret < 0)
            perror("select failed.");

        /* Check if we have sockets/STDIN to process */
        if (selret > 0)
        {
            /* Loop through socket descriptors to check which ones are ready */
            for (int sock_index = 0; sock_index <= server_head; sock_index += 1)
            {

                if (FD_ISSET(sock_index, &watch_list))
                {
                    /* Check if new command on STDIN */
                    if (sock_index == STDIN)
                    {
                        char *cmd = (char *)malloc(sizeof(char) * CMD_SIZE);
                        memset(cmd, '\0', CMD_SIZE);
                        if (fgets(cmd, CMD_SIZE - 1, stdin) == NULL) // Mind the newline character that will be written to cmd
                            exit(-1);
                        // Process PA1 commands here ...
                        if (strcmp(cmd, "AUTHOR\n") == 0)
                        {
                            cse4589_print_and_log("[%s:SUCCESS]\n", "AUTHOR");
                            cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n", "bhavysa");
                            cse4589_print_and_log("[%s:END]\n", "AUTHOR");
                        }
                        else if (strcmp(cmd, "IP\n") == 0)
                        {
                            char ip_address[INET_ADDRSTRLEN];
                            int server_socket_number = socket(AF_INET, SOCK_STREAM, 0);
                            getIpAddress(ip_address, server_socket_number);
                        }
                        else if (strcmp(cmd, "PORT\n") == 0)
                        {
                            cse4589_print_and_log("[%s:SUCCESS]\n", "PORT");
                            cse4589_print_and_log("PORT:%d\n", port_num);
                            cse4589_print_and_log("[%s:END]\n", "PORT");
                        }
                        else if (strcmp(cmd, "LIST\n") == 0)
                        {
                            getList(*first_server_reference);
                        }
                    }
                    /* Check if new client is requesting connection */
                    else if (sock_index == server_client_socket_number)
                    {
                        caddr_len = sizeof(client_addr);
                        file_descriptor_server_client_socket = accept(server_client_socket_number, (struct sockaddr *)&client_addr, (socklen_t *)&caddr_len);
                        if (file_descriptor_server_client_socket >= 0)
                        {
                            /* Add to watched socket list */
                            FD_SET(file_descriptor_server_client_socket, &master_list);
                            if (file_descriptor_server_client_socket > server_head)
                                server_head = file_descriptor_server_client_socket;

                            createClient(first_server_reference, file_descriptor_server_client_socket, client_addr);
                        }
                    }
                    /* Read from existing clients */
                    else
                    {
                        /* Initialize buffer to receieve response */
                        if (sock_index == 0)
                            break;
                        char *buffer = (char *)malloc(sizeof(char) * BUFFER_SIZE);
                        memset(buffer, '\0', BUFFER_SIZE);
                        int recd_bytes;
                        if (recv(recd_bytes = sock_index, buffer, BUFFER_SIZE, 0) <= 0)
                        {
                            close(sock_index);
                            FD_CLR(sock_index, &master_list);
                        }
                        else
                        {
                            struct client *temp = *first_server_reference;
                            char *send_to_ip = (char *)malloc(sizeof(char) * INET_ADDRSTRLEN);

                            send_to_ip = strtok(buffer, " ");

                            if (strcmp(send_to_ip, "PORT") == 0)
                            {
                                int success = 0;
                                assign_port(buffer, temp);
                                char client_list_buffer[700];
                                send_client_list(*first_server_reference, client_list_buffer);
                                // printf(" buf : %s", client_list_buffer);
                                if (send(file_descriptor_server_client_socket, client_list_buffer, strlen(client_list_buffer), 0) == strlen(client_list_buffer))
                                    // printf("Done!\n");
                                    //}
                                    success = 1;
                            }
                            else
                            {
                                char *message = (char *)malloc(sizeof(char) * MESSAGE_SIZE);
                                // buffer = strtok(NULL, "\n");
                                while (buffer != NULL)
                                {
                                    // printf("\n %s", buffer);
                                    buffer = strtok(NULL, " ");
                                    if (buffer != NULL)
                                        strcpy(message, buffer);
                                }

                                // strcpy(message,buffer);
                                // printf("\n %s", message);
                                struct client *temp = *first_server_reference;

                                // printf("\n from_ip %s");
                                temp = *first_server_reference;
                                send_to_client(sock_index, send_to_ip, message, temp);
                            }

                            fflush(stdout);
                        }

                        // free(buffer);
                    }
                }
            }
        }
    }

    return 0;
}

int client(struct client **first_client_reference, int port_num, int server_client_socket_number)
{
    struct sockaddr_in server_addr;
    int maximum_server_login_socket_number = 0;
    fd_set masterlist, watchlist;
    char *cmd = (char *)malloc(sizeof(char) * MESSAGE_SIZE);
    char *buffer_received = (char *)malloc(sizeof(char) * MESSAGE_SIZE);
    char *buffer_sent = (char *)malloc(sizeof(char) * MESSAGE_SIZE);
    char *argument_command;
    char *input_port_address = (char *)malloc(sizeof(char) * MESSAGE_SIZE);
    char *message = (char *)malloc(sizeof(char) * MESSAGE_SIZE);
    char *client_server_ip_address = (char *)malloc(sizeof(char) * MESSAGE_SIZE);
    char *ip_address_client;
    int server_login_socket;

    FD_ZERO(&masterlist);
    FD_ZERO(&watchlist);
    FD_SET(0, &masterlist);
    // FD_SET(server_client_socket_number, &masterlist);
    /// maximum_server_login_socket_number = server_client_socket_number;
    // maximum_server_login_socket_number = 0;
    int server;
    server = server_client_socket_number;
    // server = connect_to_host(client_ip, port_num);

    while (1)
    {
        // printf("\n[PA1-Client@CSE489/589]$ ");
        // fflush(stdout);
        watchlist = masterlist;
        int selret = select(maximum_server_login_socket_number + 1, &watchlist, NULL, NULL, NULL);
        if (selret == -1)
        {
            perror("select");
        }

        if (selret > 0)
        {

            for (int i = 0; i <= maximum_server_login_socket_number; i++)
            {
                if (FD_ISSET(i, &watchlist))
                {
                    memset(cmd, '\0', MESSAGE_SIZE);
                    memset(buffer_received, '\0', MESSAGE_SIZE);
                    if (i == STDIN)
                    {
                        if (fgets(cmd, CMD_SIZE - 1, stdin) == NULL) // Mind the newline character that will be written to cmd
                            exit(-1);
                        argument_command = strtok(cmd, " ");

                        if (strcmp(cmd, "AUTHOR\n") == 0)
                        {
                            cse4589_print_and_log("[%s:SUCCESS]\n", "AUTHOR");
                            cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n", "bhavyasa");
                            cse4589_print_and_log("[%s:END]\n", "AUTHOR");
                        }
                        if (strcmp(cmd, "IP\n") == 0)
                        {
                            char ip_address[INET_ADDRSTRLEN];
                            int client_socket_number = socket(AF_INET, SOCK_STREAM, 0);
                            getIpAddress(ip_address, client_socket_number);
                        }
                        else if (strcmp(cmd, "PORT\n") == 0)
                        {
                            cse4589_print_and_log("[%s:SUCCESS]\n", "PORT");
                            cse4589_print_and_log("PORT:%d\n", port_num);
                            cse4589_print_and_log("[%s:END]\n", "PORT");
                        }
                        else if (strcmp(cmd, "LIST\n") == 0)
                            getList(*first_client_reference);

                        else if (strcmp(argument_command, "LOGIN") == 0)
                        {
                            int result = 1;
                            server_login_socket = socket(AF_INET, SOCK_STREAM, 0);
                            if (server_login_socket < 0)
                            {
                                exit(EXIT_FAILURE);
                            }
                            else
                            {
                                FD_SET(server_login_socket, &masterlist);
                                if (server_login_socket > maximum_server_login_socket_number)
                                    maximum_server_login_socket_number = server_login_socket;
                            }
                            client_server_ip_address = strtok(NULL, " ");
                            for (int count = 0; count < 2; count++)
                            {
                                if (count != 0)
                                {
                                    argument_command = strtok(NULL, "\n");
                                    if (argument_command == NULL)
                                        result = 0;
                                    else
                                    {
                                        input_port_address = argument_command;
                                        break;
                                    }
                                }
                            }

                            if (result == 0)
                            {
                                cse4589_print_and_log("[%s:ERROR]\n", "LOGIN");
                                cse4589_print_and_log("[%s:END]\n", "LOGIN");
                            }
                            else
                            {
                                server_addr.sin_family = AF_INET;
                                unsigned int port_temp = atoi(input_port_address);
                                server_addr.sin_port = htons(port_temp);
                                inet_pton(AF_INET, client_server_ip_address, &(server_addr.sin_addr));
                                if (connect(server_login_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
                                {
                                    cse4589_print_and_log("[%s:ERROR]\n", "LOGIN");
                                    cse4589_print_and_log("[%s:END]\n", "LOGIN");
                                }
                                else
                                {
                                    int client_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
                                    transferPorts(port_num, server_login_socket, client_socket);
                                }

                                cse4589_print_and_log("[%s:SUCCESS]\n", "LOGIN");
                                cse4589_print_and_log("[%s:END]\n", "LOGIN");
                            }
                        }
                        else if (strcmp(argument_command, "SEND") == 0)
                        {

                            ip_address_client = strtok(NULL, " ");
                            for (int count = 0; count < 2; count++)
                            {
                                if (count != 0)
                                {
                                    argument_command = strtok(NULL, "\n");
                                    message = argument_command;
                                }
                            }

                            memset(buffer_sent, '\0', MESSAGE_SIZE);

                            server_addr.sin_family = AF_INET;
                            server_addr.sin_port = port_num;
                            inet_pton(AF_INET, ip_address_client, &(server_addr.sin_addr));

                            strcat(buffer_sent, ip_address_client);
                            strcat(buffer_sent, " ");
                            strcat(buffer_sent, message);
                            strcat(buffer_sent, "\n");

                            send(server_login_socket, buffer_sent, strlen(buffer_sent), 0);
                        }
                        else if (strcmp(argument_command, "BROADCAST") == 0)
                        {
                            while (argument_command != NULL)
                            {
                                argument_command = strtok(NULL, "\n");
                                break;
                            }

                            broadcast(*first_client_reference, argument_command, server_login_socket);
                        }
                        else if (strcmp(argument_command, "BLOCK") == 0)
                        {
                            argument_command = strtok(NULL, "\n");

                            struct client *temp = *first_client_reference;
                            int found = 0;
                            while (temp != NULL)
                            {
                                if (strcmp(argument_command, client_ip[temp->list_id]) == 0)
                                {
                                    temp->isBlocked = 1;
                                    found += 1;
                                }
                                temp = temp->next;
                            }
                            if (found < 1)
                            {
                                cse4589_print_and_log((char *)"[%s:ERROR]\n", "BLOCK");
                                cse4589_print_and_log((char *)"[%s:END]\n", "BLOCK");
                            }
                            else
                            {
                                cse4589_print_and_log((char *)"[%s:SUCCESS]\n", "BLOCK");
                                cse4589_print_and_log((char *)"[%s:END]\n", "BLOCK");
                            }
                        }
                        else if (strcmp(argument_command, "UNBLOCK") == 0)
                        {
                            argument_command = strtok(NULL, "\n");
                            struct client *temp = *first_client_reference;
                            int found = 0;
                            while (temp != NULL)
                            {
                                if (strcmp(argument_command, client_ip[temp->list_id]) == 0)
                                {
                                    temp->isBlocked = 0;
                                    found += 1;
                                }
                                temp = temp->next;
                            }
                            if (found >= 1)
                            {
                                cse4589_print_and_log((char *)"[%s:SUCCESS]\n", "UNBLOCK");
                                cse4589_print_and_log((char *)"[%s:END]\n", "UNBLOCK");
                            }
                            else
                            {
                                cse4589_print_and_log((char *)"[%s:ERROR]\n", "UNBLOCK");
                                cse4589_print_and_log((char *)"[%s:END]\n", "UNBLOCK");
                            }
                        }

                        else if (strcmp(cmd, "LOGOUT\n") == 0)
                        {
                            cse4589_print_and_log((char *)"[%s:SUCCESS]\n", "LOGOUT");
                            close(server_login_socket);
                            cse4589_print_and_log((char *)"[%s:END]\n", "LOGOUT");
                            return 0;
                        }

                        else if (strcmp(cmd, "EXIT\n") == 0)
                        {
                            cse4589_print_and_log((char *)"[%s:SUCCESS]\n", (char *)"EXIT");
                            close(server_login_socket);
                            cse4589_print_and_log((char *)"[%s:END]\n", (char *)"EXIT");
                            exit(0);
                        }
                    }
                    else
                    {
                        memset(buffer_received, '\0', MESSAGE_SIZE);
                        if (i == 0 && i == server_client_socket_number)
                            break;
                        else
                        {
                            size_t nbyte_recvd;
                            nbyte_recvd = recv(i, buffer_received, MESSAGE_SIZE, 0);
                            buffer_received[nbyte_recvd] = '\0';
                            char *argument_command = (char *)malloc(sizeof(char) * MESSAGE_SIZE);
                            ;
                            strcpy(argument_command, buffer_received);
                            struct client *temp = *first_client_reference;
                            if (strcmp(buffer_received, "LIST") == 0)
                            {
                                generateList(first_client_reference, buffer_received);
                            }
                            else
                            {
                                char *msg_from = (char *)malloc(sizeof(MESSAGE_SIZE));
                                char *message = (char *)malloc(sizeof(MESSAGE_SIZE));

                                int count = 0;
                                if (buffer_received != NULL)
                                {
                                    argument_command = strtok(argument_command, " ");
                                    strcpy(msg_from, argument_command);
                                    while (argument_command != NULL)
                                    {
                                        argument_command = strtok(NULL, " ");
                                        if (argument_command != NULL)
                                            strcpy(message, argument_command);
                                    }

                                    cse4589_print_and_log("[RECEIVED:SUCCESS]\n");
                                    cse4589_print_and_log("message from:%s\n[message]:%s\n", msg_from, message);
                                    cse4589_print_and_log("[RECEIVED:END]\n");
                                }
                                else
                                {
                                    cse4589_print_and_log("[RECEIVED:ERROR]\n");
                                    cse4589_print_and_log("message from:%s\n[message]:%s\n", msg_from, message);
                                    cse4589_print_and_log("[RECEIVED:END]\n");
                                }
                            }
                        }
                        fflush(stdout);
                    }
                }
            }
        }
    }
}

int bindSocket(int server_client_socket_number, struct sockaddr_in socket_listen_address, int port_number, int new_server_client_socket_number)
{
    socket_listen_address.sin_addr.s_addr = htonl(INADDR_ANY);
    socket_listen_address.sin_family = AF_INET;
    socket_listen_address.sin_port = htons(port_number);

    // Binding the socket to a port
    new_server_client_socket_number = bind(server_client_socket_number, (struct sockaddr *)&socket_listen_address, sizeof(socket_listen_address));
    if (new_server_client_socket_number < 0)
    {
        exit(EXIT_FAILURE);
    }
    new_server_client_socket_number = listen(server_client_socket_number, 4);
    return new_server_client_socket_number;
}

void createClient(struct client **first_server_reference, int file_descriptor_server_client_socket, struct sockaddr_in client_addr)
{
    char client_address[INET_ADDRSTRLEN], new_client_address[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_address, INET_ADDRSTRLEN);
    struct hostent *hostname = NULL;
    struct in_addr in_ip_address;
    inet_ntop(AF_INET, &(client_addr.sin_addr), new_client_address, INET_ADDRSTRLEN);
    inet_pton(AF_INET, new_client_address, &in_ip_address);
    hostname = gethostbyaddr(&in_ip_address, sizeof(in_ip_address), AF_INET);
    int host_length = (int)strlen(hostname->h_name);

    struct client *new_client = (struct client *)malloc(sizeof(client));
    new_client->ip_addr = (char *)malloc(sizeof(MESSAGE_SIZE));
    strcpy(client_ip[client_id], client_address);
    strcpy(new_client->ip_addr, client_address);
    new_client->list_id = client_id++;
    new_client->client_file_descriptor = file_descriptor_server_client_socket;
    new_client->hostname = (char *)malloc(sizeof(host_length));
    strcpy(new_client->hostname, hostname->h_name);
    new_client->logged_in = 1;

    if (*first_server_reference != NULL)
    {
        new_client->next = *first_server_reference;
        *first_server_reference = new_client;
    }
    else
    {
        *first_server_reference = new_client;
        new_client->next = NULL;
    }
}

/* function to swap data of two nodes a and b*/
void swap(struct client *inta, struct client *intb)
{
    int temp = inta->port_num;
    inta->port_num = intb->port_num;
    intb->port_num = temp;
}
/* Bubble sort the given linked lsit */
void sort(struct client *start)
{
    int flag;
    struct client *pointer1 = start;
    struct client *pointer2 = NULL;
    if (pointer1 == NULL)
        return;
    while (flag)
    {
        flag = 0;
        pointer1 = start;

        while (pointer1->next != pointer2)
        {
            if (pointer1->port_num > pointer1->next->port_num)
            {
                swap(pointer1, pointer1->next);
                flag = 1;
            }
            pointer1 = pointer1->next;
        }
        pointer2 = pointer1;
    }
}

void swap_int(char *num1, char *num2)
{
    int temp;
    temp = *num1;
    *num1 = *num2;
    *num2 = temp;
}

void getList(struct client *headref)
{
    cse4589_print_and_log("[%s:SUCCESS]\n", "LIST");
    struct client *temp = headref;
    int list_id = 1;
    sort(headref);
    while (temp != NULL)
    {
        if (temp->logged_in == 1)
        {
            cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", list_id, temp->hostname, client_ip[temp->list_id], temp->port_num);
        }
        temp = temp->next;
        list_id = list_id + 1;
    }
    cse4589_print_and_log("[%s:END]\n", "LIST");
}

void send_to_client(int sock_index, char *send_to_ip, char *buffer, struct client *temp)
{
    char *str = (char *)malloc(sizeof(MESSAGE_SIZE));
    char *from_ip = (char *)malloc(sizeof(MESSAGE_SIZE));
    int j;
    int success = 1;
    struct client *temp1 = temp;
    for (j = 1; j <= 15; j++)
    {
        if (strcmp(send_to_ip, client_ip[j]) == 0)
        {
            break;
        }
    }
    while (temp1 != NULL)
    {
        if (temp1->client_file_descriptor == sock_index)
            strcpy(from_ip, client_ip[temp1->list_id]);
        temp1 = temp1->next;
    }

    strcat(str, from_ip);
    strcat(str, " ");
    strcat(str, buffer);
    while (temp->list_id != j)
        temp = temp->next;

    if (send(temp->client_file_descriptor, str, strlen(str), 0) == -1)
        success = 0; // perror("send");
    else
    {
        success = 1;
    }

    if (success == 1)
    {
        cse4589_print_and_log("[RELAYED:SUCCESS]\n");
        cse4589_print_and_log("message from:%s, to:%s\n[message]:%s\n", from_ip, send_to_ip, buffer);
        cse4589_print_and_log("[RELAYED:END]\n");
    }
    else
    {
        cse4589_print_and_log("[RELAYED:ERROR]\n");
        cse4589_print_and_log("[RELAYED:END]\n");
    }
}

void assign_port(char *buffer, struct client *temp)
{
    char *port, *ip_address_client;
    int count = 0;
    // printf("\n Let's assign ports");
    while (count != 2)
    {
        char *a = (char *)malloc(sizeof(char) * MESSAGE_SIZE);
        strcpy(a, buffer);
        if (count == 0)
        {
            buffer = strtok(NULL, " ");
            strcpy(a, buffer);
            ip_address_client = a;
            // printf(" %s ", ip_address_client);
        }
        else if (count == 1)
        {
            buffer = strtok(NULL, "\n");
            strcpy(a, buffer);
            port = a;
            break;
        }

        count += 1;
    }
    int j;
    for (j = 1; j <= 5; j++)
    {
        if (strcmp(ip_address_client, client_ip[j]) == 0)
        {
            // printf("\n %d %s", j, client_ip[j]);
            break;
        }
    }

    if (j == 6)
    { // printf("\n Could not find receiver");
    }
    else
    {
        while (temp->list_id != j)
            temp = temp->next;
        if (temp == NULL)
            ;
        // printf("\n Could not find receiver!");
        else
        {
            temp->port_num = atoi(port);
            // printf("\n IP: %s port: %d", ip_address_client, temp->port_num);
        }
    }
}
void connectIpPort(char str_cip[INET_ADDRSTRLEN], int client_socket)
{
    struct sockaddr_in socket_address_struct;
    int len = sizeof(socket_address_struct);
    int result;

    if (client_socket == -1)
    {
    }

    memset((char *)&socket_address_struct, 0, sizeof(socket_address_struct));
    socket_address_struct.sin_family = AF_INET;
    socket_address_struct.sin_port = htons(53);
    inet_pton(AF_INET, "8.8.8.8", &socket_address_struct.sin_addr);

    if (connect(client_socket, (struct sockaddr *)&socket_address_struct, sizeof(socket_address_struct)) < 0 || getsockname(client_socket, (struct sockaddr *)&socket_address_struct, (unsigned int *)&len) == -1)
    {
        result = 0;
    }

    inet_ntop(AF_INET, &(socket_address_struct.sin_addr), str_cip, len);
}

int getIpPort(char str_cip[INET_ADDRSTRLEN], int client_socket, int res)
{
    struct sockaddr_in socket_address_struct;
    int len = sizeof(socket_address_struct);
    if (client_socket == -1)
    {
    }

    memset((char *)&socket_address_struct, 0, sizeof(socket_address_struct));
    socket_address_struct.sin_family = AF_INET;
    socket_address_struct.sin_port = htons(53);
    inet_pton(AF_INET, "8.8.8.8", &socket_address_struct.sin_addr);

    if (connect(client_socket, (struct sockaddr *)&socket_address_struct, sizeof(socket_address_struct)) < 0 || getsockname(client_socket, (struct sockaddr *)&socket_address_struct, (unsigned int *)&len) == -1)
    {
        res = 0;
    }

    inet_ntop(AF_INET, &(socket_address_struct.sin_addr), str_cip, len);
    return res;
}

void transferPorts(int listening_port, int server_login_socket, int client_socket)
{
    char transferPorts[100], ip_string[INET_ADDRSTRLEN], port[INET_ADDRSTRLEN];
    connectIpPort(ip_string, client_socket);
    toString(port, listening_port);
    strcat(transferPorts, "Port");
    strcat(transferPorts, " ");
    strcat(transferPorts, ip_string);
    strcat(transferPorts, " ");
    strcat(transferPorts, port);
    strcat(transferPorts, "\n");
}

void getIpAddress(char *ip_string, int socket_number)
{
    int res;
    res = getIpPort(ip_string, socket_number, res);
    if (res != 0)
    {
        cse4589_print_and_log("[%s:SUCCESS]\n", "IP");
        cse4589_print_and_log("IP:%s\n", ip_string);
        cse4589_print_and_log("[%s:END]\n", "IP");
    }
    else
    {
        cse4589_print_and_log("[%s:ERROR]\n", "IP");
        cse4589_print_and_log("[%s:END]\n", "IP");
    }
}

void toString(char str[], int num)
{
    int i, rem, len = 0, n;

    n = num;
    while (n != 0)
    {
        len++;
        n /= 10;
    }
    for (i = 0; i < len; i++)
    {
        rem = num % 10;
        num = num / 10;
        str[len - (i + 1)] = rem + '0';
    }
    str[len] = '\0';
}

void send_client_list(struct client *headref, char client_list[])
{
    char buf[500] = "";
    //  char client_list[500]= "";
    // int i;
    // char port[INET_ADDRSTRLEN]= "";
    struct client *temp = headref;
    // strcat(buf, "List");
    char list_id[10] = "";
    char str[20] = "";
    strcat(buf, "List");
    while (temp != NULL)
    {
        if (temp->logged_in == 1)
        {
            strcat(buf, " ");
            toString(list_id, temp->list_id);
            strcat(buf, list_id);
            strcat(buf, " ");
            toString(str, temp->port_num);
            strcat(buf, str);
            strcat(buf, " ");
            strcat(buf, temp->hostname);
            strcat(buf, "\n");
            strcat(buf, client_ip[temp->list_id]);
            // strcat(buf, temp->isBlocked);
            strcat(buf, "\n");
        }
        temp = temp->next;
    }
    // printf("\n List: %s", buf);
    strcpy(client_list, buf);

    // return client_list;
}
void generateList(struct client **first_client_reference, char *buffer)
{

    char *temp[256];
    int i = 1;
    temp[i] = strtok(buffer, "\n");
    while (temp[i] != NULL)
    {
        temp[i] = strtok(NULL, "\n");
        i++;
    }

    for (int j = 0; j < i; j++)
    {
        char *temp_client = strtok(temp[j], " ");
        int k = 0;
        struct client *new_client = (struct client *)malloc(sizeof(client));
        while (temp_client != NULL)
        {
            if (j == 0)
            {
                if (k == 0)
                    ;
                else if (k == 1)
                    new_client->list_id = atoi(temp_client);
                else if (k == 2)
                {
                    new_client->port_num = atoi(temp_client);
                }
                else if (k == 3)
                {
                    new_client->hostname = (char *)malloc(sizeof(MESSAGE_SIZE));
                    strcpy(new_client->hostname, temp_client);
                }
                else if (k == 4)
                {
                    new_client->ip_addr = (char *)malloc(sizeof(MESSAGE_SIZE));
                    strcpy(new_client->ip_addr, temp_client);
                    strcpy(client_ip[new_client->list_id], temp_client);
                }
            }
            else
            {

                if (k == 0)
                    new_client->list_id = atoi(temp_client);
                else if (k == 1)
                {
                    new_client->port_num = atoi(temp_client);
                }

                else if (k == 2)
                {
                    new_client->hostname = (char *)malloc(sizeof(MESSAGE_SIZE));
                    strcpy(new_client->hostname, temp_client);
                }
                else if (k == 3)
                {
                    new_client->ip_addr = (char *)malloc(sizeof(MESSAGE_SIZE));
                    strcpy(new_client->ip_addr, temp_client);
                    strcpy(client_ip[new_client->list_id], temp_client);
                }
            }

            temp_client = strtok(NULL, " ");
            k += 1;
        }
        if (*first_client_reference == NULL)
        {
            *first_client_reference = new_client;
            new_client->next = NULL;
        }
        else
        {
            new_client->next = *first_client_reference;
            *first_client_reference = new_client;
        }
    }
}
void broadcast(struct client *first_client_reference, char *temp_message, int server_login_socket)
{
    char *message = (char *)malloc(sizeof(char) * MESSAGE_SIZE);
    if (temp_message != NULL)
        strcpy(message, temp_message);
    char ip_str[INET_ADDRSTRLEN];
    // strcpy(ip_str,getIpAddress(ip_str));

    struct sockaddr_in socket_address_struct;
    int temp_udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int len = sizeof(socket_address_struct);
    // char str[INET_ADDRSTRLEN];
    int result;

    if (temp_udp == -1)
    {
        // printf("Socket creation failed!");
    }

    memset((char *)&socket_address_struct, 0, sizeof(socket_address_struct));
    socket_address_struct.sin_family = AF_INET;
    socket_address_struct.sin_port = htons(53);
    inet_pton(AF_INET, "8.8.8.8", &socket_address_struct.sin_addr);
    // socket_address_struct.sin_addr.s_addr = inet_addr("8.8.8.8");

    if (connect(temp_udp, (struct sockaddr *)&socket_address_struct, sizeof(socket_address_struct)) < 0 || getsockname(temp_udp, (struct sockaddr *)&socket_address_struct, (unsigned int *)&len) == -1)
    {
        result = 0;
    }

    inet_ntop(AF_INET, &(socket_address_struct.sin_addr), ip_str, len);
    // printf("%s", str);

    int count = 0;
    struct client *temp = first_client_reference;
    char *buffer_sent = (char *)malloc(sizeof(char) * MESSAGE_SIZE);
    while (temp != NULL)
    {

        if (strcmp(client_ip[temp->list_id], ip_str) != 0)
        {

            strcat(buffer_sent, client_ip[temp->list_id]);
            // printf("\n %s",client_ip[temp->list_id]);
            strcat(buffer_sent, " ");
            strcat(buffer_sent, message);
            // strcat(buffer_sent, "\n");
            //  printf("%s ' ' %s", client_ip[temp->list_id], message);
            if (send(server_login_socket, buffer_sent, strlen(buffer_sent), 0) > 0)
                count += 1;
        }
        temp = temp->next;
        // free(buffer_sent);
        // buffer_sent = NULL;
    }
    if (count > 0)
    {
        cse4589_print_and_log("[RELAYED:SUCCESS]\n");
        cse4589_print_and_log("message from:%s, to:255.255.255.255\n[message]:%s\n", ip_str, message);
        cse4589_print_and_log("[RELAYED:END]\n");
    }
    else
    {
        cse4589_print_and_log("[RELAYED:ERROR]\n");
        // cse4589_print_and_log("message from:%s, to:%s\n[message]:%s\n",from_ip,send_to_ip,buffer);
        cse4589_print_and_log("[RELAYED:END]\n");
    }
}
