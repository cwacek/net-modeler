#ifndef __KERN_NM_PROC
#define __KERN_NM_PROC

#include "nm_main.h"
#include "nm_magic.h"
#include <linux/proc_fs.h>

#define pathtable NM_PROC_PATHTABLE
#define hoptable NM_PROC_HOPTABLE
#define modelinfo NM_PROC_MODELSTATS

enum nm_proc_entries{
  pathtable,
  hoptable,
  modelinfo,
  __NM_PROC_LEN,
};


int initialize_proc_interface(void);
int cleanup_proc_interface(void);

#endif
