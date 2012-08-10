#ifndef __KERN_NM_STRUCTURES
#define __KERN_NM_STRUCTURES

struct nm_packet {
  void *data;
  uint32_t len;
  uint32_t path_id;
  uint32_t path_idx;
};
typedef struct nm_packet nm_packet_t;

#define NM_PKT_ALLOC nm_packets
#define SOCKADDR_ALLOC sockaddrs
#define MSGHDR_ALLOC msghdrs
#define KVEC_ALLOC kvecs

struct nm_obj_cache {
  struct kmem_cache *NM_PKT_ALLOC;
  struct kmem_cache *SOCKADDR_ALLOC;
  struct kmem_cache *MSGHDR_ALLOC;
  struct kmem_cache *KVEC_ALLOC;
};

#define nm_alloc(name,flags) kmem_cache_alloc(nm_objects.name,flags)
#define nm_free(name,obj) kmem_cache_free(nm_objects.name,obj)
void nm_structures_init(void);
void nm_structures_release(void);
extern struct nm_obj_cache nm_objects;

/** Initialize a packet object with packet _data_, packet length
 * _len_ and src and dst appropriately 
 **/
nm_packet_t * 
nm_packet_init(void * data, uint32_t len, uint32_t src, uint32_t dst);

/** Free a packet **/
void nm_packet_free(nm_packet_t *pkt);


#endif /*__KERN_NM_STRUCTURES*/
