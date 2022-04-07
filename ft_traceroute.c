#include "ft_traceroute.h"

int init_socket(t_traceroute_context *ctx) {
	int fd;
	int options = 1;
	int type = SOCK_RAW;
	struct timeval timeout;
	timeout.tv_sec = ctx->wait;
	timeout.tv_usec = 1;

	if (ctx->flags | FLAG_DEBUG) {
		type |= SO_DEBUG;
	}
	fd = socket(AF_INET, type, IPPROTO_ICMP);
	if (fd < 0) {
		fprintf(stderr, "%s: socket: %s\n", "ft_traceroute", ft_strerror(errno));
		exit(2);
	}
	if(setsockopt(fd, IPPROTO_IP, IP_HDRINCL, &options, sizeof(options)) < 0
			|| setsockopt (fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0
			|| setsockopt (fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        fprintf(stderr, "%s: socket: %s\n", "ft_traceroute", ft_strerror(errno));
		close(fd);
		exit(2);
    }

	return fd;
}

void init_context(t_traceroute_context *ctx) {
	ctx->first_ttl = 1;
	ctx->max_ttl = 30;
	ctx->nqueries = 3;
	ctx->wait = 3;
	ctx->sendwait = 0.0;
	ctx->flags = 0;
	ctx->socket_fd = -1;
	ft_strcpy(ctx->src, "0.0.0.0");
	ctx->host = NULL;
}

void print_options() {
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "  -d\t\t\tEnable socket level debugging\n");
	fprintf(stderr, "  -f first_ttl\t\tStart from the first_ttl hop (instead from 1)\n");
	fprintf(stderr, "  -h\t\t\tprint help and exit\n");
	fprintf(stderr, "  -m max_ttl\t\tSet the max number of hops (max TTL to be reached).\n"
					"\t\t\tDefault is 30\n");
	fprintf(stderr, "  -n\t\t\tDo not resolve IP addresses to their domain names\n");
	fprintf(stderr, "  -w wait_time\t\tWait for a probe no more than wait_time (default 3)\n");
	fprintf(stderr, "  -q nqueries\t\tSet the number of probes per each hop. Default is 3\n");
	fprintf(stderr, "  -s src_addr\t\tUse source src_addr for outgoing packets\n");
	fprintf(stderr, "  -z sendwait\t\tMinimal time interval between probes (default 0).\n"
					"\t\t\tIf the value is more than 10, then it specifies a\n"
					"\t\t\tnumber in milliseconds, else it is a number of\n"
					"\t\t\tseconds (float point values allowed too)\n");
	fprintf(stderr, "\nArguments:\n");
	fprintf(stderr, "+\thost\t\tThe host to traceroute to\n");
	// fprintf(stderr, "\tpacketlen\tThe full packet length (default is the length of an IP\n"
	// 				"\t\t\theader plus 40). Can be ignored or increased to a minimal\n"
	// 				"\t\t\tallowed value\n");
}

void print_usage(const char *executable) {
	fprintf(stderr, "Usage\n  sudo %s [options] host\n", executable);
	print_options();
	exit(0);
}

int host_to_ip(const char *host, char *dest) {
	struct addrinfo *addr;
	struct addrinfo hints;

	ft_memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	int ret = getaddrinfo(host, NULL, &hints, &addr);
	if (ret) {
		fprintf(stderr, "%s: %s\n", host, ft_gai_strerror(ret));
		return 1;
	}

	ft_strcpy(dest, inet_ntoa(((struct sockaddr_in *)addr->ai_addr)->sin_addr));
	freeaddrinfo(addr);
	return 0;
}

int process_options(char c, t_traceroute_context *ctx, char *argv[], int *x, int *y) {
	int i = *x;
	int j = *y;
	long value = 0;
	char *str = NULL;
	char *end = NULL;

	if (argv[i][j + 1] != '\0') {
		str = argv[i] + j + 1;
	} else {
		if (argv[i + 1] == NULL) {
			fprintf(stderr, "Option `-%c' (argc %d) requires an argument: `-%c %s'\n", c, i, c, "TODO");
			close(ctx->socket_fd);
			exit(2);
		}
		str = argv[i + 1];
		i++;
	}
	if (c == 's') {
		char ip[INET_ADDRSTRLEN];
		if (host_to_ip(str, ip)) {
			fprintf(stderr, "Cannot handle `-s' option with arg `%s' (argc %d)\n", str, i);
			close(ctx->socket_fd);
			exit(2);
		}
		ft_strcpy(ctx->src, ip);
		return 1;
	}
	value = ft_strtol(str, &end);
	if (end != NULL && *end != '\0') {
		fprintf(stderr, "Cannot handle `-%c' option with arg `%s' (argc %d)\n", c, str, i);
		close(ctx->socket_fd);
		exit(2);
	}
	if (c == 'f') {
		if (value < 1 || value > 30) {
			fprintf(stderr, "first hop out of range\n");
			close(ctx->socket_fd);
			exit(2);
		}
		ctx->first_ttl = (int8_t)value;
	} else if (c == 'm') {
		if (value < 0 || value > 255) {
			fprintf(stderr, "max hops cannot be more than 255\n");
			close(ctx->socket_fd);
			exit(2);
		}
		ctx->max_ttl = (int8_t)value;
	} else if (c == 'w') {
		if (value < 0) {
			fprintf(stderr, "bad wait specifications `%ld' used\n", value);
			close(ctx->socket_fd);
			exit(2);
		}
		ctx->wait = (int)value;
	} else if (c == 'q') {
		if (value < 1 || value > 10) {
			fprintf(stderr, "no more than 10 probes per hop\n");
			close(ctx->socket_fd);
			exit(2);
		}
		ctx->nqueries = (int)value;
	} else if (c == 'z') {
		if (value < 0) {
			fprintf(stderr, "bad sendtime `%lf' specified\n", (double)value);
			close(ctx->socket_fd);
			exit(2);
		}
		ctx->sendwait = (double)value;
	}
	*x = i;
	*y = j;
	return 1;
}

void process_arguments(int argc, char *argv[], t_traceroute_context *ctx) {
	int end_of_flags = 0;
	int failed = 0;
	int host_idx = 0;

	for (int i = 1; i < argc; ++i) {
		if (!end_of_flags && argv[i][0] == '-') {
			if (argv[i][1] == '-' && argv[i][2] == '\0') {
				end_of_flags = 1;
			} else if (argv[i][1] == '-' && argv[i][2] != '\0') {
				fprintf(stderr, "Bad option `%s' (argc %d)\n", argv[i], i);
				failed = 1;
			} else {
				for (int j = 1; argv[i][j] != '\0'; ++j) {
					if (search_char("dn", argv[i][j])) {
						ctx->flags |= (argv[i][j] == 'd') ? FLAG_DEBUG : FLAG_NUM;
					} else if (search_char("fmwqsz", argv[i][j])) {
						if (process_options(argv[i][j], ctx, argv, &i, &j)) {
							break;
						}
					} else if (argv[i][j] == 'h') {
						print_usage(argv[0]);
					} else {
						fprintf(stderr, "Bad option `-%c' (argc %d)\n", argv[i][j], i);
						failed = 1;
					}
				}
			}
		} else {
			ctx->host = argv[i];
			host_idx = i;
		}
	}
	if (ctx->host == NULL) {
		fprintf(stderr, "Specify \"host\" missing argument.\n");
		failed = 1;
	}
	if (host_to_ip(ctx->host, ctx->ip)) {
		fprintf(stderr, "Cannot handle \"host\" cmdline arg `%s' on position 1 (argc %d)\n", ctx->host, host_idx);
		failed = 1;
	}
	if (failed) {
		close(ctx->socket_fd);
		exit(2);
	}
}

int send_packet(t_traceroute_context *ctx, char *send_buf, int size, int ttl) {
	int sent;
	struct addrinfo hints;
	struct addrinfo *addr;
	struct sockaddr_in *dst;

	ft_memset(send_buf, 0, size);
	ft_memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	int ret = getaddrinfo(ctx->host, NULL, &hints, &addr);
    if (ret) {
        fprintf(stderr, "%s: %s\n", ctx->host, ft_gai_strerror(ret));
		return 0;
    }
	dst = (struct sockaddr_in *)addr->ai_addr;
	socklen_t dst_addr_len = sizeof(*dst);
	struct ip *ip = (struct ip *)send_buf;
	struct icmp *icmp = (struct icmp *)(ip + 1);

	inet_pton(AF_INET, ctx->src, &ip->ip_src);
	inet_pton(AF_INET, ctx->ip, &ip->ip_dst);
	dst->sin_addr = ip->ip_dst;
	dst->sin_family = AF_INET;
	ip->ip_v = 4;
    ip->ip_hl = 5;
    ip->ip_tos = 0;
	ip->ip_len = htons(size);
    ip->ip_id = htons((uint16_t)getpid());
    ip->ip_off = 0;
    ip->ip_ttl = ttl;
    ip->ip_p = IPPROTO_ICMP;
    ip->ip_sum = ft_checksum((unsigned short *)ip, ip->ip_hl);

	icmp->icmp_type = ICMP_ECHO;
    icmp->icmp_code = 0;
    icmp->icmp_id = htons((uint16_t)getpid());
    icmp->icmp_seq = ttl;
    icmp->icmp_cksum = ft_checksum((unsigned short *)icmp, size - sizeof(struct icmp));

	sent = sendto(ctx->socket_fd, send_buf, size, 0, (struct sockaddr *)dst, dst_addr_len);
	freeaddrinfo(addr);
	return sent >= 0;
}

int receive_packet(t_traceroute_context *ctx, char *recv_buf, int size) {
	int received;
	socklen_t dst_addr_len;
	struct addrinfo hints;
	struct addrinfo *addr;
	struct sockaddr_in *dst;

	ft_memset(recv_buf, 0, size);
	ft_memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	int res = getaddrinfo(ctx->host, NULL, &hints, &addr);
    if (res) {
        fprintf(stderr, "%s: %s\n", ctx->host, ft_gai_strerror(res));
		return 0;
    }
	dst = (struct sockaddr_in *)addr->ai_addr;

	dst_addr_len = sizeof(*dst);
	received = recvfrom(ctx->socket_fd, recv_buf, size, 0, (struct sockaddr *)dst, &dst_addr_len);
	freeaddrinfo(addr);

	return received >= 0;
}

double get_duration(struct timeval begin, struct timeval end) {
	double res = 0.0;
	res += end.tv_sec * 1000.0;
	res += end.tv_usec / 1000.0;
	res -= begin.tv_sec * 1000.0;
	res -= begin.tv_usec / 1000.0;
	return res;
}

void start(t_traceroute_context *ctx) {
	struct timeval begin, end;

	char recv_buf[60];
	char send_buf[60];
	char buff[NI_MAXHOST];
	struct ip *ip = (struct ip *)recv_buf;
	struct icmp *icmp = (struct icmp *)(ip + 1);

	printf("traceroute to %s (%s), 30 hops max, 60 byte packets\n", ctx->host, ctx->ip);
	int is_last = 0;
	for (int ttl = ctx->first_ttl; ttl <= ctx->max_ttl && !is_last; ++ttl) {
		printf("%2d ", ttl);
		struct in_addr last_ip;
		ft_memset(&last_ip, 0, sizeof(last_ip));
		int all_failed = 1;
		for (int i = 0; i < ctx->nqueries; ++i) {
			ft_memset(buff, 0, NI_MAXHOST);
			gettimeofday(&begin, NULL);
			if (send_packet(ctx, send_buf, 60, ttl) && receive_packet(ctx, recv_buf, 60)) {
				is_last = icmp->icmp_code == 0 && (icmp->icmp_type == ICMP_ECHOREPLY || icmp->icmp_type == ICMP_ECHO);
				gettimeofday(&end, NULL);
				if (last_ip.s_addr == ip->ip_src.s_addr && i > 0) {
					printf("  %.3fms", get_duration(begin, end));
				} else {
					if (ctx->flags & FLAG_NUM) {
						printf(" %s  %.3fms", inet_ntoa(ip->ip_src), get_duration(begin, end));
					} else {
						if (dns_resolve(inet_ntoa(ip->ip_src), buff, NI_MAXHOST)) {
							printf(" %s", inet_ntoa(ip->ip_src));
						} else {
							printf(" %s", buff);
						}
						printf(" (%s)  %.3fms", inet_ntoa(ip->ip_src), get_duration(begin, end));
					}
				}
				fflush(stdout);
				if (icmp->icmp_type == ICMP_DEST_UNREACH) {
					if (icmp->icmp_code == ICMP_NET_UNREACH) {
						printf(" !N");
					} else if (icmp->icmp_code == ICMP_HOST_UNREACH) {
						printf(" !H");
					} else if (icmp->icmp_code == ICMP_PROT_UNREACH) {
						printf(" !P");
					} else if (icmp->icmp_code == ICMP_SR_FAILED) {
						printf(" !S");
					} else if (icmp->icmp_code == ICMP_FRAG_NEEDED) {
						printf(" !F");
					} else if (icmp->icmp_code == ICMP_PREC_VIOLATION) {
						printf(" !V");
					} else if (icmp->icmp_code == ICMP_PREC_CUTOFF) {
						printf(" !C");
					} else {
						printf(" !%d", icmp->icmp_code);
					}
					fflush(stdout);
				} else {
					all_failed = 0;
				}
				last_ip = ip->ip_src;
			} else {
				all_failed = 0;
				printf(" *");
				fflush(stdout);
			}
			usleep(ctx->sendwait > 9 ? ctx->sendwait * 1000 : ctx->sendwait * 1000000);
			fflush(stdout);
		}
		printf("\n");
		if (all_failed) {
			break;
		}
	}
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		print_usage(argv[0]);
	}
	if (getuid() != 0) {
		fprintf(stderr, "%s: This program requires root privileges!\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	t_traceroute_context ctx;
	init_context(&ctx);
	process_arguments(argc, argv, &ctx);
	ctx.socket_fd = init_socket(&ctx);
	start(&ctx);
	close(ctx.socket_fd);
	return 0;
}
