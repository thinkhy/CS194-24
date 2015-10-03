/* cs194-24 Lab 1 */

#include "mimetype_file.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#define BUF_COUNT 4096

static int http_get(struct mimetype *mt, struct http_session *s);

struct mimetype *mimetype_file_new(palloc_env env, const char *fullpath)
{
    struct mimetype_file *mtf;

    mtf = palloc(env, struct mimetype_file);
    if (mtf == NULL)
	return NULL;

    mimetype_init(&(mtf->mimetype));

    mtf->http_get = &http_get;
    mtf->fullpath = palloc_strdup(mtf, fullpath);

    return &(mtf->mimetype);
}

int http_get(struct mimetype *mt, struct http_session *s)
{
    struct mimetype_file *mtf;
    int fd;
    char buf[BUF_COUNT];
    ssize_t readed;

    mtf = palloc_cast(mt, struct mimetype_file);
    if (mtf == NULL)
	return -1;

    printf("s->puts(s, \"HTTP/1.1 200 OK\\r\\n\"\n");
    s->puts(s, "HTTP/1.1 200 OK\r\n");
    printf("s->puts(s, \"Content-Type: text/html\\r\\n\"\n");
    s->puts(s, "Content-Type: text/html\r\n");
    printf("s->puts(s, \"\\r\\n\"\n");
    s->puts(s, "\r\n");

    fd = open(mtf->fullpath, O_RDONLY);

    while ((readed = read(fd, buf, BUF_COUNT)) > 0)
    {
	ssize_t written;

        int done = 0;
	written = 0;
	while (written < readed && !done)
	{
	    ssize_t w;

            /* When call send(), it puts the data into a buffer, and 
             * as it's read by the remote site, it's removed from the buffer.
             * If the buffer ever gets "full", the system will return the error 
             * 'Operation Would Block' the next time you try to write to it.    */
	    w = s->write(s, buf+written, readed-written);
            if (w == -1)
            {
                if (errno == EAGAIN
		     || errno == EWOULDBLOCK)
                {
                    /* buf is full, try to write next time*/
                    continue;
                }
                else
                {
                    perror("write");
                    done = 1;
                    break;
                }
            }
            else if (w == 0)
            {
                /* return code = 0, socket connection is closed */
                close(s->fd);
                done = 1;
                break;
            }
	    else 
		written += w;
	}
    }
    close(fd);

    return 0;
}
