//
// C Network Programing
// shutdown select client
// composed by Park Dongjun
//

#include "stdio.h"
#include "stdlib.h"
#include "sys/socket.h"
#include "string.h"
#include "netinet/in.h"
#include "arpa/inet.h"
#include "unistd.h"
#include "errno.h"

#define MAXLINE 	4096
#define bzero(ptr, n)	memset(ptr, 0, n)
#define SA		struct sockaddr
#define SERV_PORT 	9877
#define max(a, b)	((a) > (b) ? (a) : (b))

ssize_t Writen(int fd, const void *vptr, size_t n)
{
	size_t 		nleft;
	ssize_t		nwritten;
	const char	*ptr;

	ptr = vptr;
	nleft = n;
	while(nleft > 0){
		if((nwritten = write(fd, ptr, nleft)) <= 0){
			if(nwritten < 0 && errno == EINTR)
				nwritten = 0;
			else
				return (-1);
		}

		nleft -= nwritten;
		ptr += nwritten;
	}
	return (n);
}

ssize_t Readline(int fd, void *vptr, size_t maxlen)
{
	ssize_t	n, rc;
	char	c, *ptr;

	ptr = vptr;
	for(n = 1; n < maxlen; n++){
again:
		if((rc = read(fd, &c, 1)) == 1){
			*ptr++ = c;
			if(c == '\n')
				break;
		}
		else if(rc == 0){
			*ptr = 0;
			return(n - 1);
		}
		else {
			if(errno == EINTR)
				goto again;
			return (-1);
		}
	}
	*ptr = 0;
	return(n);
}

void str_cli(FILE *fp, int sockfd)
{
	int	maxfdp1, stdineof;
	fd_set	rset;
	char	buf[MAXLINE];
	int	n;

	stdineof = 0;
	FD_ZERO(&rset);
	for( ; ; ){
		if(stdineof == 0){
			FD_SET(fileno(fp), &rset);
		}
		FD_SET(sockfd, &rset);
		maxfdp1 = max(fileno(fp), sockfd) + 1;
		select(maxfdp1, &rset, NULL, NULL, NULL);
		
		if(FD_ISSET(sockfd, &rset)){
			if((n = read(sockfd, buf, MAXLINE)) == 0){
				if(stdineof == 1){
					return;
				}
				else{
					printf("terminated\n");
					exit(1);
				}
			}
			Writen(fileno(stdout), buf, n);
		}
		if(FD_ISSET(fileno(fp), &rset)){
			if((fscanf(fp, "%s", buf)) == EOF){
				stdineof = 1;
				shutdown(sockfd, SHUT_WR);
				FD_CLR(fileno(fp), &rset);
				continue;
			}
			printf("send %s\n", buf);
			n = strlen(buf);
			buf[n] = '\n';
			Writen(sockfd, buf, n + 1);
		}
	}
}		

int main (int argc, char **argv)
{
	int			sockfd;
	struct sockaddr_in	servaddr;

	FILE *fp = fopen("Text.txt", "r");
	if(fp == NULL){
		printf("can't open it\n");
		exit(1);
	}
	
	if(argc != 2){
		printf("put the ip\n");
		exit(1);
	}
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

	connect(sockfd, (SA *) &servaddr, sizeof(servaddr));
	printf("connect\n");
	
	str_cli(fp, sockfd);
	
	exit(0);
}
