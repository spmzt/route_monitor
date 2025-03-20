#include <arpa/inet.h>
#include <sys/malloc.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <net/route.h>
#include <err.h>
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "msg.h"

static const char *const msgtypes[] = {
	"",
	"RTM_ADD: Add Route",
	"RTM_DELETE: Delete Route",
	"RTM_CHANGE: Change Metrics or flags",
	"RTM_GET: Report Metrics",
	"RTM_LOSING: Kernel Suspects Partitioning",
	"RTM_REDIRECT: Told to use different route",
	"RTM_MISS: Lookup failed on this address",
	"RTM_LOCK: fix specified metrics",
	"RTM_OLDADD: caused by SIOCADDRT",
	"RTM_OLDDEL: caused by SIOCDELRT",
	"RTM_RESOLVE: Route created by cloning",
	"RTM_NEWADDR: address being added to iface",
	"RTM_DELADDR: address being removed from iface",
	"RTM_IFINFO: iface status change",
	"RTM_NEWMADDR: new multicast group membership on iface",
	"RTM_DELMADDR: multicast group membership removed from iface",
	"RTM_IFANNOUNCE: interface arrival/departure",
	"RTM_IEEE80211: IEEE 802.11 wireless event",
	"RTM_IPFWLOG: IPFW log",
};

int
rt_sock_open(int *sock, int *bufsize)
{
	int opt = 0;
	socklen_t optlen;

	*sock = socket(PF_ROUTE,
			SOCK_RAW | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
	if (*sock < 0) {
		warn("Can't open the PF_ROUTE socket: %s\n",
				strerror(*sock));
		close(*sock);
		return -1;
	}
	
	if (setsockopt(*sock, SOL_SOCKET, SO_USELOOPBACK,
				&opt, sizeof(opt)) < 0)
		warn("Can't set the SO_USELOOPBACK: %s\n",
				strerror(errno));

	optlen = sizeof(*bufsize);
	if (getsockopt(*sock, SOL_SOCKET, SO_RCVBUF,
				bufsize, &optlen) < 0)
		warn("Can't get the SO_RCVBUF: %s\n",
				strerror(errno));

	return 0;
}

int
rt_msg_read(char *buf, size_t msg_len)
{
	struct rt_msghdr *rtm;

	for (size_t offset = 0; offset < msg_len; ) {
		if (offset + sizeof(struct rt_msghdr) > msg_len) {
			warnx("Received incomplete rt_msghdr\n");
			offset += msg_len;
			break;
		}

		rtm = (struct rt_msghdr *)(buf + offset);
		size_t rtm_len = rtm->rtm_msglen;

		if (offset + rtm_len > msg_len) {
			warnx("Received incomplete routing message."
					"expected %zu, got %zu\n", rtm_len, msg_len - offset);
			offset += rtm_len;
			break;
		}

		printf("Message Type: %s\n", msgtypes[rtm->rtm_type]);
		if ((rtm->rtm_type != RTM_ADD)
			&& (rtm->rtm_type != RTM_DELETE)
			&& (rtm->rtm_type != RTM_CHANGE)
			&& (rtm->rtm_type != RTM_IFINFO)) {
			printf("Skip on %s.\n\n", msgtypes[rtm->rtm_type]);
			offset += rtm_len;
			break;
		}
		
		if (rtm->rtm_type == RTM_IFINFO) {
			struct if_msghdr *ifm;
			ifm = (struct if_msghdr *)(buf + offset);
			printf("Interface Index: %d\n", ifm->ifm_index);
			offset += rtm_len;
			break;
		}


		if (rtm->rtm_errno) {
			warnc(rtm->rtm_errno, "error on rt_msghdr: ");
			offset += rtm_len;
			break;
		}

		printf("Interface Index: %d\n", rtm->rtm_index);

		if (!(rtm->rtm_addrs & (RTA_IFA | RTA_DST))) {
			offset += rtm_len;
			printf("\n");
			break;
		}

		char *ptr = buf + sizeof(struct rt_msghdr);
		char addr_str[INET6_ADDRSTRLEN];

		for (size_t p = sizeof(struct rt_msghdr); p <= msg_len; ) {
			struct sockaddr *sa = (struct sockaddr *)ptr;
			if (!(sa->sa_len))
				break;
		
			if (sa->sa_family == AF_INET6) {
				struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)sa;
				if (inet_ntop(AF_INET6, &addr6->sin6_addr,
							addr_str, sizeof(addr_str)) == NULL)
					warnc(errno, "error on inet_ntop: ");
				else
					printf("interface address: %s\n", addr_str);
			}

			ptr += sa->sa_len;
			p += sa->sa_len;
		}

		printf("\n");
		offset += rtm_len;
	}

	return 0;
}
