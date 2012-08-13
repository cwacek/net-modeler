#ifndef __KERN_NM_STRUCTURES
#define __KERN_NM_STRUCTURES

/** The packet flagged this way is in the middle of an incomplete hop **/
#define NM_FLAG_HOP_INCOMPLETE 1

struct nm_packet {
  struct nf_queue_entry *data;
  /** The ID of the path we're looking for */
  uint32_t path_id;
  /** The hop index on the current path */
  uint32_t path_idx;               
  /* Keep track of how far we've scheduled for hops
   * longer than the max scheduler */
  uint16_t hop_progress; 
  uint16_t flags;
  /** Linked list helpers **/
  struct nm_packet *next;
  struct nm_packet *prev;
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
nm_packet_init(struct nf_queue_entry *data,  uint32_t src, uint32_t dst);

/** Free a packet **/
void nm_packet_free(nm_packet_t *pkt);


#endif /*__KERN_NM_STRUCTURES*/
