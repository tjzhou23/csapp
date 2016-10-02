/* @name common
 * @brief Common struct.
 *
 */

#ifndef __COMMON_H__
#define __COMMON_H__

typedef struct {
  char method[MAXLINE];
  char hostname[MAXLINE];
  char port[MAXLINE];
  char path[MAXLINE];
  char version[MAXLINE];
  char headers[MAXLINE];
} request_t;

#endif /* __COMMON_H__ */
