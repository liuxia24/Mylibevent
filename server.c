#include"my_events.h"

#define PORT 9999


int main(int argc, char *argv[])
{
    int readN,i;
    unsigned short   port=PORT;
    struct epoll_event Array_epev[MAXEVENTS+1];

    if(argc==2)
        port=atoi(argv[1]);//cast  to  int  return

    if((g_efd=epoll_create(MAXEVENTS))==-1)
    {
        SOCK_ERROR("epoll_create");
    }

    initSock_listenfd(g_efd,port);

    while(1){
          readN=epoll_wait(g_efd,Array_epev,MAXEVENTS+1,1000);
          if(-1==readN){
              SOCK_ERROR("epoll wait");
          }

          for(i=0;i<readN;i++){
                 myevents *p=(myevents *)Array_epev[i].data.ptr;
                if((Array_epev[i].events&EPOLLIN)&&(p->events&EPOLLIN))
                {
                       p->call_back(p->fd,p->events,p->arg);  //call back  function
                }
               if((Array_epev[i].events&EPOLLOUT)&&(p->events&EPOLLOUT))
               {
                   p->call_back(p->fd,p->events,p->arg);  //call back  function
               }
          }
    }

    return 0;
}
