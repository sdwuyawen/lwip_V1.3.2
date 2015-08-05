/**
 * @file
 * ICMP - Internet Control Message Protocol
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

/* Some ICMP messages should be passed to the transport protocols. This
   is not implemented. */

#include "lwip/opt.h"

#if LWIP_ICMP /* don't build if not configured for use in lwipopts.h */

#include "lwip/icmp.h"
#include "lwip/inet.h"
#include "lwip/inet_chksum.h"
#include "lwip/ip.h"
#include "lwip/def.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"

#include <string.h>

/*
ping xxx.xxx.xxx.xxx -l 6000�Ĳ��Խ��:

ip_input: iphdr->dest 0xe9c9c2ca netif->ip_addr 0xe9c9c2ca (0xc9c2ca, 0xc9c2ca, 0xe9000000)
ip_input: packet accepted on interface en
IP packet is a fragment (id=0x313d tot_len=1500 len=1500 MF=1 offset=0), calling ip_reass()
ip_input: iphdr->dest 0xe9c9c2ca netif->ip_addr 0xe9c9c2ca (0xc9c2ca, 0xc9c2ca, 0xe9000000)
ip_input: packet accepted on interface en
IP packet is a fragment (id=0x313d tot_len=1500 len=1500 MF=1 offset=1480), calling ip_reass()
ip_input: iphdr->dest 0xe9c9c2ca netif->ip_addr 0xe9c9c2ca (0xc9c2ca, 0xc9c2ca, 0xe9000000)
ip_input: packet accepted on interface en
IP packet is a fragment (id=0x313d tot_len=1500 len=1500 MF=1 offset=2960), calling ip_reass()
ip_input: iphdr->dest 0xe9c9c2ca netif->ip_addr 0xe9c9c2ca (0xc9c2ca, 0xc9c2ca, 0xe9000000)
ip_input: packet accepted on interface en
IP packet is a fragment (id=0x313d tot_len=1500 len=1500 MF=1 offset=4440), calling ip_reass()
ip_input: iphdr->dest 0xe9c9c2ca netif->ip_addr 0xe9c9c2ca (0xc9c2ca, 0xc9c2ca, 0xe9000000)
ip_input: packet accepted on interface en
IP packet is a fragment (id=0x313d tot_len=108 len=108 MF=0 offset=5920), calling ip_reass()
ip_input: 
IP header:
+-------------------------------+
| 4 | 5 |  0x00 |      6028     | (v, hl, tos, len)
+-------------------------------+
|    12605      |000|       0   | (id, flags, offset)
+-------------------------------+
|   64  |    1  |    0x08ff     | (ttl, proto, chksum)
+-------------------------------+
|  202  |  xxx  |  xxx  |  198  | (src)
+-------------------------------+
|  202  |  xxx  |  xxx  |  233  | (dest)
+-------------------------------+
ip_input: p->len 1500 p->tot_len 6028
icmp_input: ping
ip_output_if: en0
IP header:
+-------------------------------+
| 4 | 5 |  0x00 |      6028     | (v, hl, tos, len)
+-------------------------------+
|    12605      |000|       0   | (id, flags, offset)
+-------------------------------+
|  255  |    1  |    0x49fe     | (ttl, proto, chksum)
+-------------------------------+
|  202  |  xxx  |  xxx  |  233  | (src)
+-------------------------------+
|  202  |  xxx  |  xxx  |  198  | (dest)
+-------------------------------+
low_level_output() send 1514 length packet
low_level_output() send 1514 length packet
low_level_output() send 1514 length packet
low_level_output() send 1514 length packet
low_level_output() send 122 length packet

*/

/** Small optimization: set to 0 if incoming PBUF_POOL pbuf always can be
 * used to modify and send a response packet (and to 1 if this is not the case,
 * e.g. when link header is stripped of when receiving) */
#ifndef LWIP_ICMP_ECHO_CHECK_INPUT_PBUF_LEN
#define LWIP_ICMP_ECHO_CHECK_INPUT_PBUF_LEN 1
#endif /* LWIP_ICMP_ECHO_CHECK_INPUT_PBUF_LEN */

