#include <string.h>
#include "curl.h"
#include "proxycurlwrapper.h"
#include "proxylog.h"

/**
 * proxy_curl_global_init
 *
 * Global libcurl initialisation and internal initialize
 * 
 * Returns: CURL_SUCC on success or CURL_FAIL on any error.
 */
int32_t
proxy_curl_init ()
{
  if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK) {
    pri_error ("curl global init failed\n");
    return CURL_FAIL;
  }

  return CURL_SUCC;
}

/**
 * proxy_curl_uninit
 *
 * global libcurl cleanup and release resources internal
 * 
 * Returns: CURL_SUCC on success or CURL_FAIL on any error.
 */
void
proxy_curl_uninit ()
{
  curl_global_cleanup();
}

/**
 * proxy_curl_get_download_size:
 * @url: The target address
 *
 * Get the target @url download length
 *
 * Returns: CURL_FAIL on error or others on success.
 */
double
proxy_curl_get_download_size (char * url)
{
  CURL * handle;
  double size;
  
  p_return_val_if_fail (url != NULL, CURL_FAIL);

  if (!(handle = curl_easy_init())) {
    pri_error ("curl easy init failed\n");
    return CURL_FAIL;
  }

  curl_easy_setopt (handle, CURLOPT_URL, url);
  curl_easy_setopt (handle, CURLOPT_NOBODY, 1L);
  curl_easy_setopt (handle, CURLOPT_FOLLOWLOCATION, 1L);

  if (curl_easy_perform (handle) != CURLE_OK) {
    pri_error ("curl perform failed\n");
    return CURL_FAIL;
  }

  if (curl_easy_getinfo (handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, \
    &size) != CURLE_OK) {
    pri_error ("curl easy getinfo failed\n");
    return CURL_FAIL;
  }

  pri_info ("Got %s download size %f\n", url, size);
  curl_easy_cleanup (handle);

  return size;
}

/**
 * proxy_curl_single_obtain_header:
 * @url: The target address
 * @func: curl operation header function
 * @data: curl operation header data, can be null
 *
 * Try to get the @url header data.
 *
 * Returns: CURL_FAIL on error or others on success.
 */
