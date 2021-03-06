/**
 * @file
 * User Datagram Protocol module
 *
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */


/* udp.c
 *
 * The code for the User Datagram Protocol UDP & UDPLite (RFC 3828).
 *
 */

/* @todo Check the use of '(struct udp_pcb).chksum_len_rx'!
 */

#include "lwip/opt.h"

#if LWIP_UDP /* don't build if not configured for use in lwipopts.h */

#include "lwip/udp.h"
#include "lwip/def.h"
#include "lwip/memp.h"
#include "lwip/inet.h"
#include "lwip/inet_chksum.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/icmp.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "arch/perf.h"
#include "lwip/dhcp.h"

#include <string.h>

/* The list of UDP PCBs */
/* exported in udp.h (was static) */
/* UDP控制块链表头指针 */
struct udp_pcb *udp_pcbs;

/**
 * Process an incoming UDP datagram.
 *
 * Given an incoming UDP datagram (as a chain of pbufs) this function
 * finds a corresponding UDP PCB and hands over the pbuf to the pcbs
 * recv function. If no pcb is found or the datagram is incorrect, the
 * pbuf is freed.
 *
 * @param p pbuf to be demultiplexed to a UDP PCB.
 * @param inp network interface on which the datagram was received.
 *
 */
/* ethernetif_input() -> ethernet_input() -> ip_input() -> udp_input() 
 * 输入含有UDP数据报的IP数据报pbuf，和接收该数据报的以太网接口指针
 * 1，函数找到对应目的端口号的UDP控制块并调用用户回调函数recv()，用户函数recv()
 * 负责释放pbuf
 * 2，如果找到匹配UDP控制块，但回调函数为空，则释放pbuf后，不返回任何错误
 * 3，如果找不到匹配的UDP控制块，且UDP数据报的目的IP是本机，则返回ICMP端口不可达差错报文
 */
