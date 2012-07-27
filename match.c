/*Using the libipq library and the `ip_queue' module, almost anything which can be done inside the kernel can now be done in userspace. This means that, with some speed penalty, you can develop your code entirely in userspace. Unless you are trying to filter large bandwidths, you should find this approach superior to in-kernel packet mangling.*/

/*In the very early days of netfilter, I proved this by porting an embryonic version of iptables to userspace. Netfilter opens the doors for more people to write their own, fairly efficient netmangling modules, in whatever language they want.`*/

struct nf_hook_ops {
  /*Used to sew you into the linked list: set to '{ NULL, NULL }'*/
  list

  /*The function which is called when a packet hits this hook point. Your function must return NF_ACCEPT, NF_DROP or NF_QUEUE. If NF_ACCEPT, the next hook attached to that point will be called. If NF_DROP, the packet is dropped. If NF_QUEUE, it's queued. You receive a pointer to an skb pointer, so you can entirely replace the skb if you wish.*/
  hook

  /*Currently unused: designed to pass on packet hits when the cache is flushed. May never be implemented: set it to NULL.*/
  flush

  /*The protocol family, eg, `PF_INET' for IPv4.*/
  pf
  
  /*The number of the hook you are interested in, eg `NF_IP_LOCAL_OUT'*/
  hooknum

};
