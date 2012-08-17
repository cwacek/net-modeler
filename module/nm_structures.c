#include "nm_log.h"
#include "nm_main.h"
#include "nm_structures.h"

static inline uint32_t _lookup_path(uint32_t src,uint32_t dst);

/** Global Objects **/
struct nm_obj_cache nm_objects;
nm_model_t nm_model = {{ 0, "None", 0, 0},
                      0, 0, ATOMIC_INIT(0),ATOMIC_INIT(0), 0};

void nm_structures_init()
{
  nm_objects.NM_PKT_ALLOC = kmem_cache_create("nm_packets",
                                            sizeof(nm_packet_t),
                                            0, SLAB_HWCACHE_ALIGN, NULL);
  nm_objects.SOCKADDR_ALLOC = kmem_cache_create("sockaddrs",
                                            sizeof(struct sockaddr_in),
                                            0, 0, NULL);
  nm_objects.MSGHDR_ALLOC = kmem_cache_create("msghdrs",
                                            sizeof(struct msghdr),
                                            0, SLAB_HWCACHE_ALIGN, NULL);
  nm_objects.KVEC_ALLOC = kmem_cache_create("kvecs",
                                            sizeof(struct kvec),
                                            0, SLAB_HWCACHE_ALIGN, NULL);
}

void nm_structures_release()
{
  kmem_cache_destroy(nm_objects.NM_PKT_ALLOC);
  kmem_cache_destroy(nm_objects.SOCKADDR_ALLOC);
  kmem_cache_destroy(nm_objects.MSGHDR_ALLOC);
  kmem_cache_destroy(nm_objects.KVEC_ALLOC);
}

nm_packet_t *
nm_packet_init(struct nf_queue_entry *data,uint32_t src,uint32_t dst)
{
  nm_packet_t *pkt = nm_alloc(NM_PKT_ALLOC,GFP_ATOMIC);
  pkt->data = data;
  pkt->path_idx = 0;
  pkt->path_id = _lookup_path(src,dst);
  pkt->hop_progress = pkt->flags = 0;
  pkt->next = 0;

  return pkt;
}

void nm_packet_free(nm_packet_t *pkt)
{
  nm_free(NM_PKT_ALLOC,pkt);
  return;
}

static inline uint32_t 
_lookup_path(uint32_t src,uint32_t dst)
{
  /*nm_warn(LD_ERROR,"_lookup_path not implemented\n");*/
  return 0;
}

int nm_model_initialize(nm_model_details_t *modinfo)
{
  int i;

  if (!modinfo->valid)
    return -1;

  if (nm_model._initialized){
    nm_info(LD_GENERAL, "Freeing old model to make room for new one\n");
    for (i = 0; i < nm_model.info.n_endpoints; i++)
    {
      kfree(nm_model._pathtable[i]);
    }
    kfree(nm_model._pathtable);
    kfree(nm_model._hoptable);
  }

  /** Now we switch to the new model and reload **/
  nm_info(LD_GENERAL,"Allocating space for new model\n");
  memcpy(&nm_model.info,modinfo,sizeof(nm_model_details_t));

  nm_model._pathtable = kmalloc(sizeof(nm_path_t *)*nm_model.info.n_endpoints,GFP_KERNEL);
  nm_model._initialized = 1;
  for (i = 0; i < nm_model.info.n_endpoints; i++)
  {
    nm_model._pathtable[i] = kmalloc(sizeof(nm_path_t),GFP_KERNEL);   
  }

  nm_model._hoptable = kmalloc(sizeof(nm_hop_t)*nm_model.info.n_hops,GFP_KERNEL);
  atomic_set(&nm_model.hops_loaded,0);
  atomic_set(&nm_model.paths_loaded,0);

  nm_info(LD_GENERAL,"Model memory allocation successful");

  return 0;
}


