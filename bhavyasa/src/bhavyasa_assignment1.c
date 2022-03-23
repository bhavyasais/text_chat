/**
 * @bhavyasa_assignment1
 * @author  Bhavyasai Survepalli <bhavyasa@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */
#include <stdio.h>
#include <stdlib.h>

#include "../include/global.h"
#include "../include/logger.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>

#define BACKLOG 5
#define STDIN 0
#define TRUE 1
#define CMD_SIZE 100
#define BUFFER_SIZE 256
#define MSG_SIZE 256
#define MSG_LENGTH 1024
/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */
struct client
{
    int list_id;
    char *ip;
    int client_fd;
    char *hostname;
    int block_status;
    int client_status;
    int port_no;
    int num_msg_recv;
    int num_msg_sent;
    struct client *next;
};

int server(struct client **head_ref, int port, int listening_fd);
int client(struct client **c_ref, int port, int listening_fd);
int connect_to_host(char *server_ip, char *server_port);
int main(int argc, char **argv)
{
    /*Init. Logger*/
    cse4589_init_log(argv[2]);

    /*Clear LOGFILE*/
    fclose(fopen(LOGFILE, "w"));

    /*Start Here*/
    int listening_fd = 0, temp_result;
    struct client *head_ref = NULL;
    struct client *c_ref = NULL;
    struct sockaddr_in listen_addr;

    listening_fd = socket(AF_INET, SOCK_STREAM, 0);
    // if(listening_fd == 0)
    // {

    //     exit(EXIT_FAILURE);
    // }
    listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(atoi(argv[2]));
    temp_result = bind(listening_fd, (struct sockaddr *)&listen_addr, sizeof(listen_addr));
    // if(temp_result <0)
    // {

    //     exit(EXIT_FAILURE);
    // }
    temp_result = listen(listening_fd, 4);
    // if(temp_result< 0)
    // {

    //     exit(EXIT_FAILURE);
    // }
    if (argc == 3)
    {
        int port = atoi(argv[2]);
        if (*argv[1] == 's')
            server(&head_ref, port, listening_fd);
        else
            client(&c_ref, port, listening_fd);
    }
    return 0;
}

