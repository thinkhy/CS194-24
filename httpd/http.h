/* cs194-24 Lab 1 */

#ifndef HTTP_H
#define HTTP_H

#include "palloc.h"

/* Allows HTTP sessions to be transported over the HTTP protocol */
struct http_session
{
    const char * (*gets)(struct http_session *);

    ssize_t (*puts)(struct http_session *, const char *);

    ssize_t (*write)(struct http_session *, const char *, size_t);

    /* Stores a resizeable, circular buffer */
    char *buf;
    size_t buf_size, buf_used;

    int fd;
};

/* A server that listens for HTTP connections on a given port. */
struct http_server
{
    int (*wait_for_client)(struct http_server *);

    int fd;
    int efd;  /* Descriptor for epoll_wait, added by thinkhy, 151001 */
};

/* Creates a new HTTP server listening on the given port. */
struct http_server *http_server_new(palloc_env env, short port);

/* Added by thinkhy, 151001 */
struct http_server *create_http_server(palloc_env env, short port); 

#endif
