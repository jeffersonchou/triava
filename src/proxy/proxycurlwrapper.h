#ifndef __PROXY_CURL_WRAPPER_H__
#define __PROXY_CURL_WRAPPER_H__

#include <stdint.h>

typedef struct _CurlMultiTaskInfo CurlMultiTaskInfo;
typedef struct _CurlRange CurlRange;

#define CURL_MAX_TASK_NUM  10
#define CURL_MIN_TASK_NUM  1
#define CURL_SUCC          0
#define CURL_FAIL          -1

typedef uint32_t MULTI_HANDLE;
typedef uint32_t SINGLE_HANDLE;
typedef struct _CurlTaskHandle REGULAR_HANDLE;

typedef uint32_t (*CurlTaskWrite) (void *content, uint32_t size, uint32_t nmemb, void *user_data);

struct _CurlMultiTaskInfo {
  uint32_t count;

  CurlTaskWrite write_func;
  
  void *user_data[CURL_MAX_TASK_NUM];  
};

struct _CurlRange {
  double start;
  double end;
};

struct _CurlTaskHandle {
  MULTI_HANDLE multi_handle;

  uint32_t single_count;
  SINGLE_HANDLE single_handle[CURL_MAX_TASK_NUM];
};

/**
 * proxy_curl_get_download_size:
 * @url: The target address
 *
 * Get the target @url download length
 *
 * Returns: CURL_FAIL on error or others on success.
 */
double
proxy_curl_get_download_size (char * url);

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
proxy_curl_single_obtain_header (char * url, CurlTaskWrite func, void * data);

/**
 * proxy_curl_single_task_create:
 *
 * Create a easy task, caller responsible for destroying the task
 *
 * Returns: easy task handle
 */
SINGLE_HANDLE
proxy_curl_single_task_create ();

/**
 * proxy_curl_single_task_destroy:
 * @handle: easy task handle
 *
 * Create a easy task, caller responsible for destroying the task
 */
void
proxy_curl_single_task_destroy (SINGLE_HANDLE handle);

/**
 * proxy_curl_multi_task_create:
 *
 * Create a multi task, caller responsible for destroying the task
 *
 * Returns: multi task handle
 */
MULTI_HANDLE
proxy_curl_multi_task_create ();

/**
 * proxy_curl_multi_task_destroy:
 * @handle: multi task handle
 *
 * Create a multi task, caller responsible for destroying the task
 *
 * Returns: multi task handle
 */
void
proxy_curl_multi_task_destroy (MULTI_HANDLE handle);

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
proxy_curl_multi_add_single (MULTI_HANDLE multi_handle, SINGLE_HANDLE single_handle);

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
proxy_curl_multi_remove_single (MULTI_HANDLE multi_handle, SINGLE_HANDLE single_handle);

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
    fd_set * write_fd_set,fd_set * exc_fd_set,int * max_fd);

int32_t
proxy_curl_multi_perform_async (MULTI_HANDLE handle, int32_t * running_handles);

/**
 * proxy_curl_single_set_url:
 * @handle:single task handle 
 * @url: The target address
 *
 * Set url to the single task
 */
void
proxy_curl_single_set_url (SINGLE_HANDLE handle, char * url);

/**
 * proxy_curl_single_set_range:
 * @handle:single task handle 
 * @range: the data range
 *
 * Set url to the single task
 */
void
proxy_curl_single_set_range (SINGLE_HANDLE handle, char * range);

/**
 * proxy_curl_single_opt_header:
 * @handle:single task handle 
 * @func: curl operation header function
 * @data: curl operation header data, can be null
 *
 * Seting callback for writing received headers and data pointer to pass to the header callback.
 */
void
proxy_curl_single_opt_header (SINGLE_HANDLE handle, CurlTaskWrite func, void * data);

/**
 * proxy_curl_single_opt_body:
 * @handle:single task handle 
 * @func: curl operation data write function
 * @data: curl operation body data
 *
 * Seting callback for writing data and data pointer to pass to the write callback.
 */
void
proxy_curl_single_opt_body (SINGLE_HANDLE handle, CurlTaskWrite func, void * data);

#endif
