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
// 最大クライアント数
#define MAX_CLIENT 100

// クライアント処理スレッドに渡すデータ
struct data
{
    int id_;
    int *fd_client;
};

// クライアントごとに割り当てられるスレッド．
// クライアントからの入力を受け取り，
// ユーザ名を付加したものを他のクライアント全員に転送．
void *chat_thread(void *arg)
{
    int id_ = ((struct data *)arg)->id_;
    int *fd_client = ((struct data *)arg)->fd_client;

    int i;
    char buf[1024];
    char ret_str[1024];
    int fd;
    char username[1024];

    fd = fd_client[id_];

    // ユーザ名を取得．
    read(fd, username, 1024);

    do
    {
        // メッセージを取得
        read(fd, buf, 1024);

        // ユーザ名を付加．
        sprintf(ret_str, "%s :%s", username, buf);

        /* 変換したデータを自分以外のクライアントに送り返す */
        for (i = 0; i < MAX_CLIENT; i++)
        {
            if ((i != id_) && (fd_client[i] > 0))
            {
                write(fd_client[i], ret_str, 1024);
            }
        }

    } while (strcmp(buf, "QUIT\n") != 0);

    /* 通信が終わったらソケットを閉じる */
    close(fd);
    fd_client[id_] = -1;

    return NULL;
}

int main()
{
    int i;
    int fd_listener;
    struct sockaddr_in saddr;
    struct data d[MAX_CLIENT];
    int fd_client[MAX_CLIENT];
    struct sockaddr_in caddr[MAX_CLIENT];
    int len;
    pthread_t t[MAX_CLIENT];

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

    // MAX_CLIENTの数だけ接続を待ち続ける．
    for (i = 0; i < MAX_CLIENT; i++)
    {
        len = sizeof(caddr[i]);
        fd_client[i] = accept(fd_listener, (struct sockaddr *)&caddr[i], &len);
        if (fd_client[i] < 0)
        {
            perror("accept");
            exit(1);
        }

        // 接続完了後，クライアントに紐づけられた処理用のスレッドを実行．
        d[i].id_ = i;
        d[i].fd_client = fd_client;
        d[i].fd_client[i] = fd_client[i];
        pthread_create(&t[i], NULL, chat_thread, &d[i]);
    }

    

    close(fd_listener);

    return 0;
}
