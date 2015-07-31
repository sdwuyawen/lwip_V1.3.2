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
/* ������̫������������ */
#define IFNAME0 'e'
#define IFNAME1 'n'

//MAC��ַ
const u8 mymac[6]={0x04,0x02,0x35,0x00,0x00,0x01};	//MAC��ַ
//���巢�ͽ��ܻ�����
u8 lwip_buf[1518 - 4];


/**
 * Helper struct to hold private data used to operate your ethernet interface.
 * Keeping the ethernet address of the MAC in this struct is not necessary
 * as it is already kept in the struct netif.
 * But this is only an example, anyway...
 */
/* ������������˽�����ݣ�����ʹ��struct eth_addr *ethaddr����Ϊ����ʾ����˽�����ݵ�ʹ��
 * ��û��ʵ������
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
  if(ENC28J60_Init((u8*)mymac))	//��ʼ��ENC28J60	
  {
		return ERR_IF;			//�ײ�����ӿڴ���
  }
  //ָʾ��״̬:0x476 is PHLCON LEDA(��)=links status, LEDB(��)=receive/transmit
  //PHLCON��PHY ģ��LED ���ƼĴ���	    
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

	/* pbuf�е����ݹ������޷����Ƶ����ͻ����� */
	if(p->tot_len > sizeof(lwip_buf))
	{
		LINK_STATS_INC(link.err);
		return ERR_IF;
	}
	
	/* ��pbuf�е����ݸ��Ƶ�lwip_buf�У���д��enc28j60 */
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
  
	/* �Ѿ��������ݰ�����+1 */
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
	/* ���յ��ֽ���󳤶�Ϊ1518 - 4 = 1514����������CRCУ�����̫��֡��󳤶� */
  len = ENC28J60_Packet_Receive(MAX_FRAMELEN - 4,lwip_buf);

#if ETH_PAD_SIZE
  len += ETH_PAD_SIZE; /* allow room for Ethernet padding */
#endif

	/* �����̫��֡���Ƿ���ARP֡������IP֡���������ϵĻ�������֡���� */
//	if(...)
//	{
//		return NULL;
//	}
  /* We allocate a pbuf chain of pbufs from the pool. */
  /* PBUF_RAW��ʾ��Ԥ��offset�ռ䣬offset�ռ�ͨ���Ƿ���IP���ݱ���TCP���ݱ�ͷ�ṹ�� */
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
/* ���豸�������ã��ڲ������netif->input��ethernet_input()��
 * �����ݴ��ݸ�Э��ջ�ϲ�
 */
void ethernetif_input(struct netif *netif)
{
//  struct ethernetif *ethernetif;
	//��̫��֡ͷ
  struct eth_hdr *ethhdr;
	//pbuf������
  struct pbuf *p;

 // ethernetif = netif->state;

  /* move received packet into a new pbuf */
  p = low_level_input(netif);
  /* no packet could be read, silently ignore this */
  if (p == NULL) return;
  /* points to packet payload, which starts with an Ethernet header */
	//��ȡ��low_level_input��ȡ���Ļ������׵�ַ�����汣������̫��֡
  ethhdr = p->payload;

	//htons()�ǰ�һ�������ֽ����unsigned shortת��Ϊ�����ֽ���
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
		//��pbuf�ύ���ϲ㴦��
		//����enc28j60�ӿڣ�netif->input=ethernet_input()����netif_add()ʱ������
    if (netif->input(p, netif)!=ERR_OK)
     { LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
       pbuf_free(p);
       p = NULL;
     }
    break;
	
	//����ARPЭ���Ҳ���IPЭ�飬��ֱ�Ӷ���
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
/* ��ΪĬ�ϵ���̫���ӿڳ�ʼ����������netif_add()ʱ������
 * �����low_level_init()��������ײ��ʼ��
 */
err_t ethernetif_init(struct netif *netif)
{
	/* ��������˽������ָ�� */
  struct ethernetif *ethernetif;

  LWIP_ASSERT("netif != NULL", (netif != NULL));

	/* ��������˽�����ݿռ� */
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

	/* ��netif�е�state�ֶ�ָ������˽������ */
  netif->state = ethernetif;
	/* ����netif�е������ֶ� */
  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;
  /* We directly use etharp_output() here to save a function call.
   * You can instead declare your own function an call etharp_output()
   * from it if you have to do some checks before sending (e.g. if link
   * is available...) */
	/* IP���ݰ�������� */
  netif->output = etharp_output;
	/* ��·�����ݰ����������������̫��֡�ķ��� */
  netif->linkoutput = low_level_output;
  
	/* ����˽�����ݵ�ĳ�ֶε�ֵ */
  ethernetif->ethaddr = (struct eth_addr *)&(netif->hwaddr[0]);
  
  /* initialize the hardware */
	/* ���������ײ��ʼ������ */
  return low_level_init(netif);

}

#endif /* 0 */
