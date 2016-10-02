/* @name  proxy
 * @brief A simple multi-thread proxy server with LRU cache. Forwards
 *        whatever request from client to server, and feed that back
 *        to client. If the request is GET and the content size is not
 *        too large (< MAX_OBJECT_SIZE), the content will be cached,
 *        and next time it's requested, proxy won't make request to
 *        server again. As it's just a simple proxy server, contents
 *        won't be expired.
 *
 */

#include <stdio.h>
#include "csapp.h"
#include "proxy.h"
#include "common.h"

int main(int argc, char **argv) {
  intptr_t listen_fd, conn_fd;
  char client_name[MAXLINE], client_port[MAXLINE];
  struct sockaddr_storage client_addr;
  socklen_t client_len = sizeof(client_addr);
  pthread_t tid;

  /* Check cmd line args */
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <port>\n", argv[0]);
    exit(1);
  }

  cache = init_cache((cache_t*) malloc(sizeof(cache_t)));

  /*
   * When client continue to send request after connection is closed,
   * a SIGPIPE would be received. Ignore such requests.
   */
  Signal(SIGPIPE, SIG_IGN);

  listen_fd = Open_listenfd(argv[1]);
  while (1) {
    conn_fd = Accept(listen_fd, (SA *) &client_addr, &client_len);
    Getnameinfo((SA *) &client_addr, client_len,
                client_name, MAXLINE, client_port, MAXLINE, 0);
    Pthread_create(&tid, NULL, conn, (void *) conn_fd);
  }

  /* Should never come here */
  return 0;
}

/*
 * conn          - Establish a connection between client and proxy. This
 *                 is a new thread doing the proxy job.
 */
void *conn(void *fd) {
  request_t request;
  int conn_fd = (intptr_t) fd;
  Pthread_detach(pthread_self());

  parse_request(conn_fd, &request);
  proxy(conn_fd, &request);

  Close(conn_fd);
  return NULL;
}

/*
 * proxy         - Forward client request to server, and respond as
 *                 the server did. Only support GET method.
 */
void proxy(int client_fd, request_t *request) {
  int server_fd;
  rio_t server_rio;
  char request_str[MAXLINE];
  char buf[MAXLINE], lowerbuf[MAXLINE];  /* Line buffer */
  char cbuf[MAX_OBJECT_SIZE];            /* Content buffer */

  const int is_get = strcmp(request->method, "GET") == 0;
  if (is_get) {
    ssize_t cache_sz = read_cache(cache, request, cbuf);
    if (cache_sz > 0) {
      /* Cache hit */
      Rio_writen(client_fd, cbuf, (size_t) cache_sz);
      return;
    }
  }

  /* Cache miss; make request */
  server_fd = Open_clientfd(request->hostname, request->port);
  if (server_fd < 0) {
    sprintf(buf, "%s", "Bad Request\n");
    Rio_writen(client_fd, buf, strlen(buf));
    Close(server_fd);
    return;
  }

  build_request(request, request_str);
  Rio_writen(server_fd, request_str, strlen(request_str));
  Rio_readinitb(&server_rio, server_fd);
  int header_sz = 0;
  cbuf[0] = 0;  /* Initial cbuf as empty str */

  /* Forward headers */
  while (1) {
    ssize_t line_sz = Rio_readlineb(&server_rio, buf, MAXLINE);
    if (line_sz < 0) {
      sprintf(buf, "%s", "IO error\n");
      Rio_writen(client_fd, buf, strlen(buf));
      Close(server_fd);
      return;
    }

    /* Prepare for cache */
    header_sz += line_sz;
    if (is_get) {
      strncat(cbuf, buf, line_sz);
    }

    /* rio guarantees the last is \0, so it's safe to do str opx */
    lowerstr(buf, lowerbuf);
    Rio_writen(client_fd, buf, (size_t) line_sz);

#ifdef DEBUG
    int tmp;
    if (sscanf(lowerbuf, "http%*s %d %*s", &tmp) > 0) {
        printf("\n");
        printf("Status Code: %d\n", tmp);
    }
#endif

    if (!strcmp(buf, "\r\n")) {
      break;
    }
  }

  /* Forward body */
  /* Don't do str op on body */
  char* cbufp = cbuf + strlen(cbuf);
  int tot_sz = header_sz;
  int need_cache = is_get && tot_sz > 0 && tot_sz <= MAX_OBJECT_SIZE;
  int line_sz;
  while (1) {
    buf[0] = 0;  /* Init buf as empty str */
    /* Every time just try to read MAXLINE, don't throw error at end. */
    line_sz = (int) rio_readlineb(&server_rio, buf, MAXLINE);

    if (line_sz < 0) {
      /* Something unexpected happened. Just break. */
      need_cache = 0;
      break;
    }

    if (need_cache) {
      /* Append cache if necessary. */
      tot_sz += line_sz;
      if (tot_sz > MAX_OBJECT_SIZE) {
        need_cache = 0;
      } else {
        memcpy(cbufp, buf, line_sz);
        cbufp += line_sz;
      }
    }

    /* Write buf to client */
    if (Rio_writen(client_fd, buf, (size_t) line_sz) < 0) {
      /* This may happen because of broken pipe. */
      /* Don't cache broken stuff. */
      need_cache = 0;
      break;
    }

    if (line_sz <= 0) {
      /* If it meets an EOF, just break. */
      break;
    }
  }
  if (need_cache) {
    write_cache(cache, request, cbuf, tot_sz);
  }

  Close(server_fd);
}

