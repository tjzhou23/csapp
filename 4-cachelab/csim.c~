#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include "cachelab.h"

#define BUFSIZE 1023

typedef long addr_t;

typedef struct {
  bool valid;
  addr_t tag;
  int stale;  // staleness for LRU
} cache_line_t;  // cache line

typedef struct {
  int miss;
  int hit;
  int evict;

  int s_val;
  int S_val;
  int E_val;
  int b_val;
  int B_val;

  cache_line_t **data;
} cache_t;

typedef struct {
  addr_t tag;
  addr_t si;  // set index
  addr_t bo;  // block offset
} cache_addr_t;  // cache address

/*
 * Print help when user wants or argument is not correct.
 */
void print_help() {
  static bool is_printed = false;
  if (!is_printed) {
    fputs(
            "Usage: ./csim [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n",stderr);
    is_printed = true;
  }
}

/*
 * Parse address
 */
cache_addr_t * parse_addr(addr_t addr, int b_val, int s_val) {
  cache_addr_t *r = malloc(sizeof(cache_addr_t));
  r->bo = addr % (1 << b_val);
  addr >>= b_val;
  r->si = addr % (1 << s_val);
  addr >>= s_val;
  r->tag = addr;
  return r;
}

/*
 * Load data from cache.
 */
void load_cache(cache_t* cache, cache_addr_t* caddr, bool verbose) {
  addr_t si = caddr->si;
  addr_t tag = caddr->tag;
  bool hit = false;
  bool evict = true;
  int hit_j = -1;
  int oldest_j = -1;
  int oldest = -1;
  int non_evict_j = -1;
  for (int j = 0; j < cache->E_val; j++) {
    cache_line_t* line = &cache->data[si][j];
    if (line->valid) {
      if (line->tag == tag) {
        hit = true;
        hit_j = j;
        break;
      } else if (line->stale > oldest) {
        oldest = line->stale;
        oldest_j = j;
      }
    } else {
      evict = false;
      non_evict_j = j;
    }
  }

  if (hit) {
    /* hit */
    cache->hit++;
    if (verbose) {
      printf("hit ");
    }

    /* update cache */
    cache->data[si][hit_j].stale = 0;
  } else {
    cache->miss++;
    if (verbose) {
      printf("miss ");
    }

    int j = -1;
    if (evict) {
      /* miss and need evict */
      cache->evict++;
      j = oldest_j;
      if (verbose) {
        printf("eviction ");
      }
    } else {
      /* miss but don't need evict */
      j = non_evict_j;
    }

    /* update cache */
    cache->data[si][j].valid = true;
    cache->data[si][j].tag = tag;
    cache->data[si][j].stale = 0;
  }
}

/*
 * Store data to cache.
 */
void store_cache(cache_t* cache, cache_addr_t* caddr, bool verbose) {
  // store is the same as load
  load_cache(cache, caddr, verbose);
}

/*
 * Modify data in cache. Involves load and store.
 */
void modify_cache(cache_t* cache, cache_addr_t* caddr, bool verbose) {
  load_cache(cache, caddr, verbose);
  store_cache(cache, caddr, verbose);
}


/*
 * Update cache
 * Size is not considered given the assumption that data are well aligned.
 */
void update_cache(cache_t* cache, char type,
                  cache_addr_t* caddr, bool verbose) {
  switch (type) {
    case 'L':
      load_cache(cache, caddr, verbose);
      break;
    case 'S':
      store_cache(cache, caddr, verbose);
      break;
    case 'M':
      modify_cache(cache, caddr, verbose);
      break;
    default:
      break;
  }

  /* Update staleness */
  int S = cache->S_val;
  int E = cache->E_val;
  for (int i = 0; i < S; i++) {
    for (int j = 0; j < E; j++) {
      cache_line_t* line = &cache->data[i][j];
      if (line->valid) {
        line->stale++;
      }
    }
  }
}

/*
 * Entry of the program
 */
int main(int argc, char** argv) {
  /* Parse args */
  int h_flag = 0;
  bool v_flag = false;
  int s_val = -1;
  int S_val = -1;
  int E_val = -1;
  int b_val = -1;
  int B_val = -1;
  char *t_val = NULL;
  int opt;

  opterr = 0;
  while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
    switch (opt) {
      case 'h':
        h_flag = 1;
        break;
      case 'v':
        v_flag = true;
        break;
      case 's':
        s_val = atoi(optarg);
        S_val = 1 << s_val;
        break;
      case 'E':
        E_val = atoi(optarg);
        break;
      case 'b':
        b_val = atoi(optarg);
        B_val = 1 << b_val;
        break;
      case 't':
        t_val = optarg;
        break;
      default:
        print_help();
        opterr = 1;
        return -1;
    }
  }
  if (h_flag || s_val < 0  || E_val < 0 ||
      b_val < 0 || t_val == NULL) {
    printf("s_val = %d\n", s_val);
    printf("E_val = %d\n", E_val);
    printf("b_val = %d\n", b_val);
    printf("t_val = %s\n", t_val);
    print_help();
    return -1;
  }

  /* Init cache */
  cache_t *cache = malloc(sizeof(cache_t));
  cache->evict= 0;
  cache->hit = 0;
  cache->miss = 0;
  cache->b_val = b_val;
  cache->B_val = B_val;
  cache->E_val = E_val;
  cache->s_val = s_val;
  cache->S_val = S_val;
  cache_line_t **cache_data;
  cache_data = malloc(sizeof(void*) * S_val);
  for (int i = 0; i < S_val; i++) {
    cache_data[i] = (cache_line_t *) malloc(sizeof(cache_line_t) * E_val);
    for (int j = 0; j < E_val; j++) {
      cache_data[i][j].valid = false;
      cache_data[i][j].stale = 0;
    }
  }
  cache->data = cache_data;

  /* Read file */
  if (access(t_val, F_OK) < 0) {
    /* File not exists */
    fprintf(stderr, "File not exists!\n");
    return -1;
  }
  char line[BUFSIZE + 1];
  FILE *fp = fopen(t_val, "rt");


  while (fgets(line, BUFSIZE, fp) != NULL) {
    char type;
    addr_t addr;
    int size;

    int r = sscanf(line, " %c %lx,%d", &type, &addr, &size);
    if (r <= 0) {
      fprintf(stderr, "Line format error:\n");
      fprintf(stderr, line);
      break;
    }
    /* Ignore instruction cache */
    if (type == 'I') {
      continue;
    }

    if (v_flag) {
      printf("%c %lx,%d ", type, addr, size);
    }
    cache_addr_t *caddr = parse_addr(addr, b_val, s_val);
    update_cache(cache, type, caddr, v_flag);
    if (v_flag) {
      printf("\n");
    }
  }
  fclose(fp);
  printSummary(cache->hit, cache->miss, cache->evict);

  /* Destroy cache */
  for (int i = 0; i < S_val; i++) {
    free(cache_data[i]);
  }
  free(cache_data);
  free(cache);

  return 0;
}
