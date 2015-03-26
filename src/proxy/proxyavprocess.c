#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "proxyqueue.h"
#include "proxycurlwrapper.h"
#include "proxyavprocess.h"
#include "proxylog.h"

static BOOL avprocess_multi_task_build (ProxyAVProcessor *processor, int32_t count, BOOL need_head);
static ProxyAVBufferItem * avprocess_buffer_item_obtain (ProxyQueue * queue);

static uint32_t
avprocess_data_write (void * content, uint32_t size, uint32_t nmemb, void * user_data)
{
  ProxyAVSingleBuffer * buffer = user_data; 
  uint32_t length = size*nmemb;
  uint32_t free_length = 0;
  uint32_t write_lenth = length;

  p_return_val_if_fail (content != NULL, 0);
  p_return_val_if_fail (user_data != NULL, 0);

  if (buffer->buffer_pos >= buffer->buffer_len) {
    pri_warning ("Cannot write, buffer is full\n");
    return length;
  }

  free_length = buffer->buffer_len  - buffer->buffer_pos;
  if (free_length < length) {
    pri_warning ("Should not happen, buffer not long enough, data will be "\
        "cut off, buffer_len = %u, buffer_pos = %u, data length = %u, free_length = %u\n", \
        buffer->buffer_len, buffer->buffer_pos, length, free_length);
    write_lenth = free_length;
  }
  
  memcpy (buffer->buffer+buffer->buffer_pos, content, write_lenth);
  buffer->buffer_pos += write_lenth;
  
  return size*nmemb;
}

static BOOL
avprocess_header_end (void * content, uint32_t length)
{
  return (length == 2)&&(strcmp(content, "\r\n") == 0);
}

static uint32_t
avprocess_header_write (void * content, uint32_t size, uint32_t nmemb, void * user_data)
{
  ProxyAVProcessor *processor = user_data;
  ProxyAVBufferItem *item;
  char * p;
  uint32_t length = size*nmemb;
  uint32_t free_length = 0;
  uint32_t write_lenth;

  p_return_val_if_fail (content != NULL, 0);
  p_return_val_if_fail (user_data != NULL, 0);

  pri_debug ("Header line: length = %u, %s", length, (char *)content);

  if (!processor->handle.head_buf) {
    pri_error ("No header buffer for storing data\n");
    return length;
  }

  /* Try to get the content length */
  if (processor->content_length == 0 && (p = strstr(content, \
    "Content-Length:")) != NULL) {
    while (*p && isspace(*p)) p++;

    p += 15;

    sscanf(p , "%llu", &processor->content_length);
  }  

  item = processor->handle.head_buf;
  free_length = item->buffer_len - item->data_len;
  write_lenth = (free_length > length) ? length : free_length;
  memcpy (item->buffer + item->data_len, content, write_lenth);
  item->data_len += write_lenth;

  /* Last line of the header data, now push header into data queue */
  if (avprocess_header_end (content, length)) {
    proxy_queue_push_tail(processor->data_queue, item);
    processor->handle.head_buf = NULL;

    if (processor->content_length == 0) {
      pri_warning ("Unknown content length, aborting body\n");
      return length;
    }

    /* Now we can start task to download body data */
    if (!avprocess_multi_task_build(processor, MAX_SINGLE_COUNT, FALSE)) {
      pri_error ("Bulid multi task failed\n");
      return 0;
    }
  }

  return length;
}

static int32_t
avprocess_header_obtain (ProxyAVProcessor * processor, char * url)
{
  ProxyAVBufferItem * item;

  p_return_val_if_fail (processor != NULL, CURL_FAIL);
  p_return_val_if_fail (url != NULL, CURL_FAIL);

  if (!processor->handle.head_buf) {
    item = avprocess_buffer_item_obtain(processor->mem_queue);
    if (item == NULL) {
      pri_error ("Malloc buffer item failed\n");
      return CURL_FAIL;
    }
    processor->handle.head_buf = item;  
  }
  
  if (proxy_curl_single_obtain_header (url, avprocess_header_write, \
    processor) != CURL_SUCC) {
    pri_error ("Getting header failed\n");
    return CURL_FAIL;
  }  

  return CURL_SUCC;
}