int server(struct client **head_ref, int port, int listening_fd)
{
    int first_server, selret, caddr_len = 0, fdaccept = 0;
    struct sockaddr_in client_addr;
    fd_set master_list, watch_list;

    FD_ZERO(&master_list);
    FD_ZERO(&watch_list);

    FD_SET(listening_fd, &master_list);

    FD_SET(STDIN, &master_list);

    first_server = listening_fd;

    while (1)
    {
        memcpy(&watch_list, &master_list, sizeof(master_list));

        selret = select(first_server + 1, &watch_list, NULL, NULL, NULL);
        if (selret < 0)
            perror("select failed.");

        if (selret > 0)
        {

            for (int sock_index = 0; sock_index <= first_server; sock_index += 1)
            {

                if (FD_ISSET(sock_index, &watch_list))
                {

                    if (sock_index == STDIN)
                    {
                        char *cmd = (char *)malloc(sizeof(char) * CMD_SIZE);
                        memset(cmd, '\0', CMD_SIZE);
                        if (fgets(cmd, CMD_SIZE - 1, stdin) == NULL)
                            exit(-1);

                        if (strcmp(cmd, "AUTHOR\n") == 0)
                        {
                            cse4589_print_and_log("[%s:SUCCESS]\n", "AUTHOR");
                            cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n", "Bhavyasai");
                            cse4589_print_and_log("[%s:END]\n", "AUTHOR");
                        }
                        else if (strcmp(cmd, "IP\n") == 0)
                        {
                            char hostName[32];
                            struct in_addr address;

                            // Reference: https://man7.org/linux/man-pages/man2/gethostname.2.html
                            // https://man7.org/linux/man-pages/man3/gethostbyname.3.html

                            gethostname(hostName, 32);
                            struct hostent *hostStruct = gethostbyname(hostName);

                            // Reference: https://man7.org/linux/man-pages/man7/ip.7.html
                            address.s_addr = ((uint32_t)hostStruct->h_addr_list[0]);

                            // Reference: https://linux.die.net/man/3/inet_ntoa
                            char *ip_addr = inet_ntoa(address);

                            cse4589_print_and_log("[%s:SUCCESS]\n", "IP");
                            cse4589_print_and_log("IP:%s\n", ip_addr);
                            cse4589_print_and_log("[%s:END]\n", "IP");
                        }
                        else if (strcmp(cmd, "PORT\n") == 0)
                        {
                            cse4589_print_and_log("[%s:SUCCESS]\n", "PORT");
                            cse4589_print_and_log("PORT:%d\n", port);
                            cse4589_print_and_log("[%s:END]\n", "PORT");
                        }
                        // else if(strcmp(cmd, LIST)==0)
                        // {
                        //     print(*head_ref);
                        // }
                    }

                    // else if(sock_index == listening_fd){
                    //     caddr_len = sizeof(client_addr);
                    //     fdaccept = accept(listening_fd, (struct sockaddr *)&client_addr, (socklen_t*)&caddr_len);
                    //     if(fdaccept < 0)
                    //         perror("Accept failed.");

                    //     else
                    //     {

                    //         FD_SET(fdaccept, &master_list);
                    //         if(fdaccept > first_server)first_server = fdaccept;

                    //         add_new_client(head_ref, fdaccept, client_addr);

                    //     }

                    // }

                    // else{

                    //     if(sock_index == 0)
                    //         break;
                    //     char *buffer = (char*) malloc(sizeof(char)*BUFFER_SIZE);
                    //     memset(buffer, '\0', BUFFER_SIZE);
                    //     int recd_bytes;
                    //     if(recv(recd_bytes = sock_index, buffer, BUFFER_SIZE, 0) <= 0)
                    //     {
                    //         close(sock_index);

                    //         FD_CLR(sock_index, &master_list);

                    // }
                    //                         else
                    //                         {

                    //                             struct client *temp = *head_ref;
                    //                             char *send_to_ip = (char*) malloc(sizeof(char)*INET_ADDRSTRLEN);

                    //                             send_to_ip = strtok(buffer, " ");

                    //                             if(strcmp(send_to_ip, "Port") == 0)
                    //                             {
                    //                                 int success =0;
                    //                                 assign_port(buffer, temp);
                    //                                 char client_list_buf[500];
                    //                                 send_client_list(*head_ref, client_list_buf);

                    //                                 if(send(fdaccept, client_list_buf, strlen(client_list_buf), 0) == strlen(client_list_buf))

                    //                                     success = 1;
                    //                             }
                    //                             else
                    //                             {
                    //                                 char * msg = (char*) malloc(sizeof(char)*MSG_LENGTH);

                    // 				while(buffer != NULL)
                    //     				{

                    //         				 buffer= strtok(NULL, " ");
                    //       				        if(buffer!=NULL)
                    //         					strcpy(msg,buffer);

                    //    				 }

                    //                                 struct client * temp = *head_ref;
                    //   temp = *head_ref;
                    //                                 send_to_client(sock_index, send_to_ip, msg ,temp);
                    //                             }

                    //                             fflush(stdout);

                    //                         }

                    // }
                }
            }
        }
    }

    return 0;
}