/* The amount of data from the original packet to return in a dest-unreachable */
/* ��Ŀ�Ĳ��ɴﱨ���У����ص��������IP���ݱ���Ч�غɳ���
 * TCP��UDP�ײ���ǰ8�ֽڶ������˿ںţ�����Ŀ�Ĳ��ɴ����ĵ�������IP����Ը��ݶ˿ں�
 * ��ICMP���Ĵ��ݸ��ϲ㴦��
 */
#define ICMP_DEST_UNREACH_DATASIZE 8

static void icmp_send_response(struct pbuf *p, u8_t type, u8_t code);

/**
 * Processes ICMP input packets, called from ip_input().
 *
 * Currently only processes icmp echo requests and sends
 * out the echo response.
 *
 * @param p the icmp echo request packet, p->payload pointing to the ip header
 * @param inp the netif on which this packet was received
 */
/* ����;����ethernetif_input() -> ethernet_input() -> ip_input() -> icmp_input()*/
void
icmp_input(struct pbuf *p, struct netif *inp)
{
  u8_t type;
#ifdef LWIP_DEBUG
  u8_t code;
#endif /* LWIP_DEBUG */
  struct icmp_echo_hdr *iecho;
  struct ip_hdr *iphdr;
	/* IP ��ַ */
  struct ip_addr tmpaddr;
  s16_t hlen;

  ICMP_STATS_INC(icmp.recv);
  snmp_inc_icmpinmsgs();


	/* ָ��IP�ײ� */
  iphdr = p->payload;
	/* ���IP��ͷ���� */
  hlen = IPH_HL(iphdr) * 4;
	/* pbuf��payload����ƶ���IP�غɣ���ICMP��ͷ��
	 * ���IP���غ�С��4�ֽڣ�������lenerr�����ͷ�pbuf
	 */
  if (pbuf_header(p, -hlen) || (p->tot_len < sizeof(u16_t)*2)) {
    LWIP_DEBUGF(ICMP_DEBUG, ("icmp_input: short ICMP (%"U16_F" bytes) received\n", p->tot_len));
    goto lenerr;
  }

	/* ��ȡICMP��ͷ�е����� */
  type = *((u8_t *)p->payload);
#ifdef LWIP_DEBUG
  code = *(((u8_t *)p->payload)+1);
#endif /* LWIP_DEBUG */
  switch (type) {
	/* ���ICMP�����ǻ������� */
  case ICMP_ECHO:
		/* �ȼ��Ŀ��IP��ַ�Ƿ�Ϸ� */
#if !LWIP_MULTICAST_PING || !LWIP_BROADCAST_PING
    {
			/* accepted��ʾ�Ƿ��ICMP����������л�Ӧ */
      int accepted = 1;
#if !LWIP_MULTICAST_PING
      /* multicast destination address? */
			/* ���Ŀ��IP��ַ�Ƕಥ��ַ���򲻻�Ӧ */
      if (ip_addr_ismulticast(&iphdr->dest)) {
        accepted = 0;
      }
#endif /* LWIP_MULTICAST_PING */
#if !LWIP_BROADCAST_PING
      /* broadcast destination address? */
			/* ���Ŀ��IP��ַ�ǹ㲥��ַ���򲻻�Ӧ */
      if (ip_addr_isbroadcast(&iphdr->dest, inp)) {
        accepted = 0;
      }
#endif /* LWIP_BROADCAST_PING */
      /* broadcast or multicast destination address not acceptd? */
			/* �������Ӧ�����ͷ�pbuf�󣬷��� */
      if (!accepted) {
        LWIP_DEBUGF(ICMP_DEBUG, ("icmp_input: Not echoing to multicast or broadcast pings\n"));
        ICMP_STATS_INC(icmp.err);
        pbuf_free(p);
        return;
      }
    }
#endif /* !LWIP_MULTICAST_PING || !LWIP_BROADCAST_PING */
    LWIP_DEBUGF(ICMP_DEBUG, ("icmp_input: ping\n"));
		/* ���ICMP���ĳ����Ƿ�Ϸ�,ICMP�����ܳ��Ȳ���С��ICMP��ͷ����8�ֽ� */
    if (p->tot_len < sizeof(struct icmp_echo_hdr)) {
      LWIP_DEBUGF(ICMP_DEBUG, ("icmp_input: bad ICMP echo received\n"));
			/* ����lenerr��ִ�з��ز��� */
      goto lenerr;
    }
		/* ����ICMP��У����Ƿ���ȷ */
    if (inet_chksum_pbuf(p) != 0) {
      LWIP_DEBUGF(ICMP_DEBUG, ("icmp_input: checksum failed for received ICMP echo\n"));
			/* ICMPУ��ʹ������ͷ�pbuf�������� */
      pbuf_free(p);
      ICMP_STATS_INC(icmp.chkerr);
      snmp_inc_icmpinerrors();
      return;
    }
#if LWIP_ICMP_ECHO_CHECK_INPUT_PBUF_LEN
    if (pbuf_header(p, (PBUF_IP_HLEN + PBUF_LINK_HLEN))) {
      /* p is not big enough to contain link headers
       * allocate a new one and copy p into it
       */
      struct pbuf *r;
      /* switch p->payload to ip header */
      if (pbuf_header(p, hlen)) {
        LWIP_ASSERT("icmp_input: moving p->payload to ip header failed\n", 0);
        goto memerr;
      }
      /* allocate new packet buffer with space for link headers */
      r = pbuf_alloc(PBUF_LINK, p->tot_len, PBUF_RAM);
      if (r == NULL) {
        LWIP_DEBUGF(ICMP_DEBUG, ("icmp_input: allocating new pbuf failed\n"));
        goto memerr;
      }
      LWIP_ASSERT("check that first pbuf can hold struct the ICMP header",
                  (r->len >= hlen + sizeof(struct icmp_echo_hdr)));
      /* copy the whole packet including ip header */
      if (pbuf_copy(r, p) != ERR_OK) {
        LWIP_ASSERT("icmp_input: copying to new pbuf failed\n", 0);
        goto memerr;
      }
      iphdr = r->payload;
      /* switch r->payload back to icmp header */
      if (pbuf_header(r, -hlen)) {
        LWIP_ASSERT("icmp_input: restoring original p->payload failed\n", 0);
        goto memerr;
      }
      /* free the original p */
      pbuf_free(p);
      /* we now have an identical copy of p that has room for link headers */
      p = r;
    } else {
      /* restore p->payload to point to icmp header */
      if (pbuf_header(p, -(s16_t)(PBUF_IP_HLEN + PBUF_LINK_HLEN))) {
        LWIP_ASSERT("icmp_input: restoring original p->payload failed\n", 0);
        goto memerr;
      }
    }
#endif /* LWIP_ICMP_ECHO_CHECK_INPUT_PBUF_LEN */
    /* At this point, all checks are OK. */
    /* We generate an answer by switching the dest and src ip addresses,
     * setting the icmp type to ECHO_RESPONSE and updating the checksum. */
		/* У����ɣ�����ICMP�������������ֶΣ����ɻ���Ӧ�� 
		 * ����IP���ݱ���ԴIP��Ŀ��IP��ַ����дICMP���ĵ������ֶΣ������¼���ICMP��У���
		 */
		/* ��ȡICMP��ͷָ�� */
    iecho = p->payload;
		/* ����IP��ͷ��ԴIP��ַ��Ŀ��IP��ַ */
    tmpaddr.addr = iphdr->src.addr;
    iphdr->src.addr = iphdr->dest.addr;
    iphdr->dest.addr = tmpaddr.addr;
		/* ����ICMP��������Ϊ����Ӧ�� */
    ICMPH_TYPE_SET(iecho, ICMP_ER);
    /* adjust the checksum */
		/* ����ICMP��У��� */
    if (iecho->chksum >= htons(0xffff - (ICMP_ECHO << 8))) {
      iecho->chksum += htons(ICMP_ECHO << 8) + 1;
    } else {
      iecho->chksum += htons(ICMP_ECHO << 8);
    }

    /* Set the correct TTL and recalculate the header checksum. */
		/* ����IP�ײ���TTL */
    IPH_TTL_SET(iphdr, ICMP_TTL);
		/* ����IP�ײ�У��� */
    IPH_CHKSUM_SET(iphdr, 0);
#if CHECKSUM_GEN_IP
    IPH_CHKSUM_SET(iphdr, inet_chksum(iphdr, IP_HLEN));
#endif /* CHECKSUM_GEN_IP */
		/* ע�⣺����û���޸�IP�ײ��ı�ʶ�ֶΣ�����ICMP����Ӧ���IP�ײ��ı�ʶ�ֶκ�
		 * ICMP��������ı�ʶ�ֶ�����ͬ�ġ���������˵ICMP����Ӧ���IP�ײ���ʶ�ֶ�Ӧ��
		 * ���޸ģ���Ϊʲô���޸ģ�
		 * ��Linux��������ping��Linux�����ظ���ICMP����Ӧ���IP�ײ��ı�ʶ�ֶξ��޸���
		 */

    ICMP_STATS_INC(icmp.xmit);
    /* increase number of messages attempted to send */
    snmp_inc_icmpoutmsgs();
    /* increase number of echo replies attempted to send */
    snmp_inc_icmpoutechoreps();

		/* pbuf��payload��ICMP�ı�ͷ�ƶ���IP�ı�ͷ��hlen������IP��ͷ�ĳ��� */
    if(pbuf_header(p, hlen)) {
      LWIP_ASSERT("Can't move over header in packet", 0);
			/* �ƶ�ʧ�� */
    } else {
			/* �ƶ��ɹ� */
      err_t ret;
			/* ����ip_output_if����IP���ݱ���IP_HDRINCL��ʾIP�ײ��Ѿ���д�ã�����
			 * pbuf��payloadָ��IP���ݱ��ײ���������IP�غ��ײ�
			 */
      ret = ip_output_if(p, &(iphdr->src), IP_HDRINCL,
                   ICMP_TTL, 0, IP_PROTO_ICMP, inp);
      if (ret != ERR_OK) {
        LWIP_DEBUGF(ICMP_DEBUG, ("icmp_input: ip_output_if returned an error: %c.\n", ret));
      }
    }
    break;
		
	/* ���ICMP���Ͳ���ICMP����������ֱ�Ӻ��� */
  default:
    LWIP_DEBUGF(ICMP_DEBUG, ("icmp_input: ICMP type %"S16_F" code %"S16_F" not supported.\n", 
                (s16_t)type, (s16_t)code));
		/* ����ͳ���� */
    ICMP_STATS_INC(icmp.proterr);
    ICMP_STATS_INC(icmp.drop);
  }
	/* �ͷ�pbuf */
  pbuf_free(p);
  return;
lenerr:
  pbuf_free(p);
  ICMP_STATS_INC(icmp.lenerr);
  snmp_inc_icmpinerrors();
  return;
#if LWIP_ICMP_ECHO_CHECK_INPUT_PBUF_LEN
memerr:
  pbuf_free(p);
  ICMP_STATS_INC(icmp.err);
  snmp_inc_icmpinerrors();
  return;
#endif /* LWIP_ICMP_ECHO_CHECK_INPUT_PBUF_LEN */
}