static void
avprocess_init (ProxyAVProcessor * processor)
{
  int32_t count;
  ProxyAVSingleBuffer * single;
  
  p_return_if_fail (processor != NULL);

  processor->url = NULL;
  processor->content_length = 0;
  processor->start = 0;
  
  processor->data_queue = proxy_queue_new();
  processor->mem_queue = proxy_queue_new();
  processor->data = NULL;
  
  processor->func = NULL;
  processor->user_data = NULL;  

  /* init the handle */
  processor->handle.multi = 0;
  processor->handle.head_buf = NULL;
  processor->handle.data_buf = NULL;
  processor->handle.single_count = 0;
  for (count = 0; count < MAX_SINGLE_COUNT; count++) {
    single = &processor->handle.singles[count];
    single->single_handle = 0;
    single->buffer = NULL;
    single->buffer_len = 0;
    single->buffer_pos = 0;
  }
}

static ProxyAVBufferItem *
avprocess_buffer_item_malloc (uint32_t size)
{
  ProxyAVBufferItem * item;
  
  item = malloc (sizeof(ProxyAVBufferItem));
  if (item == NULL) {
    pri_error ("Buffer item malloc failed\n");
    return NULL;
  }

  item->buffer = malloc (size);
  if (item->buffer == NULL) {
    pri_error ("Data cach malloc failed\n");
    goto error_out;
  }
  item->buffer_len = size; 
  item->data_len = 0;
  item->offset = 0;
  
  return item;
error_out:
  if (item) free (item);
  return NULL;
}

static void
avprocess_buffer_item_free (ProxyAVBufferItem * item)
{
  p_return_if_fail (item != NULL);

  if (item->buffer) {
    free (item->buffer);
    item->buffer = NULL;
  }

  free (item);
}

static ProxyAVBufferItem *
avprocess_buffer_item_obtain (ProxyQueue * queue)
{
  p_return_val_if_fail (queue != NULL, NULL);

  if (!proxy_queue_is_empty (queue))
    return proxy_queue_pop_head(queue);
  
  return avprocess_buffer_item_malloc(DEFAULT_AV_BUFFER_SIZE);
}

/**
 * avprocess_multi_task_build:
 * @handle: processor handle 
 * @count: how many single task need
 * @do we need the header data
 *
 * Setting up the task to start download the next part data.
 *
 * Returns: TRUE on success and FALSE on error.
 */
static BOOL
avprocess_multi_task_build (ProxyAVProcessor *processor, int32_t count, BOOL need_head)
{
  uint32_t piece_start;
  uint32_t piece_end;
  uint32_t piece_size;
  uint32_t piece_pos = 0;
  uint32_t download_length = 0;
  ProxyAVSingleBuffer *single_buf;
  char piece_range[256];
  uint32_t single_handle;
  ProxyAVBufferItem * item;
  int32_t i;

  p_return_val_if_fail (processor != NULL, FALSE);
  p_return_val_if_fail (count > 0, FALSE);

  /* The content length is unkonwn */
  if (processor->content_length == 0) {
    pri_error ("Unkonwn content length, aborting task bulid\n");
    return FALSE;
  }

  /* calculating how many data we wil download in this task */
  piece_start = processor->start;
  if ((processor->content_length - piece_start) > DEFAULT_AV_BUFFER_SIZE) {
    download_length = DEFAULT_AV_BUFFER_SIZE;
  } else {
    download_length = processor->content_length - piece_start;
  }

  /* refresh next start position */
  processor->start += download_length;

  pri_debug ("This task will download data from pos %u, "\
        "length is %u, next start will be %u\n", piece_start, \
        download_length, processor->start);

  /* If the download_length is less than @DEFAULT_AV_BUFFER_SIZE just use a task */
  if (download_length < DEFAULT_AV_BUFFER_SIZE) {
    count = 1;
  }

  /* Calculta each task download size */
  piece_size = download_length/count;

  /* malloc a buffer item for storing body data */
  item = avprocess_buffer_item_obtain(processor->mem_queue);
  if (item == NULL) {
    pri_error ("Malloc buffer item failed\n");
    return FALSE;
  }
  processor->handle.data_buf = item;

  /* The total data should be received in this task */
  item->data_len = download_length;
  item->offset = 0;
  
  for (i = 0; i < count; i++) {
    single_buf = &processor->handle.singles[i];
    if ((single_handle = proxy_curl_single_task_create()) == 0) {
      pri_error ("Creating single task failed\n");
      goto task_bulid_failed;
    }
    single_buf->single_handle = single_handle;
    processor->handle.single_count++;
    
    if (need_head && i == 0) {
      /* malloc a buffer item for storing header data */
      item = avprocess_buffer_item_obtain(processor->mem_queue);
      if (item == NULL) {
        pri_error ("Malloc buffer item failed\n");
        goto task_bulid_failed;;
      }
      processor->handle.head_buf = item;       
      proxy_curl_single_opt_header(single_handle, avprocess_header_write, item);
    }

    /* Calculate each piece range */
    if (i == count - 1) {
      /* The last piece */
      piece_end = processor->start - 1;
      piece_size = processor->start - piece_start;
    } else {
      piece_end = piece_start + piece_size - 1;
    }
    snprintf (piece_range, sizeof(piece_range), "%u-%u", \
        piece_start, piece_end);
    single_buf->buffer = processor->handle.data_buf->buffer + piece_pos;
    single_buf->buffer_len = piece_size;
    pri_debug ("The %dth piece's range %s, size = %u\n", \
          i, piece_range, piece_size);

    /* Setting single task options */
    proxy_curl_single_set_url(single_handle, processor->url);
    proxy_curl_single_opt_body(single_handle, avprocess_data_write, single_buf);
    proxy_curl_single_set_range(single_handle, piece_range);
    if (need_head && i == 0) {
      /* malloc a buffer item for storing header data */
      item = avprocess_buffer_item_obtain(processor->mem_queue);
      if (item == NULL) {
        pri_error ("Malloc buffer item failed\n");
        goto task_bulid_failed;;
      }
      processor->handle.head_buf = item;       
      proxy_curl_single_opt_header(single_handle, avprocess_header_write, processor);
    }

    if (proxy_curl_multi_add_single(processor->handle.multi, \
        single_handle) != CURL_SUCC) {
      pri_error ("Adding single to multi failed\n");
      goto task_bulid_failed;
    }

    piece_start += piece_size;
    piece_pos += piece_size;
  }

  return TRUE;
task_bulid_failed:
  return FALSE;
}

