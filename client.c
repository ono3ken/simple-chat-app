/*
 * client.c
 *    ソケットを使用して、サーバーに接続するクライアントプログラム。
 *    
 *    入力された文字列をサーバーに送り、サーバーが大文字に変換したデータを
 *    受け取る。
 * 
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>

#define HOST "localhost"
#define PORT 8500

int flag_end = 0;

struct data {
    int fd;
};

void *receiver(void *arg)
{
    int fd = ((struct data *)arg)->fd;
    char buf[1024];

    while (!flag_end) {
        if (read(fd, buf, 1024) > 0)
        {
        printf("%s", buf);
        }
        sleep(0.5);
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    struct sockaddr_in addr;
    struct hostent *hp;
    int fd;
    char buf[1024];
    pthread_t t_rcv;
    struct data d;

    /*
     *  ストリーム型ソケット作る．
     */
    if ((fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        exit(1);
    }

    /* 
     * addrの中身を0にしておかないと、bind()でエラーが起こることがある
     */
    bzero((char *)&addr, sizeof(addr));

    /*
     * ソケットの名前を入れておく
     */

    if ((hp = gethostbyname(HOST)) == NULL)
    {
        perror("No such host");
        exit(1);
    }
    bcopy(hp->h_addr, &addr.sin_addr, hp->h_length);
    addr.sin_family = PF_INET;
    addr.sin_port = htons(PORT);

    /*
     *  サーバーとの接続を試みる。これが成功するためには、サーバーがすでに
     *  このアドレスをbind()して、listen()を発行していなければならない。
     */
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("connect");
        exit(1);
    }

    d.fd = fd;
    pthread_create(&t_rcv, NULL, receiver, &d);

    /*
     *  入力されたデータをソケットに書き込んでサーバーに送り、
     *  サーバーが変換して送ってきたデータを読み込む。
     */
    do
    {
        fgets(buf, 1024, stdin);
        if (buf[0] != '\n')
        {
            write(fd, buf, 1024);
        }

    } while (strcmp(buf, "QUIT\n") != 0);

    flag_end = 1;    
    pthread_join(t_rcv, NULL);
    close(fd);
    exit(0);
}
