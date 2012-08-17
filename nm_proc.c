#include "nm_proc.h"

static struct proc_dir_entry *nm_proc_root;
static struct proc_dir_entry *nm_entries[__NM_PROC_LEN];

static int read_modelstat(char *page, char **start, off_t off, int count, int *eof, void *data)
{
  int len;
  if (nm_model_details.valid) 
  {
    len = sprintf(page,"Loaded Model: %s [ID: %u]\n"
                       " %u hops\n"
                       " %u endpoints\n",
                       nm_model_details.name, nm_model_details.valid, nm_model_details.n_hops, nm_model_details.n_endpoints);
  } 
  else {
    len = sprintf(page,"No model loaded\n");
  }
  return len;
}

static int write_modelstat(struct file *filp, const char __user *buf, unsigned long len, void *data)
{

  if ( len > sizeof(nm_model_details_t))
    return -EOVERFLOW;

  if (copy_from_user(&nm_model_details,buf,sizeof(nm_model_details_t)))
    return -EINVAL;

  nm_notice(LD_GENERAL,"Loaded model details - Name: %s n_hops: %u n_endpoints: %u\n",
                nm_model_details.name,nm_model_details.n_hops,nm_model_details.n_endpoints);

  return 0;
}


int initialize_proc_interface(void)
{
  int ret;
  ret = 0;
  nm_proc_root = proc_mkdir_mode("net-modeler",0644,NULL);

  if (!nm_proc_root){
    ret = -1;
  } else 
  {
    /*CREATE_ENTRY(pathtable,nm_proc_root);*/
    /*CREATE_ENTRY(hoptable,nm_proc_root);*/
    CREATE_ENTRY(nm_entries,modelstat,nm_proc_root);
  }

  if (ret < 0)
    nm_warn(LD_ERROR,"/proc entry creation failed.");

  return ret;
}

int cleanup_proc_interface(void)
{
  int ret;
  ret= 0;

  remove_proc_entry(stringify(pathtable),nm_proc_root);
  remove_proc_entry(stringify(hoptable),nm_proc_root);
  remove_proc_entry("net-modeler",NULL);

  return 0;
}
