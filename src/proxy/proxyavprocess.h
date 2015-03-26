#ifndef __PROXY_AV_PROCESS_H__
#define __PROXY_AV_PROCESS_H__

#include <sys/select.h>

#define MAX_SINGLE_COUNT 6 /* max single task count */

#define DEFAULT_AV_BUFFER_SIZE (1*1024*1024)

typedef void* PROCESSOR_HANDLE;

typedef uint32_t (*AVProcessWrite) (void *content, uint32_t size, uint32_t nmemb, void *user_data);

typedef struct _ProxyAVSingleBuffer ProxyAVSingleBuffer;
typedef struct _ProxyAVBufferItem ProxyAVBufferItem;
typedef struct _ProxyAVTaskHandle ProxyAVTaskHandle;
typedef struct _ProxyAVProcessor ProxyAVProcessor;

/**
 * ProxyAVSingleBuffer:
 *
 * Use as a callback param to the single task's data-write-function.
 */
struct _ProxyAVSingleBuffer {
  void * single_handle;
  
  char *  buffer;           /* buffer to store cached data*/
  uint32_t  buffer_len;       /* currently allocated buffers length */
  uint32_t  buffer_pos;       /* end of data in buffer*/    
};

struct _ProxyAVTaskHandle {
  /* multi task handle */
  void * multi;

  /* the buffer item now using for storing head */
  ProxyAVBufferItem * head_buf;

  /* the buffer item now using for storing body */
  ProxyAVBufferItem * data_buf;

  /* running single task count */
  uint32_t single_count;

  /* all single task handle */
  ProxyAVSingleBuffer singles[MAX_SINGLE_COUNT];
};

/**
 * ProxyAVProcessor:
 * 
 * The main AVProcessor message.
 */
struct _ProxyAVProcessor {

  /* the target url*/
  char * url;

  /* content length */
  uint32_t content_length;

  /* target data position to download */
  uint32_t start;
  
  /* the user callback func and data */
  AVProcessWrite func;
  void * user_data;

  /* memory queue */
  ProxyQueue *mem_queue;

  /* data queue */
  ProxyQueue *data_queue;

  /* The data buffer we can now read */
  ProxyAVBufferItem * data;

  /* task handle container */
  ProxyAVTaskHandle handle;
};

/**
 * ProxyAVBufferItem:
 * 
 * Buffer item to hold the buffer/data message in the buffer/data queue.
 */
struct _ProxyAVBufferItem {
  char *  buffer;           /* start address of the buffer*/
  uint32_t  buffer_len;     /* length of the buffer */
  uint32_t  data_len;       /* total data length in the buffer */
  uint32_t  offset;         /* data start offset in the buffer */
};

/**
 * proxy_avprocess_create:
 * @url: The target address
 * @func: Callback function user registered for write data back
 * @user_data: user param
 * 
 * Create a av processor to handle url. 
 * 
 * Returns: processor handle.
 */
PROCESSOR_HANDLE
proxy_avprocess_create (char * url, AVProcessWrite func, void * user_data);

/**
 * proxy_avprocess_destroy
 * @handle: processor handle create by @proxy_avprocess_create
 *
 * Destroy the av processor @handle.
 */
void
proxy_avprocess_destroy (PROCESSOR_HANDLE handle);

/**
 * proxy_avprocess_perform:
 * @handle: processor handle create by @proxy_avprocess_create
 *
 * Handles transfers on all the added handles
 * 
 * Returns: CURL_FAIL on error, positive value on total transfers on running, zero (0) on the 
 * return of this function, there is no longer any transfers in progress. 
 */
int32_t
proxy_avprocess_perform (PROCESSOR_HANDLE handle);

/**
 * proxy_avprocess_fdset:
 * @handle: processor handle create by @proxy_avprocess_create
 * 
 * Extracts all file descriptor information from a given @multi_handle
 *
 * Returns: CURL_SUCC on success or CURL_FAIL error.
 */
int32_t
proxy_avprocess_fdset (PROCESSOR_HANDLE handle,fd_set * read_fd_set,
    fd_set * write_fd_set,fd_set * exc_fd_set,int * max_fd);

/**
 * proxy_avprocess_read:
 * @handle: av processor handle
 * @buf: pointer to buffer where data will be written,Must be >= len bytes long
 * @len: maximum number of bytes to read
 *
 * Read data from ring buffer
 *
 * Returns: on success, the number of bytes read is returned , and the file position 
 * is advanced  by this number.It is not an error if this number is
 * smaller than the number of bytes requested.On error,-1 is returned.
 */
int32_t
proxy_avprocess_read (PROCESSOR_HANDLE handle, char * buf, uint32_t len);

#endif
