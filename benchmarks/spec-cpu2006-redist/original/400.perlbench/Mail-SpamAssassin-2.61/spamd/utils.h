#ifndef UTILS_H
#define UTILS_H

#define UNUSED_VARIABLE(v)	((void)(v))

extern int libspamc_timeout;  /* default timeout in seconds */

#ifdef SPAMC_SSL
#include <openssl/crypto.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#else
typedef int	SSL;	/* fake type to avoid conditional compilation */
typedef int	SSL_CTX;
typedef int	SSL_METHOD;
#endif

ssize_t fd_timeout_read (int fd, void *, size_t );  
int ssl_timeout_read (SSL *ssl, void *, int );  

/* these are fd-only, no SSL support */
int full_read(int fd, void *buf, int min, int len);
int full_read_ssl(SSL *ssl, unsigned char *buf, int min, int len);
int full_write(int fd, const void *buf, int len);

#endif
