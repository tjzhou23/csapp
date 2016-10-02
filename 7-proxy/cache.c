/* @name  cache
 * @brief Simple LRU cache of the requested contents. Newly cached contents
 *        will be placed to the end of the cache list, and when the space is
 *        not enough, the caches at front will be evicted until there is
 *        enough space. Implemented 2nd readers/writers problem. 1st r/w may
 *        probably starve writer. Generally 2nd r/w may starve reader, but in
 *        this scenario it won't, because writing cache always happens after
 *        reading cache.
 *
 * Implemented 2nd readers/writers problem according to
 * http://cs.nyu.edu/~lerner/spring10/MCP-S10-Read04-ReadersWriters.pdf
 *
 */

#include "cache.h"

/* Static prototypes */
static void gen_key(request_t* request, char* key);
static cnode_t* find(cache_t* cache, char* key);
static void evict_first(cache_t* cache);
static void move_to_last(cache_t* cache, cnode_t* node);
static void free_node(cnode_t* node);
static void lock_reader(cache_t* cache);
static void lock_writer(cache_t* cache);
static void unlock_reader(cache_t* cache);
static void unlock_writer(cache_t* cache);
#ifdef DEBUG
static void print_list(cache_t* cache);
#endif

/*
 * init_cache   - Initialize cache. This function does NOT allocate space.
 *
 * Returns:
 * cache_t*     - Pointer to initialized cache.
 */
cache_t* init_cache(cache_t* cache) {
  cache->first = NULL;
  cache->last = NULL;
  cache->remain_sz = MAX_CACHE_SIZE;
  cache->reader_cnt = 0;
  cache->writer_cnt = 0;
  Sem_init(&cache->reader_mutex, 0, 1);
  Sem_init(&cache->writer_mutex, 0, 1);
  Sem_init(&cache->x, 0, 1);
  Sem_init(&cache->y, 0, 1);
  Sem_init(&cache->z, 0, 1);
  return cache;
}

/*
 * read_cache   - Read content into cbuf given request.
 *
 * Returns:
 *   cache size - Hit
 *   0          - Miss
 */
int read_cache(cache_t* cache, request_t* request, char* cbuf) {
  char key[MAXLINE];
  gen_key(request, key);
#ifdef DEBUG
  printf("\n");
  printf("Reading cache for: %s\n", key);
#endif

  lock_reader(cache);
  cnode_t* node = find(cache, key);
  if (!node) {
    unlock_reader(cache);
#ifdef DEBUG
    printf("\n");
    printf("Cache miss!\n");
#endif
    return 0;
  }

  memcpy(cbuf, node->content, node->size);
  unlock_reader(cache);
#ifdef DEBUG
  printf("\n");
  printf("Cache hit!\n");
#endif

  /* Maintain LRU */
  lock_writer(cache);
  move_to_last(cache, node);
  unlock_writer(cache);
  return node->size;
}

/*
 * write_cache    - Write cbuf into cache. Evict if necessary. cbuf here
 *                  is allowed not to end with \0, so size is needed.
 *
 * Returns:
 *   1            - Written successfully
 *   0            - Not written due to size limit
 */
int write_cache(cache_t* cache, request_t* request,
                char* cbuf, ssize_t size) {
  if (size > MAX_OBJECT_SIZE) {
    return 0;
  }

  cnode_t* node = (cnode_t*) malloc(sizeof(cnode_t));
  node->key = (char*) malloc(MAXLINE);
  gen_key(request, node->key);
  node->content = (char*) malloc(size);
  memcpy(node->content, cbuf, size);
  node->size = size;

#ifdef DEBUG
  printf("\n");
  printf("Writing %d bytes cache for: %s\n",
         (int) node->size, node->key);
  printf("\n");
  printf("Before writing cache:\n");
  print_list(cache);
#endif

  lock_writer(cache);
  while (cache->remain_sz < size) {
    evict_first(cache);
  }
  cache->remain_sz -= size;

  cnode_t* old_last = cache->last;
  if (old_last) {
    cache->last->next = node;
    cache->last = node;
    node->prev = old_last;
    node->next = NULL;
  } else {
    cache->first = node;
    cache->last = node;
    node->prev = NULL;
    node->next = NULL;
  }
  unlock_writer(cache);

#ifdef DEBUG
  printf("\n");
  printf("After writing cache:\n");
  print_list(cache);
  printf("\n");
  printf("Remaining size: %d\n", (int) cache->remain_sz);
#endif

  return 1;
}

