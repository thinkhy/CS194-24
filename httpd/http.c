/* cs194-24 Lab 1 */

#define _POSIX_C_SOURCE 1
#define _BSD_SOURCE

#define MAX_PENDING_CONNECTIONS 8

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/epoll.h>

#include "http.h"
#include "lambda.h"
#include "palloc.h"

#define DEFAULT_BUFFER_SIZE 256
#define MAXEVENTS 64

struct string
{
    size_t size;
    char *data;
};

static int make_socket_non_blocking (int sfd);  /* Added by thinkhy, 151001 */
static int create_and_bind (short port);        /* Added by thinkhy, 151001 */

static int listen_on_port(short port);
static struct http_session *wait_for_client(struct http_server *serv);

static int close_session(struct http_session *s);
static int close_server(struct http_server *hs); /* Added by thinkhy, 151001 */

static const char *http_gets(struct http_session *s);
static ssize_t http_puts(struct http_session *s, const char *m);
static ssize_t http_write(struct http_session *s, const char *m, size_t l);


/* Added by thinkhy, 151001 */
int make_socket_non_blocking (int sfd)
{
  int flags, s;

  flags = fcntl (sfd, F_GETFL, 0);
  if (flags == -1)
    {
      perror ("fcntl");
      return -1;
    }

  flags |= O_NONBLOCK;
  s = fcntl (sfd, F_SETFL, flags);
  if (s == -1)
    {
      perror ("fcntl");
      return -1;
    }

  return 0;
}

/* Added by thinkhy, 151001 */
int create_and_bind (short port)
{
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int s, sfd;
  char str_port[10];

  memset (&hints, 0, sizeof (struct addrinfo));
  hints.ai_family = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */
  hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
  hints.ai_flags = AI_PASSIVE;     /* All interfaces */

  snprintf(str_port, 10, "%hd", port); /* Added by thinkhy, 151001 */

  s = getaddrinfo (NULL, str_port, &hints, &result);
  if (s != 0)
    {
      fprintf (stderr, "getaddrinfo: %s\n", gai_strerror (s));
      return -1;
    }

  for (rp = result; rp != NULL; rp = rp->ai_next)
    {
      sfd = socket (rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      if (sfd == -1)
        continue;

      s = bind (sfd, rp->ai_addr, rp->ai_addrlen);
      if (s == 0)
        {
          /* We managed to bind successfully! */
          break;
        }

      close (sfd);
    }

  if (rp == NULL)
    {
      fprintf (stderr, "Could not bind\n");
      return -1;
    }

  freeaddrinfo (result);

  return sfd;
}


struct http_server *create_http_server(palloc_env env, short port)
{
    struct http_server *hs;
    int fd, efd, ret;
    struct epoll_event event;

    hs = palloc(env, struct http_server);
    if (hs == NULL)
	return NULL;

    /* hs->wait_for_client = &wait_for_client; */
    fd = create_and_bind(port);
    if (fd < 0) 
       return NULL;

    ret = make_socket_non_blocking (fd);
    if (ret == -1)
       abort();

    ret = listen(fd, SOMAXCONN);
    if (ret == -1)
    {
       perror("listen");
       abort();
    }
    hs->fd = fd;

    efd = epoll_create1 (0);
    if (efd == -1)
    {
      perror ("epoll_create");
      abort ();
    }

    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    ret = epoll_ctl(efd, EPOLL_CTL_ADD, fd, &event);
    if (ret == -1) 
    {
       perror("epoll_ctl");
       abort();
    }
    hs->efd = efd;
    
    hs->wait_for_client = &wait_for_client;

    return hs;
}

struct http_server *http_server_new(palloc_env env, short port)
{
    struct http_server *hs;

    hs = palloc(env, struct http_server);
    if (hs == NULL)
	return NULL;

    hs->wait_for_client = &wait_for_client;
    hs->fd = listen_on_port(port);

    palloc_destructor(hs, &close_server);
    return hs;
}

int listen_on_port(short port)
{
    int fd;
    struct sockaddr_in addr;
    socklen_t addr_len;
    int so_true;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
	return -1;

    /* SO_REUSEADDR allows a socket to bind to a port while there
     * are still outstanding TCP connections there.  This is
     * extremely common when debugging a server, so we're going to
     * use it.  Note that this option shouldn't be used in
     * production, it has some security implications.  It's OK if
     * this fails, we'll just sometimes get more errors about the
     * socket being in use. */
    so_true = true;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &so_true, sizeof(so_true));

    addr_len = sizeof(addr);
    memset(&addr, 0, addr_len);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if (bind(fd, (struct sockaddr *)&addr, addr_len) < 0)
    {
	perror("Unable to bind to HTTP port");
	close(fd);
	return -1;
    }

    if (listen(fd, MAX_PENDING_CONNECTIONS) < 0)
    {
	perror("Unable to listen on HTTP port");
	close(fd);
	return -1;
    }

    return fd;
}

/* Modified by thinkhy, 151001 */
struct http_session *wait_for_client(struct http_server *serv)
{
    struct http_session *sess;
    /* Astruct sockaddr_in addr; */
    /* socklen_t addr_len; */
    int ret;
 
    sess = palloc(serv, struct http_session);
    if (sess == NULL)
	return NULL;

    sess->gets = &http_gets;
    sess->puts = &http_puts;
    sess->write = &http_write;

    sess->buf = palloc_array(sess, char, DEFAULT_BUFFER_SIZE);
    memset(sess->buf, '\0', DEFAULT_BUFFER_SIZE);
    sess->buf_size = DEFAULT_BUFFER_SIZE;
    sess->buf_used = 0;

