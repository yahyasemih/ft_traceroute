#ifndef UTILITIES_H
#define UTILITIES_H

#include <stdlib.h>
#include <errno.h>

char *ft_strcpy(char *dst, const char *src);
int search_char(const char *s, char c);
long ft_strtol(const char *nptr, char **endptr);
int is_ip(const char *host);
const char *ft_strerror(int errcode);

#endif
