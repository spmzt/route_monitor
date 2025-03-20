#ifndef _LIBRTSOCK_MSG_H_
#define _LIBRTSOCK_MSG_H_

int rt_sock_open(int *sock, int *bufsize);
int rt_msg_read(char *buf, size_t msg_len);

#endif // _LIBRTSOCK_MSG_H_
