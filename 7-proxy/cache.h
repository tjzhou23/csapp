/* @name cache
 * @brief Header of cache. See cache.c for elaborations.
 *
 */

#ifndef __CACHE_H__
#define __CACHE_H__

#include "csapp.h"
#include "common.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

typedef struct _cnode_t {
  char* key;
  char* content;
  ssize_t size;
  struct _cnode_t* prev;
  struct _cnode_t* next;
} cnode_t;

typedef struct {
  cnode_t* first;
  cnode_t* last;
  ssize_t remain_sz;
  /* 2nd readers/writers problem */
  int reader_cnt, writer_cnt;
  sem_t reader_mutex, writer_mutex;
  sem_t x, y, z;
} cache_t;

cache_t* init_cache(cache_t* cache);
int read_cache(cache_t* cache, request_t* request, char* cbuf);
int write_cache(cache_t* cache, request_t* request,
                char* cbuf, ssize_t size);

#endif /* __CACHE_H__ */
