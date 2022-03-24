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
#define BUFFER_SIZE 256
#define EPHEMERAL_PORT 53
#define TRUE 1

struct client
{
    int list_id;
    char *ip;
    int client_fd;
    char *hostname;
    int block_status;
    int client_status;
    int port_no;
    struct client *next;
};

int port_no[5];
char client_ip[5][100];
int c = 1;
int server(struct client **first_server_reference, int port_no, int server_client_socket_number);
int client(struct client **first_client_reference, int port_no, int server_client_socket_number);
void print(struct client *headref);
void send_to_client(int sock_index, char *send_to_ip, char *buffer, struct client *temp);
char *get_IP_address(char *str, int socket_number);
int bind_socket(int server_client_socket_number, struct sockaddr_in socket_listen_address, int port_number, int new_server_client_socket_number);
void send_port(int listening_port, int server_fd);
void assign_port(char *buffer, struct client *temp);
void tostring(char str[], int num);
void send_client_list(struct client *headref, char client_list[]);
void create_client_list(struct client **first_client_reference, char *buffer);
void broadcast(struct client *first_client_reference, char *msg, int srver_fd);
void add_new_client(struct client **first_server_reference, int fdaccept, struct sockaddr_in client_addr);
char from_ip[25];

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
    new_server_client_socket_number = bind_socket(server_client_socket_number, socket_listen_address, atoi(argv[2]), new_server_client_socket_number);
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

