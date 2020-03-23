#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>

/* 原理见服务器源程序 */
#define ERR_EXIT(m)                             \
    do{                                         \
       perror(m);                               \
       exit(1);                                 \
       }while(0)

typedef struct{
    struct in_addr ip;
    int port;
}clientInfo;

void print_buf(const char *buf, int len)
{
    int i;
    printf("#buf len: %d bytes,",len);
    for (i = 0; i < len; i++) {
        printf("%02x", buf[i]);
        if (i != len-1)
            printf(",");
    }
}
/* 用于udp打洞成功后两个客户端跨服务器通信 */
void echo_ser(int sockfd, struct sockaddr* addr, socklen_t *len)
{   
    char buf[1024];
    while(1)
    {
        int len1;
        bzero(buf, sizeof(buf));
        printf(">> ");
        fflush(stdout);
        fflush(stdin);
        fgets(buf, sizeof(buf)-1, stdin);

        //向A发送数据
        sendto(sockfd, buf, strlen(buf), 0, addr, sizeof(struct sockaddr_in));
        printf("send ok,");
        print_buf(buf,strlen(buf));
        printf("\n");
        //sleep(3);
        
    }
}

void *recv_proc(void *param)
{
    char buf[1024];
    struct sockaddr_in addr;
    socklen_t len;
    int sockfd = *((int *)(param));
    while(1)
    {
        int len1;
        
        //接收A发来的数据
        bzero(buf, sizeof(buf));
        //printf("start recv A data...\n");
        len1 = recvfrom(sockfd, buf, sizeof(buf)-1, 0, (struct sockadd *)&addr, &len);
        if (len1 > 0) {
            buf[len1] = '\0';
            printf("recv %d bytes,from[%s:%d]:%s\n", len1,inet_ntoa(addr.sin_addr),ntohs(addr.sin_port), buf);
        
            if(strcmp(buf, "exit") == 0)
                break;
        }
    }
}
int main()
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd == -1)
        ERR_EXIT("SOCKET");
    //向服务器发送心跳包的一个字节的数据
    char ch = 'a';
    /* char str[] = "abcdefgh"; */
    clientInfo info;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    bzero(&info, sizeof(info));
    struct sockaddr_in clientaddr, serveraddr;
    /* 客户端自身的ip+port */
    /* memset(&clientaddr, 0, sizeof(clientaddr)); */
    /* clientaddr.sin_port = htons(8888); */
    /* clientaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); */   
    /* clientaddr.sin_family = AF_INET; */

    /* 服务器的信息 */
    memset(&clientaddr, 0, sizeof(clientaddr));
    //实际情况下为一个已知的外网的服务器port
    serveraddr.sin_port = htons(8888);
    //实际情况下为一个已知的外网的服务器ip,这里仅用本地ip填充，下面这行的ip自己换成已知的外网服务器的ip
    serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");   
    /* clientaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); */   
    serveraddr.sin_family = AF_INET;

    /* 向服务器S发送数据包 */
    sendto(sockfd, &ch, sizeof(ch), 0, (struct sockaddr *)&serveraddr, sizeof(struct sockaddr_in));
    /* sendto(sockfd, str, sizeof(str), 0, (struct sockaddr *)&serveraddr, sizeof(struct sockaddr_in)); */
    /* 接收B的ip+port */
    printf("send success\n");
    recvfrom(sockfd, &info, sizeof(clientInfo), 0, (struct sockaddr *)&serveraddr, &addrlen);
    printf("IP: %s\tPort: %d\n", inet_ntoa(info.ip), ntohs(info.port));

    serveraddr.sin_addr = info.ip;
    serveraddr.sin_port = info.port;

    sendto(sockfd, &ch, sizeof(ch), 0, (struct sockaddr *)&serveraddr, sizeof(struct sockaddr_in));
    {
        pthread_t tid;
        pthread_create(&tid, NULL, recv_proc, (void*)&sockfd);
    }

    echo_ser(sockfd, (struct sockaddr *)&serveraddr, &addrlen);
    close(sockfd);
    return 0;
}
