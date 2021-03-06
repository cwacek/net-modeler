#ifndef __KERN_NM_LOG__
#define __KERN_NM_LOG__


/* Specific Formatters */
#define IPH_FMT "[id:%hu src:%pI4 dst:%pI4 proto:%u ttl:%u]"
#define IPH_FMT_DATA(iph) (iph)->id, &(iph)->saddr, &(iph)->daddr, (iph)->protocol, (iph)->ttl
#define PKT_FMT "[ Hop %u of %u (%u) ; sched|prog|cost|wait %u|%u|%u|%u ; ]"
#define PKT_FMT_DATA(pkt) (pkt)->path_idx, (pkt)->path->len, pkt->path->hops[pkt->path_idx], \
                          (pkt)->scheduled_amt, (pkt)->hop_progress, (pkt)->hop_cost, (pkt->hop_tailwait)


#define NM_WARN warn
#define NM_NOTICE notice
#define NM_INFO info
#define NM_DEBUG debug
#define NM_WARN_ID 1
#define NM_NOTICE_ID 3
#define NM_INFO_ID 7
#define NM_DEBUG_ID 15

/** Enabled logging levels **/
#define NM_LOG_LEVEL NM_DEBUG_ID
#define NM_ENABLED_DOMAINS ( LD_GENERAL | LD_ERROR  | LD_SCHEDULE )

#define nm_loglevel_string(lev) \
   ((lev == NM_DEBUG_ID ) ? "NM_DEBUG" :  \
   (lev == NM_INFO_ID) ? "NM_INFO" : \
   (lev == NM_NOTICE_ID) ? "NM_NOTICE" : \
   (lev == NM_WARN_ID) ? "NM_WARN" : "NM_UNDEF")

#define LD_TRACE 0x1 << 0
#define LD_TIMING 0x1 << 1
#define LD_GENERAL 0x1 << 2
#define LD_ERROR 0x1 << 3
#define LD_SCHEDULE 0x1 << 4

#define nm_ld_string(ld) \
   ((ld == LD_TRACE ) ? "TRACE" :  \
   (ld == LD_TIMING) ? "TIMING" : \
   (ld == LD_GENERAL) ? "GENERAL" : \
   (ld == LD_SCHEDULE) ? "SCHEDULE" : \
   (ld == LD_ERROR) ? "ERROR": "UNDEF" )

#define NM_LOG_PREFIX "net-modeler:"


#define stringify(x) #x

#if NM_DEBUG_ID <= NM_LOG_LEVEL 
#define nm_debug(log_ld,_fmt, ...) \
        if (log_ld & NM_ENABLED_DOMAINS) \
          printk(KERN_INFO NM_LOG_PREFIX "%s:%s: " _fmt, \
              nm_ld_string(log_ld), stringify(NM_DEBUG) ,##__VA_ARGS__) 
#else
#define nm_debug(_fmt, ...) ;
#endif

#if NM_INFO_ID <= NM_LOG_LEVEL 
#define nm_info(log_ld, _fmt, ...)   \
    if (log_ld & NM_ENABLED_DOMAINS) \
          printk(KERN_INFO NM_LOG_PREFIX "%s:%s: " _fmt,  \
            nm_ld_string(log_ld), stringify(NM_INFO), ##__VA_ARGS__) 

#else
#define nm_info(_fmt, ...) ;
#endif

#if NM_NOTICE_ID <= NM_LOG_LEVEL 
#define nm_notice(log_ld, _fmt, ...)   \
    if (log_ld & NM_ENABLED_DOMAINS) \
          printk(KERN_INFO NM_LOG_PREFIX "%s:%s: " _fmt,  \
            nm_ld_string(log_ld), stringify(NM_NOTICE), ##__VA_ARGS__) 
#else
#define nm_notice(_fmt, ...) ;
#endif

#if NM_WARN_ID <= NM_LOG_LEVEL 
#define nm_warn(log_ld, _fmt, ...)   \
    if (log_ld & NM_ENABLED_DOMAINS) \
          printk(KERN_EMERG NM_LOG_PREFIX "%s:%s: " _fmt,  \
            nm_ld_string(log_ld), stringify(NM_WARN), ##__VA_ARGS__) 
#else
#define nm_warn(_fmt, ...) ;
#endif

#define log_func_entry nm_debug(LD_TRACE,"Entered %s\n", __func__)

#endif /*__KERN_NM_LOG__*/
