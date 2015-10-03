/* cs194-24 Lab 1 */

#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#include "http.h"
#include "mimetype.h"
#include "palloc.h"

#define PORT 8088
#define LINE_MAX 1024
#define MAX_THREADS 16

void *event_loop(void *ptr_env);

int main(int argc, char **argv)
{
    palloc_env env;
    struct http_server *server;

    signal(SIGPIPE,SIG_IGN);
    signal(SIGCHLD,SIG_IGN);

    env = palloc_init("httpd root context");
    server = create_http_server(env, PORT); 
    if (server == NULL)
    {
	perror("Unable to open HTTP server");
	return 1;
    }

    int i, rc;
    pthread_t threads[MAX_THREADS];
    for (i = 0; i < MAX_THREADS; i++)
    {
        rc = pthread_create(&threads[i], NULL, event_loop, (void *)server);
	if (rc != 0) 
	{
	    perror("Failed to create new thread");
	}
    } 

    for (i = 0; i < MAX_THREADS; i++)
    {
        rc = pthread_join(threads[i], NULL);
        if (rc != 0) 
        {
            perror("Failed to join thread");
        }
    }
    
    pfree(env);
    
    return 0;
}


void *event_loop(void *ptr_env)
{
    int ret = 0;
    struct http_server *server = palloc_cast(ptr_env, struct http_server);
    if (server == NULL)
        pthread_exit(NULL);
     
    ret = server->wait_for_client(server);
    if (ret != 0)
    {
        perror("server->wait_for_client() failed ...");
    }

    pthread_exit(NULL);
} /* event_loop */ 