/**
 * Send an icmp 'destination unreachable' packet, called from ip_input() if
 * the transport layer protocol is unknown and from udp_input() if the local
 * port is not bound.
 *
 * @param p the input packet for which the 'unreachable' should be sent,
 *          p->payload pointing to the IP header
 * @param t type of the 'unreachable' packet
 */
/* ����һ��Ŀ�ĵ�ַ���ɴ�����
 * pbuf��������ı���
 * t��Ҫ��д��ICMP�����ײ��Ĵ����ֶ�
 */
void
icmp_dest_unreach(struct pbuf *p, enum icmp_dur_type t)
{
	/* ICMP_DUR��ICMP���ĵ��ײ��������ֶ� */
  icmp_send_response(p, ICMP_DUR, t);
}

#if IP_FORWARD || IP_REASSEMBLY
/**
 * Send a 'time exceeded' packet, called from ip_forward() if TTL is 0.
 *
 * @param p the input packet for which the 'time exceeded' should be sent,
 *          p->payload pointing to the IP header
 * @param t type of the 'time exceeded' packet
 */
/* ����һ�����ݱ���ʱ����� */
void
icmp_time_exceeded(struct pbuf *p, enum icmp_te_type t)
{
	/* ICMP_TE��ICMP���ĵ��ײ��������ֶ� */
  icmp_send_response(p, ICMP_TE, t);
}