void
udp_input(struct pbuf *p, struct netif *inp)
{
	/* UDP报文首部 */
  struct udp_hdr *udphdr;
	/* UDP控制块，用于找到对应的UDP控制块 */
  struct udp_pcb *pcb, *prev;
	/* 第一个匹配的处于非连接状态的UDP控制块 */
  struct udp_pcb *uncon_pcb;
	/* IP数据报首部 */
  struct ip_hdr *iphdr;
	/* 源端口、目的端口 */
  u16_t src, dest;
	/* 控制块是否匹配 */
  u8_t local_match;
	/* UDP数据报是否是广播 */
  u8_t broadcast;

  PERF_START;

  UDP_STATS_INC(udp.recv);

	/* IP数据报首部 */
  iphdr = p->payload;

  /* Check minimum length (IP header + UDP header)
   * and move payload pointer to UDP header */
	/* IP数据报长度不能小于IP首部中首部长度字段标识的首部长度 + UDP首部长度
	 * 同时，移动pbuf的payload，使其指向UDP数据报首部
	 */
  if (p->tot_len < (IPH_HL(iphdr) * 4 + UDP_HLEN) || pbuf_header(p, -(s16_t)(IPH_HL(iphdr) * 4))) {
    /* drop short packets */
    LWIP_DEBUGF(UDP_DEBUG,
                ("udp_input: short UDP datagram (%"U16_F" bytes) discarded\n", p->tot_len));
    UDP_STATS_INC(udp.lenerr);
    UDP_STATS_INC(udp.drop);
    snmp_inc_udpinerrors();
		/* 校验不成功或移动payload不成功，释放pbuf，并跳转到end执行 */
    pbuf_free(p);
    goto end;
  }

	/* 获取UDP首部指针 */
  udphdr = (struct udp_hdr *)p->payload;

	/* 判断IP首部中目的IP地址是不是对应网络接口的广播地址 */
  /* is broadcast packet ? */
  broadcast = ip_addr_isbroadcast(&(iphdr->dest), inp);

  LWIP_DEBUGF(UDP_DEBUG, ("udp_input: received datagram of length %"U16_F"\n", p->tot_len));

  /* convert src and dest ports to host byte order */
	/* 获取主机字节序的UDP源端口和目的端口号 */
  src = ntohs(udphdr->src);
  dest = ntohs(udphdr->dest);

  udp_debug_print(udphdr);

  /* print the UDP source and destination */
  LWIP_DEBUGF(UDP_DEBUG,
              ("udp (%"U16_F".%"U16_F".%"U16_F".%"U16_F", %"U16_F") <-- "
               "(%"U16_F".%"U16_F".%"U16_F".%"U16_F", %"U16_F")\n",
               ip4_addr1(&iphdr->dest), ip4_addr2(&iphdr->dest),
               ip4_addr3(&iphdr->dest), ip4_addr4(&iphdr->dest), ntohs(udphdr->dest),
               ip4_addr1(&iphdr->src), ip4_addr2(&iphdr->src),
               ip4_addr3(&iphdr->src), ip4_addr4(&iphdr->src), ntohs(udphdr->src)));

#if LWIP_DHCP
  pcb = NULL;
  /* when LWIP_DHCP is active, packets to DHCP_CLIENT_PORT may only be processed by
     the dhcp module, no other UDP pcb may use the local UDP port DHCP_CLIENT_PORT */
  if (dest == DHCP_CLIENT_PORT) {
    /* all packets for DHCP_CLIENT_PORT not coming from DHCP_SERVER_PORT are dropped! */
    if (src == DHCP_SERVER_PORT) {
      if ((inp->dhcp != NULL) && (inp->dhcp->pcb != NULL)) {
        /* accept the packe if 
           (- broadcast or directed to us) -> DHCP is link-layer-addressed, local ip is always ANY!
           - inp->dhcp->pcb->remote == ANY or iphdr->src */
        if ((ip_addr_isany(&inp->dhcp->pcb->remote_ip) ||
           ip_addr_cmp(&(inp->dhcp->pcb->remote_ip), &(iphdr->src)))) {
          pcb = inp->dhcp->pcb;
        }
      }
    }
  } else
#endif /* LWIP_DHCP */
  {
    prev = NULL;
		/* 当前控制块的匹配情况 */
    local_match = 0;
		/* 第一个匹配的非活动UDP控制块 */
    uncon_pcb = NULL;
    /* Iterate through the UDP pcb list for a matching pcb.
     * 'Perfect match' pcbs (connected to the remote port & ip address) are
     * preferred. If no perfect match is found, the first unconnected pcb that
     * matches the local port and ip address gets the datagram. */
		/* 从UDP控制块链表中查找匹配的UDP控制块
		 * 第一匹配目标是UDP数据报的目的IP和目的端口都匹配的处于连接状态的UDP控制块
		 * 匹配不到则选择第一个UDP数据报的目的IP和目的端口都匹配的处于非连接状态的UDP控制块
		 */
    for (pcb = udp_pcbs; pcb != NULL; pcb = pcb->next) {
      local_match = 0;
      /* print the PCB local and remote address */
      LWIP_DEBUGF(UDP_DEBUG,
                  ("pcb (%"U16_F".%"U16_F".%"U16_F".%"U16_F", %"U16_F") --- "
                   "(%"U16_F".%"U16_F".%"U16_F".%"U16_F", %"U16_F")\n",
                   ip4_addr1(&pcb->local_ip), ip4_addr2(&pcb->local_ip),
                   ip4_addr3(&pcb->local_ip), ip4_addr4(&pcb->local_ip), pcb->local_port,
                   ip4_addr1(&pcb->remote_ip), ip4_addr2(&pcb->remote_ip),
                   ip4_addr3(&pcb->remote_ip), ip4_addr4(&pcb->remote_ip), pcb->remote_port));

      /* compare PCB local addr+port to UDP destination addr+port */
			/* 判断UDP控制块是否和UDP数据报匹配(目的端口、目的IP)
			 * 匹配条件：(UDP数据报的目的端口和本地端口相同) && ((UDP数据报的目的IP不是广播 && UDP控制块的本地IP是0)
			 * 或(UDP数据报的目的IP和UDP控制块的本地IP匹配))
			 */
      if ((pcb->local_port == dest) &&
          ((!broadcast && ip_addr_isany(&pcb->local_ip)) ||
           ip_addr_cmp(&(pcb->local_ip), &(iphdr->dest)) ||
#if LWIP_IGMP
           ip_addr_ismulticast(&(iphdr->dest)) ||
#endif /* LWIP_IGMP */
#if IP_SOF_BROADCAST_RECV
           (broadcast && (pcb->so_options & SOF_BROADCAST)))) {
#else  /* IP_SOF_BROADCAST_RECV */
           (broadcast))) {
#endif /* IP_SOF_BROADCAST_RECV */
				/* 当前控制块匹配 */
        local_match = 1;
				/* 当前控制块未连接且uncon_pcb为空 */
        if ((uncon_pcb == NULL) && 
            ((pcb->flags & UDP_FLAGS_CONNECTED) == 0)) {
          /* the first unconnected matching PCB */
					/* 记录第一个匹配的未连接的UDP控制块 */
          uncon_pcb = pcb;
        }
      }
			/* 目的端口号和目的IP地址匹配成功，继续匹配源端口号和源IP地址，
			 * 源端口号和源IP地址匹配成功，说明UDP控制块的远端端口号不为0，UDP控制块是连接状态
			 */
      /* compare PCB remote addr+port to UDP source addr+port */
      if ((local_match != 0) &&
          (pcb->remote_port == src) &&
          (ip_addr_isany(&pcb->remote_ip) ||
           ip_addr_cmp(&(pcb->remote_ip), &(iphdr->src)))) {
        /* the first fully matching PCB */
				/* 目的IP：目的端口，源IP：源端口完全匹配
				 * 且当前UDP控制块是连接状态，则是第一个完全匹配的UDP控制块
				 */
				/* prev指向当前UDP控制块的上一个节点，prev为NULL表明当前UDP控制块在链表第一个节点 */
        if (prev != NULL) {
					/* 当前UDP控制块不是链表第一个节点，把它移动到第一个节点 */
          /* move the pcb to the front of udp_pcbs so that is
             found faster next time */
					/* 删除当前节点 */
          prev->next = pcb->next;
					/* 当前节点插入链表首部 */
          pcb->next = udp_pcbs;
          udp_pcbs = pcb;
        }
				/* 当前UDP控制块在链表首部 */
				else {
          UDP_STATS_INC(udp.cachehit);
        }
				/* 找到第一个完全匹配的UDP控制块后，跳出遍历 */
        break;
      }
			/* 指向当前链表节点的上一个节点 */
      prev = pcb;
			/* 进行下一个控制块的比较 */
    }	/* for (pcb = udp_pcbs; pcb != NULL; pcb = pcb->next) */
		
		/* 遍历完UDP控制块，没有找到完全匹配的连接状态的UDP控制块，则选取
		 * 第一个完全匹配的未连接的状态的UDP控制块(可能也为空)
		 */
    /* no fully matching pcb found? then look for an unconnected pcb */
    if (pcb == NULL) {
      pcb = uncon_pcb;
    }
  }

  /* Check checksum if this is a match or if it was directed at us. */
	/* 如果找到匹配的UDP控制块，或者UDP数据报的目的IP确实是接收数据报的接口 */
  if (pcb != NULL || ip_addr_cmp(&inp->ip_addr, &iphdr->dest)) {
    LWIP_DEBUGF(UDP_DEBUG | LWIP_DBG_TRACE, ("udp_input: calculating checksum\n"));
#if LWIP_UDPLITE
    if (IPH_PROTO(iphdr) == IP_PROTO_UDPLITE) {
      /* Do the UDP Lite checksum */
#if CHECKSUM_CHECK_UDP
      u16_t chklen = ntohs(udphdr->len);
      if (chklen < sizeof(struct udp_hdr)) {
        if (chklen == 0) {
          /* For UDP-Lite, checksum length of 0 means checksum
             over the complete packet (See RFC 3828 chap. 3.1) */
          chklen = p->tot_len;
        } else {
          /* At least the UDP-Lite header must be covered by the
             checksum! (Again, see RFC 3828 chap. 3.1) */
          UDP_STATS_INC(udp.chkerr);
          UDP_STATS_INC(udp.drop);
          snmp_inc_udpinerrors();
          pbuf_free(p);
          goto end;
        }
      }
      if (inet_chksum_pseudo_partial(p, (struct ip_addr *)&(iphdr->src),
                             (struct ip_addr *)&(iphdr->dest),
                             IP_PROTO_UDPLITE, p->tot_len, chklen) != 0) {
       LWIP_DEBUGF(UDP_DEBUG | LWIP_DBG_LEVEL_SERIOUS,
                   ("udp_input: UDP Lite datagram discarded due to failing checksum\n"));
        UDP_STATS_INC(udp.chkerr);
        UDP_STATS_INC(udp.drop);
        snmp_inc_udpinerrors();
        pbuf_free(p);
        goto end;
      }
#endif /* CHECKSUM_CHECK_UDP */
    } else
#endif /* LWIP_UDPLITE */
    {
#if CHECKSUM_CHECK_UDP
			/* 如果UDP数据报中填写了校验和，则验证 */
      if (udphdr->chksum != 0) {
				/* 计算包含UDP伪首部的UDP校验和 */
        if (inet_chksum_pseudo(p, (struct ip_addr *)&(iphdr->src),
                               (struct ip_addr *)&(iphdr->dest),
                               IP_PROTO_UDP, p->tot_len) != 0) {
          LWIP_DEBUGF(UDP_DEBUG | LWIP_DBG_LEVEL_SERIOUS,
                      ("udp_input: UDP datagram discarded due to failing checksum\n"));
          UDP_STATS_INC(udp.chkerr);
          UDP_STATS_INC(udp.drop);
          snmp_inc_udpinerrors();
					/* 校验失败，释放数据报，并返回 */
          pbuf_free(p);
          goto end;
        }
      }
#endif /* CHECKSUM_CHECK_UDP */
    }
		/* UDP校验和通过，则把pbuf的payload移动到UDP的用户数据区 */
    if(pbuf_header(p, -UDP_HLEN)) {
      /* Can we cope with this failing? Just assert for now */
      LWIP_ASSERT("pbuf_header failed\n", 0);
      UDP_STATS_INC(udp.drop);
      snmp_inc_udpinerrors();
			/* 移动失败，则释放整个数据报 */
      pbuf_free(p);
      goto end;
    }
		/* 如果有匹配的UDP控制块(可能该UDP控制块是未连接状态)
		 * 调用用户回调函数
		 */
    if (pcb != NULL) {
      snmp_inc_udpindatagrams();
      /* callback */
			/* 如果用户在UDP控制块中注册了回调函数，则调用回调函数 */
      if (pcb->recv != NULL) {
        /* now the recv function is responsible for freeing p */
				/* 用户回调函数，!!!!!!负责释放pbuf!!!!!! */
        pcb->recv(pcb->recv_arg, pcb, p, &iphdr->src, src);
      }
			/* 用户没有注册回调函数，则释放pbuf并返回，不返回任何错误信息给发送端 */
			else {
        /* no recv function registered? then we have to free the pbuf! */
        pbuf_free(p);
        goto end;
      }
    }
		/*  */
		else {
      LWIP_DEBUGF(UDP_DEBUG | LWIP_DBG_TRACE, ("udp_input: not for us.\n"));

#if LWIP_ICMP
      /* No match was found, send ICMP destination port unreachable unless
         destination address was broadcast/multicast. */
			/* 没有匹配的UDP控制块，且UDP数据报不是广播且不是多播，则返回端口不可达ICMP报文 */
      if (!broadcast &&
          !ip_addr_ismulticast(&iphdr->dest)) {
        /* move payload pointer back to ip header */
				/* 修改pbuf的payload指针到IP首部 */
        pbuf_header(p, (IPH_HL(iphdr) * 4) + UDP_HLEN);
        LWIP_ASSERT("p->payload == iphdr", (p->payload == iphdr));
				/* 发送ICMP端口不可达差错报文，引起差错的报文是p，ICMP会引用其IP首部+IP数据区前8字节 */
        icmp_dest_unreach(p, ICMP_DUR_PORT);
      }
#endif /* LWIP_ICMP */
      UDP_STATS_INC(udp.proterr);
      UDP_STATS_INC(udp.drop);
      snmp_inc_udpnoports();
			/* 释放pbuf */
      pbuf_free(p);
    }
  }
	/* UDP数据报的目的IP不是本机的网络接口 */
	else {
		/* 释放pbuf */
    pbuf_free(p);
  }
