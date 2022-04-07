#ifndef UTILITIES_H
#define UTILITIES_H

#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

char *ft_strcpy(char *dst, const char *src);
void *ft_memset(void *s, int c, size_t n);
int search_char(const char *s, char c);
long ft_strtol(const char *nptr, char **endptr);
int is_ip(const char *host);
const char *ft_strerror(int errcode);
const char *ft_gai_strerror(int errcode);
unsigned short ft_checksum(unsigned short *addr, int len);
int dns_resolve(const char *host, char *dest, int size);

#endif
