#ifndef __KERN_NM_LOG__
#define __KERN_NM_LOG__

#define NM_WARN warn
#define NM_NOTICE notice
#define NM_INFO info
#define NM_DEBUG debug
#define NM_WARN_ID 1
#define NM_NOTICE_ID 3
#define NM_INFO_ID 7
#define NM_DEBUG_ID 15

#define NM_LOG_PREFIX "net-modeler:"

#define NM_LOG_LEVEL NM_NOTICE_ID

#define nm_loglevel_string(lev) \
   ((lev == NM_DEBUG_ID ) ? "NM_DEBUG" :  \
   (lev == NM_INFO_ID) ? "NM_INFO" : \
   (lev == NM_NOTICE_ID) ? "NM_NOTICE" : \
   (lev == NM_WARN_ID) ? "NM_WARN" : "NM_UNDEF")

#define stringify(x) #x

#if NM_DEBUG_ID <= NM_LOG_LEVEL 
#define nm_debug(_fmt, ...)  printk(KERN_INFO NM_LOG_PREFIX "%s: " _fmt, stringify(NM_DEBUG), ##__VA_ARGS__) 
#else
#define nm_debug(_fmt, ...) ;
#endif

#if NM_INFO_ID <= NM_LOG_LEVEL 
#define nm_info(_fmt, ...)  printk(KERN_INFO NM_LOG_PREFIX "%s: " _fmt, stringify(NM_INFO), ##__VA_ARGS__) 
#else
#define nm_info(_fmt, ...) ;
#endif

#if NM_NOTICE_ID <= NM_LOG_LEVEL 
#define nm_notice(_fmt, ...)  printk(KERN_INFO NM_LOG_PREFIX "%s: " _fmt, stringify(NM_NOTICE),##__VA_ARGS__) 
#else
#define nm_notice(_fmt, ...) ;
#endif

#if NM_WARN_ID <= NM_LOG_LEVEL 
#define nm_warn(_fmt, ...)  printk(KERN_INFO NM_LOG_PREFIX "%s: " _fmt, stringify(NM_WARN), ##__VA_ARGS__) 
#else
#define nm_warn(_fmt, ...) ;
#endif

#define str_concat(x,y) x ## y

#define nm_log(level, _fmt, ...) \
    str_concat(nm_,level)(_fmt, ##__VA_ARGS__)
/*  //if (level & NM_LOG_LEVEL) \
    //printk(KERN_INFO NM_LOG_PREFIX "%s: " _fmt, nm_loglevel_string(level), ##__VA_ARGS__) 
    */


#define log_func_entry if (NM_LOG_LEVEL == NM_DEBUG_ID ) printk(KERN_EMERG "Entered %s",__func__)

#endif /*__KERN_NM_LOG__*/
