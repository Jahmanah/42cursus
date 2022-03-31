#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

# define RECV_SIZE	4096
# define INFO_SIZE	64

typedef struct		s_client
{
    int		fd;
    int		id;
    char		*save;
    struct s_client	*next;
}			t_client;

typedef struct		s_server
{
    int		listen_sd;
    int		max_sd;
    int		last_id;

    char		info[INFO_SIZE];
    char		buff[RECV_SIZE];

    fd_set		master_set;
    fd_set		read_set;
    fd_set		write_set;

    t_client	*clients;
}			t_server;

void	fatal(t_server *server)
{
    write(2, "Fata error\n", strlen("Fata error\n"));
    close(server->listen_sd);
    exit(1);
}

int	has_return(char *str)
{
    int i = 0;

    while (str[i])
    {
        if (str[i] == '\n')
            return (1);
        i++;
    }
    return (0);
}

char	*ft_strjoin(t_server *server, char *s1, char *s2)
{
    size_t i, k, s1_size, s2_size;
    char *result;

    i = k = -1;
    s1_size = (!s1) ? 0 : strlen(s1);
    s2_size = (!s2) ? 0 : strlen(s2);

    if (!(result = malloc((s1_size + s2_size + 1) * sizeof(char))))
        fatal(server);
    while (++i < s1_size)
        result[i] = s1[i];
    while (++k < s2_size)
        result[i + k] = s2[k];
    result[i + k] = '\0';
    free(s1);
    return (result);
}

char	*update_save(t_server *server, char *save)
{
    int i = 0;
    int k = 0;
    char	*new_save;

    while (save[i] && save[i] != '\n')
        i++;
    if (!save[i])
    {
        free(save);
        return (0);
    }
    i++;

    if (!(new_save = malloc((strlen(save) - i + 1) * sizeof(char))))
        fatal(server);
    while (save[i])
        new_save[k++] = save[i++];
    new_save[k] = '\0';
    free(save);
    return (new_save);
}

int	get_id(t_server *server, int fd)
{
    t_client	*tmp = server->clients;

    if (!tmp)
        return (-1);
    while (tmp)
    {
        if (tmp->fd == fd)
            return (tmp->id);
        tmp = tmp->next;
    }
    return (-1);
}

void	send_all(t_server *server, int fd, char *message)
{
    t_client	*tmp = server->clients;

    if (!tmp)
        return ;
    while (tmp)
    {
        if (FD_ISSET(tmp->fd, &(server->write_set)) && tmp->fd != fd)
        {
            if (send(tmp->fd, message, strlen(message), 0) < 0)
                fatal(server);
        }
        tmp = tmp->next;
    }
}

void	setup_server(t_server *server, char *port)
{
    struct sockaddr_in	sockaddr;

    bzero(server->info, INFO_SIZE);
    bzero(server->buff, RECV_SIZE);
    server->last_id = 0;
    server->clients = 0;

    if ((server->listen_sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        fatal(server);

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(atoi(port));
    sockaddr.sin_addr.s_addr = htonl(2130706433);
    if (bind(server->listen_sd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0)
        fatal(server);
    if (listen(server->listen_sd, 50) < 0)
        fatal(server);

    FD_ZERO(&(server->master_set));
    FD_SET(server->listen_sd, &(server->master_set));
    server->max_sd = server->listen_sd;
}

int	add_client_to_list(t_server *server, int fd)
{
    t_client	*tmp = server->clients;
    t_client	*new;

    if (!(new = calloc(1, sizeof(t_client))))
        fatal(server);
    new->fd = fd;
    new->id = server->last_id++;
    new->next = 0;

    if (!tmp)
        server->clients = new;
    else
    {
        while (tmp->next)
            tmp = tmp->next;
        tmp->next = new;
    }
    return (new->id);
}

void	add_client(t_server *server)
{
    struct sockaddr_in	sockaddr;
    socklen_t		len = sizeof(sockaddr);
    int			new_sd;

    if ((new_sd = accept(server->listen_sd, (struct sockaddr *)&sockaddr, &len)) < 0)
        fatal(server);
    if (new_sd > server->max_sd)
        server->max_sd = new_sd;
    sprintf(server->info, "server: client %d just arrived\n", add_client_to_list(server, new_sd));
    send_all(server, new_sd, server->info);
    FD_SET(new_sd, &(server->master_set));
}

int	rm_client(t_server *server, int fd)
{
    int		id = get_id(server, fd);
    t_client	*tmp = server->clients;
    t_client	*del;

    if (!tmp)
        return (-1);

    if (tmp->fd == fd)
    {
        server->clients = tmp->next;
        free(tmp);
    }
    else
    {
        while (tmp && tmp->next && tmp->next->fd != fd)
            tmp = tmp->next;
        del = tmp->next;
        tmp->next = tmp->next->next;
        free(del);
    }
    return (id);
}

void	extract_and_send(t_server *server, int fd)
{
    t_client	*tmp = server->clients;
    while (tmp && tmp->fd != fd)
        tmp = tmp->next;
    if (!tmp)
        return ;
    tmp->save = ft_strjoin(server, tmp->save, server->buff);

    if (has_return(tmp->save))
    {
        int	i = 0;
        char	*tool;
        char	*tool2;

        if (!(tool = malloc((strlen(tmp->save) + 1) * sizeof(char))))
            fatal(server);
        if (!(tool2 = malloc((strlen(tmp->save) + 64 + 1) * sizeof(char))))
            fatal(server);
        bzero(tool, strlen(tmp->save) + 1);
        bzero(tool2, strlen(tmp->save) + 64 + 1);

        while (tmp->save[i])
        {
            tool[i] = tmp->save[i];
            if (tmp->save[i] == '\n')
            {
                sprintf(tool2, "client %d: %s", get_id(server, fd), tool);
                send_all(server, fd, tool2);
                bzero(tool, strlen(tool));
                bzero(tool2, strlen(tool2));
                i = 0;
                tmp->save = update_save(server, tmp->save);
            }
            else
                i++;
        }
    }
}


int	main(int argc, char **argv)
{
    t_server	server;
    int		rc;

    if (argc != 2)
    {
        write(2, "Wrong number of arguments\n", strlen("Wrong number of arguments\n"));
        exit(1);
    }

    setup_server(&server, argv[1]);

    while (1)
    {
        server.read_set = server.write_set = server.master_set;
        if (select(server.max_sd + 1, &(server.read_set), &(server.write_set), 0, 0) < 0)
            continue ;
        for (int i = 0; i <= server.max_sd; i++)
        {
            if (FD_ISSET(i, &(server.read_set)))
            {
                if (i == server.listen_sd)
                {
                    bzero(server.info, INFO_SIZE);
                    add_client(&server);
                    break ;
                }
                else
                {
                    rc = recv(i, server.buff, RECV_SIZE - 1, 0);
                    if (rc <= 0)
                    {
                        bzero(server.info, INFO_SIZE);
                        sprintf(server.info, "server: client %d just left\n", rm_client(&server, i));
                        send_all(&server, i, server.info);
                        FD_CLR(i, &(server.master_set));
                        close(i);
                        while (!FD_ISSET(server.max_sd, &(server.master_set)))
                            server.max_sd--;
                        break ;
                    }
                    else
                    {
                        server.buff[rc] = '\0';
                        extract_and_send(&server, i);
                    }
                }
            }

        }

    }

    return (0);
}