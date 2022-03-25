#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SIZE 1024
#define LOCALHOST "127.0.0.1"
#define PORT 8080
#define TRUE 1

void client1();
int client2();
int main(int argc, char *argv[])
{
    if (atoi(argv[1]) == 1)
        client1();
    else
        client2();
    return 0;
}

void client1()
{
    printf("Client 1");
    int bind_value;

    int socket_file_descriptor, new_client_socket;
    struct sockaddr_in client_address, client_struct;
    char buffer[SIZE];

    socket_file_descriptor = socket(AF_INET, SOCK_STREAM, 0);

    client_address.sin_family = AF_INET;
    client_address.sin_port = PORT;
    client_address.sin_addr.s_addr = inet_addr(LOCALHOST);
    bind_value = bind(socket_file_descriptor, (struct sockaddr *)&client_address, sizeof(client_address));
    bind_value = listen(socket_file_descriptor, 5);
    socklen_t size = sizeof(client_struct);
    new_client_socket = accept(socket_file_descriptor, (struct sockaddr *)&client_struct, &size);

    int n;
    FILE *fp;
    char new_buffer[SIZE];
    fp = fopen("file2.txt", "w");
    while (TRUE)
    {
        n = recv(new_client_socket, new_buffer, SIZE, 0);
        if (n <= 0)
        {
            break;
        }
        fprintf(fp, "%s", new_buffer);
        bzero(new_buffer, SIZE);
    }
}

int client2()
{
    printf("Client 2");
    int bind_value;
    int socket_file_descriptor;
    struct sockaddr_in client_address;
    FILE *fp;
    socket_file_descriptor = socket(AF_INET, SOCK_STREAM, 0);

    client_address.sin_family = AF_INET;
    client_address.sin_port = PORT;
    client_address.sin_addr.s_addr = inet_addr(LOCALHOST);

    bind_value = connect(socket_file_descriptor, (struct sockaddr *)&client_address, sizeof(client_address));
    fp = fopen("file1.txt", "r");
    char data[SIZE] = {0};

    while (fgets(data, SIZE, fp) != NULL)
    {
        if (send(socket_file_descriptor, data, sizeof(data), 0) == -1)
        {
            perror("[-] Error in sendung data");
            exit(1);
        }
        bzero(data, SIZE);
    }
    close(socket_file_descriptor);
    return 0;
}