//
// C Network Programing
// 1 process daytime server
// composed by Park Dongjun
//

#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#define MAXLINE 4096
#define bzero(ptr, n)	memset(ptr, 0, n)
#define SA	struct sockaddr
#define LISTENQ 1024	


int main (int argc, int **argv)
{
	int listenfd, connfd;
	struct sockaddr_in servaddr;
	char buff[MAXLINE];
	time_t ticks;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(13);

	bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

	listen(listenfd, LISTENQ);

	for( ; ; ){
		printf("server waiting\n");

		connfd = accept(listenfd, (SA *) NULL, NULL);

		printf("accept success\n\n");

		ticks = time(NULL);
		snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));
		write(connfd, buff, strlen(buff));

		close(connfd);
	}
}