int server(struct client **first_server_reference, int port_no, int server_client_socket_number)
{
    // first_server_reference = (struct client*) malloc(sizeof(struct client));

    int server_head, selret, caddr_len = 0, fdaccept = 0;
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
                            char ip_str[INET_ADDRSTRLEN];
                            int server_socket_number = socket(AF_INET, SOCK_STREAM, 0);
                            strcpy(ip_str, get_IP_address(ip_str, server_socket_number));
                        }
                        else if (strcmp(cmd, "PORT\n") == 0)
                        {
                            cse4589_print_and_log("[%s:SUCCESS]\n", "PORT");
                            cse4589_print_and_log("PORT:%d\n", port_no);
                            cse4589_print_and_log("[%s:END]\n", "PORT");
                        }
                        else if (strcmp(cmd, "LIST\n") == 0)
                        {
                            print(*first_server_reference);
                        }
                    }
                    /* Check if new client is requesting connection */
                    else if (sock_index == server_client_socket_number)
                    {
                        caddr_len = sizeof(client_addr);
                        fdaccept = accept(server_client_socket_number, (struct sockaddr *)&client_addr, (socklen_t *)&caddr_len);
                        if (fdaccept < 0)
                            perror("Accept failed.");
                        else
                        {
                            // printf("\nRemote Host connected!\n");

                            /* Add to watched socket list */
                            FD_SET(fdaccept, &master_list);
                            if (fdaccept > server_head)
                                server_head = fdaccept;

                            add_new_client(first_server_reference, fdaccept, client_addr);
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
                            // if(recd_bytes == 0)
                            //  printf("Socket %d congested",sock_index);
                            // else{
                            //  printf("Remote Host terminated connection!\n");
                            // }
                            close(sock_index);

                            /* Remove from watched list */
                            FD_CLR(sock_index, &master_list);
                            // printf("Removed sock_index %d", sock_index);
                        }
                        else
                        {
                            // Process incoming data from existing clients here ...
                            //  sending to everyone

                            // strcpy(buffer, strtok(NULL, "\n"));

                            // printf("\n Executing Send\n");
                            // unsigned int client_port = ntohs(client_addr.sin_port);
                            // char str[INET_ADDRSTRLEN];
                            // inet_ntop(AF_INET, &(client_addr.sin_addr), str, INET_ADDRSTRLEN);

                            struct client *temp = *first_server_reference;
                            char *send_to_ip = (char *)malloc(sizeof(char) * INET_ADDRSTRLEN);

                            // printf("%s", buffer);
                            send_to_ip = strtok(buffer, " ");

                            if (strcmp(send_to_ip, "Port") == 0)
                            {
                                int success = 0;
                                assign_port(buffer, temp);
                                char client_list_buf[500];
                                send_client_list(*first_server_reference, client_list_buf);
                                // printf(" buf : %s", client_list_buf);
                                if (send(fdaccept, client_list_buf, strlen(client_list_buf), 0) == strlen(client_list_buf))
                                    // printf("Done!\n");
                                    //}
                                    success = 1;
                            }
                            else
                            {
                                char *msg = (char *)malloc(sizeof(char) * MESSAGE_SIZE);
                                // buffer = strtok(NULL, "\n");
                                while (buffer != NULL)
                                {
                                    // printf("\n %s", buffer);
                                    buffer = strtok(NULL, " ");
                                    if (buffer != NULL)
                                        strcpy(msg, buffer);
                                }

                                // strcpy(msg,buffer);
                                // printf("\n %s", msg);
                                struct client *temp = *first_server_reference;

                                // printf("\n from_ip %s");
                                temp = *first_server_reference;
                                send_to_client(sock_index, send_to_ip, msg, temp);
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

int bind_socket(int server_client_socket_number, struct sockaddr_in socket_listen_address, int port_number, int new_server_client_socket_number)
{
    socket_listen_address.sin_addr.s_addr = htonl(INADDR_ANY);
    socket_listen_address.sin_family = AF_INET;
    socket_listen_address.sin_port = htons(port_number);

    // Binding the socket to a port
    new_server_client_socket_number = bind(server_client_socket_number, (struct sockaddr *)&socket_listen_address, sizeof(socket_listen_address));
    if (new_server_client_socket_number < 0)
    {
        // printf("Bind failed\n");
        exit(EXIT_FAILURE);
    }
    new_server_client_socket_number = listen(server_client_socket_number, 4);
    return new_server_client_socket_number;
}

void add_new_client(struct client **first_server_reference, int fdaccept, struct sockaddr_in client_addr)
{
    // unsigned int client_port = ntohs(client_addr.sin_port);
    char str[INET_ADDRSTRLEN];
    char str1[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), str, INET_ADDRSTRLEN);
    // printf("%d %s", client_port, str);
    struct hostent *hostname = NULL;
    struct in_addr ipv4addr;
    inet_ntop(AF_INET, &(client_addr.sin_addr), str1, INET_ADDRSTRLEN);
    inet_pton(AF_INET, str1, &ipv4addr);
    hostname = gethostbyaddr(&ipv4addr, sizeof(ipv4addr), AF_INET);
    int hlen = (int)strlen(hostname->h_name);

    struct client *new_node = (struct client *)malloc(sizeof(client));
    // new_node->port_no = client_port;
    new_node->ip = (char *)malloc(sizeof(MESSAGE_SIZE));
    // printf("%s", str);
    strcpy(new_node->ip, str);
    // printf("%s", new_node->ip);
    strcpy(client_ip[c], str);
    // printf("\n %s", client_ip[c]);
    new_node->list_id = c;
    c++;
    new_node->client_fd = fdaccept;
    new_node->hostname = (char *)malloc(sizeof(hlen));
    strcpy(new_node->hostname, hostname->h_name);
    new_node->client_status = 1;
    // new_node->next = NULL;

    if (*first_server_reference == NULL)
    {

        *first_server_reference = new_node;
        new_node->next = NULL;
    }
    else
    {
        // printf("oye here!")
        new_node->next = *first_server_reference;
        *first_server_reference = new_node;
    }
}

/* function to swap data of two nodes a and b*/
void swap(struct client *a, struct client *b)
{
    int temp = a->port_no;
    a->port_no = b->port_no;
    b->port_no = temp;
}
/* Bubble sort the given linked lsit */
void bubbleSort(struct client *start)
{
    int swapped;
    struct client *ptr1 = start;
    struct client *lptr = NULL;

    /* Checking for empty list */
    if (ptr1 == NULL)
        return;

    do
    {
        swapped = 0;
        ptr1 = start;

        while (ptr1->next != lptr)
        {
            if (ptr1->port_no > ptr1->next->port_no)
            {
                swap(ptr1, ptr1->next);
                swapped = 1;
            }
            ptr1 = ptr1->next;
        }
        lptr = ptr1;
    } while (swapped);
}

void swap_int(char *num1, char *num2)
{
    int temp;
    temp = *num1;
    *num1 = *num2;
    *num2 = temp;
}

void print(struct client *headref)
{
    cse4589_print_and_log((char *)"[%s:SUCCESS]\n", "LIST");
    struct client *temp = headref;
    int list_id = 1;
    bubbleSort(headref);
    while (temp != NULL)
    {
        // printf("%d %s %d %d \n", list_id, temp->ip, temp->port_no, temp->client_fd);
        // printf("%-5d%-35s%-20s%-8d\n", list_id, "xyz", temp->ip, temp->port_no);
        if (temp->client_status == 1)
        {
            cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", list_id, temp->hostname, client_ip[temp->list_id], temp->port_no);
        }
        temp = temp->next;
        list_id = list_id + 1;
    }
    cse4589_print_and_log((char *)"[%s:END]\n", "LIST");
}

void send_to_client(int sock_index, char *send_to_ip, char *buffer, struct client *temp)
{
    char *str = (char *)malloc(sizeof(MESSAGE_SIZE));
    char *str1 = (char *)malloc(sizeof(MESSAGE_SIZE));
    char *str2 = (char *)malloc(sizeof(MESSAGE_SIZE));
    char *from_ip = (char *)malloc(sizeof(MESSAGE_SIZE));
    int j;
    int success = 1;
    struct client *temp1 = temp;
    for (j = 1; j <= 5; j++)
    {
        if (strcmp(send_to_ip, client_ip[j]) == 0)
        {
            // printf("\n %d %s", j, client_ip[j]);
            break;
        }
    }
    while (temp1 != NULL)
    {
        if (temp1->client_fd == sock_index)
            strcpy(from_ip, client_ip[temp1->list_id]);
        temp1 = temp1->next;
    }

    // if(j==6)
    //{ success = 0;//printf("\n Could not find receiver");
    // }
    // else

    // printf("\n Here");
    strcat(str, from_ip);
    strcat(str, " ");
    // strcat(str, send_to_ip);
    // strcat(str, " ");
    strcat(str, buffer);
    // strcat(str, "\0")
    while (temp->list_id != j)
        temp = temp->next;
    // printf("\n me here %d, %d",j,temp->client_fd);

    if (send(temp->client_fd, str, strlen(str), 0) == -1)
        success = 0; // perror("send");
    else
    {
        success = 1;
    }

    if (success == 1)
    {
        cse4589_print_and_log("[RELAYED:SUCCESS]\n");
        cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", from_ip, send_to_ip, buffer);
        cse4589_print_and_log("[RELAYED:END]\n");
    }
    else
    {
        cse4589_print_and_log("[RELAYED:ERROR]\n");
        // cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n",from_ip,send_to_ip,buffer);
        cse4589_print_and_log("[RELAYED:END]\n");
    }
}

void assign_port(char *buffer, struct client *temp)
{
    char *port, *clientip;
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
            clientip = a;
            // printf(" %s ", clientip);
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
        if (strcmp(clientip, client_ip[j]) == 0)
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
            temp->port_no = atoi(port);
            // printf("\n IP: %s port: %d", clientip, temp->port_no);
        }
    }
}

