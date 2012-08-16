#include "nm_proc.h"

static struct proc_dir_entry *nm_proc_root;
static struct proc_dir_entry *nm_entries[__NM_PROC_LEN];


int initialize_proc_interface(void)
{
  int ret;
  ret = 0;
  nm_proc_root = proc_mkdir_mode("net-modeler",0644,NULL);

  if (!nm_proc_root)
    ret = -1;
  else if (!(nm_entries[nm_proc_pathtable] = create_proc_entry(stringify(nm_proc_pathtable), 0644, nm_proc_root)))
    ret = -1;
  else if (!(nm_entries[nm_proc_hoptable] = create_proc_entry(stringify(nm_proc_hoptable), 0644, nm_proc_root)))
    ret = -1;

  if (ret < 0)
    nm_warn(LD_ERROR,"/proc entry creation failed.");

  return ret;
}
