/**
 * @file
 * This is the IPv4 layer implementation for incoming and outgoing IP traffic.
 * 
 * @see ip_frag.c
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

#include "lwip/opt.h"
#include "lwip/ip.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/ip_frag.h"
#include "lwip/inet.h"
#include "lwip/inet_chksum.h"
#include "lwip/netif.h"
#include "lwip/icmp.h"
#include "lwip/igmp.h"
#include "lwip/raw.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"
#include "lwip/snmp.h"
#include "lwip/dhcp.h"
#include "lwip/stats.h"
#include "arch/perf.h"

#include <string.h>

/**
 * The interface that provided the packet for the current callback
 * invocation.
 */
/* 当前处理的IP数据报的来自哪个网络接口 */
struct netif *current_netif;

/**
 * Header of the input packet currently being processed.
 */
/* 当前处理的IP数据报首部 */
const struct ip_hdr *current_header;

/**
 * Finds the appropriate network interface for a given IP address. It
 * searches the list of network interfaces linearly. A match is found
 * if the masked IP address of the network interface equals the masked
 * IP address given to the function.
 *
 * @param dest the destination IP address for which to find the route
 * @return the netif on which to send to reach dest
 */
/* 根据目的IP地址选择一个最合适的网络接口
 * 先查找网络接口的IP有没有和目的IP在同一个子网的，如果有，则选择该接口
 * 如果所有接口IP和目的IP都不在同一个子网，则选择默认接口
 */
struct netif *
ip_route(struct ip_addr *dest)
{
  struct netif *netif;

  /* iterate through netifs */
	/* 遍历netif_list链表，检查启动的网络接口中
	 * 有没有和目的IP在同一个子网的
	 */
  for(netif = netif_list; netif != NULL; netif = netif->next) {
    /* network mask matches? */
		/* 接口已经启动 */
    if (netif_is_up(netif)) {
			/* 接口IP和目的IP在同一个子网 */
      if (ip_addr_netcmp(dest, &(netif->ip_addr), &(netif->netmask))) {
        /* return netif on which to forward IP packet */
				/* 返回该接口 */
        return netif;
      }
    }
  }
	
	/* 执行到这里，说明没找到合适的接口，返回系统的默认接口 */
  if ((netif_default == NULL) || (!netif_is_up(netif_default))) {
    LWIP_DEBUGF(IP_DEBUG | LWIP_DBG_LEVEL_SERIOUS, ("ip_route: No route to 0x%"X32_F"\n", dest->addr));
		/* 默认接口未设置，或者默认接口设置了但未启动，返回NULL */
    IP_STATS_INC(ip.rterr);
    snmp_inc_ipoutnoroutes();
    return NULL;
  }
	/* 返回默认网络接口 */
  /* no matching netif found, use default netif */
  return netif_default;
}

#if IP_FORWARD
/**
 * Forwards an IP packet. It finds an appropriate route for the
 * packet, decrements the TTL value of the packet, adjusts the
 * checksum and outputs the packet on the appropriate interface.
 *
 * @param p the packet to forward (p->payload points to IP header)
 * @param iphdr the IP header of the input packet
 * @param inp the netif on which this packet was received
 * @return the netif on which the packet was sent (NULL if it wasn't sent)
 */
static struct netif *
ip_forward(struct pbuf *p, struct ip_hdr *iphdr, struct netif *inp)
{
  struct netif *netif;

  PERF_START;
  /* Find network interface where to forward this IP packet to. */
  netif = ip_route((struct ip_addr *)&(iphdr->dest));
  if (netif == NULL) {
    LWIP_DEBUGF(IP_DEBUG, ("ip_forward: no forwarding route for 0x%"X32_F" found\n",
                      iphdr->dest.addr));
    snmp_inc_ipoutnoroutes();
    return (struct netif *)NULL;
  }
  /* Do not forward packets onto the same network interface on which
   * they arrived. */
  if (netif == inp) {
    LWIP_DEBUGF(IP_DEBUG, ("ip_forward: not bouncing packets back on incoming interface.\n"));
    snmp_inc_ipoutnoroutes();
    return (struct netif *)NULL;
  }

  /* decrement TTL */
  IPH_TTL_SET(iphdr, IPH_TTL(iphdr) - 1);
  /* send ICMP if TTL == 0 */
  if (IPH_TTL(iphdr) == 0) {
    snmp_inc_ipinhdrerrors();
#if LWIP_ICMP
    /* Don't send ICMP messages in response to ICMP messages */
    if (IPH_PROTO(iphdr) != IP_PROTO_ICMP) {
      icmp_time_exceeded(p, ICMP_TE_TTL);
    }
#endif /* LWIP_ICMP */
    return (struct netif *)NULL;
  }

  /* Incrementally update the IP checksum. */
  if (IPH_CHKSUM(iphdr) >= htons(0xffff - 0x100)) {
    IPH_CHKSUM_SET(iphdr, IPH_CHKSUM(iphdr) + htons(0x100) + 1);
  } else {
    IPH_CHKSUM_SET(iphdr, IPH_CHKSUM(iphdr) + htons(0x100));
  }

  LWIP_DEBUGF(IP_DEBUG, ("ip_forward: forwarding packet to 0x%"X32_F"\n",
                    iphdr->dest.addr));

  IP_STATS_INC(ip.fw);
  IP_STATS_INC(ip.xmit);
  snmp_inc_ipforwdatagrams();

  PERF_STOP("ip_forward");
  /* transmit pbuf on chosen interface */
  netif->output(netif, p, (struct ip_addr *)&(iphdr->dest));
  return netif;
}
#endif /* IP_FORWARD */

