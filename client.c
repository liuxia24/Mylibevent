#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<strings.h>
#include<errno.h>


#define BUFSIZE 1024
#define PORT 9999
#define IP "127.0.0.1"

char Recvbuf[BUFSIZE]={0};
char Sendbuf[BUFSIZE]={0};
int main(){
		
	int cfd;
	struct sockaddr_in serveraddr;
	
	bzero(&serveraddr,sizeof(serveraddr));
	serveraddr.sin_port=htons(PORT);
	serveraddr.sin_family=AF_INET;
	inet_pton(AF_INET,IP,&serveraddr.sin_addr.s_addr);
	
	cfd=socket(AF_INET,SOCK_STREAM,0);
	if(cfd==-1)
	{
		perror("socket");
		exit(1);
	}
	if(connect(cfd,(struct sockaddr *)&serveraddr,sizeof(serveraddr))==-1)
	{
		perror("connect");
		exit(1);
	}
	
	int ret;
	while((fgets(Sendbuf,BUFSIZE,stdin))!=0){

		write(cfd,Sendbuf,BUFSIZE);

		ret=read(cfd,Recvbuf,BUFSIZE);
		if(ret==0){
			close(cfd);
		}

		write(STDOUT_FILENO,Recvbuf,ret);
		
	}
	


	return 0;
}