int client(struct client **c_ref, int port, int listening_fd)
{
    struct sockaddr_in server_addr;
    int server_fd, cmax;
    fd_set masterlist, watchlist;
    char *cmd = (char *)malloc(sizeof(char) * MSG_LENGTH);
    char *recv_buf = (char *)malloc(sizeof(char) * MSG_LENGTH);
    char *send_buf = (char *)malloc(sizeof(char) * MSG_LENGTH);
    char *input;
    char *in_port = (char *)malloc(sizeof(char) * MSG_LENGTH);
    char *msg = (char *)malloc(sizeof(char) * MSG_LENGTH);
    char *serverip = (char *)malloc(sizeof(char) * MSG_LENGTH);
    char *clientip;
    size_t nbyte_recvd;
    char *command = (char *)malloc(sizeof(char) * MSG_LENGTH);

    FD_ZERO(&masterlist);
    FD_ZERO(&watchlist);
    FD_SET(0, &masterlist);
    cmax = 0;
    int server;
    server = listening_fd;

    while (1)
    {
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
                    memset(cmd, '\0', MSG_LENGTH);
                    memset(recv_buf, '\0', MSG_LENGTH);
                    if (i == STDIN)
                    {
                        if (fgets(cmd, CMD_SIZE - 1, stdin) == NULL)
                            exit(-1);
                        strcpy(command, cmd);
                        input = strtok(cmd, " ");

                        if (strcmp(cmd, "AUTHOR\n") == 0)
                        {
                            cse4589_print_and_log("[%s:SUCCESS]\n", "AUTHOR");
                            cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n", "Bhavyasai");
                            cse4589_print_and_log("[%s:END]\n", "AUTHOR");
                        }
                        else if (strcmp(cmd, "IP\n") == 0)
                        {
                            char hostName[32];
                            struct in_addr address;

                            // Reference: https://man7.org/linux/man-pages/man2/gethostname.2.html
                            // https://man7.org/linux/man-pages/man3/gethostbyname.3.html

                            gethostname(hostName, 32);
                            struct hostent *hostStruct = gethostbyname(hostName);

                            // Reference: https://man7.org/linux/man-pages/man7/ip.7.html
                            address.s_addr = ((uint32_t)hostStruct->h_addr_list[0]);

                            // Reference: https://linux.die.net/man/3/inet_ntoa
                            char *ip_addr = inet_ntoa(address);

                            cse4589_print_and_log("[%s:SUCCESS]\n", "IP");
                            cse4589_print_and_log("IP:%s\n", ip_addr);
                            cse4589_print_and_log("[%s:END]\n", "IP");
                        }
                        else if (strcmp(cmd, "PORT\n") == 0)
                        {
                            cse4589_print_and_log("[%s:SUCCESS]\n", "PORT");
                            cse4589_print_and_log("PORT:%d\n", port);
                            cse4589_print_and_log("[%s:END]\n", "PORT");
                        }
                        // else if(strcmp(cmd,"LIST\n")==0)
                        //     print(*c_ref);
                        // else if (strcmp(input ,"LOGIN") == 0)
                        // {
                        //     int count =0, result = 1;
                        //     server_fd = socket(AF_INET, SOCK_STREAM, 0);
                        //     if(server_fd < 0)
                        //     {

                        //         exit(EXIT_FAILURE);
                        //     }
                        //     else
                        //     {
                        //         FD_SET(server_fd, &masterlist);
                        //         if(server_fd > cmax)cmax = server_fd;

                        //     }

                        //     while(count != 2)
                        //     {
                        //         char *a = (char*) malloc(sizeof(char)*CMD_SIZE);

                        //         if(count == 0)
                        //         {
                        //             input = strtok(NULL, " ");
                        //             strcpy(a,input);
                        //             serverip = a;
                        //         }
                        //         else if(count == 1)
                        //         {

                        //             input = strtok(NULL, "\n");
                        //             if(input == NULL)
                        //                 result = 0;
                        //             else{
                        //                 strcpy(a,input);
                        //                 in_port = a;
                        //                 break;
                        //             }
                        //         }

                        //         count +=1;
                        //     }

                        //     char *temp = serverip;

                        //     if(result == 0)
                        //     {
                        //         cse4589_print_and_log("[%s:ERROR]\n", "LOGIN");
                        //         cse4589_print_and_log("[%s:END]\n", "LOGIN");
                        //     }
                        //     else{

                        //         server_addr.sin_family = AF_INET;
                        //         unsigned int port_temp = atoi(in_port);
                        //         server_addr.sin_port = htons(port_temp);
                        //         inet_pton(AF_INET, serverip, &(server_addr.sin_addr));

                        //         if (connect(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <0)

                        //         {
                        //             cse4589_print_and_log("[%s:ERROR]\n", "LOGIN");
                        //             cse4589_print_and_log("[%s:END]\n", "LOGIN");
                        //         }

                        //         else{
                        //             send_port(port_no, server_fd);
                        //         }

                        //         cse4589_print_and_log("[%s:SUCCESS]\n", "LOGIN");
                        //         cse4589_print_and_log("[%s:END]\n", "LOGIN");
                        //     }

                        // }
                        // else if(strcmp(input, "SEND")==0)
                        // {
                        //     int count =0;
                        //     while(count != 2)
                        //     {
                        //         char *a = (char*) malloc(sizeof(char)*MSG_LENGTH);
                        //         strcpy(a,input);
                        //         if(count == 0)
                        //         {
                        //             input = strtok(NULL, " ");
                        //             strcpy(a,input);
                        //             clientip = a;
                        //         }
                        //         else if(count == 1)
                        //         {
                        //             input = strtok(NULL, "\n");
                        //             strcpy(a,input);
                        //             msg = a;
                        //             break;
                        //         }

                        //         count +=1;
                        //     }
                        //     memset(send_buf, '\0', MSG_LENGTH);

                        //     server_addr.sin_family = AF_INET;
                        //     server_addr.sin_port = port_no;
                        //     inet_pton(AF_INET, clientip, &(server_addr.sin_addr));

                        //     strcat(send_buf,clientip);
                        //     strcat(send_buf," ");
                        //     strcat(send_buf,msg);
                        //     strcat(send_buf, "\n");

                        //     send(server_fd, send_buf, strlen(send_buf),0);

                        // }
                        //         else if(strcmp(input, "BROADCAST")==0)
                        //         {
                        //             char *a = (char*) malloc(sizeof(char)*MSG_LENGTH);
                        // while(input != NULL)
                        // 		{

                        // 			input= strtok(NULL, "\n");

                        // 			 if(input!=NULL)
                        // 			strcpy(a,input);
                        // 		break;

                        // 		}

                        //             broadcast(*c_ref,a, server_fd);

                        //         }
                        // else if(strcmp(input, "BLOCK")== 0)
                        // {

                        //     input = strtok(NULL, "\n");

                        //     struct client *temp = *c_ref;
                        //     int found = 0;
                        //     while(temp != NULL)
                        //     {
                        //     if(strcmp(input,client_ip[temp->list_id]) == 0)
                        //     {

                        //        temp->block_status = 1;
                        //         found+=1;
                        //     }

                        //     temp = temp->next;

                        //     }
                        //     if(found>=1)
                        //     {
                        //         cse4589_print_and_log((char *)"[%s:SUCCESS]\n","BLOCK");
                        //         cse4589_print_and_log((char *)"[%s:END]\n","BLOCK");
                        //     }
                        //     else{
                        //         cse4589_print_and_log((char *)"[%s:ERROR]\n","BLOCK");
                        //         cse4589_print_and_log((char *)"[%s:END]\n","BLOCK");
                        //     }

                        // }
                        // else if(strcmp(input, "UNBLOCK")== 0)
                        // {

                        //     input = strtok(NULL, "\n");

                        //    // printf("%s", input);

                        //     struct client *temp = *c_ref;
                        //     int found = 0;
                        //     while(temp != NULL)
                        //     {
                        //         if(strcmp(input,client_ip[temp->list_id]) == 0)
                        //         {
                        //             temp->block_status = 0;
                        //             found+=1;
                        //         }

                        //         temp = temp->next;

                        //     }
                        //     if(found>=1)
                        //     {
                        //         cse4589_print_and_log((char *)"[%s:SUCCESS]\n","UNBLOCK");
                        //         cse4589_print_and_log((char *)"[%s:END]\n","UNBLOCK");
                        //     }
                        //     else{
                        //         cse4589_print_and_log((char *)"[%s:ERROR]\n","UNBLOCK");
                        //         cse4589_print_and_log((char *)"[%s:END]\n","UNBLOCK");
                        //     }

                        // }

                        // else if(strcmp(cmd,"LOGOUT\n")==0){
                        //     cse4589_print_and_log((char *)"[%s:SUCCESS]\n","LOGOUT");
                        //     close(server_fd );
                        //     cse4589_print_and_log((char *)"[%s:END]\n","LOGOUT");
                        //     return 0;
                        // }

                        // else if(strcmp(cmd, "EXIT\n")==0)
                        // {
                        //     cse4589_print_and_log((char *)"[%s:SUCCESS]\n", (char*)"EXIT");
                        //     close(server_fd);
                        //     cse4589_print_and_log((char *)"[%s:END]\n", (char*)"EXIT");
                        //     exit(0);
                        // }
                    }

                    //     else
                    //     {

                    //         memset(recv_buf, '\0', MSG_LENGTH);
                    //         if(i == 0 && i==listening_fd)
                    //             break;
                    //         else
                    //         {
                    //             nbyte_recvd = recv(i, recv_buf, MSG_LENGTH, 0);
                    //             recv_buf[nbyte_recvd] = '\0';
                    // char *input = (char*) malloc(sizeof(char)*MSG_LENGTH);;
                    //             strcpy(input, recv_buf);
                    //             struct client *temp = *c_ref;
                    //             char *identify = (char*) malloc(sizeof(char)*MSG_LENGTH);
                    //             strcpy(identify,recv_buf);
                    //             identify = strtok(identify, " ");

                    //             if(strcmp(identify, "List") == 0)
                    //             {
                    //                 create_client_list(c_ref, recv_buf);
                    //             }
                    //             else
                    //             {
                    // char *msg_from = (char*)malloc(sizeof(MSG_LENGTH));
                    //     char *a = (char*)malloc(sizeof(MSG_LENGTH));
                    //     char *msg = (char*)malloc(sizeof(MSG_LENGTH));

                    //                 int count = 0;
                    //                 if(recv_buf!=NULL)
                    //                 {

                    // 	input = strtok(input, " ");
                    // 		strcpy(msg_from, input);
                    // 		while(input != NULL)
                    // 		{

                    // 			input= strtok(NULL, " ");
                    // 			 if(input!=NULL)
                    // 			strcpy(msg,input);

                    // 		}

                    //                     cse4589_print_and_log("[RECEIVED:SUCCESS]\n");
                    //                     cse4589_print_and_log("msg from:%s\n[msg]:%s\n",msg_from,msg);
                    //                     cse4589_print_and_log("[RECEIVED:END]\n");

                    //                 }
                    //                 else
                    //                 {
                    //                     cse4589_print_and_log("[RECEIVED:ERROR]\n");
                    //                     cse4589_print_and_log("msg from:%s\n[msg]:%s\n",msg_from,msg);
                    //                     cse4589_print_and_log("[RECEIVED:END]\n");
                    //                 }

                    //             }
                    //         }
                    //         fflush(stdout);
                    //     }
                }
            }
        }
    }
}

int connect_to_host(char *server_ip, char *server_port)
{
    int fdsocket;
    struct addrinfo hints, *res;

    /* Set up hints structure */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    /* Fill up address structures */
    if (getaddrinfo(server_ip, server_port, &hints, &res) != 0)
        perror("getaddrinfo failed");

    /* Socket */
    fdsocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fdsocket < 0)
        perror("Failed to create socket");

    /* Connect */
    if (connect(fdsocket, res->ai_addr, res->ai_addrlen) < 0)
        perror("Connect failed");

    freeaddrinfo(res);

    return fdsocket;
}
