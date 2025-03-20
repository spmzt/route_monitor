PROG = monitor
SRCS = msg.c monitor.c
MAN  =

CFLAGS += -ggdb -O0 -Wall -Wextra

.include <bsd.prog.mk>
