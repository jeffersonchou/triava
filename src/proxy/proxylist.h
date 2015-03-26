#ifndef __PROXY_LIST_H__
#define __PROXY_LIST_H__

typedef struct _ProxyList ProxyList;

struct _ProxyList
{
  void      *data;
  ProxyList *next;
  ProxyList *prev;
};

/**
 * proxy_list_malloc:
 *
 * Allocates space for one #ProxyList element. It is called by
 * g_list_append(), and so is rarely used on its own.
 *
 * Returns: a pointer to the newly-allocated #ProxyList element
 **/
ProxyList *
proxy_list_malloc (void);

/**
 * g_list_free: 
 * @list: a #ProxyList
 *
 * Frees all of the memory used by a #ProxyList.
 * The freed elements are returned to the slice allocator.
 *
 * If list elements contain dynamically-allocated memory, you should
 * either use g_list_free_full() or free them manually first.
 */
void
proxy_list_free (ProxyList *list);

/**
 * proxy_list_append:
 * @list: a pointer to a #ProxyList
 * @data: the data for the new element
 *
 * Adds a new element on to the end of the list.
 *
 * Note that the return value is the new start of the list,
 * if @list was empty; make sure you store the new value.
 *
 * Returns: either @list or the new start of the #ProxyList if @list was %NULL
 */
ProxyList *
proxy_list_append (ProxyList * list, void * data);

/**
 * proxy_list_last:
 * @list: any #ProxyList element
 *
 * Gets the last element in a #ProxyList.
 *
 * Returns: the last element in the #ProxyList,
 *     or %NULL if the #ProxyList has no elements
 */
ProxyList *
proxy_list_last (ProxyList *list);

/**
 * proxy_list_free_1:
 * @list: a #ProxyList element
 *
 * Frees one #ProxyList element.
 * It is usually used after g_list_remove_link().
 */
void
proxy_list_free_1 (ProxyList *list);

#endif
