//基于libevent事件驱动的服务器
#include<event.h> 
#include <sys/socket.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define _MAX_CLIENT_ 1024

struct event* readev = NULL;  //造成内存泄漏

typedef struct Fd_EventMap{
    int fd; //文件描述符
    struct event *ev;   //对应事件
}fdEvent;

fdEvent mfdEvent[_MAX_CLIENT_];

void initEventArray()
{
    int i;
    for(i=0;i<_MAX_CLIENT_;i++)
    {
        mfdEvent[i].fd = -1;
        mfdEvent[i].ev = NULL;
    }
}

int addEvent(int fd,struct event* ev)
{
    int i;
    for(i=0;i<_MAX_CLIENT_;++i)
    {
        if(mfdEvent[i].fd <0)
        {
            break;
        }
    }
    if(i == _MAX_CLIENT_)
    {
        printf("too many client...\n");
        return -1;
    }
    mfdEvent[i].fd = fd;
    mfdEvent[i].ev = ev;
    return 0;
}

void destroyEventArray()
{
    int i;
    for(i=0;i<_MAX_CLIENT_;i++)
    {
        if(mfdEvent[i].fd > 0&&mfdEvent[i].ev)
        {
            close(mfdEvent[i].fd);
            mfdEvent[i].fd= -1;
            event_free(mfdEvent[i].ev);
        }
    }
}

struct event* getEventByFd(int fd)
{
    int i;
    for(i = 0;i<_MAX_CLIENT_;++i)
    {
        if(mfdEvent[i].fd == fd)
            return mfdEvent[i].ev;
    }
}

void readcb(evutil_socket_t fd,short events,void* arg)
{
    char buf[256] = {0};
    int ret = recv(fd,buf,sizeof(buf),0);
    if(ret <=0 )
    {
        close(fd);
        event_del(getEventByFd(fd));
    }
    else
    {
        int i = 0;
        for(i=0;i<strlen(buf);i++)
            buf[i]=toupper(buf[i]);
        send(fd,buf,ret,0);
    }
}
void conncb(evutil_socket_t fd,short events,void* arg)
{
    struct event_base *base = (struct event_base*)arg;
    struct sockaddr_in client;
    socklen_t len=sizeof(client);
    int cfd = accept(fd,(struct sockaddr*)&client,&len);
    if(cfd > 0)
    {
        //应该继续监听
        readev = event_new(base,cfd,EV_READ|EV_PERSIST,readcb,base);
        event_add(readev,NULL);
        addEvent(cfd,readev);
    }
}

int main()
{
    //根节点
    struct event_base *base = event_base_new();
    //创建套接字
    int lfd = socket(AF_INET,SOCK_STREAM,0);
    //绑定
    struct sockaddr_in serv;
    serv.sin_family = AF_INET;
    serv.sin_port = htons(8888);
    serv.sin_addr.s_addr = htonl(INADDR_ANY);
    int opt = 10;
    setsockopt(lfd,SOL_SOCKET,SO_REUSEADDR,(void*)&opt,sizeof(opt));
    if(bind(lfd,(struct sockaddr*)&serv,sizeof(serv)) < 0)
    {
        perror("Bind");
        exit(-1);
    }

    //初始化事件数组
    initEventArray();
    //监听
    listen(lfd,128);
    //创建事件-设置回掉
    struct event *connev = event_new(base,lfd,EV_READ|EV_PERSIST,conncb,base);
    //监听事件 -- event_add
    event_add(connev,NULL);
    //循环等待
    event_base_dispatch(base);

    //释放根结点
    event_free(connev);
    //event_free(readev);
    destroyEventArray();
    event_base_free(base);


    return 0;
}