end:
  PERF_STOP("udp_input");
}

/**
 * Send data using UDP.
 *
 * @param pcb UDP PCB used to send the data.
 * @param p chain of pbuf's to be sent.
 *
 * The datagram will be sent to the current remote_ip & remote_port
 * stored in pcb. If the pcb is not bound to a port, it will
 * automatically be bound to a random port.
 *
 * @return lwIP error code.
 * - ERR_OK. Successful. No error occured.
 * - ERR_MEM. Out of memory.
 * - ERR_RTE. Could not find route to destination address.
 * - More errors could be returned by lower protocol layers.
 *
 * @see udp_disconnect() udp_sendto()
 */
/* 使用一个处于连接状态的UDP控制块发送用户数据pbuf
 * 如果当前UDP控制块没有绑定到本地端口，会自动绑定到本地随机端口
 */
err_t
udp_send(struct udp_pcb *pcb, struct pbuf *p)
{
  /* send to the packet using remote ip and port stored in the pcb */
  return udp_sendto(pcb, p, &pcb->remote_ip, pcb->remote_port);
}

/**
 * Send data to a specified address using UDP.
 *
 * @param pcb UDP PCB used to send the data.
 * @param p chain of pbuf's to be sent.
 * @param dst_ip Destination IP address.
 * @param dst_port Destination UDP port.
 *
 * dst_ip & dst_port are expected to be in the same byte order as in the pcb.
 *
 * If the PCB already has a remote address association, it will
 * be restored after the data is sent.
 * 
 * @return lwIP error code (@see udp_send for possible error codes)
 *
 * @see udp_disconnect() udp_send()
 */
