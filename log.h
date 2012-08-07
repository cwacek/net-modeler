#ifndef __KERN_NM_LOG__
#define __KERN_NM_LOG__

#ifdef __KERNEL__
#define LOG_FUNC "printk(KERN_INFO "
#else
#define LOG_FUNC "printf("
#endif

#define NM_NOTICE 1
#define NM_INFO 3
#define NM_DEBUG 7
#define NM_LOG_PREFIX "net-modeler: "

#define NM_LOG_LEVEL NM_DEBUG

#define nm_log(level, _fmt, ...) \
  if (level | NM_LOG_LEVEL) \
    LOG_FUNC KERN_INFO NM_LOG_PREFIX _fmt,__VA_ARGS__) 

#endif /*__KERN_NM_LOG__*/
