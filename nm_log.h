#ifndef __KERN_NM_LOG__
#define __KERN_NM_LOG__

#define NM_WARN 1
#define NM_NOTICE 3
#define NM_INFO 7
#define NM_DEBUG 15
#define NM_LOG_PREFIX "net-modeler:"

#define nm_loglevel_string(lev) \
   ((lev == NM_DEBUG ) ? "NM_DEBUG" :  \
   (lev == NM_INFO) ? "NM_INFO" : \
   (lev == NM_NOTICE) ? "NM_NOTICE" : \
   (lev == NM_WARN) ? "NM_WARN" : "NM_UNDEF")

#define NM_LOG_LEVEL NM_DEBUG

#ifdef __KERNEL__
#define nm_log(level, _fmt, ...) \
  if (level & NM_LOG_LEVEL) \
    printk(KERN_INFO NM_LOG_PREFIX "%s: " _fmt, nm_loglevel_string(level), ##__VA_ARGS__) 
#else
#define nm_log(level, _fmt, ...) \
  if (level & NM_LOG_LEVEL) \
    printf(NM_LOG_PREFIX _fmt,##__VA_ARGS__) 
#endif

#define log_func_entry if (NM_LOG_LEVEL == NM_DEBUG ) printk(KERN_EMERG "Entered %s",__func__)

#endif /*__KERN_NM_LOG__*/