static void
avprocess_multi_task_free (ProxyAVProcessor *processor)
{
  uint32_t task_handle;
  int32_t i;
  ProxyAVSingleBuffer *single;

  for (i = 0; i < processor->handle.single_count; i++) {
    task_handle = processor->handle.singles[i].single_handle;
    if (task_handle != 0) {
      proxy_curl_multi_remove_single(processor->handle.multi, task_handle);
      proxy_curl_single_task_destroy (task_handle);
    }
  }
  processor->handle.single_count = 0;

  for (i = 0; i < MAX_SINGLE_COUNT; i++) {
    single = &processor->handle.singles[i];
    single->single_handle = 0;
    single->buffer = NULL;
    single->buffer_len = 0;
    single->buffer_pos = 0;
  } 
}

BOOL
avprocess_data_recv_done (ProxyAVProcessor *processor)
{
  uint32_t data_expect_recv = 0;
  uint32_t data_current_recv = 0;
  int32_t i;
  
  p_return_val_if_fail (processor != NULL, FALSE);

  data_expect_recv = processor->handle.data_buf->data_len;
  
  for (i = 0; i < processor->handle.single_count; i++) {
    data_current_recv += processor->handle.singles[i].buffer_pos;
  }

  return (data_expect_recv == data_current_recv);
}

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
proxy_avprocess_create (char * url, AVProcessWrite func, void * user_data)
{
  ProxyAVProcessor *processor;
  uint32_t multi_handle;
  
  p_return_val_if_fail (url != NULL, 0);

  processor = (ProxyAVProcessor *)malloc(sizeof(ProxyAVProcessor));
  if (processor == NULL) {
    pri_error ("malloc AV Processor failed\n");
    return 0;
  }

  avprocess_init (processor);
  processor->url = strdup(url);
  processor->func = func;
  processor->user_data = user_data;

  if ((multi_handle = proxy_curl_multi_task_create()) == 0) {
    pri_error ("Creating multi task failed\n");
    return FALSE;
  }
  processor->handle.multi = multi_handle;

  /* Try to get the requst header at first, the body receive will after that */
  if (avprocess_header_obtain (processor, url) != CURL_SUCC) {
    pri_error ("Getting header failed\n");
    goto avprocessor_create_failed;
  }   

  return (PROCESSOR_HANDLE)processor; 
avprocessor_create_failed:
  proxy_avprocess_destroy((PROCESSOR_HANDLE)processor);  
  return 0;  
}

/**
 * proxy_avprocess_destroy
 * @handle: processor handle create by @proxy_avprocess_create
 *
 * Destroy the av processor @handle.
 */