    /* Buffer where events are returned */
    struct epoll_event event;
    struct epoll_event *events = calloc (MAXEVENTS, sizeof event);

    int done = 0;
    /* The event loop */
    while (!done) 
    {
      int n, i;
      n = epoll_wait(serv->efd, events, MAXEVENTS, -1);
      for ( i = 0; i < n; i++)
      {
         if ( (events[i].events & EPOLLERR) ||
              (events[i].events & EPOLLHUP) ||
              (!(events[i].events & EPOLLIN)) 
            )
         {
            /* An error has occurred on this fd, or the socket is not ready 
             * for reading (why were notified then?)                         */        
           perror("epoll error\n");
           close (events[i].data.fd);
           continue;
         }
         else if (serv->fd == events[i].data.fd)
         {
           /* We have a notification on the listening socket, which means one 
            * or more incoming connections                                  */
          while(1) 
          {
             struct sockaddr in_addr;
             socklen_t in_len;
             int infd;
             char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
          
             in_len = sizeof(in_addr);
             infd = accept(serv->fd, &in_addr, &in_len);
             if (infd == -1)
             {
                 if (errno == EAGAIN 
                     || errno == EWOULDBLOCK)
                 {
                    /* We have processed all incoming connections. */
                    break;
                 }
                 else 
                 {
                    perror("accept");
                    break;
                 }
             }

             ret = getnameinfo(&in_addr, in_len,
                               hbuf,     sizeof(hbuf),
                               sbuf,     sizeof(sbuf),
                               NI_NUMERICHOST | NI_NUMERICSERV);
             if (ret == 0)
             {
                printf("Accepted connection on descriptor %d "
                       "(host=%s, port=%s)\n", infd, hbuf, sbuf);
             }

             /* Make the incoming socket non-blocking and add it to 
              * the list of fds to monitor                           */
             ret = make_socket_non_blocking(infd);             
             if (ret == -1)
                abort();
            
             event.data.fd = infd;
             event.events = EPOLLIN | EPOLLET;
	     ret = epoll_ctl(serv->efd, EPOLL_CTL_ADD, infd, &event);
             if (ret == -1) 
             {
                perror("epoll_ctl");
                abort();
             }
          }
          continue;
         } /* Have incoming connections */
         else 
         { 
            /* Have data on the fd waiting to be read */
	    printf("Have data on the fd waiting to be read\n");
            sess->fd = events[i].data.fd;
            done = 1;
            break; /* main to handle request from web client */
         } 
      } /* for loop after epoll_wait */

    } /* epoll event loop */

    free (events);
    palloc_destructor(sess, &close_session);

    return sess;
}

struct http_session *wait_for_client_old(struct http_server *serv)
{
    struct http_session *sess;
    struct sockaddr_in addr;
    socklen_t addr_len;

    sess = palloc(serv, struct http_session);
    if (sess == NULL)
	return NULL;

    sess->gets = &http_gets;
    sess->puts = &http_puts;
    sess->write = &http_write;

    sess->buf = palloc_array(sess, char, DEFAULT_BUFFER_SIZE);
    memset(sess->buf, '\0', DEFAULT_BUFFER_SIZE);
    sess->buf_size = DEFAULT_BUFFER_SIZE;
    sess->buf_used = 0;

    /* Wait for a client to connect. */
    addr_len = sizeof(addr);
    sess->fd = accept(serv->fd, (struct sockaddr *)&addr, &addr_len);
    if (sess->fd < 0)
    {
	perror("Unable to accept on client socket");
	pfree(sess);
	return NULL;
    }

    palloc_destructor(sess, &close_session);

    return sess;
}

int close_session(struct http_session *s)
{
    if (s->fd == -1)
	return 0;

    close(s->fd);
    s->fd = -1;

    return 0;
}

int close_server(struct http_server *hs)
{
    if (hs->fd == -1)
	return 0;

    close(hs->fd);
    hs->fd = -1;

    return 0;
}

const char *http_gets(struct http_session *s)
{
    while (true)
    {
	char *newline;
	ssize_t readed;

	if ((newline = strstr(s->buf, "\r\n")) != NULL)
	{
	    char *new;

	    *newline = '\0';
	    new = palloc_array(s, char, strlen(s->buf) + 1);
	    strcpy(new, s->buf);

	    memmove(s->buf, s->buf + strlen(new) + 1,
		    s->buf_size - strlen(new));
	    s->buf_used -= strlen(new);
	    s->buf[s->buf_used] = '\0';

	    return new;
	}

	readed = read(s->fd, s->buf + s->buf_used, s->buf_size - s->buf_used);
        if (readed == -1)
        {
           /* errno == EAGAIN, means we have read all data.
            * So go back to the main loop.                  */
           if (errno == EAGAIN)
           {
              perror("read return EAGAIN");
           }
           break; 
        }
        else if (readed == 0)
        {
           /* End of file. The remote has closed the connection. */
           break;
        }
	else
	    s->buf_used += readed;

	if (s->buf_used >= s->buf_size)
	{
	    s->buf_size *= 2;
	    s->buf = prealloc(s->buf, s->buf_size);
	}
    }

    return NULL;
}

ssize_t http_puts(struct http_session *s, const char *m)
{
    size_t written;

    written = 0;
    while (written < strlen(m))
    {
	ssize_t writed;

	writed = write(s->fd, m + written, strlen(m) - written);
	if (writed < 0)
	    return -1 * written;

	written += writed;
    }

    return written;
}

ssize_t http_write(struct http_session *s, const char *m, size_t l)
{
    return write(s->fd, m, l);
}