/*
 * gen_key      - Generate cache key given request.
 */
static void gen_key(request_t* request, char* key) {
  snprintf(key, MAXLINE,
           "%s:%s%s",
           request->hostname,
           request->port,
           request->path);
}

/*
 * find         - Find a cache node given key.
 *
 * Returns:
 *   node       - Found
 *   NULL       - Not found
 */
static cnode_t* find(cache_t* cache, char* key) {
  cnode_t* node;
  for (node = cache->first; node; node = node->next) {
    if (strcmp(node->key, key) == 0) {
      break;
    }
  }
  return node;
}

/*
 * evict_first  - Evict first element of LRU cache. Update remain_sz
 *                as well.
 */
static void evict_first(cache_t* cache) {
  cnode_t* old_first = cache->first;
  if (!old_first) {
    return;
  }

#ifdef DEBUG
  printf("\n");
  printf("Before evicting first:\n");
  print_list(cache);
#endif

  cache->remain_sz += cache->first->size;
  cache->first = old_first->next;
  free_node(old_first);
  if (cache->first) {
    cache->first->prev = NULL;
  } else {
    cache->last = NULL;
  }

#ifdef DEBUG
  printf("\n");
  printf("After evicting first:\n");
  print_list(cache);
#endif
}

/*
 * move_to_last - Move the node to last according to LRU policy.
 */
static void move_to_last(cache_t* cache, cnode_t* node) {
  if (!node || !cache->first || !cache->last) {
    return;
  }

  if (!node->next) {  /* already at last */
    return;
  }

  if (!node->prev) {  /* node is first */
    cache->first = node->next;
    cache->first->prev = NULL;
  } else {            /* node in the middle */
    node->prev->next = node->next;
    node->next->prev = node->prev;
  }

  cnode_t* old_last = cache->last;
  old_last->next = node;
  node->prev = old_last;
  node->next = NULL;
  cache->last = node;

#ifdef DEBUG
  printf("\n");
  printf("Move %s to last\n", node->key);
  print_list(cache);
#endif
}

#ifdef DEBUG
/*
 * print_list   - Print the list of keys in cache. For debug.
 */
static void print_list(cache_t* cache) {
  cnode_t* node;
  printf("\n");
  printf("First to last:\n");
  for (node = cache->first; node; node = node->next) {
    printf("%s\n", node->key);
  }
  printf("\n");
  printf("Last to first:\n");
  for (node = cache->last; node; node = node->prev) {
    printf("%s\n", node->key);
  }
}
#endif

/*
 * free_node    - Free a cache node.
 */
static void free_node(cnode_t* node) {
  if (!node) {
    return;
  }
  free(node->key);
  free(node->content);
  free(node);
}

/*
 * lock_reader  - Acquire a reader lock.
 */
static void lock_reader(cache_t* cache) {
  P(&cache->z);
  P(&cache->reader_mutex);
  P(&cache->x);
  cache->reader_cnt++;
  if (cache->reader_cnt == 1) {
    P(&cache->writer_mutex);
  }
  V(&cache->x);
  V(&cache->reader_mutex);
  V(&cache->z);
}

/*
 * unlock_reader  - Release a reader lock.
 */
static void unlock_reader(cache_t* cache) {
  P(&cache->x);
  cache->reader_cnt--;
  if (cache->reader_cnt == 0) {
    V(&cache->writer_mutex);
  }
  V(&cache->x);
}

/*
 * lock_writer  - Acquire a writer lock.
 */
static void lock_writer(cache_t* cache) {
  P(&cache->y);
  cache->writer_cnt++;
  if (cache->writer_cnt == 1) {
    P(&cache->reader_mutex);
  }
  V(&cache->y);
  P(&cache->writer_mutex);
}

/*
 * unlock_writer  - Release the writer lock.
 */
static void unlock_writer(cache_t* cache) {
  V(&cache->writer_mutex);
  P(&cache->y);
  cache->writer_cnt--;
  if (cache->writer_cnt == 0) {
    V(&cache->reader_mutex);
  }
  V(&cache->y);
}
