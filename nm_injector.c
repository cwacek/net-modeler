#include <linux/types.h>
#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/net.h>
#include <linux/socket.h>
#include<linux/in.h>
#include<linux/slab.h>
#include<linux/sched.h>
#include <linux/syscalls.h>
#include <asm/uaccess.h>
#include <linux/ip.h>

#include "nm_main.h"
#include "nm_magic.h"
#include "nm_log.h"
#include "nm_structures.h"

static struct socket *sock;

int nm_init_injector()
{
  int err;
  err = sock_create_kern(AF_INET, SOCK_RAW, IPPROTO_RAW, &sock);
  if (err < 0)
    nm_log(NM_WARN,"Socket couldn't be created.");
    return err;
}

void nm_cleanup_injector()
{
  log_func_entry;
  if (sock)
    sock_release(sock);
}

/** Inject 'pkt' back into the networking subsystem.
 *  Return the number of bytes sent, or -1 if it failed.
 *
 **/
int nm_inject(struct iphdr *pkt, uint32_t len)
{
  struct sockaddr_in *sin;
  struct msghdr *msg;
  struct kvec *iov;
  int sent;

  sin = nm_alloc(SOCKADDR_ALLOC,GFP_ATOMIC);
  msg = nm_alloc(MSGHDR_ALLOC,GFP_ATOMIC);
  iov = nm_alloc(KVEC_ALLOC,GFP_ATOMIC);

  if (!sock)
    nm_log(NM_WARN,"Socket is null\n");

  if (len < 0)
    return -2;

  if (pkt->tos != 0){
    nm_log(NM_WARN,"TOS contains %u",pkt->tos);
  }

  pkt->tos = TOS_MAGIC;
  sin->sin_family = AF_INET;
  sin->sin_port = pkt->protocol;
  sin->sin_addr.s_addr = pkt->daddr;

  iov->iov_base = (void *)pkt;
  iov->iov_len = len;
  msg->msg_name=sin;
  msg->msg_namelen = sizeof(struct sockaddr_in);
  msg->msg_control = NULL;
  msg->msg_controllen = 0;

  sent = kernel_sendmsg(sock,msg,iov,1,len); 

  if ((sent != len)){
    nm_log(NM_WARN,"sendto sent fewer bytes than expected. Expected %u. Got %d\n",len,sent);
  } 
  nm_free(SOCKADDR_ALLOC,sin);
  nm_free(MSGHDR_ALLOC,msg);
  nm_free(KVEC_ALLOC,iov);
  return sent;
}

MODULE_LICENSE("GPL");
