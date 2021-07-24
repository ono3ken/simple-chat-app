/*
 * server.c
 *   クライアントからの接続要求を受け付けるサーバープログラム。
 *   
 *   クライアントから送られてきた文字列を大文字に変換して送り返す。
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>

#define PORT 8500
#define NUM_CLIENT 2

struct data
{
    int id_;
    int fd_listener;
    int *fd_client;
};

void *chat_thread(void *arg)
{
    int id_ = ((struct data *)arg)->id_;
    int fd_listener = ((struct data *)arg)->fd_listener;
    int *fd_client = ((struct data *)arg)->fd_client;

    int i;
    char buf[1024];
    char ret_str[1024];
    int fd;
    struct sockaddr_in caddr;
    int len;

    len = sizeof(caddr);
    if ((fd = accept(fd_listener, (struct sockaddr *)&caddr, &len)) < 0)
    {
        perror("accept");
        exit(1);
    }

    fd_client[id_] = fd;

    do
    {
        read(fd, buf, 1024);

        sprintf(ret_str, "Client %d:%s", id_, buf);

        /* 変換したデータをクライアントに送り返す */
        for (i = 0; i < NUM_CLIENT; i++)
        {
            if ((i != id_) && (fd_client[i] > 0))
            {
                write(fd_client[i], ret_str, 1024);
            }
        }

    } while (strcmp(buf, "QUIT\n") != 0);

    /* 通信が終わったらソケットを閉じる */
    close(fd);

    return NULL;
}

int main()
{
    int i;
    int fd_listener;
    struct sockaddr_in saddr;
    struct data d[NUM_CLIENT];
    int fd_client[NUM_CLIENT];
    pthread_t t[NUM_CLIENT];

    /*
     *  ストリーム型ソケット作る．
     */
    if ((fd_listener = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        exit(1);
    }

    /* 
     * saddrの中身を0にしておかないと、bind()でエラーが起こることがある
     */
    bzero((char *)&saddr, sizeof(saddr));

    /*
     * ソケットの名前を入れておく
     */
    saddr.sin_family = PF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(PORT);

    /*
     * ソケットにアドレスをバインドする。
     */
    if (bind(fd_listener, (struct sockaddr *)&saddr, sizeof(saddr)) < 0)
    {
        perror("bind");
        exit(1);
    }

    /*
     * listenをソケットに対して発行する
     */
    if (listen(fd_listener, 1) < 0)
    {
        perror("listen");
        exit(1);
    }

    for (i = 0; i < NUM_CLIENT; i++)
    {
        d[i].id_ = i;
        d[i].fd_listener = fd_listener;
        d[i].fd_client = fd_client;
        
        pthread_create(&t[i], NULL, chat_thread, &d[i]);
    }

    pthread_join(t[0], NULL);
    pthread_join(t[1], NULL);

    close(fd_listener);

    return 0;
}