void
proxy_avprocess_destroy (PROCESSOR_HANDLE handle)
{
  ProxyAVProcessor *processor = (ProxyAVProcessor *)handle;;
  ProxyAVBufferItem * item;

  p_return_if_fail (processor != NULL);
  
  avprocess_multi_task_free(processor);

  if (processor->handle.multi != 0) {
    proxy_curl_multi_task_destroy (processor->handle.multi);
    processor->handle.multi = 0;
  }

  /* Now free all buffer items */
  if (processor->handle.head_buf) {
    avprocess_buffer_item_free (processor->handle.head_buf);
    processor->handle.head_buf = NULL;
  }
  if (processor->handle.data_buf) {
    avprocess_buffer_item_free (processor->handle.data_buf);
    processor->handle.data_buf = NULL;
  }
  if (processor->mem_queue) {
    item = proxy_queue_pop_head (processor->mem_queue);
    while (item) {
      avprocess_buffer_item_free (item);
      item = proxy_queue_pop_head (processor->mem_queue);
    }
    proxy_queue_free (processor->mem_queue);
  }
  if (processor->data_queue) {
    item = proxy_queue_pop_head (processor->data_queue);
    while (item) {
      avprocess_buffer_item_free (item);
      item = proxy_queue_pop_head (processor->data_queue);
    }
    proxy_queue_free (processor->data_queue);
  }
  
  free (processor);
}

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
proxy_avprocess_perform (PROCESSOR_HANDLE handle)
{
  ProxyAVProcessor *processor = (ProxyAVProcessor *)handle;
  int32_t running_handles;
  
  p_return_val_if_fail (processor != NULL, CURL_FAIL);

  if (proxy_curl_multi_perform_async(processor->handle.multi, \
      &running_handles) != CURL_SUCC) {
    pri_error ("multi perform failed\n");
    return CURL_FAIL;
  }

  /* No longer any transfers in progress, now pushing buffer item to data queue */
  if (running_handles == 0) {
    /* Save the header buffer item to the data queue if needed */
    if (processor->handle.head_buf) {
      proxy_queue_push_tail (processor->data_queue, \
        processor->handle.head_buf);
      processor->handle.head_buf = NULL;
    }

    /* Save the body buffer item to the data queue if needed */
    if (processor->handle.data_buf) {
      if (!avprocess_data_recv_done(processor)) {
        pri_warning ("Expect data not receive done.\n");
      }
      proxy_queue_push_tail (processor->data_queue, \
        processor->handle.data_buf);
      processor->handle.data_buf = NULL;
    }

    /* Data content receive done, no more task needed */
    if (processor->start >= processor->content_length 
        && processor->content_length > 0) {
      pri_debug ("All content download done\n");
      return 0;
    }

    /* Now rebuild and start data downloading task without asking for header data */
    avprocess_multi_task_free (processor);    
    if (!avprocess_multi_task_build (processor, MAX_SINGLE_COUNT, FALSE)) {
      pri_error ("Bulid multi task failed\n");
      return 0;
    }
    if (proxy_curl_multi_perform_async(processor->handle.multi, \
        &running_handles) != CURL_SUCC) {
      pri_error ("multi perform failed\n");
      return CURL_FAIL;
    }
  }

  return running_handles; 
}

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
    fd_set * write_fd_set,fd_set * exc_fd_set,int * max_fd)
{
  ProxyAVProcessor *processor = (ProxyAVProcessor *)handle;

  p_return_val_if_fail (processor != NULL, CURL_FAIL);

  return proxy_curl_multi_fdset (processor->handle.multi, \
      read_fd_set, write_fd_set, exc_fd_set, max_fd);
}

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
proxy_avprocess_read (PROCESSOR_HANDLE handle, char * buf, int32_t len)
{
  ProxyAVProcessor * processor = (ProxyAVProcessor *)handle;
  ProxyAVBufferItem * item;
  uint32_t remain_length;
  uint32_t read_length;
  BOOL exhausted = FALSE;

  p_return_val_if_fail (processor != NULL, CURL_FAIL);
  p_return_val_if_fail (buf != NULL, CURL_FAIL);

  if (!processor->data)
    processor->data = proxy_queue_pop_head (processor->data_queue);
    
  /* No data available now */
  if (!processor->data)
    return 0;

  item = processor->data;
  if (item->offset >= item->data_len) {
    exhausted = TRUE;
    goto Exhausted;
  }

  remain_length = item->data_len - item->offset;  
  if (remain_length > len) {
    read_length = len;
  } else {
    read_length = remain_length;
    exhausted = TRUE;
  }

  memcpy (buf, item->buffer+item->offset, read_length);  
  item->offset += read_length;

Exhausted:  
  if (exhausted) {
    proxy_queue_push_tail(processor->mem_queue, item);
    processor->data = NULL;
  }

  return read_length;
}

