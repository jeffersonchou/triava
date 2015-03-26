#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>

#include "proxylog.h"
#include "proxylist.h"
#include "proxyqueue.h"

#define PROXY_QUEUE_GET_LOCK(queue) (&(((ProxyQueue *)queue)->lock))
#define PROXY_QUEUE_LOCK(queue)   pthread_mutex_lock(PROXY_QUEUE_GET_LOCK(queue))
#define PROXY_QUEUE_UNLOCK(queue) pthread_mutex_unlock(PROXY_QUEUE_GET_LOCK(queue))

/**
 * proxy_queue_new:
 *
 * Creates a new @ProxyQueue.
 *
 * Returns: a newly allocated @ProxyQueue
 **/
ProxyQueue *
proxy_queue_new (void)
{
  ProxyQueue * queue;

  queue = malloc (sizeof(ProxyQueue));
  if (queue) {
     pthread_mutex_init(&queue->lock, NULL);
     PROXY_QUEUE_LOCK (queue);
     queue->head = queue->tail = NULL;
     queue->length = 0;
     PROXY_QUEUE_UNLOCK (queue);
  }
  
  return queue;
}

/**
 * proxy_queue_free:
 * @queue: a #ProxyQueue
 *
 * Frees the memory allocated for the #ProxyQueue. Only call this function
 * if @queue was created with proxy_queue_new(). If queue elements contain
 * dynamically-allocated memory, they should be freed first.
 **/
void
proxy_queue_free (ProxyQueue *queue)
{
  p_return_if_fail (queue != NULL);

  PROXY_QUEUE_LOCK (queue);
  proxy_list_free (queue->head);
  PROXY_QUEUE_UNLOCK (queue);
  
  pthread_mutex_destroy (&queue->lock);
  free (queue);
}

/**
 * proxy_queue_init:
 * @queue: an uninitialized #ProxyQueue
 *
 * A statically-allocated #ProxyQueue must be initialized with this function
 * before it can be used. 
 */
void
proxy_queue_init (ProxyQueue *queue)
{
  p_return_if_fail (queue != NULL);

  PROXY_QUEUE_LOCK (queue);
  queue->head = queue->tail = NULL;
  queue->length = 0;
  PROXY_QUEUE_UNLOCK (queue);
}

/**
 * proxy_queue_push_tail:
 * @queue: a #ProxyQueue
 * @data: the data for the new element
 *
 * Adds a new element at the tail of the queue.
 */
void
proxy_queue_push_tail (ProxyQueue * queue, void * data)
{
  p_return_if_fail (queue != NULL);

  PROXY_QUEUE_LOCK (queue);
  queue->tail = proxy_list_append (queue->tail, data);
  if (queue->tail->next)
    queue->tail = queue->tail->next;
  else
    queue->head = queue->tail;
  queue->length++;
  PROXY_QUEUE_UNLOCK (queue);
}

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
proxy_queue_pop_head (ProxyQueue *queue)
{
  p_return_val_if_fail (queue != NULL, NULL);

  PROXY_QUEUE_LOCK (queue);
  if (queue->head) {
    ProxyList *node = queue->head;
    void * data = node->data;

    queue->head = node->next;
    if (queue->head)
      queue->head->prev = NULL;
    else
      queue->tail = NULL;
    proxy_list_free_1 (node);
    queue->length--;
    PROXY_QUEUE_UNLOCK (queue);
    
    return data;
  }
  PROXY_QUEUE_UNLOCK (queue);

  return NULL;
}

/**
 * proxy_queue_is_empty:
 * @queue: a #ProxyQueue.
 *
 * Returns %TRUE if the queue is empty.
 */
BOOL
proxy_queue_is_empty (ProxyQueue *queue)
{
  p_return_val_if_fail (queue != NULL, TRUE);

  return queue->head == NULL;
}

