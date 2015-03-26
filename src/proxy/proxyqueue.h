#ifndef __PROXY_QUEUE_H__
#define __PROXY_QUEUE_H__

#include <stdint.h>
#include "proxylist.h"

#ifndef BOOL
typedef int BOOL;
#endif

#ifndef FALSE
#define FALSE   0
#endif

#ifndef TRUE
#define TRUE    1
#endif

typedef struct _ProxyQueue ProxyQueue;

/**
 * ProxyQueue:
 * @head: a pointer to the first element of the queue
 * @tail: a pointer to the last element of the queue
 * @length: the number of elements in the queue
 */
struct _ProxyQueue
{
  ProxyList *head;
  ProxyList *tail;
  uint32_t  length;
  pthread_mutex_t lock;
};

/**
 * proxy_queue_new:
 *
 * Creates a new @ProxyQueue.
 *
 * Returns: a newly allocated @ProxyQueue
 **/
ProxyQueue *
proxy_queue_new (void);

/**
 * proxy_queue_free:
 * @queue: a #ProxyQueue
 *
 * Frees the memory allocated for the #ProxyQueue. Only call this function
 * if @queue was created with proxy_queue_new(). If queue elements contain
 * dynamically-allocated memory, they should be freed first.
 **/
void
proxy_queue_free (ProxyQueue *queue);

/**
 * proxy_queue_push_tail:
 * @queue: a #ProxyQueue
 * @data: the data for the new element
 *
 * Adds a new element at the tail of the queue.
 */
void
proxy_queue_push_tail (ProxyQueue * queue, void * data);

/**
 * proxy_queue_pop_head:
 * @queue: a #ProxyQueue
 *
 * Removes the first element of the queue and returns its data.
 *
 * Returns: the data of the first element in the queue, or %NULL
 *     if the queue is empty
 */
void *
proxy_queue_pop_head (ProxyQueue *queue);

/**
 * proxy_queue_is_empty:
 * @queue: a #ProxyQueue.
 *
 * Returns %TRUE if the queue is empty.
 */
BOOL
proxy_queue_is_empty (ProxyQueue *queue);

#endif
