#include <stdlib.h>
#include <stdint.h>

#include "proxyqueue.h"
#include "proxyinterface.h"
#include "proxyavprocess.h"
#include "proxylog.h"
#include "project.h"

typedef struct _ProxyInterface ProxyInterface;

typedef enum {
  HANDLE_SOCKET,
  HANDLE_CURL,  
}HandleType;

struct _ProxyInterface {
  HandleType handle_type;
  ProxyContentType content_type;

  union {
    jb_socket fd;
    void * curl;
  }handle;
};

/**
 * proxy_interface_url_parse
 * @url: The target address
 *
 * Parse the url to tell the content type.
 *
 * Returns: Content type.
 */
static ProxyContentType
interface_url_parse (char * url)
{
  return PROXY_CONTENT_TYPE_MEDIA;
}

/**
 * proxy_interface_create
 * @url: The target address
 *
 * Create a proxy interface via which can do read and write.
 *
 * Returns: The proxy interface handle.
 */
PROXY_HANDLE
proxy_interface_create (char * url)
{
  ProxyInterface * proxy = NULL;
  ProxyContentType content_type;

  proxy = (ProxyInterface *)malloc(sizeof(ProxyInterface));
  if (proxy == NULL) {
    pri_error ("malloc proxy interface failed\n");
    return 0;
  }

  /* Parse the url to get the content type if needed */
  if ( url != NULL ) {
    pri_debug ("Connecting server [%s] via curl\n", url);
    content_type = interface_url_parse (url);   
  }

  /* Connect server by different way according to the content type */
  if (content_type == PROXY_CONTENT_TYPE_MEDIA) {
    proxy->handle.curl = proxy_avprocess_create (url, NULL, NULL);
    if (proxy->handle.curl == NULL) {
      pri_error("create avprocess failed\n");
      goto creating_failed;
    }
    proxy->handle_type = HANDLE_CURL;
    proxy->content_type = PROXY_CONTENT_TYPE_MEDIA;
  } else if (content_type == PROXY_CONTENT_TYPE_FILE_NORMAL) {
    proxy->handle_type = HANDLE_CURL;
    proxy->content_type = PROXY_CONTENT_TYPE_FILE_NORMAL;
    pri_warning("Not support now\n");
    goto creating_failed;
  } else {
    proxy->handle_type = HANDLE_SOCKET;
    pri_warning("Not support now\n");
    goto creating_failed;
  }

  return (PROXY_HANDLE)proxy;
  
creating_failed:
  if (!proxy) free(proxy);
  return 0;
}

/**
 * proxy_interface_destroy
 * @handle: The interface handle create by @proxy_interface_create
 * 
 * Destroy the proxy interface.
 */
void
proxy_interface_destroy (PROXY_HANDLE handle)
{
  ProxyInterface * proxy = (ProxyInterface *)handle;
  
  p_return_if_fail (proxy != NULL);

  if (proxy->handle_type == HANDLE_CURL) {
    if (proxy->handle.curl != 0) {
      if (proxy->content_type == PROXY_CONTENT_TYPE_MEDIA)
        proxy_avprocess_destroy(proxy->handle.curl);
      else if (proxy->content_type == PROXY_CONTENT_TYPE_FILE_NORMAL)
        pri_warning("Not support now\n");
    }
  } else {
    pri_warning("Not support now\n");
  }

  free (proxy);
}

/**
 * proxy_interface_perform:
 * @handle: The interface handle create by @proxy_interface_create
 * 
 * Handles transfers on all the added handles for curl.
 * 
 * Returns: -1 on error, positive value on total transfers on running, zero (0) on the 
 * return of this function, data receive done. 
 */
int32_t
proxy_interface_perform (PROXY_HANDLE handle)
{
  ProxyInterface * proxy = (ProxyInterface *)handle;
  
  if (proxy->handle_type == HANDLE_CURL) {
    if (proxy->handle.curl != 0) {
      if (proxy->content_type == PROXY_CONTENT_TYPE_MEDIA)
        return proxy_avprocess_perform (proxy->handle.curl);
      else if (proxy->content_type == PROXY_CONTENT_TYPE_FILE_NORMAL)
        pri_warning("Not support now\n");
    }
  } else {
    pri_warning("Not support now\n");
  }

  return -1;
}

/**
 * proxy_interface_fdset:
 * @handle: The interface handle create by @proxy_interface_create
 * 
 * Extracts all file descriptor information.
 * 
 * Returns: 0 on success or -1 error.. 
 */
int32_t
proxy_interface_fdset (PROXY_HANDLE handle,fd_set * read_fd_set,
    fd_set * write_fd_set,fd_set * exc_fd_set,int * max_fd)
{
  ProxyInterface * proxy = (ProxyInterface *)handle;
  
  if (proxy->handle_type == HANDLE_CURL) {
    if (proxy->handle.curl != 0) {
      if (proxy->content_type == PROXY_CONTENT_TYPE_MEDIA)
        return proxy_avprocess_fdset (proxy->handle.curl,\
            read_fd_set, write_fd_set, exc_fd_set, max_fd);
      else if (proxy->content_type == PROXY_CONTENT_TYPE_FILE_NORMAL)
        pri_warning("Not support now\n");
    }
  } else {
    pri_warning("Not support now\n");
  }

  return -1;
}

/**
 * proxy_interface_read
 *
 * @handle: The proxy interface handle
 * @buf: pointer to buffer where data will be written, Must be >= len bytes long.
 * @len: maximum number of bytes to read
 * 
 * Returns: On success, the number of bytes read is returned and the file position 
 * is advanced by this number.It is not an error if this number is  smaller than the 
 * number of bytes requested; this may happen for example because fewer bytes 
 * are actually available right now.On error,  -1 is returned.
 */
int32_t 
proxy_interface_read (PROXY_HANDLE handle, char * buf, uint32_t len)
{
  ProxyInterface * proxy = (ProxyInterface *)handle;
  
  p_return_val_if_fail (proxy != NULL, -1);
  p_return_val_if_fail (buf != NULL, -1);
  
  if (proxy->handle_type == HANDLE_CURL) {
    if (proxy->handle.curl != 0) {
      if (proxy->content_type == PROXY_CONTENT_TYPE_MEDIA)
        return proxy_avprocess_read (proxy->handle.curl, buf, len);
      else if (proxy->content_type == PROXY_CONTENT_TYPE_FILE_NORMAL)
        pri_warning("Not support now\n");
    }
  } else {
    pri_warning("Not support now\n");
  }

  return -1;
}

/**
 * proxy_interface_write
 * @handle: The proxy interface handle
 * @buf: pointer to data to be written.
 * @len: length of data to be written to the interface.
 * 
 * Returns: 0 on success (entire buffer sent), nonzero on error.
 */
int32_t
proxy_interface_write (PROXY_HANDLE handle, const char * buf, uint32_t len)
{
  p_return_val_if_fail (handle != 0, -1);
  p_return_val_if_fail (buf != NULL, -1);

  return 0;
}