/*
 * build_request - Build request string from parsed request.
 */
void build_request(request_t *request, char *request_str) {
  request_str[0] = 0;  /* Init as empty str */
  strncat(request_str, request->method, MAXLINE);
  strncat(request_str, " ", MAXLINE);
  strncat(request_str, request->path, MAXLINE);
  strncat(request_str, " ", MAXLINE);
  strncat(request_str, request->version, MAXLINE);
  strncat(request_str, "\r\n", MAXLINE);
  strncat(request_str, request->headers, MAXLINE);

#ifdef DEBUG
  printf("\n");
  printf("Request string is\n");
  printf("%s", request_str);
#endif
}

/*
 * parse_request - Parse the request into fields. Request is contained
 *                 in fd. Parsed fields would be manipulated as required,
 *                 e.g. port will be 80 if not specified; version would
 *                 be replaced to HTTP/1.0 silently; hostname would be
 *                 put into headers if not specified; several headers
 *                 will be replaced as default ones.
 */
void parse_request(int fd, request_t *request) {
  rio_t rio;
  char buf[MAXLINE], lowerbuf[MAXLINE];
  char uri[MAXLINE];
  Rio_readinitb(&rio, fd);

  /* Parse uri */
  Rio_readlineb(&rio, buf, MAXLINE);
  sscanf(buf, "%s %s %*s", request->method, uri);
  upperstr(request->method, request->method);
  lowerstr(uri, uri);
  const ssize_t uri_len = strlen(uri);

#ifdef DEBUG
  printf("\n");
  printf("Original uri: %s\n", uri);
#endif

  char *pstart = strstr(uri, "://");
  if (pstart) {
    pstart += 3;  /* length of :// */
  } else {
    pstart = uri; /* scheme not specified */
  }

  char *pcol = strchr(pstart, ':');
  char *pslash = strchr(pstart, '/');
  if (!pslash) {
    pslash = uri + strlen(uri);
  }
  if (pcol && pslash <= pcol + 1) {
    pcol = NULL;
  }

  /* Parse hostname and port */
  if (pcol) {     /* port specified */
    strncpy(request->hostname, pstart, pcol - pstart);
    strncpy(request->port, pcol + 1, pslash - pcol - 1);
  } else {        /* port not specified */
    strncpy(request->hostname, pstart, pslash - pstart);
    strcpy(request->port, "80");
  }

  /* Parse path */
  strcpy(request->path, "/");
  ssize_t plen = uri + uri_len - pslash;
  if (plen > 1) {
    strncat(request->path, pslash + 1, MAXLINE);
  }

  /* Replace version with HTTP/1.0 silently */
  strcpy(request->version, version);

  /* Parse headers */
  request->headers[0] = 0;  /* Initialize as empty str */
  strncat(request->headers, user_agent_hdr, MAXLINE);
  strncat(request->headers, conn_hdr, MAXLINE);
  strncat(request->headers, proxy_conn_hdr, MAXLINE);
  int host_specified = 0;
  while (1) {
    Rio_readlineb(&rio, buf, MAXLINE);
    lowerstr(buf, lowerbuf);
    if (!strcmp(buf, "\r\n")) break;

    /* Ignore certain fields */
    if (!strstr(lowerbuf, "user-agent:") &&
        !strstr(lowerbuf, "connection:") &&
        !strstr(lowerbuf, "proxy-connection:")) {
      strncat(request->headers, buf, MAXLINE);
    }

    /* See if host is specified by client */
    if (strstr(lowerbuf, "host:")) {
      host_specified = 1;
    }
  }

  /* Append Host field if necessary */
  if (!host_specified) {
    char host[MAXLINE];
    host[0] = 0;  /* Initialize as empty str */
    strncat(host, "Host: ", MAXLINE);
    strncat(host, request->hostname, MAXLINE);
    strncat(host, "\r\n", MAXLINE);
    strncat(request->headers, host, MAXLINE);
  }
  strncat(request->headers, "\r\n", MAXLINE);

#ifdef DEBUG
  printf("Parsed method: %s\n", request->method);
  printf("Parsed server_name: %s\n", request->hostname);
  printf("Parsed server_port: %s\n", request->port);
  printf("Parsed server_path: %s\n", request->path);
  printf("Parsed headers:\n%s", request->headers);
#endif
}

/*
 * lowerstr         - Convert a string to lower case
 */
void lowerstr(char *str, char *lower) {
  for (; *str; ++str, ++lower) *lower = (char) tolower(*str);
}

/*
 * upperstr         - Convert a string to upper case
 */
void upperstr(char *str, char *upper) {
  for (; *str; ++str, ++upper) *upper = (char) toupper(*str);
}
