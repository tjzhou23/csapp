/* @name proxy
 * @brief Header of proxy. See proxy.c for elaborations.
 *
 */

#ifndef __PROXY_H__
#define __PROXY_H__

#include "cache.h"
#include "common.h"

/* Constants */
static const char* version = "HTTP/1.0";
static const char* user_agent_hdr = "User-Agent: Mozilla/5.0 "
    "(X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char* conn_hdr = "Connection: close\r\n";
static const char* proxy_conn_hdr = "Proxy-Connection: close\r\n";

/* Function prototypes */
void* conn(void* fd);
void parse_request(int fd, request_t* request);
void build_request(request_t* request, char* request_str);
void proxy(int client_fd, request_t* request);
void lowerstr(char* str, char* lower);
void upperstr(char* str, char* upper);

/* Global cache */
cache_t* cache;

#endif /* __PROXY_H__ */