/* 把用户数据发送到指定的远端IP地址和端口号上 */
err_t
udp_sendto(struct udp_pcb *pcb, struct pbuf *p,
  struct ip_addr *dst_ip, u16_t dst_port)
{
  struct netif *netif;

  LWIP_DEBUGF(UDP_DEBUG | LWIP_DBG_TRACE, ("udp_send\n"));

  /* find the outgoing network interface for this packet */
#if LWIP_IGMP
  netif = ip_route((ip_addr_ismulticast(dst_ip))?(&(pcb->multicast_ip)):(dst_ip));
#else
	/* 与IP层交互，得到将要发送本报文的网络接口，以获得源IP */
  netif = ip_route(dst_ip);
#endif /* LWIP_IGMP */

  /* no outgoing network interface could be found? */
	/* 找不到合适的网络接口，则返回错误 */
  if (netif == NULL) {
    LWIP_DEBUGF(UDP_DEBUG | LWIP_DBG_LEVEL_SERIOUS, ("udp_send: No route to 0x%"X32_F"\n", dst_ip->addr));
    UDP_STATS_INC(udp.rterr);
    return ERR_RTE;
  }
	/* 调用udp_sendto_if()组装并发送UDP报文 */
  return udp_sendto_if(pcb, p, dst_ip, dst_port, netif);
}

/**
 * Send data to a specified address using UDP.
 * The netif used for sending can be specified.
 *
 * This function exists mainly for DHCP, to be able to send UDP packets
 * on a netif that is still down.
 *
 * @param pcb UDP PCB used to send the data.
 * @param p chain of pbuf's to be sent.
 * @param dst_ip Destination IP address.
 * @param dst_port Destination UDP port.
 * @param netif the netif used for sending.
 *
 * dst_ip & dst_port are expected to be in the same byte order as in the pcb.
 *
 * @return lwIP error code (@see udp_send for possible error codes)
 *
 * @see udp_disconnect() udp_send()
 */
/* 组装并发送UDP报文到远端IP:远端端口，使用netif接口发送 */
err_t
udp_sendto_if(struct udp_pcb *pcb, struct pbuf *p,
  struct ip_addr *dst_ip, u16_t dst_port, struct netif *netif)
{
	/* UDP首部结构指针 */
  struct udp_hdr *udphdr;
  struct ip_addr *src_ip;
  err_t err;
	/* 指向组装好的UDP报文 */
  struct pbuf *q; /* q will be sent down the stack */

#if IP_SOF_BROADCAST
  /* broadcast filter? */
  if ( ((pcb->so_options & SOF_BROADCAST) == 0) && ip_addr_isbroadcast(dst_ip, netif) ) {
    LWIP_DEBUGF(UDP_DEBUG | LWIP_DBG_LEVEL_SERIOUS,
      ("udp_sendto_if: SOF_BROADCAST not enabled on pcb %p\n", (void *)pcb));
    return ERR_VAL;
  }
#endif /* IP_SOF_BROADCAST */

  /* if the PCB is not yet bound to a port, bind it here */
	/*如果UDP控制块没有绑定到本地端口，则绑定到本地随机端口，但本地IP怎么处理? */
  if (pcb->local_port == 0) {
    LWIP_DEBUGF(UDP_DEBUG | LWIP_DBG_TRACE, ("udp_send: not yet bound to a port, binding now\n"));
		/* 绑定UDP控制块到本地IP:本地随机端口 */
    err = udp_bind(pcb, &pcb->local_ip, pcb->local_port);
    if (err != ERR_OK) {
      LWIP_DEBUGF(UDP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS, ("udp_send: forced port bind failed\n"));
      return err;
    }
  }

