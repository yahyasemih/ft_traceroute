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
	fprintf(stderr, "\tpacketlen\tThe full packet length (default is the length of an IP\n"
					"\t\t\theader plus 40). Can be ignored or increased to a minimal\n"
					"\t\t\tallowed value\n");
}

void print_usage(const char *executable) {
	fprintf(stderr, "Usage\n  sudo %s [options] host\n", executable);
	print_options();
	exit(0);
}

void process_arguments(int argc, char *argv[], t_traceroute_context *ctx) {
	(void)ctx;
	(void)argc;
	(void)argv;
}

void start(t_traceroute_context *ctx) {}

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