/**
 * This function is called by the network interface device driver when
 * an IP packet is received. The function does the basic checks of the
 * IP header such as packet size being at least larger than the header
 * size etc. If the packet was not destined for us, the packet is
 * forwarded (using ip_forward). The IP checksum is always checked.
 *
 * Finally, the packet is sent to the upper layer protocol input function.
 * 
 * @param p the received IP packet (p->payload points to IP header)
 * @param inp the netif on which this packet was received
 * @return ERR_OK if the packet was processed (could return ERR_* if it wasn't
 *         processed, but currently always returns ERR_OK)
 */
/* 调用途径：ethernetif_input() -> ethernet_input() -> ip_input() */
err_t
ip_input(struct pbuf *p, struct netif *inp)
{
	/* IP数据报首部指针 */
  struct ip_hdr *iphdr;
  struct netif *netif;
	/* 数据报首部长度 */
  u16_t iphdr_hlen;
	/* 数据报总长度 */
  u16_t iphdr_len;
#if LWIP_DHCP
  int check_ip_src=1;
#endif /* LWIP_DHCP */

	/* 更新统计量 */
  IP_STATS_INC(ip.recv);
  snmp_inc_ipinreceives();

	/* 获取IP数据报首部指针 */
  /* identify the IP header */
  iphdr = p->payload;
	/* 如果IP数据报的版本号不是4，则返回错误 */
  if (IPH_V(iphdr) != 4) {
    LWIP_DEBUGF(IP_DEBUG | LWIP_DBG_LEVEL_WARNING, ("IP packet dropped due to bad version number %"U16_F"\n", IPH_V(iphdr)));
    ip_debug_print(p);
		/* 释放pbuf */
    pbuf_free(p);
		/* 更新统计量 */
    IP_STATS_INC(ip.err);
    IP_STATS_INC(ip.drop);
    snmp_inc_ipinhdrerrors();
    return ERR_OK;
  }

	/* 获取IP首部长度，单位是4字节 */
  /* obtain IP header length in number of 32-bit words */
  iphdr_hlen = IPH_HL(iphdr);
  /* calculate IP header length in bytes */
	/* 获取IP首部长度字节数 */
  iphdr_hlen *= 4;
  /* obtain ip length in bytes */
	/* 获取IP数据报总长度，转换为主机字节序 */
  iphdr_len = ntohs(IPH_LEN(iphdr));

  /* header length exceeds first pbuf length, or ip length exceeds total pbuf length? */
	/* 当IP数据报首部长度大于第一个pbuf中所有数据长度
	 * 或者IP首部中指示的IP数据报总长度大于pbuf链表所有数据长度时，丢弃数据报
	 */
  if ((iphdr_hlen > p->len) || (iphdr_len > p->tot_len)) {
		/* 首部长度大于第一个pbuf中所有数据长度 */
    if (iphdr_hlen > p->len) {
      LWIP_DEBUGF(IP_DEBUG | LWIP_DBG_LEVEL_SERIOUS,
        ("IP header (len %"U16_F") does not fit in first pbuf (len %"U16_F"), IP packet dropped.\n",
        iphdr_hlen, p->len));
    }
		/* IP首部中指示的IP数据报总长度大于pbuf链表所有数据长度 */
    if (iphdr_len > p->tot_len) {
      LWIP_DEBUGF(IP_DEBUG | LWIP_DBG_LEVEL_SERIOUS,
        ("IP (len %"U16_F") is longer than pbuf (len %"U16_F"), IP packet dropped.\n",
        iphdr_len, p->tot_len));
    }
    /* free (drop) packet pbufs */
		/* 释放pbuf链表 */
    pbuf_free(p);
		/* 更新统计量 */
    IP_STATS_INC(ip.lenerr);
    IP_STATS_INC(ip.drop);
    snmp_inc_ipindiscards();
    return ERR_OK;
  }

  /* verify checksum */
	/*  计算校验和*/
#if CHECKSUM_CHECK_IP
	/* 接收数据报首部的校验和计算结果应为0 */
  if (inet_chksum(iphdr, iphdr_hlen) != 0) {

    LWIP_DEBUGF(IP_DEBUG | LWIP_DBG_LEVEL_SERIOUS,
      ("Checksum (0x%"X16_F") failed, IP packet dropped.\n", inet_chksum(iphdr, iphdr_hlen)));
    ip_debug_print(p);
		/* 校验和错误，释放pbuf链表 */
    pbuf_free(p);
		/* 更新统计量 */
    IP_STATS_INC(ip.chkerr);
    IP_STATS_INC(ip.drop);
    snmp_inc_ipinhdrerrors();
    return ERR_OK;
  }
#endif

  /* Trim pbuf. This should have been done at the netif layer,
   * but we'll do it anyway just to be sure that its done. */
	/* 调整pbuf长度，删除以太网帧尾部的填充字节 */
  pbuf_realloc(p, iphdr_len);

  /* match packet against an interface, i.e. is this packet for us? */
#if LWIP_IGMP
  if (ip_addr_ismulticast(&(iphdr->dest))) {
    if ((inp->flags & NETIF_FLAG_IGMP) && (igmp_lookfor_group(inp, &(iphdr->dest)))) {
      netif = inp;
    } else {
      netif = NULL;
    }
  } else
#endif /* LWIP_IGMP */
	
	/* 对比所有网络接口的IP地址 */
  {
    /* start trying with inp. if that's not acceptable, start walking the
       list of configured netifs.
       'first' is used as a boolean to mark whether we started walking the list */
		/* 是否已经开始遍历netif_list链表 */
    int first = 1;
		/* 首先判断IP数据报的目的地址是不是接收到数据包的网络接口 */
    netif = inp;
    do {
      LWIP_DEBUGF(IP_DEBUG, ("ip_input: iphdr->dest 0x%"X32_F" netif->ip_addr 0x%"X32_F" (0x%"X32_F", 0x%"X32_F", 0x%"X32_F")\n",
          iphdr->dest.addr, netif->ip_addr.addr,
          iphdr->dest.addr & netif->netmask.addr,
          netif->ip_addr.addr & netif->netmask.addr,
          iphdr->dest.addr & ~(netif->netmask.addr)));

      /* interface is up and configured? */
			/* 接口已经使能，且IP地址已经配置 */
      if ((netif_is_up(netif)) && (!ip_addr_isany(&(netif->ip_addr)))) {
        /* unicast to this interface address? */
				/* IP数据报目的IP和该网络接口IP地址相同
				 * 或者是该网络接口的广播地址
				 */
        if (ip_addr_cmp(&(iphdr->dest), &(netif->ip_addr)) ||
            /* or broadcast on this interface network address? */
            ip_addr_isbroadcast(&(iphdr->dest), netif)) {
          LWIP_DEBUGF(IP_DEBUG, ("ip_input: packet accepted on interface %c%c\n",
              netif->name[0], netif->name[1]));
          /* break out of for loop */
					/* 找到对应的网络接口，跳出循环 */
          break;
        }
      }
			/* 目的IP地址不是接收到数据报的网络接口，则遍历接口链表 */
      if (first) {
        first = 0;
        netif = netif_list;
      } else {
        netif = netif->next;
      }
			/* 如果遍历的接口是接收数据报的接口，已经判断过，继续判断下一个接口 */
      if (netif == inp) {
        netif = netif->next;
      }
    } while(netif != NULL);
  }

#if LWIP_DHCP
  /* Pass DHCP messages regardless of destination address. DHCP traffic is addressed
   * using link layer addressing (such as Ethernet MAC) so we must not filter on IP.
   * According to RFC 1542 section 3.1.1, referred by RFC 2131).
   */
  if (netif == NULL) {
    /* remote port is DHCP server? */
    if (IPH_PROTO(iphdr) == IP_PROTO_UDP) {
      LWIP_DEBUGF(IP_DEBUG | LWIP_DBG_TRACE, ("ip_input: UDP packet to DHCP client port %"U16_F"\n",
        ntohs(((struct udp_hdr *)((u8_t *)iphdr + iphdr_hlen))->dest)));
      if (ntohs(((struct udp_hdr *)((u8_t *)iphdr + iphdr_hlen))->dest) == DHCP_CLIENT_PORT) {
        LWIP_DEBUGF(IP_DEBUG | LWIP_DBG_TRACE, ("ip_input: DHCP packet accepted.\n"));
        netif = inp;
        check_ip_src = 0;
      }
    }
  }
#endif /* LWIP_DHCP */

  /* broadcast or multicast packet source address? Compliant with RFC 1122: 3.2.1.3 */
#if LWIP_DHCP
  /* DHCP servers need 0.0.0.0 to be allowed as source address (RFC 1.1.2.2: 3.2.1.3/a) */
  if (check_ip_src && (iphdr->src.addr != 0))
#endif /* LWIP_DHCP */
	
	/* 校验源IP地址有效性，不能为接口的广播地址或者是多播地址 */
  {  if ((ip_addr_isbroadcast(&(iphdr->src), inp)) ||
         (ip_addr_ismulticast(&(iphdr->src)))) {
      /* packet source is not valid */
      LWIP_DEBUGF(IP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING, ("ip_input: packet source is not valid.\n"));
      /* free (drop) packet pbufs */
			/* 源IP地址无效的数据报直接丢弃 */
			/* 释放pbuf链表 */
      pbuf_free(p);
			/* 更新统计量 */
      IP_STATS_INC(ip.drop);
      snmp_inc_ipinaddrerrors();
      snmp_inc_ipindiscards();
      return ERR_OK;
    }
  }

  /* packet not for us? */
	/* IP数据报的目的IP地址不是任何一个网络接口的IP地址 */
  if (netif == NULL) {
    /* packet not for us, route or discard */
    LWIP_DEBUGF(IP_DEBUG | LWIP_DBG_TRACE, ("ip_input: packet not for us.\n"));
#if IP_FORWARD
    /* non-broadcast packet? */
		/* 如果使能了IP转发，则转发该数据报
		 * 判断目的IP是不是网络接口对应的广播IP，若是，则不转发，否则就转发
		 */
    if (!ip_addr_isbroadcast(&(iphdr->dest), inp)) {
      /* try to forward IP packet on (other) interfaces */
      ip_forward(p, iphdr, inp);
    } else
#endif /* IP_FORWARD */
    {
      snmp_inc_ipinaddrerrors();
      snmp_inc_ipindiscards();
    }
		/* IP转发完成后，释放pbuf链表 */
    pbuf_free(p);
    return ERR_OK;
  }
	
	/* 执行到这里，说明该IP数据报目的IP地址是接收该IP数据报的网络接口IP地址 */
  /* packet consists of multiple fragments? */
	/* 如果片偏移量不为0，或者更多分片标志不为0，说明是分片的数据报
	 (分片中的最后一个数据报更多分片标志位0，但是片偏移量不为0)
	 */
  if ((IPH_OFFSET(iphdr) & htons(IP_OFFMASK | IP_MF)) != 0) {
#if IP_REASSEMBLY /* packet fragment reassembly code present? */
    LWIP_DEBUGF(IP_DEBUG, ("IP packet is a fragment (id=0x%04"X16_F" tot_len=%"U16_F" len=%"U16_F" MF=%"U16_F" offset=%"U16_F"), calling ip_reass()\n",
      ntohs(IPH_ID(iphdr)), p->tot_len, ntohs(IPH_LEN(iphdr)), !!(IPH_OFFSET(iphdr) & htons(IP_MF)), (ntohs(IPH_OFFSET(iphdr)) & IP_OFFMASK)*8));
    /* reassemble the packet*/
		/* 允许分片重装，则调用ip_reass重装数据报 */
    p = ip_reass(p);
    /* packet not fully reassembled yet? */
		/* p==NULL表示重装未全部完成，分片的数据报未全部达到，则返回 */
    if (p == NULL) {
      return ERR_OK;
    }
		/* 分片重装完成，则iphdr指向重装后的IP数据报首部 */
    iphdr = p->payload;
#else /* IP_REASSEMBLY == 0, no packet fragment reassembly code present */
    pbuf_free(p);
    LWIP_DEBUGF(IP_DEBUG | LWIP_DBG_LEVEL_SERIOUS, ("IP packet dropped since it was fragmented (0x%"X16_F") (while IP_REASSEMBLY == 0).\n",
      ntohs(IPH_OFFSET(iphdr))));
    IP_STATS_INC(ip.opterr);
    IP_STATS_INC(ip.drop);
    /* unsupported protocol feature */
    snmp_inc_ipinunknownprotos();
    return ERR_OK;
#endif /* IP_REASSEMBLY */
  }

#if IP_OPTIONS_ALLOWED == 0 /* no support for IP options in the IP header? */

#if LWIP_IGMP
  /* there is an extra "router alert" option in IGMP messages which we allow for but do not police */
  if((iphdr_hlen > IP_HLEN &&  (IPH_PROTO(iphdr) != IP_PROTO_IGMP)) {
#else
  if (iphdr_hlen > IP_HLEN) {
#endif /* LWIP_IGMP */
    LWIP_DEBUGF(IP_DEBUG | LWIP_DBG_LEVEL_SERIOUS, ("IP packet dropped since there were IP options (while IP_OPTIONS_ALLOWED == 0).\n"));
    pbuf_free(p);
    IP_STATS_INC(ip.opterr);
    IP_STATS_INC(ip.drop);
    /* unsupported protocol feature */
    snmp_inc_ipinunknownprotos();
    return ERR_OK;
  }
#endif /* IP_OPTIONS_ALLOWED == 0 */

  /* send to upper layers */
  LWIP_DEBUGF(IP_DEBUG, ("ip_input: \n"));
  ip_debug_print(p);
  LWIP_DEBUGF(IP_DEBUG, ("ip_input: p->len %"U16_F" p->tot_len %"U16_F"\n", p->len, p->tot_len));

	/* 执行到这里，不论IP数据报是否经过重装，都已经是一个完整的IP数据报,
	 * 向上层提交
	 */
	/* 接收当前处理的IP数据报的网络接口 */
  current_netif = inp;
	/* 当前处理的IP数据报首部 */
  current_header = iphdr;

#if LWIP_RAW
  /* raw input did not eat the packet? */
	/* 为用户直接与IP数据报交互预留的接口
	 * 返回0表示用户回调函数没有处理该数据报
	 * 其他值表示用户回调函数已经处理了该数据报，并且已经释放了pbuf链表
	 */
  if (raw_input(p, inp) == 0)
#endif /* LWIP_RAW */
  {
		/* 根据IP首部中的协议字段，提交给不同的传输层 */
    switch (IPH_PROTO(iphdr)) {
#if LWIP_UDP
		/* UDP协议数据 */
    case IP_PROTO_UDP:
#if LWIP_UDPLITE
    case IP_PROTO_UDPLITE:
#endif /* LWIP_UDPLITE */
      snmp_inc_ipindelivers();
      udp_input(p, inp);
      break;
#endif /* LWIP_UDP */
#if LWIP_TCP
		/* TCP协议数据 */
    case IP_PROTO_TCP:
      snmp_inc_ipindelivers();
      tcp_input(p, inp);
      break;
#endif /* LWIP_TCP */
#if LWIP_ICMP
		/* ICMP协议数据 */
    case IP_PROTO_ICMP:
      snmp_inc_ipindelivers();
      icmp_input(p, inp);
      break;
#endif /* LWIP_ICMP */
#if LWIP_IGMP
		/* IGMP协议数据 */
    case IP_PROTO_IGMP:
      igmp_input(p,inp,&(iphdr->dest));
      break;
#endif /* LWIP_IGMP */
		/* 找不到对应的上层协议，则返回ICMP目的不可达报文 */
    default:
#if LWIP_ICMP
      /* send ICMP destination protocol unreachable unless is was a broadcast */
			/* 发送ICMP目的不可达报文，除非IP数据报的目的IP地址是广播或者是多播地址 */
      if (!ip_addr_isbroadcast(&(iphdr->dest), inp) &&
          !ip_addr_ismulticast(&(iphdr->dest))) {
				/* pbuf的payload指向IP数据报首部 */
        p->payload = iphdr;
				/* 发送ICMP目的不可达报文 */
        icmp_dest_unreach(p, ICMP_DUR_PROTO);
      }
#endif /* LWIP_ICMP */
			/* 释放pbuf链表 */
      pbuf_free(p);

      LWIP_DEBUGF(IP_DEBUG | LWIP_DBG_LEVEL_SERIOUS, ("Unsupported transport protocol %"U16_F"\n", IPH_PROTO(iphdr)));
			/* 更新统计量 */
      IP_STATS_INC(ip.proterr);
      IP_STATS_INC(ip.drop);
      snmp_inc_ipinunknownprotos();
    }
  }

	/* 接收当前处理IP数据报的网络接口清0 */
  current_netif = NULL;
	/* 当前处理IP数据报的首部指针清0 */
  current_header = NULL;

  return ERR_OK;
}

/**
 * Sends an IP packet on a network interface. This function constructs
 * the IP header and calculates the IP header checksum. If the source
 * IP address is NULL, the IP address of the outgoing network
 * interface is filled in as source address.
 * If the destination IP address is IP_HDRINCL, p is assumed to already
 * include an IP header and p->payload points to it instead of the data.
 *
 * @param p the packet to send (p->payload points to the data, e.g. next
            protocol header; if dest == IP_HDRINCL, p already includes an IP
            header and p->payload points to that IP header)
 * @param src the source IP address to send from (if src == IP_ADDR_ANY, the
 *         IP  address of the netif used to send is used as source address)
 * @param dest the destination IP address to send the packet to
 * @param ttl the TTL value to be set in the IP header
 * @param tos the TOS value to be set in the IP header
 * @param proto the PROTOCOL to be set in the IP header
 * @param netif the netif on which to send this packet
 * @return ERR_OK if the packet was sent OK
 *         ERR_BUF if p doesn't have enough space for IP/LINK headers
 *         returns errors returned by netif->output
 *
 * @note ip_id: RFC791 "some host may be able to simply use
 *  unique identifiers independent of destination"
 */
/* 填写IP首部中各个字段的值(不包括选项字段)
 * 并通过netif发送数据报
 */
err_t
ip_output_if(struct pbuf *p, struct ip_addr *src, struct ip_addr *dest,
             u8_t ttl, u8_t tos,
             u8_t proto, struct netif *netif)
{
#if IP_OPTIONS_SEND
  return ip_output_if_opt(p, src, dest, ttl, tos, proto, netif, NULL, 0);
}

/**
 * Same as ip_output_if() but with the possibility to include IP options:
 *
 * @ param ip_options pointer to the IP options, copied into the IP header
 * @ param optlen length of ip_options
 */
err_t ip_output_if_opt(struct pbuf *p, struct ip_addr *src, struct ip_addr *dest,
       u8_t ttl, u8_t tos, u8_t proto, struct netif *netif, void *ip_options,
       u16_t optlen)
{
#endif /* IP_OPTIONS_SEND */
  struct ip_hdr *iphdr;
	/* 记录IP数据报编号，即标识字段 */
  static u16_t ip_id = 0;

  snmp_inc_ipoutrequests();

	/* dest为IP_HDRINCL时，表示pbuf中已经填写好了IP首部，且payload已经指向了IP首部
	 * dest不为IP_HDRINCL时，dest是目的IP地址，payload指向IP数据报包裹的上一层协议的首部，
	 * IP首部还未填写
	 */
  /* Should the IP header be generated or is it already included in p? */
  if (dest != IP_HDRINCL) {
		/* IP首部长度20字节 */
    u16_t ip_hlen = IP_HLEN;
#if IP_OPTIONS_SEND
    u16_t optlen_aligned = 0;
    if (optlen != 0) {
      /* round up to a multiple of 4 */
      optlen_aligned = ((optlen + 3) & ~3);
      ip_hlen += optlen_aligned;
      /* First write in the IP options */
      if (pbuf_header(p, optlen_aligned)) {
        LWIP_DEBUGF(IP_DEBUG | LWIP_DBG_LEVEL_SERIOUS, ("ip_output_if_opt: not enough room for IP options in pbuf\n"));
        IP_STATS_INC(ip.err);
        snmp_inc_ipoutdiscards();
        return ERR_BUF;
      }
      MEMCPY(p->payload, ip_options, optlen);
      if (optlen < optlen_aligned) {
        /* zero the remaining bytes */
        memset(((char*)p->payload) + optlen, 0, optlen_aligned - optlen);
      }
    }
#endif /* IP_OPTIONS_SEND */
    /* generate IP header */
		/* 移动pbuf的payload指针，向前移动20字节到IP首部 */
    if (pbuf_header(p, IP_HLEN)) {
			/* 移动失败，则返回错误信息 */
      LWIP_DEBUGF(IP_DEBUG | LWIP_DBG_LEVEL_SERIOUS, ("ip_output: not enough room for IP header in pbuf\n"));

      IP_STATS_INC(ip.err);
      snmp_inc_ipoutdiscards();
      return ERR_BUF;
    }
		/* iphdr指向IP首部 */
    iphdr = p->payload;
    LWIP_ASSERT("check that first pbuf can hold struct ip_hdr",
               (p->len >= sizeof(struct ip_hdr)));

		/* 填写TTL */
    IPH_TTL_SET(iphdr, ttl);
		/* 填写协议 */
    IPH_PROTO_SET(iphdr, proto);

		/* 填写目的IP地址 */
    ip_addr_set(&(iphdr->dest), dest);

		/* 填写版本号、首部长度、服务类型 */
    IPH_VHLTOS_SET(iphdr, 4, ip_hlen / 4, tos);
		/* 填写IP报文总长度 */
    IPH_LEN_SET(iphdr, htons(p->tot_len));
		/* 填写分片偏移量 */
    IPH_OFFSET_SET(iphdr, 0);
		/* 填写报文标识 */
    IPH_ID_SET(iphdr, htons(ip_id));
		/* 数据报编号+1 */
    ++ip_id;

		/* 传入的源IP地址为空，则把源IP地址填写为网络接口的IP地址 */
    if (ip_addr_isany(src)) {
      ip_addr_set(&(iphdr->src), &(netif->ip_addr));
    }
		/* 传入的源IP地址不为空，则把源IP地址填写到IP报头 */
		else {
      ip_addr_set(&(iphdr->src), src);
    }

		/* 清0IP报头中的首部校验和 */
    IPH_CHKSUM_SET(iphdr, 0);
#if CHECKSUM_GEN_IP
		/* 计算并填写首部校验和 */
    IPH_CHKSUM_SET(iphdr, inet_chksum(iphdr, ip_hlen));
#endif
  }
	/* dest为IP_HDRINCL时，表示pbuf中已经填写好了IP首部，且payload已经指向了IP首部 */
	else {
    /* IP header already included in p */
		/* 设置iphdr指针 */
    iphdr = p->payload;
		/* 获取上层填写好的IP报头中的目的IP地址 */
    dest = &(iphdr->dest);
  }

	/* 更新发送统计量 */
  IP_STATS_INC(ip.xmit);

  LWIP_DEBUGF(IP_DEBUG, ("ip_output_if: %c%c%"U16_F"\n", netif->name[0], netif->name[1], netif->num));
  ip_debug_print(p);

#if ENABLE_LOOPBACK
	/* 如果目的IP地址和网络接口IP地址相同，则调用环回输出
	 */
  if (ip_addr_cmp(dest, &netif->ip_addr)) {
    /* Packet to self, enqueue it for loopback */
    LWIP_DEBUGF(IP_DEBUG, ("netif_loop_output()"));
		/* 把pbuf挂到netif的loop_first链表上 */
    return netif_loop_output(netif, p, dest);
  }
#endif /* ENABLE_LOOPBACK */
#if IP_FRAG
  /* don't fragment if interface has mtu set to 0 [loopif] */
	/* 如果网络接口的MTU已经设置，且数据包长度大于MTU，则进行分片 */
  if (netif->mtu && (p->tot_len > netif->mtu)) {
    return ip_frag(p,netif,dest);
  }
#endif

  LWIP_DEBUGF(IP_DEBUG, ("netif->output()"));
	/* 不是环回IP地址，且不用分片，则调用etharp_output()发送pbuf */
  return netif->output(netif, p, dest);
}

/**
 * Simple interface to ip_output_if. It finds the outgoing network
 * interface and calls upon ip_output_if to do the actual work.
 *
 * @param p the packet to send (p->payload points to the data, e.g. next
            protocol header; if dest == IP_HDRINCL, p already includes an IP
            header and p->payload points to that IP header)
 * @param src the source IP address to send from (if src == IP_ADDR_ANY, the
 *         IP  address of the netif used to send is used as source address)
 * @param dest the destination IP address to send the packet to
 * @param ttl the TTL value to be set in the IP header
 * @param tos the TOS value to be set in the IP header
 * @param proto the PROTOCOL to be set in the IP header
 *
 * @return ERR_RTE if no route is found
 *         see ip_output_if() for more return values
 */
/* 被传输层函数调用完成数据报的发送
 * 查找网络接口并调用ip_output_if()完成最终的发送工作
 */
err_t
ip_output(struct pbuf *p, struct ip_addr *src, struct ip_addr *dest,
          u8_t ttl, u8_t tos, u8_t proto)
{
  struct netif *netif;

	/* 根据目的IP地址为数据报寻找一个合适的网络接口 */
  if ((netif = ip_route(dest)) == NULL) {
		/* 若找不到，则返回错误信息 */
    LWIP_DEBUGF(IP_DEBUG, ("ip_output: No route to 0x%"X32_F"\n", dest->addr));
    IP_STATS_INC(ip.rterr);
    return ERR_RTE;
  }

	/* 找到合适的网络接口，则调用ip_output_if()，并传递netif参数 */
  return ip_output_if(p, src, dest, ttl, tos, proto, netif);
}

#if LWIP_NETIF_HWADDRHINT
/** Like ip_output, but takes and addr_hint pointer that is passed on to netif->addr_hint
 *  before calling ip_output_if.
 *
 * @param p the packet to send (p->payload points to the data, e.g. next
            protocol header; if dest == IP_HDRINCL, p already includes an IP
            header and p->payload points to that IP header)
 * @param src the source IP address to send from (if src == IP_ADDR_ANY, the
 *         IP  address of the netif used to send is used as source address)
 * @param dest the destination IP address to send the packet to
 * @param ttl the TTL value to be set in the IP header
 * @param tos the TOS value to be set in the IP header
 * @param proto the PROTOCOL to be set in the IP header
 * @param addr_hint address hint pointer set to netif->addr_hint before
 *        calling ip_output_if()
 *
 * @return ERR_RTE if no route is found
 *         see ip_output_if() for more return values
 */
err_t
ip_output_hinted(struct pbuf *p, struct ip_addr *src, struct ip_addr *dest,
          u8_t ttl, u8_t tos, u8_t proto, u8_t *addr_hint)
{
  struct netif *netif;
  err_t err;

  if ((netif = ip_route(dest)) == NULL) {
    LWIP_DEBUGF(IP_DEBUG, ("ip_output: No route to 0x%"X32_F"\n", dest->addr));
    IP_STATS_INC(ip.rterr);
    return ERR_RTE;
  }

  netif->addr_hint = addr_hint;
  err = ip_output_if(p, src, dest, ttl, tos, proto, netif);
  netif->addr_hint = NULL;

  return err;
}
#endif /* LWIP_NETIF_HWADDRHINT*/

#if IP_DEBUG
/* Print an IP header by using LWIP_DEBUGF
 * @param p an IP packet, p->payload pointing to the IP header
 */
void
ip_debug_print(struct pbuf *p)
{
  struct ip_hdr *iphdr = p->payload;
  u8_t *payload;

  payload = (u8_t *)iphdr + IP_HLEN;

  LWIP_DEBUGF(IP_DEBUG, ("IP header:\n"));
  LWIP_DEBUGF(IP_DEBUG, ("+-------------------------------+\n"));
  LWIP_DEBUGF(IP_DEBUG, ("|%2"S16_F" |%2"S16_F" |  0x%02"X16_F" |     %5"U16_F"     | (v, hl, tos, len)\n",
                    IPH_V(iphdr),
                    IPH_HL(iphdr),
                    IPH_TOS(iphdr),
                    ntohs(IPH_LEN(iphdr))));
  LWIP_DEBUGF(IP_DEBUG, ("+-------------------------------+\n"));
  LWIP_DEBUGF(IP_DEBUG, ("|    %5"U16_F"      |%"U16_F"%"U16_F"%"U16_F"|    %4"U16_F"   | (id, flags, offset)\n",
                    ntohs(IPH_ID(iphdr)),
                    ntohs(IPH_OFFSET(iphdr)) >> 15 & 1,
                    ntohs(IPH_OFFSET(iphdr)) >> 14 & 1,
                    ntohs(IPH_OFFSET(iphdr)) >> 13 & 1,
                    ntohs(IPH_OFFSET(iphdr)) & IP_OFFMASK));
  LWIP_DEBUGF(IP_DEBUG, ("+-------------------------------+\n"));
  LWIP_DEBUGF(IP_DEBUG, ("|  %3"U16_F"  |  %3"U16_F"  |    0x%04"X16_F"     | (ttl, proto, chksum)\n",
                    IPH_TTL(iphdr),
                    IPH_PROTO(iphdr),
                    ntohs(IPH_CHKSUM(iphdr))));
  LWIP_DEBUGF(IP_DEBUG, ("+-------------------------------+\n"));
  LWIP_DEBUGF(IP_DEBUG, ("|  %3"U16_F"  |  %3"U16_F"  |  %3"U16_F"  |  %3"U16_F"  | (src)\n",
                    ip4_addr1(&iphdr->src),
                    ip4_addr2(&iphdr->src),
                    ip4_addr3(&iphdr->src),
                    ip4_addr4(&iphdr->src)));
  LWIP_DEBUGF(IP_DEBUG, ("+-------------------------------+\n"));
  LWIP_DEBUGF(IP_DEBUG, ("|  %3"U16_F"  |  %3"U16_F"  |  %3"U16_F"  |  %3"U16_F"  | (dest)\n",
                    ip4_addr1(&iphdr->dest),
                    ip4_addr2(&iphdr->dest),
                    ip4_addr3(&iphdr->dest),
                    ip4_addr4(&iphdr->dest)));
  LWIP_DEBUGF(IP_DEBUG, ("+-------------------------------+\n"));
}
#endif /* IP_DEBUG */
