/**
 * @file
 * Ethernet Interface Skeleton
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

/*
 * This file is a skeleton for developing Ethernet network interface
 * drivers for lwIP. Add code to the low_level functions and do a
 * search-and-replace for the word "ethernetif" to replace it with
 * something that better describes your network interface.
 */

#include "lwip/opt.h"

#if 1 /* don't build, this is only a skeleton, see previous comment */

#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include <lwip/stats.h>
#include <lwip/snmp.h>
#include "netif/etharp.h"
#include "netif/ppp_oe.h"
#include "enc28j60.h"
#include "netif/ethernetif.h"
#include "string.h"
//#include "delay.h"

/* Define those to better describe your network interface. */
/* 定义以太网网卡的名字 */
#define IFNAME0 'e'
#define IFNAME1 'n'

//MAC地址
const u8 mymac[6]={0x04,0x02,0x35,0x00,0x00,0x01};	//MAC地址
//定义发送接受缓冲区
u8 lwip_buf[1518 - 4];


/**
 * Helper struct to hold private data used to operate your ethernet interface.
 * Keeping the ethernet address of the MAC in this struct is not necessary
 * as it is already kept in the struct netif.
 * But this is only an example, anyway...
 */
/* 用于网卡驱动私有数据，这里使用struct eth_addr *ethaddr仅是为了演示驱动私有数据的使用
 * 并没有实际意义
 */
struct ethernetif {
  struct eth_addr *ethaddr;
  /* Add whatever per-interface state that is needed here. */
};

/* Forward declarations. */
//static void  ethernetif_input(struct netif *netif);

/**
 * In this function, the hardware should be initialized.
 * Called from ethernetif_init().
 *
 * @param netif the already initialized lwip network interface structure
 *        for this ethernetif
 */
static err_t
low_level_init(struct netif *netif)
{
//  struct ethernetif *ethernetif = netif->state;
  /* set MAC hardware address length */
  netif->hwaddr_len = ETHARP_HWADDR_LEN;

  /* set MAC hardware address */
  netif->hwaddr[0] = mymac[0];
  netif->hwaddr[1] = mymac[1];
  netif->hwaddr[2] = mymac[2];
  netif->hwaddr[3] = mymac[3];
  netif->hwaddr[4] = mymac[4];
  netif->hwaddr[5] = mymac[5];
	
  /* maximum transfer unit */
  netif->mtu = MAX_FRAMELEN; 
  if(ENC28J60_Init((u8*)mymac))	//初始化ENC28J60	
  {
		return ERR_IF;			//底层网络接口错误
  }
  //指示灯状态:0x476 is PHLCON LEDA(绿)=links status, LEDB(红)=receive/transmit
  //PHLCON：PHY 模块LED 控制寄存器	    
  ENC28J60_PHY_Write(PHLCON,0x0476);  	
	
  /* device capabilities */
  /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
   
   return ERR_OK;
  /* Do whatever else is needed to initialize interface. */  
}

/**
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 *
 * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
 *       strange results. You might consider waiting for space in the DMA queue
 *       to become availale since the stack doesn't retry to send a packet
 *       dropped because of memory failure (except for the TCP timers).
 */

static err_t
low_level_output(struct netif *netif, struct pbuf *p)
{
//  struct ethernetif *ethernetif = netif->state;
  struct pbuf *q;
  int send_len=0;
 // initiate transfer();
  
#if ETH_PAD_SIZE
  pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif

	/* pbuf中的数据过长，无法复制到发送缓冲区 */
	if(p->tot_len > sizeof(lwip_buf))
	{
		LINK_STATS_INC(link.err);
		return ERR_IF;
	}
	
	/* 把pbuf中的数据复制的lwip_buf中，以写入enc28j60 */
  for(q = p; q != NULL; q = q->next) {
    /* Send the data from the pbuf to the interface, one pbuf at a
       time. The size of the data in each pbuf is kept in the ->len
       variable. */
    //send data from(q->payload, q->len);
	memcpy((u8_t*)&lwip_buf[send_len], (u8_t*)q->payload, q->len);
	send_len +=q->len;
  }
   // signal that packet should be sent();
  ENC28J60_Packet_Send(send_len,lwip_buf);

#if ETH_PAD_SIZE
  pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif
  
	/* 已经发送数据包数量+1 */
  LINK_STATS_INC(link.xmit);

  return ERR_OK;
}

/**
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return a pbuf filled with the received packet (including MAC header)
 *         NULL on memory error
 */
