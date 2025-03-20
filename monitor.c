#include <arpa/inet.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <net/route.h>
#include <err.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "msg.h"

#define MAX_MESSAGES 10
#define BUFSIZE 2048

int
main()
{
	int rtsock, n, ret, bufsize;
	struct msghdr msg;
	struct iovec iov[1];
	char buf[2048];
	struct pollfd pfd[1];

	if (rt_sock_open(&rtsock, &bufsize) < 0)
		err(-1, "rt_sock_open");

	memset(pfd, 0, sizeof(struct pollfd));
	pfd->fd = rtsock;
	pfd->events = POLLIN;
	pfd->revents = 0;
	
	while(1) {
		ret = poll(pfd, 1, INFTIM);
		if (ret < 0) {
			warnc(ret, "poll error: ");
			break;
		}
		
		if (pfd->revents & POLLIN) {
			memset(&msg, 0, sizeof(msg));
			iov[0].iov_base = buf;
			iov[0].iov_len = sizeof(buf);
			msg.msg_iov = iov;
			msg.msg_iovlen = 1;

			n = recvmsg(rtsock, &msg, 0);
			if (n < 0) {
				warn("recvmsg error: ");
				close(rtsock);
			}
			rt_msg_read(buf, n);
		}
	}

	close(rtsock);
	return 0;
}
