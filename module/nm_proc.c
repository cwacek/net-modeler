#include "nm_proc.h"

static struct proc_dir_entry *nm_proc_root;
static struct proc_dir_entry *nm_entries[__NM_PROC_LEN];

#define NUM_PAIRWISE(x) ( (x * x) - x)

static int read_modelinfo(char *page, char **start, off_t off, int count, int *eof, void *data)
{
  int len;
  if (nm_model.info.valid) 
  {
    len = sprintf(page,"Loaded Model: %s [ID: %u]\n"
                       " - %u hops [%u loaded]\n"
                       " - %u endpoints [%u/%u paths loaded]\n",
                       nm_model.info.name,
                       nm_model.info.valid,
                       nm_model.info.n_hops, atomic_read(&nm_model.hops_loaded),
                       nm_model.info.n_endpoints,
                       atomic_read(&nm_model.paths_loaded),
                       NUM_PAIRWISE(nm_model.info.n_endpoints));
  } 
  else {
    len = sprintf(page,"No model loaded\n");
  }
  return len;
}

static int write_modelinfo(struct file *filp, const char __user *buf, unsigned long len, void *data)
{
  nm_model_details_t newmodel;

  if ( len > sizeof(nm_model.info))
    return -EOVERFLOW;

  if (copy_from_user(&newmodel,buf,sizeof(nm_model.info)))
    return -EINVAL;

  nm_notice(LD_GENERAL,"Loaded model details - Name: '%s' [ID: %u] n_hops: %u n_endpoints: %u\n",
                newmodel.name,
                newmodel.valid,
                newmodel.n_hops,
                newmodel.n_endpoints);

  nm_notice(LD_GENERAL,"Initializing data structures\n");

  if (nm_model_initialize(&newmodel) < 0)
    return -EINVAL;

  return len;
}

static int read_hoptable(char *page, char **start, off_t off, int count, int *eof, void *data)
{
  int len;
  len = 0;

  if (nm_model.info.valid) 
  {
    len += sprintf(page+len,"Hoptable (%u/%u entries loaded):\n",
                            atomic_read(&nm_model.hops_loaded),
                            nm_model.info.n_hops);
  } 
  else {
    len = sprintf(page,"No model loaded\n");
  }
  return len;
}

struct proc_hop_data {
  uint32_t id;
  uint32_t bw_limit;
  uint32_t delay_ms;
};

static int write_hoptable(struct file *filp, const char __user *buf, unsigned long len, void *data)
{
  struct proc_hop_data hop;

  if (!nm_model._initialized)
  {
    nm_warn(LD_ERROR,"Must provide 'modelinfo' before attempting to load hops.\n");
    return -EINVAL;
  }

  if ( len != sizeof(struct proc_hop_data))
    return -EOVERFLOW;

  if (copy_from_user(&hop,buf,sizeof(struct proc_hop_data)))
    return -EINVAL;

  if (hop.id >= nm_model.info.n_hops){
    nm_warn(LD_ERROR, "Hop ID '%u' provided was too large. Modelinfo registered only %u hops\n",
                      hop.id, nm_model.info.n_hops);
    return -EINVAL;
  }

  nm_model._hoptable[hop.id].bw_limit = hop.bw_limit;
  nm_model._hoptable[hop.id].delay_ms = hop.delay_ms;

  atomic_inc(&nm_model.hops_loaded);

  return len;
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
    CREATE_ENTRY(nm_entries,hoptable,nm_proc_root);
    CREATE_ENTRY(nm_entries,modelinfo,nm_proc_root);
  }

  if (ret < 0)
    nm_warn(LD_ERROR,"/proc entry creation failed.");

  return ret;
}

int cleanup_proc_interface(void)
{
  int ret;
  ret= 0;

  /*remove_proc_entry(stringify(pathtable),nm_proc_root);*/
  remove_proc_entry(stringify(modelinfo),nm_proc_root);
  /*remove_proc_entry(stringify(hoptable),nm_proc_root);*/
  remove_proc_entry("net-modeler",NULL);

  return 0;
}
