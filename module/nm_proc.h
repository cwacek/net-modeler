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

#define CREATE_ENTRY(container,name,root) \
  if (!(container[name] = create_proc_entry(#name, 0644, root))){ \
    ret = -1; \
  } else { \
    container[name]->write_proc = write_ ## name; \
    container[name]->read_proc = read_ ## name; \
  }


int initialize_proc_interface(void);
int cleanup_proc_interface(void);

#endif
