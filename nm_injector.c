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

#include "net-modeler.h"
#include "nm_magic.h"
#include "nm_log.h"

static struct socket *sock;

int nm_init_injector()
{
  int err;
  err = sock_create_kern(PF_INET, SOCK_RAW, IPPROTO_RAW, &sock);
  if (err < 0)
    nm_log(NM_WARN,"Socket couldn't be created.");
    return err;
}

void nm_cleanup_injector()
{
  if (sock)
    sock_release(sock);
}

/** Inject 'pkt' back into the networking subsystem.
 *  Return the number of bytes sent, or -1 if it failed.
 **/
int nm_inject(struct iphdr *pkt, uint32_t len)
{
  struct sockaddr_in sin;
  struct msghdr msg;
  struct iovec iov;
  int sent;

  if (len < 0)
    return -2;

  pkt->tos = TOS_MAGIC;
  sin.sin_family = AF_INET;
  sin.sin_port = pkt->protocol;
  sin.sin_addr.s_addr = pkt->daddr;

  iov.iov_base = (void *)pkt;
  iov.iov_len = len;
  msg.msg_name=NULL;
  msg.msg_namelen = 0;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = NULL;
  msg.msg_controllen = 0;

  sent = sock_sendmsg(sock,&msg,len); 


  if ((sent != len)){
    nm_log(NM_WARN,"sendto sent fewer bytes than expected");
  } 
  return sent;
}
