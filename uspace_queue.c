#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <linux/types.h>
#include <linux/netfilter.h>
#include <linux/ip.h>

#include <libnetfilter_queue/libnetfilter_queue.h>
#include <libnfnetlink/libnfnetlink.h>
#include "log.h" 
#include "magic.h"

int sock;

#define SNPRINTF_FAILURE(size, len, offset)      \
do {               \
 if (size < 0 || (unsigned int) size >= len)   \
   return size;          \
 offset += size;           \
 len -= size;            \
} while (0)

#undef __KERNEL__

static u_int32_t print_pkt (struct nfq_data *tb)
{

  char buf[4096];
  int offset = 0,len=4096,size=0;
  int id = 0;
  struct nfqnl_msg_packet_hdr *ph;
  struct nfqnl_msg_packet_hw *hwph;
  u_int32_t mark,ifi; 
  int ret;
  char *data;

  ret = nfq_get_payload(tb, &data);
  offset = 0;
  if (ret >= 0) {
    int i;

    for (i=0; i<ret; i++) {
      size = snprintf(buf + offset, len, "%02x",
          data[i] & 0xff);
      SNPRINTF_FAILURE(size, len, offset);
    }
    snprintf(buf+offset,len,"\0");
  }

  printf("%s\n",buf);


  ph = nfq_get_msg_packet_hdr(tb);
  if (ph) {
    id = ntohl(ph->packet_id);
    printf("hw_protocol=0x%04x hook=%u id=%u ",
        ntohs(ph->hw_protocol), ph->hook, id);
  }

  hwph = nfq_get_packet_hw(tb);
  if (hwph) {
    int i, hlen = ntohs(hwph->hw_addrlen);

    printf("hw_src_addr=");
    for (i = 0; i < hlen-1; i++)
      printf("%02x:", hwph->hw_addr[i]);
    printf("%02x ", hwph->hw_addr[hlen-1]);
  }

  mark = nfq_get_nfmark(tb);
  if (mark)
    printf("mark=%u ", mark);

  ifi = nfq_get_indev(tb);
  if (ifi)
    printf("indev=%u ", ifi);

  ifi = nfq_get_outdev(tb);
  if (ifi)
    printf("outdev=%u ", ifi);
  ifi = nfq_get_physindev(tb);
  if (ifi)
    printf("physindev=%u ", ifi);

  ifi = nfq_get_physoutdev(tb);
  if (ifi)
    printf("physoutdev=%u ", ifi);

  ret = nfq_get_payload(tb, &data);
  if (ret >= 0)
    printf("payload_len=%d ", ret);

  fputc('\n', stdout);

  return id;
}

static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
              struct nfq_data *nfa, void *data)
{
  nm_log(NM_DEBUG,"entering callback\n");
  u_int32_t id = print_pkt(nfa);
  
  if (reinject(nfa) < 0 ){
    perror("reinject()");
  }
  return nfq_set_verdict(qh, id, NF_STOLEN, 0, NULL);
}

int reinject(struct nfq_data *nfa){
  int len;
  char *payload;
  struct iphdr *iph;
  struct sockaddr_in sin;

  len = nfq_get_payload(nfa, &payload);
  if (len <= 0)
    return 0;

  iph = (struct iphdr *) payload;
  iph->tos = TOS_MAGIC;
  
  sin.sin_family = AF_INET;
  sin.sin_port = iph->protocol;
  sin.sin_addr.s_addr = iph->daddr;

  if (sendto(sock, payload, len, 0, (struct sockaddr *) &sin, sizeof(struct sockaddr_in)) != len){
    perror("sendto sent fewer bytes than expected");
  } 

}


int main(int argc, const char *argv[])
{
  struct nfq_handle *h;
  struct nfq_q_handle *qh;

  int fd;
  int rv;
  char buf[4096] __attribute ((aligned));

  nm_log(NM_NOTICE,"Opening netfilter_queue library handle\n");
  h = nfq_open();
  if (!h) {
    perror("nfq_open()");
    exit(1);;;;
  }

  nm_log(NM_INFO,"Unbinding existing nf_queue handler for AF_INET\n");
  if (nfq_unbind_pf(h,AF_INET) < 0){
    perror("nfq_unbind_pf()");
    exit(1);
  }

  nm_log(NM_INFO,"Binding to nfnetlink_queue for AF_INET\n");
  if (nfq_bind_pf(h,AF_INET) < 0){
    perror("nfq_bind_pf()");
  }

  nm_log(NM_INFO,"Binding this socket to queue '0'\n");
  qh = nfq_create_queue(h, 0, &cb, NULL);
  if (!qh){
    perror("nfq_create_queue()");
  }

  nm_log(NM_INFO,"Setting copy packet mode \n");
  if (nfq_set_mode(qh,NFQNL_COPY_PACKET, 0xFFFF) < 0) {
    perror("Couldn't set copy packet mode");
  }

  fd = nfq_fd(h);

  sock = socket(PF_INET, SOCK_RAW, IPPROTO_RAW);
  if (!sock)
    perror("socket()");
  
  while ((rv = recv(fd, buf, sizeof(buf), 0)) && rv >= 0) {
    nm_log(NM_INFO,"pkt received\n");
    nfq_handle_packet(h, buf, rv);
  }

  nm_log(NM_INFO,"unbinding from queue 0\n");
  nfq_destroy_queue(qh);

  nm_log(NM_INFO,"Closing library handle\n");
  nfq_close(h);


  return 0;
}