#endif /* IP_FORWARD || IP_REASSEMBLY */

/**
 * Send an icmp packet in response to an incoming packet.
 *
 * @param p the input packet for which the 'unreachable' should be sent,
 *          p->payload pointing to the IP header
 * @param type Type of the ICMP header
 * @param code Code of the ICMP header
 */
/* ����һ��ICMP����� */
static void
icmp_send_response(struct pbuf *p, u8_t type, u8_t code)
{
  struct pbuf *q;
  struct ip_hdr *iphdr;
  /* we can use the echo header here */
	/* �û���������ײ�����������ײ� */
  struct icmp_echo_hdr *icmphdr;

  /* ICMP header + IP header + 8 bytes of data */
	/* ��IP������һ��pbuf�����Զ�Ԥ��IP�ײ�����̫��֡�ײ���
	 * ����Ĵ�С�ǻ��������ײ� + �������IP���ݱ��ײ���С + IP���ݱ��غɵ�ǰ8�ֽ�
	 */
  q = pbuf_alloc(PBUF_IP, sizeof(struct icmp_echo_hdr) + IP_HLEN + ICMP_DEST_UNREACH_DATASIZE,
                 PBUF_RAM);
  if (q == NULL) {
		/* ����ʧ�ܣ��򷵻� */
    LWIP_DEBUGF(ICMP_DEBUG, ("icmp_time_exceeded: failed to allocate pbuf for ICMP packet.\n"));
    return;
  }
  LWIP_ASSERT("check that first pbuf can hold icmp message",
             (q->len >= (sizeof(struct icmp_echo_hdr) + IP_HLEN + ICMP_DEST_UNREACH_DATASIZE)));

	/* ָ���������IP���ݱ� */
  iphdr = p->payload;
  LWIP_DEBUGF(ICMP_DEBUG, ("icmp_time_exceeded from "));
  ip_addr_debug_print(ICMP_DEBUG, &(iphdr->src));
  LWIP_DEBUGF(ICMP_DEBUG, (" to "));
  ip_addr_debug_print(ICMP_DEBUG, &(iphdr->dest));
  LWIP_DEBUGF(ICMP_DEBUG, ("\n"));

	/* ָ��ICMP�����ײ� */
  icmphdr = q->payload;
	/* ��д�����ֶ� */
  icmphdr->type = type;
	/* ��д�����ֶ� */
  icmphdr->code = code;
	/* ICMP�ײ���4-7�ֽ���0 */
  icmphdr->id = 0;
  icmphdr->seqno = 0;

  /* copy fields from original packet */
	/* �����������IP���ݱ��ײ�+8�ֽڵ�ICMP�����ֶ� */
  SMEMCPY((u8_t *)q->payload + sizeof(struct icmp_echo_hdr), (u8_t *)p->payload,
          IP_HLEN + ICMP_DEST_UNREACH_DATASIZE);

  /* calculate checksum */
	/* ���㲢��дУ��� */
  icmphdr->chksum = 0;
  icmphdr->chksum = inet_chksum(icmphdr, q->len);
  ICMP_STATS_INC(icmp.xmit);
  /* increase number of messages attempted to send */
  snmp_inc_icmpoutmsgs();
  /* increase number of destination unreachable messages attempted to send */
  snmp_inc_icmpouttimeexcds();
	/* ����IP�㺯�����ICMP���� */
  ip_output(q, NULL, &(iphdr->src), ICMP_TTL, 0, IP_PROTO_ICMP);
	/* ������ɣ��ͷ������ICMP�õ�pbuf */
  pbuf_free(q);
}

#endif /* LWIP_ICMP */
