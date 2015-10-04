/* cs194-24 Lab 1 */

#define _BSD_SOURCE

#include "mimetype.h"
#include "mimetype_file.h"

#include <string.h>

#ifndef HTTPD_ROOT
#define HTTPD_ROOT "/var/www" 
/* #define HTTPD_ROOT "/home/osboxes/CS194-24/www  debug */
#endif

void mimetype_init(struct mimetype *mt)
{
    memset(mt, 0, sizeof(*mt));
}

struct mimetype *mimetype_new(palloc_env env, const char *path)
{
    int fullpath_len;
    char *fullpath;
    struct mimetype *mt;
 
    fullpath_len = snprintf(NULL, 0, "%s%s", HTTPD_ROOT, path) + 1;  /* %s/%s ==> %s%s, 151006 thinkhy */
    fullpath = palloc_array(env, char, fullpath_len);
    snprintf(fullpath, fullpath_len, "%s%s", HTTPD_ROOT, path);

    printf("Try to open file: %s\r\n", fullpath);   /* Debug thinkhy */

    mt = mimetype_file_new(env, fullpath);
    pfree(fullpath);

    return mt;
}

