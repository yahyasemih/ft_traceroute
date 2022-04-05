#ifndef FT_TRACEROUTE_H
#define FT_TRACEROUTE_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#include "utilities.h"

#define FLAG_DEBUG	0x1
#define FLAG_NUM	0x2

typedef struct s_traceroute_context {
	uint8_t nqueries;
	uint8_t max_ttl; // max_hops
	uint8_t first_ttl;
	uint8_t flags;
	double sendwait;
	int wait; // timeout
	int socket_fd;
	char src[INET_ADDRSTRLEN];
	char *host;
	char ip[INET_ADDRSTRLEN];
} t_traceroute_context;

void init_context(t_traceroute_context *ctx);

#endif