	/* 开始构造UDP首部，判断用户数据的pbuf数据区前面能否放下UDP首部
	 * 如果放不下，为UDP首部重新申请一个pbuf空间 
	 */
  /* not enough space to add an UDP header to first pbuf in given p chain? */
  if (pbuf_header(p, UDP_HLEN)) {
    /* allocate header in a separate new pbuf */
		/* 在内存堆中申请UDP首部空间，在IP申请，将会额外预留MAC层空间 */
    q = pbuf_alloc(PBUF_IP, UDP_HLEN, PBUF_RAM);
    /* new header pbuf could not be allocated? */
    if (q == NULL) {
			/* 申请失败，则返回内存错误 */
      LWIP_DEBUGF(UDP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS, ("udp_send: could not allocate header\n"));
      return ERR_MEM;
    }
    /* chain header q in front of given pbuf p */
		/* 把q和用户UDP数据p连接成pbuf链表 */
    pbuf_chain(q, p);
    /* first pbuf q points to header pbuf */
    LWIP_DEBUGF(UDP_DEBUG,
                ("udp_send: added header pbuf %p before given pbuf %p\n", (void *)q, (void *)p));
  }
	/* 在pbuf中已经预留了UDP首部空间 */
	else {
    /* adding space for header within p succeeded */
    /* first pbuf q equals given pbuf */
		/* q指向以UDP首部开始的pbuf */
    q = p;
    LWIP_DEBUGF(UDP_DEBUG, ("udp_send: added header in given pbuf %p\n", (void *)p));
  }
  LWIP_ASSERT("check that first pbuf can hold struct udp_hdr",
              (q->len >= sizeof(struct udp_hdr)));
  /* q now represents the packet to be sent */
	/* 填写UDP首部 */
  udphdr = q->payload;
	/* 本地端口号 */
  udphdr->src = htons(pcb->local_port);
  /* 远程端口号 */
	udphdr->dest = htons(dst_port);
  /* in UDP, 0 checksum means 'no checksum' */
	/* 清0UDP校验和 */
  udphdr->chksum = 0x0000; 

	/* 得到伪首部的值，首先得到源IP地址 */
  /* PCB local address is IP_ANY_ADDR? */
  if (ip_addr_isany(&pcb->local_ip)) {
		/* UDP控制块中源IP地址为0，则使用IP层提供的发送该UDP数据报的网络接口的IP地址 */
    /* use outgoing network interface IP address as source address */
    src_ip = &(netif->ip_addr);
  }
	/* UDP控制块中记录了源IP地址 */
	else {
    /* check if UDP PCB local IP address is correct
     * this could be an old address if netif->ip_addr has changed */
		/* 如果UDP控制块中记录的源IP地址和网络接口的IP地址不一致，说明netif的IP地址修改过 */
    if (!ip_addr_cmp(&(pcb->local_ip), &(netif->ip_addr))) {
      /* local_ip doesn't match, drop the packet */
			/* 如果为该UDP报文申请过首部空间 */
      if (q != p) {
        /* free the header pbuf */
				/* 释放申请的首部空间，但是不释放p指向的pbuf，因为p->ref在pbuf_chain(q, p)时+1 */
        pbuf_free(q);
				/* 指向UDP数据报pbuf的指针清0 */
        q = NULL;
        /* p is still referenced by the caller, and will live on */
      }
			/* 返回错误信息 */
      return ERR_VAL;
    }
		/* UDP控制块中记录的源IP地址和网络接口的IP地址一致
		 * 则使用UDP控制块中的本地IP地址作为UDP伪首部的源IP地址
		 */
    /* use UDP PCB local IP address as source address */
    src_ip = &(pcb->local_ip);
  }

  LWIP_DEBUGF(UDP_DEBUG, ("udp_send: sending datagram of length %"U16_F"\n", q->tot_len));

#if LWIP_UDPLITE
  /* UDP Lite protocol? */
  if (pcb->flags & UDP_FLAGS_UDPLITE) {
    u16_t chklen, chklen_hdr;
    LWIP_DEBUGF(UDP_DEBUG, ("udp_send: UDP LITE packet length %"U16_F"\n", q->tot_len));
    /* set UDP message length in UDP header */
    chklen_hdr = chklen = pcb->chksum_len_tx;
    if ((chklen < sizeof(struct udp_hdr)) || (chklen > q->tot_len)) {
      if (chklen != 0) {
        LWIP_DEBUGF(UDP_DEBUG, ("udp_send: UDP LITE pcb->chksum_len is illegal: %"U16_F"\n", chklen));
      }
      /* For UDP-Lite, checksum length of 0 means checksum
         over the complete packet. (See RFC 3828 chap. 3.1)
         At least the UDP-Lite header must be covered by the
         checksum, therefore, if chksum_len has an illegal
         value, we generate the checksum over the complete
         packet to be safe. */
      chklen_hdr = 0;
      chklen = q->tot_len;
    }
    udphdr->len = htons(chklen_hdr);
    /* calculate checksum */
#if CHECKSUM_GEN_UDP
    udphdr->chksum = inet_chksum_pseudo_partial(q, src_ip, dst_ip,
                                        IP_PROTO_UDPLITE, q->tot_len, chklen);
    /* chksum zero must become 0xffff, as zero means 'no checksum' */
    if (udphdr->chksum == 0x0000)
      udphdr->chksum = 0xffff;
#endif /* CHECKSUM_CHECK_UDP */
    /* output to IP */
    LWIP_DEBUGF(UDP_DEBUG, ("udp_send: ip_output_if (,,,,IP_PROTO_UDPLITE,)\n"));
#if LWIP_NETIF_HWADDRHINT
    netif->addr_hint = &(pcb->addr_hint);
#endif /* LWIP_NETIF_HWADDRHINT*/
    err = ip_output_if(q, src_ip, dst_ip, pcb->ttl, pcb->tos, IP_PROTO_UDPLITE, netif);
#if LWIP_NETIF_HWADDRHINT
    netif->addr_hint = NULL;
#endif /* LWIP_NETIF_HWADDRHINT*/
  } else
#endif /* LWIP_UDPLITE */
  {      /* UDP */
		/* 填写UDP首部总长度字段。计算校验和，填写到UDP首部 */
    LWIP_DEBUGF(UDP_DEBUG, ("udp_send: UDP packet length %"U16_F"\n", q->tot_len));
		/* 填写UDP首部总长度字段，网络字节序 */
    udphdr->len = htons(q->tot_len);
    /* calculate checksum */
#if CHECKSUM_GEN_UDP
		/* 如果UDP控制块允许计算UDP校验和,传入q指向UDP首部，其它参数是UDP伪首部 */
    if ((pcb->flags & UDP_FLAGS_NOCHKSUM) == 0) {
      udphdr->chksum = inet_chksum_pseudo(q, src_ip, dst_ip, IP_PROTO_UDP, q->tot_len);
      /* chksum zero must become 0xffff, as zero means 'no checksum' */
			/* 如果校验和是0，则填写0xffff，因为校验和是0表示无校验 */
      if (udphdr->chksum == 0x0000) udphdr->chksum = 0xffff;
    }
#endif /* CHECKSUM_CHECK_UDP */
    LWIP_DEBUGF(UDP_DEBUG, ("udp_send: UDP checksum 0x%04"X16_F"\n", udphdr->chksum));
    LWIP_DEBUGF(UDP_DEBUG, ("udp_send: ip_output_if (,,,,IP_PROTO_UDP,)\n"));
    /* output to IP */
#if LWIP_NETIF_HWADDRHINT
    netif->addr_hint = &(pcb->addr_hint);
#endif /* LWIP_NETIF_HWADDRHINT*/
		/* 调用IP层的输出函数，填写IP首部，并发送IP数据报 */
    err = ip_output_if(q, src_ip, dst_ip, pcb->ttl, pcb->tos, IP_PROTO_UDP, netif);
#if LWIP_NETIF_HWADDRHINT
    netif->addr_hint = NULL;
#endif /* LWIP_NETIF_HWADDRHINT*/
  }
  /* TODO: must this be increased even if error occured? */
  snmp_inc_udpoutdatagrams();

