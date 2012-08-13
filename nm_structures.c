#include "nm_log.h"
#include "nm_main.h"
#include "nm_structures.h"

static uint32_t _lookup_path(uint32_t src,uint32_t dst);

struct nm_obj_cache nm_objects;
void nm_structures_init()
{
  nm_objects.NM_PKT_ALLOC = kmem_cache_create("nm_packets",
                                            sizeof(nm_packet_t),
                                            0, 0, NULL);
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
  pkt->next = pkt->prev = 0;

  return pkt;
}

void nm_packet_free(nm_packet_t *pkt)
{
  nm_free(NM_PKT_ALLOC,pkt);
  return;
}

static uint32_t 
_lookup_path(uint32_t src,uint32_t dst)
{
  nm_log(NM_WARN,"_lookup_path not implemented\n");
  return 0;
}



