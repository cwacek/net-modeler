#ifndef __KERN_NM_STRUCTURES
#define __KERN_NM_STRUCTURES

struct nm_path {
  uint32_t src;
  uint32_t dst;
  uint8_t valid;
  uint8_t len;
  uint32_t *hops;
};
typedef struct nm_path nm_path_t;

/** The packet flagged this way is in the middle of an incomplete hop **/
#define NM_FLAG_HOP_INCOMPLETE 1
/** A packet flagged this way is not scheduleable because of a taildelay preceding it **/
#define NM_FLAG_HOP_TAILDELAYED 2

struct nm_packet {
  struct nf_queue_entry *data;
  /** The the path this packet is going to transit.*/
  nm_path_t *path;
  /** The hop index on the current path */
  uint32_t path_idx;               
  /* The current amount scheduled. May be equal to or less than hop_progress */
  uint16_t scheduled_amt;
  /* Keep track of how far we've progressed, including waiting time and transit time. */
  uint16_t hop_progress; 
  /* The cost of *transitting* the current hop. */
  uint16_t hop_cost;
  /* The cost of waiting for us to be queued on the current hop. */
  uint16_t hop_tailwait;
  uint16_t flags;
  /** Linked list helpers **/
  struct nm_packet *next;
};
typedef struct nm_packet nm_packet_t;

#define total_pkt_cost(pkt) (pkt)->hop_cost + (pkt)->hop_tailwait

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

struct nm_model_details {
  uint8_t valid;
  char name[32];
  uint32_t n_hops;
  uint32_t n_endpoints;
};
typedef struct nm_model_details nm_model_details_t;

struct nm_hop {
  /* The speed of the hop in bytes/ms */
  uint32_t bw_limit;
  uint32_t delay_ms;
  /** The time the last packet queued is scheduled to exit 
   * this hop */
  uint32_t tailexit;
};
typedef struct nm_hop nm_hop_t;

typedef struct {
  nm_model_details_t info;
  nm_hop_t *_hoptable;
  nm_path_t **_pathtable;
  atomic_t hops_loaded;
  atomic_t paths_loaded;
  uint8_t _initialized;
} nm_model_t;

/** Initialize the datastructures needed to hold our model based on the 
 *  currently loaded nm_model_details.
 **/
int nm_model_initialize(nm_model_details_t *newmodel);

extern nm_model_t nm_model;

#endif /*__KERN_NM_STRUCTURES*/