void send_port(int listening_port, int server_fd)
{
    char send_port[100];
    char str_cip[INET_ADDRSTRLEN];

    struct sockaddr_in socket_address_struct;
    int temp_udp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int len = sizeof(socket_address_struct);
    int result;

    if (temp_udp == -1)
    {
        // printf("Socket creation failed!");
    }

    memset((char *)&socket_address_struct, 0, sizeof(socket_address_struct));
    socket_address_struct.sin_family = AF_INET;
    socket_address_struct.sin_port = htons(EPHEMERAL_PORT);
    inet_pton(AF_INET, "8.8.8.8", &socket_address_struct.sin_addr);
    // socket_address_struct.sin_addr.s_addr = inet_addr("8.8.8.8");

    if (connect(temp_udp, (struct sockaddr *)&socket_address_struct, sizeof(socket_address_struct)) < 0)
    {
        // printf("\nConnection Failed \n");
        result = 0;
    }
    if (getsockname(temp_udp, (struct sockaddr *)&socket_address_struct, (unsigned int *)&len) == -1)
    {
        perror("getsockname");
        result = 0;
    }

    inet_ntop(AF_INET, &(socket_address_struct.sin_addr), str_cip, len);
    // printf("%s", str);

    // strcpy(str_cip,str_cip);
    // printf("\n IP: %s ", str_cip);
    char port[INET_ADDRSTRLEN];
    // printf("\n Listening_fd = %d:", listening_port);
    tostring(port, listening_port);
    // printf("%s\n", port);
    strcat(send_port, "Port");
    strcat(send_port, " ");
    strcat(send_port, str_cip);
    strcat(send_port, " ");
    strcat(send_port, port);
    strcat(send_port, "\n");

    if (send(server_fd, send_port, strlen(send_port), 0) == -1)
        perror("Send");
}

