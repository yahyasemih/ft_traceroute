#include "ft_traceroute.h"

int init_socket(t_traceroute_context *ctx) {
	int fd;
	int options = 1;
	int type = SOCK_RAW;
	struct timeval timeout;
	timeout.tv_sec = ctx->sendwait;
	timeout.tv_usec = 0;

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
	ctx->wait = 5;
	ctx->sendwait = 0.0;
	ctx->flags = 0;
	ctx->socket_fd = init_socket(ctx);
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

void start(t_traceroute_context *ctx) {

	printf("traceroute to %s (%s), 30 hops max, 60 byte packets\n", ctx->host, ctx->ip);
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
	start(&ctx);
	close(ctx.socket_fd);
	return 0;
}
