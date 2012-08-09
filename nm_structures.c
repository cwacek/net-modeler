#include "nm_log.h"
#include "nm_main.h"
#include "nm_structures.h"

static uint32_t _lookup_path(uint32_t src,uint32_t dst);

nm_packet_t *
nm_packet_init(void *data,uint32_t len,uint32_t src,uint32_t dst)
{
  nm_packet_t *pkt = kmalloc(sizeof(struct nm_packet),GFP_ATOMIC);
  pkt->data = kmalloc(len,GFP_ATOMIC);
  memcpy(pkt->data,data,len);

  pkt->path_idx = 0;
  pkt->path_id = _lookup_path(src,dst);

  return pkt;
}

void nm_packet_free(nm_packet_t *pkt)
{
  kfree(pkt->data);
  kfree(pkt);
  return;
}

static uint32_t 
_lookup_path(uint32_t src,uint32_t dst)
{
  nm_log(NM_WARN,"_lookup_path not implemented\n");
  return 0;
}
