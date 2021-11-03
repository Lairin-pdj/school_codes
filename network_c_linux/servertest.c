//
// C Network Programing
// 1 process select server
// composed by Park Dongjun
//

#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "arpa/inet.h"
#include "unistd.h"

#define MAXLINE 4096
#define bzero(ptr, n)	memset(ptr, 0, n)
#define SA	struct sockaddr
#define LISTENQ 1024
#define SERV_PORT 9877

int main (int argc, int **argv)
{
	int 			i, maxi, maxfd, listenfd, connfd,sockfd;
	int			nready, client[FD_SETSIZE];
	ssize_t			n;
	fd_set			rset, allset;
	char			buf[MAXLINE];
	socklen_t		clilen;
	struct sockaddr_in	cliaddr, servaddr;
	
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);

	bind(listenfd, (SA *) &servaddr, sizeof(servaddr));
	
	listen(listenfd, LISTENQ);
	maxfd = listenfd;
	maxi = -1;
	for(i = 0; i < FD_SETSIZE; i++){
		client[i] = -1;
	}
	FD_ZERO(&allset);
	FD_SET(listenfd, &allset);

	for( ; ; ){
		rset = allset;
		nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
		
		if(FD_ISSET(listenfd, &rset)){
			clilen = sizeof(cliaddr);
			connfd = accept(listenfd, (SA *) &cliaddr, &clilen);
			printf("someone connect\n");
			
			for(i = 0; i < FD_SETSIZE; i++){
				if(client[i] < 0) {
					client[i] = connfd;
					break;
				}
			}
			if(i == FD_SETSIZE){
				printf("too many clients");
				exit(0);
			}
			FD_SET(connfd , &allset);
			if(connfd > maxfd){
				maxfd = connfd;
			}
			if(i > maxi){
				maxi = i;
			}
			if(--nready <= 0){
				continue;
			}
		}
		for(i = 0; i <= maxi; i++){
			if((sockfd = client[i]) < 0){
				continue;
			}
			if(FD_ISSET(sockfd, &rset)){
				if((n = read(sockfd, buf, MAXLINE)) == 0){
					close(sockfd);
					FD_CLR(sockfd, &allset);
					client[i] = -1;
					printf("close\n");
				}
				else{
					write(sockfd, buf, n);
					printf("%d : write\n", i);
				}
				
				if(--nready <= 0){
					break;
				}
			}
		}
	}
}
