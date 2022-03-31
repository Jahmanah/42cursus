#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>




typedef struct s_client{
    int fd;
    int id;
    struct s_client *next;
} t_client;

int listen_fd;
int g_id;
fd_set write_fd, read_fd, master_fd;
t_client *g_clients = NULL;
char msg[42];
char str[42*4096], tmp[42*4096], buff[42*4096 + 42];

void fatal(){
    write(2, "Fatal\n", strlen("fatal\n"));
    close(listen_fd);
    exit(1);
}

int getMaxFd(){
   int max = listen_fd;
   t_client *tmp = g_clients;

   while(tmp){
       if (tmp->fd > max)
           max = tmp->fd;
       tmp = tmp->next;
   }
    return (max);
}

int addClientToList(int fd){

}

void addClient(){
    int client_fd;
    struct sockaddr_in clientaddr;
    socklen_t len = sizeof (clientaddr);

    if ((client_fd = accept(listen_fd, (struct sockaddr *)&clientaddr, &len)) < 0)
        fatal();
    sprintf(msg, "server: client %d just arrived\n", addClientToList(client_fd));
}

int main(int ac, char** av){
    if (ac != 2){
        write(2, "needmore\n", strlen("needmore\n"));
        exit(1);
    }

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof (servaddr));

    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        fatal();
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
    servaddr.sin_port = htons(atoi(av[1]));

    if (bind(listen_fd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        fatal();
    if (listen(listen_fd, 10) < 0)
        fatal();

    FD_ZERO(&master_fd);
    FD_SET(listen_fd, &master_fd);
    bzero(str, sizeof (str));
    bzero(buff, sizeof (buff));
    bzero(tmp, sizeof (tmp));

    while(1){
        write_fd = read_fd = master_fd;
        if (select(getMaxFd() + 1, &read_fd, &write_fd, 0,0) < 0)
            continue ;
        for (int fd = 0; fd <= getMaxFd(); fd++){
            if (FD_ISSET(listen_fd, &read_fd)){
                if (fd == listen_fd){
                    bzero(&msg, sizeof (msg));
                    addClient();
                }
            }
        }
    }
    return (0);
}