static struct pbuf *
low_level_input(struct netif *netif)
{
//  struct ethernetif *ethernetif = netif->state;
  struct pbuf *p, *q;
  u16_t len;
  int rev_len=0;
	
  /* Obtain the size of the packet and put it into the "len"
     variable. */
	/* 接收的字节最大长度为1518 - 4 = 1514，即不包含CRC校验的以太网帧最大长度 */
  len = ENC28J60_Packet_Receive(MAX_FRAMELEN - 4,lwip_buf);

#if ETH_PAD_SIZE
  len += ETH_PAD_SIZE; /* allow room for Ethernet padding */
#endif

	/* 检查以太网帧里是否是ARP帧或者是IP帧，都不符合的话丢弃本帧数据 */
//	if(...)
//	{
//		return NULL;
//	}
  /* We allocate a pbuf chain of pbufs from the pool. */
  /* PBUF_RAW表示不预留offset空间，offset空间通常是放置IP数据报或TCP数据报头结构的 */
  p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
  
  if (p != NULL) {

#if ETH_PAD_SIZE
    pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif

    /* We iterate over the pbuf chain until we have read the entire
     * packet into the pbuf. */
    for(q = p; q != NULL; q = q->next) {
      /* Read enough bytes to fill this pbuf in the chain. The
       * available data in the pbuf is given by the q->len
       * variable.
       * This does not necessarily have to be a memcpy, you can also preallocate
       * pbufs for a DMA-enabled MAC and after receiving truncate it to the
       * actually received size. In this case, ensure the tot_len member of the
       * pbuf is the sum of the chained pbuf len members.
       */
      //read data into(q->payload, q->len);
		memcpy((u8_t*)q->payload, (u8_t*)&lwip_buf[rev_len],q->len);
		rev_len +=q->len;
		
    }
   // acknowledge that packet has been read();

#if ETH_PAD_SIZE
    pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

    LINK_STATS_INC(link.recv);
  } else {
    //drop packet();
    LINK_STATS_INC(link.memerr);
    LINK_STATS_INC(link.drop);
  }

  return p;  
}

/**
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface. Then the type of the received packet is determined and
 * the appropriate input function is called.
 *
 * @param netif the lwip network interface structure for this ethernetif
 */
/* 由设备驱动调用，内部会调用netif->input即ethernet_input()，
 * 把数据传递给协议栈上层
 */
void ethernetif_input(struct netif *netif)
{
//  struct ethernetif *ethernetif;
	//以太网帧头
  struct eth_hdr *ethhdr;
	//pbuf缓冲区
  struct pbuf *p;

 // ethernetif = netif->state;

  /* move received packet into a new pbuf */
  p = low_level_input(netif);
  /* no packet could be read, silently ignore this */
  if (p == NULL) return;
  /* points to packet payload, which starts with an Ethernet header */
	//获取从low_level_input读取到的缓冲区首地址，里面保存了以太网帧
  ethhdr = p->payload;

	//htons()是把一个网络字节序的unsigned short转换为主机字节序
  switch (htons(ethhdr->type)) {
  /* IP or ARP packet? */
  case ETHTYPE_IP:
  case ETHTYPE_ARP:
#if PPPOE_SUPPORT
  /* PPPoE packet? */
  case ETHTYPE_PPPOEDISC:
  case ETHTYPE_PPPOE:
#endif /* PPPOE_SUPPORT */
    /* full packet send to tcpip_thread to process */
		//把pbuf提交给上层处理
		//对于enc28j60接口，netif->input=ethernet_input()，在netif_add()时被设置
    if (netif->input(p, netif)!=ERR_OK)
     { LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
       pbuf_free(p);
       p = NULL;
     }
    break;
	
	//不是ARP协议且不是IP协议，则直接丢弃
  default:
    pbuf_free(p);
    p = NULL;
    break;
  }
}

/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
/* 作为默认的以太网接口初始化函数，在netif_add()时被调用
 * 会调用low_level_init()完成网卡底层初始化
 */
err_t ethernetif_init(struct netif *netif)
{
	/* 定义驱动私有数据指针 */
  struct ethernetif *ethernetif;

  LWIP_ASSERT("netif != NULL", (netif != NULL));

	/* 申请驱动私有数据空间 */
  ethernetif = mem_malloc(sizeof(struct ethernetif));
  if (ethernetif == NULL) {
    LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_init: out of memory\n"));
    return ERR_MEM;
  }

#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

  /*
   * Initialize the snmp variables and counters inside the struct netif.
   * The last argument should be replaced with your link speed, in units
   * of bits per second.
   */
  NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd, LINK_SPEED_OF_YOUR_NETIF_IN_BPS);

	/* 把netif中的state字段指向驱动私有数据 */
  netif->state = ethernetif;
	/* 设置netif中的名字字段 */
  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;
  /* We directly use etharp_output() here to save a function call.
   * You can instead declare your own function an call etharp_output()
   * from it if you have to do some checks before sending (e.g. if link
   * is available...) */
	/* IP数据包输出函数 */
  netif->output = etharp_output;
	/* 链路层数据包输出函数，发送以太网帧的方法 */
  netif->linkoutput = low_level_output;
  
	/* 设置私有数据的某字段的值 */
  ethernetif->ethaddr = (struct eth_addr *)&(netif->hwaddr[0]);
  
  /* initialize the hardware */
	/* 调用网卡底层初始化函数 */
  return low_level_init(netif);

}

#endif /* 0 */
