#ifndef __PROXY_INTERFACE_H__
#define __PROXY_INTERFACE_H__

typedef uint32_t PROXY_HANDLE;

typedef enum {
  PROXY_CONTENT_TYPE_NONE        = 0,
  PROXY_CONTENT_TYPE_MEDIA       = 1,
  PROXY_CONTENT_TYPE_FILE_NORMAL = 2,
}ProxyContentType;

/**
 * proxy_interface_create
 * @url: The target address
 *
 * Create a proxy interface via which can do read and write.
 *
 * Returns: The proxy interface handle.
 */
PROXY_HANDLE
proxy_interface_create (char * url);

/**
 * proxy_interface_destroy
 * @handle: The interface handle create by @proxy_interface_create
 * 
 * Destroy the proxy interface.
 */
void
proxy_interface_destroy (PROXY_HANDLE handle);

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
proxy_interface_perform (PROXY_HANDLE handle);

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
    fd_set * write_fd_set,fd_set * exc_fd_set,int * max_fd);

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
proxy_interface_read (PROXY_HANDLE handle, char * buf, int32_t len);

/**
 * proxy_interface_write
 * @handle: The proxy interface handle
 * @buf: pointer to data to be written.
 * @len: length of data to be written to the interface.
 * 
 * Returns: 0 on success (entire buffer sent), nonzero on error.
 */
int32_t
proxy_interface_write (PROXY_HANDLE handle, const char * buf, uint32_t len);

#endif