  /* did we chain a separate header pbuf earlier? */
	/* 如果为UDP首部申请过内存，则释放该内存,传入的pbuf不会被释放 */
  if (q != p) {
    /* free the header pbuf */
    pbuf_free(q);
    q = NULL;
    /* p is still referenced by the caller, and will live on */
  }

  UDP_STATS_INC(udp.xmit);
	/* 返回发送结果 */
  return err;
}

/**
 * Bind an UDP PCB.
 *
 * @param pcb UDP PCB to be bound with a local address ipaddr and port.
 * @param ipaddr local IP address to bind with. Use IP_ADDR_ANY to
 * bind to all local interfaces.
 * @param port local UDP port to bind with. Use 0 to automatically bind
 * to a random port between UDP_LOCAL_PORT_RANGE_START and
 * UDP_LOCAL_PORT_RANGE_END.
 *
 * ipaddr & port are expected to be in the same byte order as in the pcb.
 *
 * @return lwIP error code.
 * - ERR_OK. Successful. No error occured.
 * - ERR_USE. The specified ipaddr and port are already bound to by
 * another UDP PCB.
 *
 * @see udp_disconnect()
 */
/* 绑定UDP控制块到本地IP:端口。即设置UDP控制块的本地IP和本地端口，并把控制块插入udp_pcbs链表
 * ipaddr: 使用IP_ADDR_ANY表示本地任意网络接口的IP地址
 * port: 使用0表示为控制块随机分配一个有效的短暂端口号
 */
