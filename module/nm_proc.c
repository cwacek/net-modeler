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

static int read_pathtable(char *page, char **start, off_t off, int count, int *eof, void *data)
{
  int len,i,j,k;
  len = 0;

  if (nm_model.info.valid) 
  {
    len += sprintf(page+len,"Pathtable - (%u/%u entries loaded)\n",
                            atomic_read(&nm_model.paths_loaded),
                            NUM_PAIRWISE(nm_model.info.n_endpoints));
    for (i = 0; i < nm_model.info.n_endpoints; i++)
    {
      for (j = 0; j < nm_model.info.n_endpoints; j++)
      {
        if (unlikely(i == j))
          continue;
        if (nm_model._pathtable[i][j].valid != TOS_MAGIC)
          continue;

        len += sprintf(page +len, "Path %u -> %u: [",i,j);
        for (k = 0; k < nm_model._pathtable[i][j].len; k++)
          len += sprintf(page +len, "%u ",nm_model._pathtable[i][j].hops[k]);
        len += sprintf(page + len, "]\n");

      }
    }
  } 
  else {
    len = sprintf(page,"No model loaded\n");
  }
  return len;
}

static int write_pathtable(struct file *filp, const char __user *buf, unsigned long len, void *data)
{
  nm_path_t path;
  size_t hops_offset;
  hops_offset = sizeof(nm_path_t) - sizeof(nm_hop_t *) ;

  if (!nm_model._initialized)
  {
    nm_warn(LD_ERROR,"Must provide 'modelinfo' before attempting to load paths.\n");
    return -EINVAL;
  }

  /** Check if we have enough data. Don't count the pointer
   *  size though.
   **/
  if ( len < (sizeof(nm_path_t) - sizeof(uint32_t*)))
    return -EOVERFLOW;

  /* We copy the first three elements of path_t, but not the hop array. 
   * Memory needs to be alloc-ed before we can copy the hop array, and 
   * we don't know what size to make it yet */
  if (copy_from_user(&path,buf,hops_offset)){
    nm_warn(LD_ERROR,"Failed to copy data from user space\n");
    return -EINVAL;                           
  }

  if (unlikely(ip_int_idx(path.src) >= nm_model.info.n_endpoints 
              || ip_int_idx(path.dst) >= nm_model.info.n_endpoints)){
    nm_warn(LD_ERROR, "Path source '%u' or destination '%u' provided was too large. "
                      "Modelinfo registered only %u endpoints\n",
                      ip_int_idx(path.src), ip_int_idx(path.dst), nm_model.info.n_hops);
    return -EINVAL;
  }
  
  if (unlikely(path.len == 0)){
    nm_warn(LD_ERROR, "Path hop length was zero\n");
    return -EINVAL;
  }

  nm_info(LD_GENERAL,"Registering path from %u to %u @[%u][%u].\n",
                      path.src,path.dst,
                      ip_int_idx(path.src),ip_int_idx(path.dst));

  #define find(src,dst) nm_model._pathtable[ip_int_idx(src)][ip_int_idx(dst)]
  find(path.src,path.dst).src = path.src;
  find(path.src,path.dst).dst = path.dst;
  find(path.src,path.dst).len = path.len;

  /** Mark it as loaded if we haven't loaded it before. **/
  if (find(path.src,path.dst).valid != TOS_MAGIC){
    find(path.src,path.dst).valid = TOS_MAGIC;
    atomic_inc(&nm_model.paths_loaded);
  } else {
    kfree(find(path.src,path.dst).hops);
  }

  find(path.src,path.dst).hops = kmalloc(sizeof(uint32_t) * path.len,GFP_KERNEL);
  copy_from_user((find(path.src,path.dst).hops),buf+hops_offset,sizeof(uint32_t)*path.len);
  #undef find
  
  return len;
}

static int read_hoptable(char *page, char **start, off_t off, int count, int *eof, void *data)
{
  int len,i;
  len = 0;

  if (nm_model.info.valid) 
  {
    len += sprintf(page+len,"Hoptable - (%u/%u entries loaded)\n",
                            atomic_read(&nm_model.hops_loaded),
                            nm_model.info.n_hops);
    for (i=0; i < atomic_read(&nm_model.hops_loaded); i++){
      len += sprintf(page+len, "Hop %u: %u b/ms %u ms\n",i,nm_model._hoptable[i].bw_limit,nm_model._hoptable[i].delay_ms);
    } 
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

  if (hop.id > atomic_read(&nm_model.hops_loaded)+1)
  {
    nm_warn(LD_ERROR, "Hops must be loaded sequentially. Last hop loaded was %u. Requested load of %u.\n",
            atomic_read(&nm_model.hops_loaded),hop.id);
    return -EINVAL;
  }

  nm_model._hoptable[hop.id].bw_limit = hop.bw_limit;
  nm_model._hoptable[hop.id].delay_ms = hop.delay_ms;
  nm_model._hoptable[hop.id].tailexit = 0;

  if (hop.id >= atomic_read(&nm_model.hops_loaded)){
    atomic_inc(&nm_model.hops_loaded);
    nm_info(LD_GENERAL, "Loaded hop %u\n",hop.id);
  } else 
  {
    nm_info(LD_GENERAL, "Reloaded existing hop %u\n",hop.id);
  }

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
    CREATE_ENTRY(nm_entries,pathtable,nm_proc_root);
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

  remove_proc_entry(stringify(pathtable),nm_proc_root);
  remove_proc_entry(stringify(modelinfo),nm_proc_root);
  remove_proc_entry(stringify(hoptable),nm_proc_root);
  remove_proc_entry("net-modeler",NULL);

  return 0;
}
