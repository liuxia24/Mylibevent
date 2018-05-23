#ifndef MY_EVENTS_H
#define MY_EVENTS_H
#include <stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/epoll.h>
#include<sys/types.h>
#include<strings.h>
#include<errno.h>
#include<fcntl.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<time.h>
#include<ctype.h>

#define BUFLEN 4096
#define MAXEVENTS 1024
#define MAXLISTEN 20
#define IP "127.0.0.1"
#define SOCK_ERROR(args) do{ \
    perror(args);\
    exit(1);\
}while(0)

//函数指针类型:约定好返回类型和参数个数类型  将来按照该格式写接口函数即可
typedef  void (*callback)(int fd,int events,void *arg); //func pointer type

//自定义结构体  用来让红黑树结点带有更多的 fd信息
typedef struct myevents{
    int fd;
    int events;
    void *arg;
    int status ;
    int len;
    char buf[BUFLEN];
    void (*call_back)(int fd,int events,void *arg);
    long loginTree_time;
}myevents;

//全局变量 该myevents数组 生命周期应一直存在
int g_efd=0;
myevents Array_myevents[MAXEVENTS+1]={0};


void Recvfunc(int fd,int events,void *arg);
void Sendfunc(int fd,int events,void *arg);

//功能:初始化myevents结构体成员  
void eventset(int fd,callback m_callback,myevents *ev,void *arg){
    ev->fd=fd;
    ev->arg=arg;
    ev->events=0;
    ev->call_back=m_callback;
    ev->status=0;                       //status ==0  No in tree  /status ==1 In the tree
    ev->loginTree_time=time(NULL);  //login in the tree  time
}


//功能:往红黑树上添加fd结点并且设置好对应的 epoll_event所维护的信息
void eventadd(int epfd,int events,myevents *ev){
    struct epoll_event evn;
    int op;
    if(ev->status==1){                              //if  in tree
        op=EPOLL_CTL_MOD;
    }else{
        op=EPOLL_CTL_ADD;
        ev->status=1;               //set add for tree
    }

//bind this  ev->fd  and eopll_event list.
    evn.events=ev->events=events;
    evn.data.ptr=(void *)ev;

    epoll_ctl(epfd,op,ev->fd,&evn);
}


//功能:回调函数，当监听套接字lfd有客户端请求发送过来时触发  
void aceptfunc(int fd,int events,void *arg){
    int cfd,i;
    struct sockaddr_in clientaddr;
    socklen_t clientlen=sizeof(clientaddr);
    if((cfd=accept(fd,(struct sockaddr *)&clientaddr,&clientlen))<0)
    {
        if(errno!=EINTR||errno!=EAGAIN)
        {
            printf("accept  return EINTR AND EAGAIN\n");
        }
        perror("accept");
        exit(1);
    }

    do{
                for(i=0;i<MAXEVENTS;i++){
                        if(Array_myevents[i].fd==0)         //查找myevents剩余的表位(空表)来保存结点信息
                            break;
                }
                if(i==MAXEVENTS){						//如果此时等于1024个了 则不可在监听新的客户端了
                       printf("this server  Full\n");
                       exit(1);
                }
				
				//设置套接字为非阻塞IO 模式  
				fcntl(cfd,F_SETFL,O_NONBLOCK);  
				

                eventset(cfd,Recvfunc,&Array_myevents[i],&Array_myevents[i]);
                eventadd(g_efd,EPOLLIN,&Array_myevents[i]);

    }while(0);

    printf("Client IP:%s ,Client port:%d ",inet_ntoa(clientaddr.sin_addr),ntohs(clientaddr.sin_port));
    fflush(stdout);
    return;
}



//功能:初始化一个监听套接字，并且将其加入到红黑树中去。监听其读事件   参数:1.红黑树的句柄 2.端口号
void initSock_listenfd(int epfd,int  port)
{
       int lfd=0;
       int ret;
        struct sockaddr_in serveraddr;
        if((lfd=socket(AF_INET,SOCK_STREAM,0))<0)
        {
            SOCK_ERROR("socket");
            return ;
        }

        //port reuse   .--> NO  WAIT  (2MSL TIME )
        int opt=1;
        setsockopt(lfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

        //first  set lfd  is NO BLOCK
       int flags=fcntl(lfd,F_GETFL,0);// current  lfd   flags
       flags=flags|O_NONBLOCK;
       fcntl(lfd,F_SETFL,flags);

       //Set lfd  port and ip
       bzero(&serveraddr,sizeof(serveraddr));
       serveraddr.sin_family=AF_INET;
       serveraddr.sin_port=htons(port);
       inet_aton(IP,&serveraddr.sin_addr);

      if((ret=bind(lfd,(struct sockaddr *)&serveraddr,sizeof(serveraddr)))<0)
      {
            SOCK_ERROR("bind");
      }
      listen(lfd,MAXLISTEN);

       eventset(lfd,aceptfunc,&Array_myevents[MAXEVENTS],&Array_myevents[MAXEVENTS]);
       //set  Array_myevents[MAXEVENTS] lfdlist  attrituble

       eventadd(epfd,EPOLLIN,&Array_myevents[MAXEVENTS]);   // add  lfd  for redblack tree .

       return ;

}


void eventdel(int efd,myevents *ev){

    struct epoll_event evn={0,{0}};  // clear evn 

    if(ev->status!=1)    //if no in tree
        return;
    evn.data.ptr=ev;
    ev->status=0;

    epoll_ctl(efd,EPOLL_CTL_DEL,ev->fd,&evn);       //delete this node

    return ;
}

//CLIENT  :SEND DATA
void Sendfunc(int fd,int events,void *arg){
    struct myevents *ev=(struct myevents *)arg;
    int len;

    len=write(fd,ev->buf,ev->len);

    if(len>0){
        eventdel(g_efd,ev);
        eventset(ev->fd,Recvfunc,ev,ev);
        eventadd(g_efd,EPOLLIN,ev);
    }else{
        close(ev->fd);
        eventdel(g_efd,ev);
        perror("SEND DATA:");
    }
}

//CLIENT  :RECV DATA
void Recvfunc(int fd,int events,void *arg){
        struct myevents *ev=(struct myevents *)arg;
        int len,i;

        len=recv(fd,ev->buf,sizeof(ev->buf),0);//read  socket  readbuf  data copy to  ev->buf[]

        eventdel(g_efd,ev);        //del   in the tree
        if(len>0){
            ev->len=len;
            for(i=0;i<len;i++)
                ev->buf[i]=toupper(ev->buf[i]);
            ev->buf[len]='\0';

            eventset(fd,Sendfunc,ev,ev);
            eventadd(g_efd,EPOLLOUT,ev);

            printf("recvdata:%s \n",ev->buf);
        }else if(len==0){
            close(ev->fd);
            printf("Recvfunc  error:Client  Unconnect \n");
        }else{
            close(ev->fd);
            perror("Recvfunc");
        }
        return ;
}

#endif // MY_EVENTS_H
