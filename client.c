#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>

#define SERVER_ADDR "127.0.0.1"

void *receive_messages(void *arg)
{
    int socket_fd = *((int *)arg);
    char buffer[256];
    ssize_t read_size;

    while ((read_size = recv(socket_fd, buffer, sizeof(buffer), 0)) > 0)
    {
        buffer[read_size] = '\0';
        printf("%s\n", buffer);
    }
    return NULL;
}

int main(int argc, char const *argv[])
{
    const char * bienvenue = "cat 'welcome'";
    system(bienvenue);
    int socket_fd;
    struct sockaddr_in server_addr;
    char buffer[256];

    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Problème lors de la création du socket");
        exit(1);
    }
    int port = atoi(argv[1]);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

    if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("La connexion a échoué");
        exit(1);
    }

    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, receive_messages, &socket_fd);

    while (1)
    {

        fgets(buffer, sizeof(buffer), stdin);
        send(socket_fd, buffer, strlen(buffer), 0);
    }

    close(socket_fd);
    return 0;
}


