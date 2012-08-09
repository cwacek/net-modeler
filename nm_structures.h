#ifndef __KERN_NM_STRUCTURES
#define __KERN_NM_STRUCTURES


struct nm_packet {
  void *data;
  uint32_t len;
  uint32_t path_id;
  uint32_t path_idx;
};

typedef struct nm_packet nm_packet_t;

/** Initialize a packet object with packet _data_, packet length
 * _len_ and src and dst appropriately 
 **/
nm_packet_t * 
nm_packet_init(void * data, uint32_t len, uint32_t src, uint32_t dst);

/** Free a packet **/
void nm_packet_free(nm_packet_t *pkt);

#endif /*__KERN_NM_STRUCTURES*/