err_t
udp_bind(struct udp_pcb *pcb, struct ip_addr *ipaddr, u16_t port)
{
  struct udp_pcb *ipcb;
	/* pcb是否已经在udp_pcbs链表中
	 * 0: 不在udp_pcbs链表中
	 * 1: 在udp_pcbs链表中，不需要再次插入链表
	 */
  u8_t rebind;

  LWIP_DEBUGF(UDP_DEBUG | LWIP_DBG_TRACE, ("udp_bind(ipaddr = "));
  ip_addr_debug_print(UDP_DEBUG, ipaddr);
  LWIP_DEBUGF(UDP_DEBUG | LWIP_DBG_TRACE, (", port = %"U16_F")\n", port));

  rebind = 0;
	/* 检查pcb是否在udp_pcbs链表中，如果在，rebind置1
	 * 表示不再把pcb插入到链表中。否则会形成链表环路
	 */
  /* Check for double bind and rebind of the same pcb */
  for (ipcb = udp_pcbs; ipcb != NULL; ipcb = ipcb->next) {
    /* is this UDP PCB already on active list? */
    if (pcb == ipcb) {
      /* pcb may occur at most once in active list */
      LWIP_ASSERT("rebind == 0", rebind == 0);
      /* pcb already in list, just rebind */
      rebind = 1;
    }

    /* this code does not allow upper layer to share a UDP port for
       listening to broadcast or multicast traffic (See SO_REUSE_ADDR and
       SO_REUSE_PORT under *BSD). TODO: See where it fits instead, OR
       combine with implementation of UDP PCB flags. Leon Woestenberg. */
#ifdef LWIP_UDP_TODO
    /* port matches that of PCB in list? */
    else
      if ((ipcb->local_port == port) &&
          /* IP address matches, or one is IP_ADDR_ANY? */
          (ip_addr_isany(&(ipcb->local_ip)) ||
           ip_addr_isany(ipaddr) ||
           ip_addr_cmp(&(ipcb->local_ip), ipaddr))) {
        /* other PCB already binds to this local IP and port */
        LWIP_DEBUGF(UDP_DEBUG,
                    ("udp_bind: local port %"U16_F" already bound by another pcb\n", port));
        return ERR_USE;
      }
#endif
  }

	/* 1，设置pcb控制块的local_ip为ipaddr */
  ip_addr_set(&pcb->local_ip, ipaddr);

  /* no port specified? */
	/* 没有指定本地端口,则寻找一个有效的短暂端口 */
  if (port == 0) {
#ifndef UDP_LOCAL_PORT_RANGE_START
#define UDP_LOCAL_PORT_RANGE_START 4096			/* 起始短暂端口号 */
#define UDP_LOCAL_PORT_RANGE_END   0x7fff		/* 结束短暂端口号 */
#endif
		/* 遍历所有短暂端口号，判断该端口号是否被其他控制块占用。
		 * 若未被占用，则使用该端口号
		 */
		/* 起始的短暂端口号 */
    port = UDP_LOCAL_PORT_RANGE_START;
		/* 从UDP控制块链表头开始遍历 */
    ipcb = udp_pcbs;
		/* 未到链表尾，且未到结束短暂端口号 */
    while ((ipcb != NULL) && (port != UDP_LOCAL_PORT_RANGE_END)) {
			/* 如果控制块使用了端口号， */
      if (ipcb->local_port == port) {
        /* port is already used by another udp_pcb */
				/* 检查下一个端口号 */
        port++;
        /* restart scanning all udp pcbs */
				/* 从链表头重新遍历 */
        ipcb = udp_pcbs;
      }
			/* 该控制块没有使用该端口，则继续判断下一个控制块 */
			else
        /* go on with next udp pcb */
        ipcb = ipcb->next;
    }
		/* ipcb == NULL时，UDP控制块链表中所有节点都没有使用port
		 * ipcb != NULL时，说明是port == UDP_LOCAL_PORT_RANGE_END而退出循环
		 * 即所有短暂端口号都被使用了
		 */
    if (ipcb != NULL) {
			/* 未找到可用的短暂端口号，返回错误信息 */
      /* no more ports available in local range */
      LWIP_DEBUGF(UDP_DEBUG, ("udp_bind: out of free UDP ports\n"));
      return ERR_USE;
    }
  }
	
	/* 2，设置UDP控制块中的本地端口号，主机字节序 */
  pcb->local_port = port;
  snmp_insert_udpidx_tree(pcb);
  /* pcb not active yet? */
	/* 3，控制块没有在链表中，则插入链表首部 */
  if (rebind == 0) {
    /* place the PCB on the active list if not already there */
    pcb->next = udp_pcbs;
    udp_pcbs = pcb;
  }
  LWIP_DEBUGF(UDP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE,
              ("udp_bind: bound to %"U16_F".%"U16_F".%"U16_F".%"U16_F", port %"U16_F"\n",
               (u16_t)((ntohl(pcb->local_ip.addr) >> 24) & 0xff),
               (u16_t)((ntohl(pcb->local_ip.addr) >> 16) & 0xff),
               (u16_t)((ntohl(pcb->local_ip.addr) >> 8) & 0xff),
               (u16_t)(ntohl(pcb->local_ip.addr) & 0xff), pcb->local_port));
  return ERR_OK;
}
/**
 * Connect an UDP PCB.
 *
 * This will associate the UDP PCB with the remote address.
 *
 * @param pcb UDP PCB to be connected with remote address ipaddr and port.
 * @param ipaddr remote IP address to connect with.
 * @param port remote UDP port to connect with.
 *
 * @return lwIP error code
 *
 * ipaddr & port are expected to be in the same byte order as in the pcb.
 *
 * The udp pcb is bound to a random local port if not already bound.
 *
 * @see udp_disconnect()
 */
/* 为UDP控制块设置远端IP和远端端口，并设置UDP_FLAGS_CONNECTED标志 */
err_t
udp_connect(struct udp_pcb *pcb, struct ip_addr *ipaddr, u16_t port)
{
  struct udp_pcb *ipcb;

	/* 如果本地端口号未绑定，则绑定到本地随机端口 */
  if (pcb->local_port == 0) {
    err_t err = udp_bind(pcb, &pcb->local_ip, pcb->local_port);
		/* 绑定失败，则返回错误信息 */
    if (err != ERR_OK)
      return err;
  }

	/* 设置UDP控制块的remote_ip字段 */
  ip_addr_set(&pcb->remote_ip, ipaddr);
	/* 设置UDP控制块的remote_port字段，主机字节序 */
  pcb->remote_port = port;
	/* 设置UDP控制块状态为连接状态 */
  pcb->flags |= UDP_FLAGS_CONNECTED;
/** TODO: this functionality belongs in upper layers */
#ifdef LWIP_UDP_TODO
  /* Nail down local IP for netconn_addr()/getsockname() */
  if (ip_addr_isany(&pcb->local_ip) && !ip_addr_isany(&pcb->remote_ip)) {
    struct netif *netif;

    if ((netif = ip_route(&(pcb->remote_ip))) == NULL) {
      LWIP_DEBUGF(UDP_DEBUG, ("udp_connect: No route to 0x%lx\n", pcb->remote_ip.addr));
      UDP_STATS_INC(udp.rterr);
      return ERR_RTE;
    }
    /** TODO: this will bind the udp pcb locally, to the interface which
        is used to route output packets to the remote address. However, we
        might want to accept incoming packets on any interface! */
    pcb->local_ip = netif->ip_addr;
  } else if (ip_addr_isany(&pcb->remote_ip)) {
    pcb->local_ip.addr = 0;
  }
#endif
  LWIP_DEBUGF(UDP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE,
              ("udp_connect: connected to %"U16_F".%"U16_F".%"U16_F".%"U16_F",port %"U16_F"\n",
               (u16_t)((ntohl(pcb->remote_ip.addr) >> 24) & 0xff),
               (u16_t)((ntohl(pcb->remote_ip.addr) >> 16) & 0xff),
               (u16_t)((ntohl(pcb->remote_ip.addr) >> 8) & 0xff),
               (u16_t)(ntohl(pcb->remote_ip.addr) & 0xff), pcb->remote_port));

	/* 遍历udp_pcbs链表，查找pcb是否在链表中 */
  /* Insert UDP PCB into the list of active UDP PCBs. */
  for (ipcb = udp_pcbs; ipcb != NULL; ipcb = ipcb->next) {
		/* 如果pcb已经在链表中，则返回 */
    if (pcb == ipcb) {
      /* already on the list, just return */
      return ERR_OK;
    }
  }
	/* pcb不在udp_pcbs链表中，则插入到链表首部 */
  /* PCB not yet on the list, add PCB now */
  pcb->next = udp_pcbs;
  udp_pcbs = pcb;
  return ERR_OK;
}

