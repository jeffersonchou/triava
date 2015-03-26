#ifndef __PROXY_LOG_H__
#define __PROXY_LOG_H__

#include <stdio.h>
#include <stddef.h>

#if     __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 8)
#define P_GNUC_EXTENSION __extension__
#else
#define P_GNUC_EXTENSION
#endif

#if defined(__GNUC__) && (__GNUC__ > 2) && defined(__OPTIMIZE__)
#define _P_BOOLEAN_EXPR(expr)                   \
 P_GNUC_EXTENSION ({                            \
   int _p_boolean_var_;                         \
   if (expr)                                    \
      _p_boolean_var_ = 1;                      \
   else                                         \
      _p_boolean_var_ = 0;                      \
   _p_boolean_var_;                             \
})
#define P_LIKELY(expr) (__builtin_expect (_P_BOOLEAN_EXPR(expr), 1))
#define P_UNLIKELY(expr) (__builtin_expect (_P_BOOLEAN_EXPR(expr), 0))
#else
#define P_LIKELY(expr) (expr)
#define P_UNLIKELY(expr) (expr)
#endif

#if !(defined (P_STMT_START) && defined (P_STMT_END))
#define P_STMT_START  do
#define P_STMT_END    while (0)
#endif

#define PRI_LOG_PRINT
#ifdef PRI_LOG_PRINT
#define COLOR_BLACK			"\033[0;30m"
#define COLOR_RED			"\033[0;31m"
#define COLOR_LIGHT_RED		"\033[1;31m"
#define COLOR_GREEN			"\033[0;32m"
#define COLOR_LIGHT_GREEN	"\033[1;32m"
#define LOG_TAG_ERROR	"Error"
#define LOG_TAG_WARNING	"Warning"
#define LOG_TAG_INFO	"Info"
#define LOG_TAG_DEBUG	"Debug"
#define LOG_TAG_LOG		"Log"
#define LOG_TAG_FIXME	"Fixme"
#define LOG_TAG_TRACE	"Trace"
typedef enum 
{
	LOG_LEVEL_NONE,
	LOG_LEVEL_ERROR,
	LOG_LEVEL_WARNING,
	LOG_LEVEL_INFO,
	LOG_LEVEL_DEBUG,
	LOG_LEVEL_LOG,
	LOG_LEVEL_FIXME,
	LOG_LEVEL_TRACE,
	LOG_LEVEL_COUNT
}LogLevel;
static unsigned int log_level_min = LOG_LEVEL_INFO;
static inline void log_level_set (unsigned int level){log_level_min = level;}
#define LOG_PRI_LEVEL(tag, level, color, fmt, args...) do{ \
        if (level <= log_level_min)  { \
            fprintf (stderr, color"[%s][%s():%d]"fmt, tag, __func__, __LINE__, ##args); \
            fprintf (stderr, "\e[0m"); \
        } \
}while(0)
#define pri_error(...)      LOG_PRI_LEVEL(LOG_TAG_ERROR,    LOG_LEVEL_ERROR,    COLOR_RED,          __VA_ARGS__)
#define pri_warning(...)    LOG_PRI_LEVEL(LOG_TAG_WARNING,  LOG_LEVEL_WARNING,  COLOR_LIGHT_RED,    __VA_ARGS__)
#define pri_info(...)       LOG_PRI_LEVEL(LOG_TAG_INFO,     LOG_LEVEL_INFO,     COLOR_BLACK,        __VA_ARGS__)
#define pri_debug(...)      LOG_PRI_LEVEL(LOG_TAG_DEBUG,    LOG_LEVEL_DEBUG,    COLOR_BLACK,        __VA_ARGS__)
#define pri_log(...)        LOG_PRI_LEVEL(LOG_TAG_LOG,      LOG_LEVEL_LOG,      COLOR_BLACK,        __VA_ARGS__)
#define pri_fixme(...)      LOG_PRI_LEVEL(LOG_TAG_FIXME,    LOG_LEVEL_FIXME,    COLOR_BLACK,        __VA_ARGS__)
#define pri_trace(...)      LOG_PRI_LEVEL(LOG_TAG_TRACE,    LOG_LEVEL_TRACE,    COLOR_BLACK,        __VA_ARGS__)
#else
#define pri_error(...)
#define pri_warning(...)
#define pri_info(...)
#define pri_debug(...)
#define pri_log(...)
#define pri_fixme(...)
#define pri_trace(...)
#endif

#define p_return_if_fail(expr)		P_STMT_START{			\
     if P_LIKELY(expr) { } else       					\
       {								\
	 pri_warning ("now return for illegal operation\n");				\
	 return;							\
       };				}P_STMT_END

#define p_return_val_if_fail(expr,val)	P_STMT_START{			\
     if P_LIKELY(expr) { } else						\
       {								\
     pri_warning ("now return for illegal operation\n");              \
	 return (val);							\
       };				}P_STMT_END

#endif