char *get_IP_address(char *str, int socket_number)
{
    struct sockaddr_in socket_address_struct;
    int len = sizeof(socket_address_struct);
    int result;

    if (socket_number == -1)
    {
    }

    memset((char *)&socket_address_struct, 0, sizeof(socket_address_struct));
    socket_address_struct.sin_family = AF_INET;
    socket_address_struct.sin_port = htons(53);
    inet_pton(AF_INET, "8.8.8.8", &socket_address_struct.sin_addr);

    if (connect(socket_number, (struct sockaddr *)&socket_address_struct, sizeof(socket_address_struct)) < 0 || getsockname(socket_number, (struct sockaddr *)&socket_address_struct, (unsigned int *)&len) == -1)
    {
        result = 0;
    }

    inet_ntop(AF_INET, &(socket_address_struct.sin_addr), str, len);

    if (result != 0)
    {
        cse4589_print_and_log("[%s:SUCCESS]\n", "IP");
        cse4589_print_and_log("IP:%s\n", str);
        cse4589_print_and_log("[%s:END]\n", "IP");
    }
    else
    {
        cse4589_print_and_log("[%s:ERROR]\n", "IP");
        cse4589_print_and_log("[%s:END]\n", "IP");
    }
    return str;
}

void tostring(char str[], int num)
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
        if (temp->client_status == 1)
        {
            strcat(buf, " ");
            tostring(list_id, temp->list_id);
            strcat(buf, list_id);
            strcat(buf, " ");
            tostring(str, temp->port_no);
            strcat(buf, str);
            strcat(buf, " ");
            strcat(buf, temp->hostname);
            strcat(buf, "\n");
            strcat(buf, client_ip[temp->list_id]);
            // strcat(buf, temp->block_status);
            strcat(buf, "\n");
        }
        temp = temp->next;
    }
    // printf("\n List: %s", buf);
    strcpy(client_list, buf);

    // return client_list;
}
void create_client_list(struct client **first_client_reference, char *buffer)
{

    char *string[256];
    // char delim = "\n";
    int i = 0;
    string[i] = strtok(buffer, "\n");
    while (string[i] != NULL)
    {
        // printf("string [%d]=%s\n",i,string[i]);
        i++;
        string[i] = strtok(NULL, "\n");
    }

    for (int j = 0; j < i; j++)
    {
        // printf("%s \n", string[j]);
        char *val = strtok(string[j], " ");
        int counter = 0;
        struct client *new_node = (struct client *)malloc(sizeof(client));
        while (val != NULL)
        {

            // printf("%s\n", val);
            if (j == 0)
            {
                if (counter == 0)
                    ;
                else if (counter == 1)
                    new_node->list_id = atoi(val);
                else if (counter == 2)
                { // printf("\n %s",val);
                    new_node->port_no = atoi(val);
                    // printf("\n port no %d	",new_node->port_no);
                }
                else if (counter == 3)
                {
                    new_node->hostname = (char *)malloc(sizeof(MESSAGE_SIZE));
                    strcpy(new_node->hostname, val);
                    // printf("\nhostname %s", new_node->hostname);
                }
                else if (counter == 4)
                {
                    new_node->ip = (char *)malloc(sizeof(MESSAGE_SIZE));
                    strcpy(new_node->ip, val);
                    strcpy(client_ip[new_node->list_id], val);
                    // printf("%s Client_ip",client_ip[new_node->list_id]);
                }
            }
            else
            {

                if (counter == 0)
                    new_node->list_id = atoi(val);
                else if (counter == 1)
                {
                    new_node->port_no = atoi(val);
                    // printf("\n Port no %d", new_node->port_no);
                }

                else if (counter == 2)
                {
                    new_node->hostname = (char *)malloc(sizeof(MESSAGE_SIZE));
                    strcpy(new_node->hostname, val);
                }
                else if (counter == 3)
                {
                    new_node->ip = (char *)malloc(sizeof(MESSAGE_SIZE));
                    strcpy(new_node->ip, val);
                    strcpy(client_ip[new_node->list_id], val);
                }
            }

            val = strtok(NULL, " ");
            counter += 1;
        }
        if (*first_client_reference == NULL)
        {

            *first_client_reference = new_node;
            new_node->next = NULL;
        }
        else
        {

            new_node->next = *first_client_reference;
            *first_client_reference = new_node;
        }
    }
}
void broadcast(struct client *first_client_reference, char *msg, int server_fd)
{
    char ip_str[INET_ADDRSTRLEN];
    // strcpy(ip_str,get_IP_address(ip_str));

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
    socket_address_struct.sin_port = htons(EPHEMERAL_PORT);
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
    char *send_buf = (char *)malloc(sizeof(char) * MESSAGE_SIZE);
    while (temp != NULL)
    {

        if (strcmp(client_ip[temp->list_id], ip_str) != 0)
        {

            strcat(send_buf, client_ip[temp->list_id]);
            // printf("\n %s",client_ip[temp->list_id]);
            strcat(send_buf, " ");
            strcat(send_buf, msg);
            // strcat(send_buf, "\n");
            //  printf("%s ' ' %s", client_ip[temp->list_id], msg);
            if (send(server_fd, send_buf, strlen(send_buf), 0) > 0)
                count += 1;
        }
        temp = temp->next;
        // free(send_buf);
        // send_buf = NULL;
    }
    if (count > 0)
    {
        cse4589_print_and_log("[RELAYED:SUCCESS]\n");
        cse4589_print_and_log("msg from:%s, to:255.255.255.255\n[msg]:%s\n", ip_str, msg);
        cse4589_print_and_log("[RELAYED:END]\n");
    }
    else
    {
        cse4589_print_and_log("[RELAYED:ERROR]\n");
        // cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n",from_ip,send_to_ip,buffer);
        cse4589_print_and_log("[RELAYED:END]\n");
    }
}
int client(struct client **first_client_reference, int port_no, int server_client_socket_number)
{
    struct sockaddr_in server_addr;
    int cmax;
    fd_set masterlist, watchlist;
    char *cmd = (char *)malloc(sizeof(char) * MESSAGE_SIZE);
    char *recv_buf = (char *)malloc(sizeof(char) * MESSAGE_SIZE);
    char *send_buf = (char *)malloc(sizeof(char) * MESSAGE_SIZE);
    char *argument_command;
    char *input_port_address = (char *)malloc(sizeof(char) * MESSAGE_SIZE);
    char *msg = (char *)malloc(sizeof(char) * MESSAGE_SIZE);
    char *client_server_ip_address = (char *)malloc(sizeof(char) * MESSAGE_SIZE);
    char *clientip;
    size_t nbyte_recvd;
    char *command = (char *)malloc(sizeof(char) * MESSAGE_SIZE);
    int server_login_socket;

    FD_ZERO(&masterlist);
    FD_ZERO(&watchlist);
    FD_SET(0, &masterlist);
    // FD_SET(server_client_socket_number, &masterlist);
    /// cmax = server_client_socket_number;
    cmax = 0;
    int server;
    server = server_client_socket_number;
    // server = connect_to_host(client_ip, port_no);

    while (1)
    {
        // printf("\n[PA1-Client@CSE489/589]$ ");
        // fflush(stdout);
        watchlist = masterlist;
        int selret = select(cmax + 1, &watchlist, NULL, NULL, NULL);
        if (selret == -1)
        {
            perror("select");
        }

        if (selret > 0)
        {

            for (int i = 0; i <= cmax; i++)
            {
                if (FD_ISSET(i, &watchlist))
                {
                    memset(cmd, '\0', MESSAGE_SIZE);
                    memset(recv_buf, '\0', MESSAGE_SIZE);
                    if (i == STDIN)
                    {
                        if (fgets(cmd, CMD_SIZE - 1, stdin) == NULL) // Mind the newline character that will be written to cmd
                            exit(-1);
                        strcpy(command, cmd);
                        argument_command = strtok(cmd, " ");

                        if (strcmp(cmd, "AUTHOR\n") == 0)
                        {
                            cse4589_print_and_log("[%s:SUCCESS]\n", "AUTHOR");
                            cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n", "bhavyasa");
                            cse4589_print_and_log("[%s:END]\n", "AUTHOR");
                        }
                        if (strcmp(cmd, "IP\n") == 0)
                        {
                            char ip_str[INET_ADDRSTRLEN];
                            int client_socket_number = socket(AF_INET, SOCK_STREAM, 0);
                            strcpy(ip_str, get_IP_address(ip_str, client_socket_number));
                        }
                        else if (strcmp(cmd, "PORT\n") == 0)
                        {
                            cse4589_print_and_log("[%s:SUCCESS]\n", "PORT");
                            cse4589_print_and_log("PORT:%d\n", port_no);
                            cse4589_print_and_log("[%s:END]\n", "PORT");
                        }
                        else if (strcmp(cmd, "LIST\n") == 0)
                            print(*first_client_reference);

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
                                if (server_login_socket > cmax)
                                    cmax = server_login_socket;
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
                                // connect using there args
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
                                    send_port(port_no, server_login_socket);
                                }

                                cse4589_print_and_log("[%s:SUCCESS]\n", "LOGIN");
                                cse4589_print_and_log("[%s:END]\n", "LOGIN");
                            }
                        }
                        else if (strcmp(argument_command, "SEND") == 0)
                        {
                            int count = 0;
                            while (count != 2)
                            {
                                char *a = (char *)malloc(sizeof(char) * MESSAGE_SIZE);
                                strcpy(a, argument_command);
                                if (count == 0)
                                {
                                    argument_command = strtok(NULL, " ");
                                    strcpy(a, argument_command);
                                    clientip = a;
                                }
                                else if (count == 1)
                                {
                                    argument_command = strtok(NULL, "\n");
                                    strcpy(a, argument_command);
                                    msg = a;
                                    break;
                                }

                                count += 1;
                            }
                            memset(send_buf, '\0', MESSAGE_SIZE);

                            server_addr.sin_family = AF_INET;
                            server_addr.sin_port = port_no;
                            inet_pton(AF_INET, clientip, &(server_addr.sin_addr));

                            strcat(send_buf, clientip);
                            strcat(send_buf, " ");
                            strcat(send_buf, msg);
                            strcat(send_buf, "\n");
                            // printf("%s ' ' %s", clientip, msg);

                            //                            struct client *temp = *first_client_reference;
                            //                            int found;
                            //                            while(temp != NULL)
                            //                            {
                            //                                if(strcmp(argument_command,client_ip[temp->list_id]) == 0)
                            //                                {
                            //                                    if(temp->block_status == 1)
                            //                                    break;
                            //                                    else
                            //
                            //                                }
                            //
                            //                                temp = temp->next;
                            //
                            //                            }
                            send(server_login_socket, send_buf, strlen(send_buf), 0);
                        }
                        else if (strcmp(argument_command, "BROADCAST") == 0)
                        {
                            char *a = (char *)malloc(sizeof(char) * MESSAGE_SIZE);
                            while (argument_command != NULL)
                            {

                                argument_command = strtok(NULL, "\n");
                                // printf("\n %s here", argument_command);
                                if (argument_command != NULL)
                                    strcpy(a, argument_command);
                                break;
                            }
                            // printf("\n", a);
                            // argument_command = strtok(NULL, "\n");
                            // printf("\n Tokenized");
                            // strcpy(a,argument_command);
                            broadcast(*first_client_reference, a, server_login_socket);
                        }
                        else if (strcmp(argument_command, "BLOCK") == 0)
                        {
                            // char *a = (char*) malloc(sizeof(char)*MESSAGE_SIZE);
                            argument_command = strtok(NULL, "\n");

                            // printf("%s", argument_command);

                            struct client *temp = *first_client_reference;
                            int found = 0;
                            while (temp != NULL)
                            {
                                if (strcmp(argument_command, client_ip[temp->list_id]) == 0)
                                {
                                    // printf("\n Client ip %s", client_ip[temp->list_id]);
                                    temp->block_status = 1;
                                    found += 1;
                                }

                                temp = temp->next;
                            }
                            if (found >= 1)
                            {
                                cse4589_print_and_log((char *)"[%s:SUCCESS]\n", "BLOCK");
                                cse4589_print_and_log((char *)"[%s:END]\n", "BLOCK");
                            }
                            else
                            {
                                cse4589_print_and_log((char *)"[%s:ERROR]\n", "BLOCK");
                                cse4589_print_and_log((char *)"[%s:END]\n", "BLOCK");
                            }
                        }
                        else if (strcmp(argument_command, "UNBLOCK") == 0)
                        {
                            // char *a = (char*) malloc(sizeof(char)*MESSAGE_SIZE);
                            argument_command = strtok(NULL, "\n");

                            // printf("%s", argument_command);

                            struct client *temp = *first_client_reference;
                            int found = 0;
                            while (temp != NULL)
                            {
                                if (strcmp(argument_command, client_ip[temp->list_id]) == 0)
                                {
                                    temp->block_status = 0;
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

                        else
                        {
                            // printf("\n Wrong Command");
                        }
                    }

                    else
                    {
                        // printf("\n In client receive! ");
                        memset(recv_buf, '\0', MESSAGE_SIZE);
                        if (i == 0 && i == server_client_socket_number)
                            break;
                        else
                        {
                            nbyte_recvd = recv(i, recv_buf, MESSAGE_SIZE, 0);
                            recv_buf[nbyte_recvd] = '\0';
                            char *argument_command = (char *)malloc(sizeof(char) * MESSAGE_SIZE);
                            ;
                            strcpy(argument_command, recv_buf);
                            struct client *temp = *first_client_reference;
                            char *identify = (char *)malloc(sizeof(char) * MESSAGE_SIZE);
                            strcpy(identify, recv_buf);
                            identify = strtok(identify, " ");
                            // printf("\n %s identify: ", identify);
                            if (strcmp(identify, "List") == 0)
                            { // printf("\n HI");
                                create_client_list(first_client_reference, recv_buf);
                            }
                            else
                            {
                                char *msg_from = (char *)malloc(sizeof(MESSAGE_SIZE));
                                char *a = (char *)malloc(sizeof(MESSAGE_SIZE));
                                char *msg = (char *)malloc(sizeof(MESSAGE_SIZE));

                                int count = 0;
                                if (recv_buf != NULL)
                                {
                                    // printf(" recv_buf %s", recv_buf);

                                    argument_command = strtok(argument_command, " ");
                                    strcpy(msg_from, argument_command);
                                    while (argument_command != NULL)
                                    {
                                        // printf("\n %s here", argument_command);
                                        argument_command = strtok(NULL, " ");
                                        if (argument_command != NULL)
                                            strcpy(msg, argument_command);
                                    }

                                    cse4589_print_and_log("[RECEIVED:SUCCESS]\n");
                                    cse4589_print_and_log("msg from:%s\n[msg]:%s\n", msg_from, msg);
                                    cse4589_print_and_log("[RECEIVED:END]\n");
                                }
                                else
                                {
                                    cse4589_print_and_log("[RECEIVED:ERROR]\n");
                                    cse4589_print_and_log("msg from:%s\n[msg]:%s\n", msg_from, msg);
                                    cse4589_print_and_log("[RECEIVED:END]\n");
                                }

                                // strcat(identify,recv_buf);
                                // printf("%s\n" , recv_buf);
                            }
                        }
                        fflush(stdout);
                    }
                }
            }
        }
    }
}