/**
 * Disconnect a UDP PCB
 *
 * @param pcb the udp pcb to disconnect.
 */
/* 清除远端IP和远端端口，将控制块设置为非连接状态 */
void
udp_disconnect(struct udp_pcb *pcb)
{
  /* reset remote address association */
	/* 远端IP清0 */
  ip_addr_set(&pcb->remote_ip, IP_ADDR_ANY);
	/* 远端端口清0 */
  pcb->remote_port = 0;
  /* mark PCB as unconnected */
	/* 清除连接状态标志 */
  pcb->flags &= ~UDP_FLAGS_CONNECTED;
}

/**
 * Set a receive callback for a UDP PCB
 *
 * This callback will be called when receiving a datagram for the pcb.
 *
 * @param pcb the pcb for wich to set the recv callback
 * @param recv function pointer of the callback function
 * @param recv_arg additional argument to pass to the callback function
 */
/* 为UDP控制块注册回调函数 */
void
udp_recv(struct udp_pcb *pcb,
         void (* recv)(void *arg, struct udp_pcb *upcb, struct pbuf *p,
                       struct ip_addr *addr, u16_t port),
         void *recv_arg)
{
  /* remember recv() callback and user data */
	/* 填写UDP控制块的recv字段 */
  pcb->recv = recv;
	/* 填写UDP控制块的recv_arg字段 */
  pcb->recv_arg = recv_arg;
}

/**
 * Remove an UDP PCB.
 *
 * @param pcb UDP PCB to be removed. The PCB is removed from the list of
 * UDP PCB's and the data structure is freed from memory.
 *
 * @see udp_new()
 */
/* 把UDP控制块从udp_pcbs链表删除，并释放UDP控制块占用的内存 */
void
udp_remove(struct udp_pcb *pcb)
{
  struct udp_pcb *pcb2;

  snmp_delete_udpidx_tree(pcb);
  /* pcb to be removed is first in list? */
	/* 如果pcb是链表第一个节点，需要调整链表头指针 */
  if (udp_pcbs == pcb) {
    /* make list start at 2nd pcb */
    udp_pcbs = udp_pcbs->next;
    /* pcb not 1st in list */
  }
	/* pcb不是链表第一个节点，则需要调整它的前一个节点的next指针 */
	else
    for (pcb2 = udp_pcbs; pcb2 != NULL; pcb2 = pcb2->next) {
      /* find pcb in udp_pcbs list */
			/* 找到待删除节点的前一个节点，并调整它的next指针 */
      if (pcb2->next != NULL && pcb2->next == pcb) {
        /* remove pcb from list */
        pcb2->next = pcb->next;
      }
    }
	/* 释放被删除的节点占用的内存 */
  memp_free(MEMP_UDP_PCB, pcb);
}

/**
 * Create a UDP PCB.
 *
 * @return The UDP PCB which was created. NULL if the PCB data structure
 * could not be allocated.
 *
 * @see udp_remove()
 */
/* 新建UDP控制块 */
struct udp_pcb *
udp_new(void)
{
  struct udp_pcb *pcb;
	/* 从内存池中申请一个UDP控制块 */
  pcb = memp_malloc(MEMP_UDP_PCB);
  /* could allocate UDP PCB? */
	/* 申请成功 */
  if (pcb != NULL) {
    /* UDP Lite: by initializing to all zeroes, chksum_len is set to 0
     * which means checksum is generated over the whole datagram per default
     * (recommended as default by RFC 3828). */
    /* initialize PCB to all zeroes */
		/* 把控制块全部字段清0 */
    memset(pcb, 0, sizeof(struct udp_pcb));
		/* 设置UDP控制块TTL字段 */
    pcb->ttl = UDP_TTL;
  }
  return pcb;
}

#if UDP_DEBUG
/**
 * Print UDP header information for debug purposes.
 *
 * @param udphdr pointer to the udp header in memory.
 */
void
udp_debug_print(struct udp_hdr *udphdr)
{
  LWIP_DEBUGF(UDP_DEBUG, ("UDP header:\n"));
  LWIP_DEBUGF(UDP_DEBUG, ("+-------------------------------+\n"));
  LWIP_DEBUGF(UDP_DEBUG, ("|     %5"U16_F"     |     %5"U16_F"     | (src port, dest port)\n",
                          ntohs(udphdr->src), ntohs(udphdr->dest)));
  LWIP_DEBUGF(UDP_DEBUG, ("+-------------------------------+\n"));
  LWIP_DEBUGF(UDP_DEBUG, ("|     %5"U16_F"     |     0x%04"X16_F"    | (len, chksum)\n",
                          ntohs(udphdr->len), ntohs(udphdr->chksum)));
  LWIP_DEBUGF(UDP_DEBUG, ("+-------------------------------+\n"));
}
#endif /* UDP_DEBUG */

#endif /* LWIP_UDP */
