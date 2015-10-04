/* cs194-24 Lab 1 */

#include "mimetype_file.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
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


    /*
     * Implement CGI: when a client requests a file from a web server 
     * that has the "execute" bit set, the web server will instead run
     * said file with that process's standard out piped back to the client.
     * 151006, thinkhy
     */
    struct stat sb;
    int rc = stat(mtf->fullpath, &sb);
    if (rc == 0 && (sb.st_mode & S_IXUSR)) /* run file and pipe back stdout */
    {
       printf("Run CGI program and pipe back stdout to the client\n");
       int pfd[2];  /* pipe file descriptor */
       if (pipe(pfd) < 0)
       {
           perror("pipe");
           abort();
       }

       int pid;
       if ((pid = fork()) < 0)
       {
           perror("fork");
           close(pfd[0]);
           close(pfd[1]);
           return -1;
       }
       else if (pid == 0)    /* child process */
       {
           close(pfd[0]);
           dup2(pfd[1], STDOUT_FILENO);
           close(pfd[1]);

           if (execl(mtf->fullpath, mtf->fullpath, NULL) < 0)
              perror("execl");
       }
       else                 /* parent process */
       {
           printf("wait for child process %d\n", pid);
           
           close(pfd[1]);

           int done = 0;
           while (!done && (readed = read(pfd[0], buf, BUF_COUNT)) > 0)
           {
               ssize_t written;

               written = 0;
               while (written < readed && !done)
               {
                   ssize_t w;

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

           close(pfd[0]);
           rc = waitpid(pid, NULL, 0);
           if (rc < 0 && errno != EINTR) 
           {
              perror("waitpid");
	      return -1;
           }
       }
    }
    else /* open non-executable file and read full content into buffer */
    {
       printf("Open non-executable file and send back html content to the client\n");

       s->puts(s, "HTTP/1.1 200 OK\r\n");
       s->puts(s, "Content-Type: text/html\r\n");
       s->puts(s, "\r\n");

       fd = open(mtf->fullpath, O_RDONLY);

       int done = 0;
       while (!done && (readed = read(fd, buf, BUF_COUNT)) > 0)
       {
           ssize_t written;

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
    }

    return 0;
}