int32_t
proxy_curl_single_obtain_header (char * url, CurlTaskWrite func, void * data)
{
  CURL * handle;

  p_return_val_if_fail (url != NULL, CURL_FAIL);
  p_return_val_if_fail (func != NULL, CURL_FAIL);
  
  if (!(handle = curl_easy_init())) {
    pri_error ("curl easy init failed\n");
    return CURL_FAIL;
  }

  curl_easy_setopt (handle, CURLOPT_URL, url);
  curl_easy_setopt (handle, CURLOPT_NOBODY, 1L);
  curl_easy_setopt (handle, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt ((CURL *)handle, CURLOPT_HEADERFUNCTION, func);
  curl_easy_setopt ((CURL *)handle, CURLOPT_HEADERDATA, data);
  
  if (curl_easy_perform (handle) != CURLE_OK) {
    pri_error ("curl perform failed\n");
    curl_easy_cleanup (handle);
    return CURL_FAIL;
  }

  curl_easy_cleanup (handle);

  return CURL_SUCC;
}


/**
 * proxy_curl_single_task_create:
 *
 * Create a easy task, caller responsible for destroying the task
 *
 * Returns: easy task handle
 */
SINGLE_HANDLE
proxy_curl_single_task_create ()
{
  return (SINGLE_HANDLE)curl_easy_init();
}

/**
 * proxy_curl_single_task_destroy:
 * @handle: easy task handle
 *
 * Create a easy task, caller responsible for destroying the task
 */
void
proxy_curl_single_task_destroy (SINGLE_HANDLE handle)
{
  p_return_if_fail (handle != 0);
  
  curl_easy_cleanup((CURL *)handle);
}

/**
 * proxy_curl_single_perform
 *
 * Performs the entire request in a blocking manner and returns when done, or if it failed.
 *
 * Returns: CURL_SUCC on success or CURL_FAIL on error.
 */
int32_t
proxy_curl_single_perform (SINGLE_HANDLE handle)
{
  p_return_val_if_fail (handle != 0, CURL_FAIL);
  
  if (curl_easy_perform ((CURL *)handle)){
    pri_error ("easy perfom failed\n");
    return CURL_FAIL;
  }

  return CURL_SUCC;
}

/**
 * proxy_curl_multi_task_create:
 *
 * Create a multi task, caller responsible for destroying the task
 *
 * Returns: multi task handle
 */
MULTI_HANDLE
proxy_curl_multi_task_create ()
{
  return (MULTI_HANDLE)curl_multi_init();
}

/**
 * proxy_curl_multi_task_destroy:
 * @handle: multi task handle
 *
 * Create a multi task, caller responsible for destroying the task
 *
 * Returns: multi task handle
 */
void
proxy_curl_multi_task_destroy (MULTI_HANDLE handle)
{
  p_return_if_fail (handle != NULL);
  
  curl_multi_cleanup((CURL *)handle);
}

/**
 * proxy_curl_multi_perform_sync:
 * @handle: task handle create by @proxy_curl_multi_task_create
 *  
 * Handles transfers on all the added handles, caller will be blocked untile
 * there is no longer any transfers in progress
 *
 * Returns: CURL_SUCC on success or CURL_FAIL on error.
 */
int32_t
proxy_curl_multi_perform_sync (MULTI_HANDLE handle)
{
  int32_t running_handles;
  p_return_val_if_fail (handle != NULL, CURL_FAIL);

  curl_multi_perform ((CURLM *)handle, &running_handles);

  do {
    struct timeval timeout;
    int32_t rc; /* select() return code */
    CURLMcode mc; /* curl_multi_fdset() return code */

    fd_set fdread;
    fd_set fdwrite;
    fd_set fdexcep;
    int32_t maxfd = -1;

    long curl_timeo = -1;

    FD_ZERO (&fdread);
    FD_ZERO (&fdwrite);
    FD_ZERO (&fdexcep);

    /* set a suitable timeout to play around with */
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    curl_multi_timeout ((CURLM *)handle, &curl_timeo);
    if(curl_timeo >= 0) {
      timeout.tv_sec = curl_timeo / 1000;
      if(timeout.tv_sec > 1)
        timeout.tv_sec = 1;
      else
        timeout.tv_usec = (curl_timeo % 1000) * 1000;
    }

    /* get file descriptors from the transfers */
    mc = curl_multi_fdset ((CURLM *)handle, &fdread, &fdwrite, &fdexcep, &maxfd);
    if(mc != CURLM_OK) {
      pri_error ("curl_multi_fdset() failed, code %d.\n", mc);
      return CURL_FAIL;
    }

    /* On success the value of maxfd is guaranteed to be >= -1. We call
       select(maxfd + 1, ...); specially in case of (maxfd == -1) there are
       no fds ready yet so we call select(0, ...) --or Sleep() on Windows--
       to sleep 100ms, which is the minimum suggested value in the
       curl_multi_fdset() doc. */

    if(maxfd == -1) {
#ifdef _WIN32
      Sleep(100);
      rc = 0;
#else
      /* Portable sleep for platforms other than Windows. */
      struct timeval wait = { 0, 100 * 1000 }; /* 100ms */
      rc = select (0, NULL, NULL, NULL, &wait);
#endif
    }
    else {
      /* Note that on some platforms 'timeout' may be modified by select().
         If you need access to the original value save a copy beforehand. */
      rc = select (maxfd+1, &fdread, &fdwrite, &fdexcep, &timeout);
    }

    switch(rc) {
    case -1:
      /* select error */
      return CURL_FAIL;
    case 0:
    default:
      /* timeout or readable/writable sockets */
      curl_multi_perform ((CURLM *)handle, &running_handles);
      break;
    }
  } while(running_handles);

  return CURL_SUCC;
}

int32_t
proxy_curl_multi_perform_async (MULTI_HANDLE handle, int32_t * running_handles)
{
  p_return_val_if_fail (handle != NULL, CURL_FAIL);

  if ( curl_multi_perform ((CURLM *)handle, running_handles) != CURLM_OK)
    return CURL_FAIL;

  return CURLM_OK;
}

/**
 * proxy_curl_multi_add_single:
 * @multi_handle: multi task handle
 * @single_handle: easy task handle
 * 
 * Add a single task to the @multi_handle task
 *
 * Returns: CURL_SUCC on success or CURL_FAIL on any error.
 */
int32_t
proxy_curl_multi_add_single (MULTI_HANDLE multi_handle, SINGLE_HANDLE single_handle)
{
  p_return_val_if_fail (multi_handle != NULL, CURL_FAIL);
  p_return_val_if_fail (single_handle != NULL, CURL_FAIL);
  
  if (CURLM_OK != curl_multi_add_handle ((CURLM *)multi_handle, \
    (CURL *)single_handle)) {
    pri_error ("Adding multi handle failed\n");
    return CURL_FAIL;
  }

  return CURL_SUCC;
}

/**
 * proxy_curl_multi_remove_single:
 * @multi_handle: multi task handle
 * @single_handle: easy task handle
 * 
 * Remove a single task from the @multi_handle task
 *
 * Returns: CURL_SUCC on success or CURL_FAIL on any error.
 */
int32_t
proxy_curl_multi_remove_single (MULTI_HANDLE multi_handle, SINGLE_HANDLE single_handle)
{
  p_return_val_if_fail (multi_handle != NULL, CURL_FAIL);
  p_return_val_if_fail (single_handle != NULL, CURL_FAIL);
  
  if (CURLM_OK != curl_multi_remove_handle ((CURLM *)multi_handle, \
    (CURL *)single_handle)) {
    pri_error ("Remove multi handle failed\n");
    return CURL_FAIL;
  }

  return CURL_SUCC;
}


/**
 * proxy_curl_multi_add_single:
 * @multi_handle: multi task handle
 * 
 * Extracts file descriptor information from a given @multi_handle
 *
 * Returns: CURL_SUCC on success or CURL_FAIL on error.
 */
int32_t
proxy_curl_multi_fdset (MULTI_HANDLE multi_handle,fd_set * read_fd_set,
    fd_set * write_fd_set,fd_set * exc_fd_set,int * max_fd)
{
  p_return_val_if_fail (multi_handle != NULL, CURL_FAIL);
  
  if (CURLM_OK != curl_multi_fdset((CURLM *)multi_handle, read_fd_set, \
    write_fd_set, exc_fd_set, max_fd)) {
    pri_error ("curl multi fdset failed\n");
    return CURL_FAIL;
  }

  return CURL_SUCC;
}

/**
 * proxy_curl_single_set_url:
 * @handle:single task handle 
 * @url: The target address
 *
 * Set url to the single task
 */
void
proxy_curl_single_set_url (SINGLE_HANDLE handle, char * url)
{
  p_return_if_fail (handle != NULL);
  p_return_if_fail (url != NULL);
    
  curl_easy_setopt ((CURL *)handle, CURLOPT_URL, url);
}

/**
 * proxy_curl_single_set_range:
 * @handle:single task handle 
 * @range: the data range
 *
 * Set url to the single task
 */
void
proxy_curl_single_set_range (SINGLE_HANDLE handle, char * range)
{
  p_return_if_fail (handle != NULL);
  p_return_if_fail (range != NULL);
    
  curl_easy_setopt ((CURL *)handle, CURLOPT_RANGE, range);
}

/**
 * proxy_curl_single_opt_header:
 * @handle:single task handle 
 * @func: curl operation header function
 * @data: curl operation header data, can be null
 *
 * Seting callback for writing received headers and data pointer to pass to the header callback.
 */
void
proxy_curl_single_opt_header (SINGLE_HANDLE handle, CurlTaskWrite func, void * data)
{
  p_return_if_fail (handle != NULL);

  curl_easy_setopt ((CURL *)handle, CURLOPT_HEADERFUNCTION, func);
  curl_easy_setopt ((CURL *)handle, CURLOPT_HEADERDATA, data);
}

/**
 * proxy_curl_single_opt_body:
 * @handle:single task handle 
 * @func: curl operation data write function
 * @data: curl operation body data
 *
 * Seting callback for writing data and data pointer to pass to the write callback.
 */
void
proxy_curl_single_opt_body (SINGLE_HANDLE handle, CurlTaskWrite func, void * data)
{
  p_return_if_fail (handle != NULL);
    
  curl_easy_setopt ((CURL *)handle, CURLOPT_WRITEFUNCTION, func);
  curl_easy_setopt ((CURL *)handle, CURLOPT_WRITEDATA, data);
}

/**
 * proxy_curl_multi_task_create:
 * @handle: The task handle will be store.
 * @url: The target address
 * @info: The task information
 * @range: The data range if needed
 *
 * Create a multi task according to the @info.
 *
 * Returns: CURL_SUCC on success or CURL_FAIL on any error.
 */
int32_t
proxy_curl_regular_task_create (REGULAR_HANDLE * handle, char * url, 
    CurlMultiTaskInfo * info, CurlRange * range)
{
  double download_length = 0.0;
  double piece_start = 0;
  double piece_end = 0;
  double piece_size;
  char piece_range[sizeof(piece_start) + sizeof(piece_end) + 1];
  int32_t i;

  p_return_val_if_fail (handle != NULL, CURL_FAIL);
  p_return_val_if_fail (url != NULL, CURL_FAIL);
  p_return_val_if_fail (info != NULL, CURL_FAIL);

  /* If we get range, we will create task according to the range, otherwise we will get by url */
  if (range && ((range->start <= range->end)||\
    (range->start < 0 || range->end < 0))) {
    pri_error ("Invalid range start = %f, end = %f\n", \
        range->start, range->end);
    return CURL_FAIL;
  } else if (!range) {
    /* No range, get the download length by the url */
    download_length = proxy_curl_get_download_size (url);
    if (download_length <= 0) {
      pri_error ("Cannot get download length\n");
      return CURL_FAIL;
    }
  } else {
    /* Get valid range */
    download_length = range->end - range->start;
    piece_start = range->start;
  }

  if (info->count >= CURL_MAX_TASK_NUM) {
    info->count = CURL_MAX_TASK_NUM;
  } else if (info->count < CURL_MIN_TASK_NUM) {
    info->count = CURL_MIN_TASK_NUM;
  }
  piece_size = download_length/info->count;
  
  pri_info ("%s download_length = %f, task count = %d\n", \
    url, download_length, info->count);
  memset (handle, 0, sizeof (REGULAR_HANDLE));

  /* Create the multi handle */ 
  handle->multi_handle = (MULTI_HANDLE)curl_multi_init ();
  if (handle->multi_handle == 0) {
    pri_error ("curl multi init failed\n");
    return CURL_FAIL;
  }

  /* Create all the easy task according to the task info */
  for (i = 0; i < info->count; i++) {
    CURL * single_handle = NULL;
    single_handle = curl_easy_init ();
    if (single_handle == NULL) {
      pri_error ("curl easy init failed\n");
      goto curl_eays_opt_fail;
    }

    /* Calculate each piece range */
    if (i == info->count) {
      /* The last piece */
      piece_end = download_length;
    } else {
      piece_end = piece_start + piece_size - 1;
    }
    snprintf (piece_range, sizeof(piece_range), "%0.0f-%0.0f", \
        piece_start, piece_end);
    pri_info("piece_range %d range %s\n", i, piece_range);

    curl_easy_setopt (single_handle, CURLOPT_URL, url);
    curl_easy_setopt (single_handle, CURLOPT_RANGE, piece_range);
    curl_easy_setopt (single_handle, CURLOPT_WRITEFUNCTION, info->write_func);
    curl_easy_setopt (single_handle, CURLOPT_WRITEDATA, info->user_data[i]);

    handle->single_handle[i] = (SINGLE_HANDLE) single_handle; 
    handle->single_count++;
    /* Adds a standard easy handle to the multi stack */
    if (curl_multi_add_handle((CURLM *)handle->multi_handle, \
        single_handle) != CURLM_OK) {
      pri_error ("multi add handle failed\n");
      goto curl_eays_opt_fail;
    }

    piece_start += piece_size;
  }

  return CURL_SUCC;
curl_eays_opt_fail:
  for (i = 0; i < handle->single_count; i++) {
    if (handle->single_handle[i] == 0)
        break;
    curl_easy_cleanup ((CURL *)handle->single_handle[i]);
  }
  curl_multi_cleanup ((CURL *)handle->multi_handle);
  return CURL_FAIL;
}

/**
 * proxy_curl_multi_task_destroy:
 * @handle: The handle create by @proxy_curl_regular_task_create
 *
 * Destroy the task @handle.
 */
void
proxy_curl_regular_task_destroy (REGULAR_HANDLE * handle)
{
  int32_t i;
  p_return_if_fail (handle != NULL);

  for (i = 0; i < handle->single_count; i++) {
    if (handle->single_handle[i] == 0)
      break;
    curl_easy_cleanup ((CURL *)handle->single_handle[i]);
  }
  curl_multi_cleanup ((CURL *)handle->multi_handle);
}

/**
 * proxy_curl_regular_perform_sync:
 * @handle: task handle create
 *  
 * Perform a created task to download data
 *
 * Returns: CURL_SUCC on success or CURL_FAIL on any error.
 */
int32_t
proxy_curl_regular_perform_sync (REGULAR_HANDLE * handle)
{
  p_return_val_if_fail (handle != NULL, CURL_FAIL);

  return proxy_curl_multi_perform_sync(handle->multi_handle);
}

