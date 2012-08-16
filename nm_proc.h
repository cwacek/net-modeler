#ifndef __KERN_NM_PROC
#define __KERN_NM_PROC

#include "nm_main.h"
#include <linux/proc_fs.h>

#define nm_proc_pathtable NM_PROC_PATHTABLE
#define nm_proc_hoptable NM_PROC_HOPTABLE

enum nm_proc_entries{
  nm_proc_pathtable,
  nm_proc_hoptable,
  __NM_PROC_LEN,
};

int initialize_proc_interface(void);

#endif
