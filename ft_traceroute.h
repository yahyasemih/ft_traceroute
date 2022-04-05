#ifndef FT_TRACEROUTE_H
#define FT_TRACEROUTE_H

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "utilities.h"

#define FLAG_DEBUG	0x1
#define FLAG_NUM	0x2

typedef struct s_traceroute_context {
	int8_t nqueries;
	int8_t max_ttl; // max_hops
	int8_t first_ttl;
	uint8_t flags;
	double sendwait;
	int wait; // timeout
	int socket_fd;
	char src[INET_ADDRSTRLEN];
} t_traceroute_context;

void init_context(t_traceroute_context *ctx);

#endif
