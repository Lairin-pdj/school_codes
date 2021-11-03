//
// C Network Programing
// daytime client
// composed by Park Dongjun
//

#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/types.h>

#define MAXLINE 4096
#define bzero(ptr, n)	memset(ptr, 0, n)
#define SA	struct sockaddr


#include <stdarg.h>
#include <syslog.h>

int daemon_proc;

static void err_doit(int errnoflag, int level, const char *fmt, va_list ap)
{
	int errno_save, n;
	char buf[MAXLINE + 1];

	errno_save = errno;
#ifdef HAVE_VSNPRINTF
	vsnprintf(buf, MAXLINE, fmt, ap);
#else
	vsprintf(buf, fmt, ap);
#endif
	n = strlen(buf);
	if(errnoflag)
		snprintf(buf + n, MAXLINE - n, ": %s", strerror(errno_save));
	strcat(buf, "\n");

	if(daemon_proc){
		syslog(level,"%s", buf);
	}
	else{
		fflush(stdout);
		fputs(buf, stderr);
		fflush(stderr);
	}
	return;
}

void err_sys(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	err_doit(1, LOG_ERR, fmt, ap);
	va_end(ap);

	exit(1);
}

void err_quit(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	err_doit(0, LOG_ERR, fmt, ap);
	va_end(ap);

	exit(1);
}

int main (int argc, char **argv)
{
	int sockfd, n;
	char recvline[MAXLINE + 1];
	struct sockaddr_in servaddr;

	if(argc != 2)
		err_quit("usage: a.out <IPaddress>");

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		err_sys("socket error");

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(13);
	if(inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
		err_quit("inet_pton error for %s", argv[1]);

	if(connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0)
		err_sys("connect error");

	printf("connect success\n");

	while((n = read(sockfd, recvline, MAXLINE)) > 0){
		recvline[n] = 0;
		if(fputs(recvline, stdout) == EOF)
			err_sys("fputs error");
	}
	if(n < 0)
		err_sys("read error");

	exit(0);
}
