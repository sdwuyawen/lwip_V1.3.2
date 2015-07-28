/**
 * @file
 * Dynamic pool memory manager
 *
 * lwIP has dedicated pools for many structures (netconn, protocol control blocks,
 * packet buffers, ...). All these pools are managed here.
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

#include "lwip/memp.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/raw.h"
#include "lwip/tcp.h"
#include "lwip/igmp.h"
#include "lwip/api.h"
#include "lwip/api_msg.h"
#include "lwip/tcpip.h"
#include "lwip/sys.h"
#include "lwip/stats.h"
#include "netif/etharp.h"
#include "lwip/ip_frag.h"

#include <string.h>

#if !MEMP_MEM_MALLOC /* don't build if not configured for use in lwipopts.h */

struct memp {
  struct memp *next;
#if MEMP_OVERFLOW_CHECK
  const char *file;
  int line;
#endif /* MEMP_OVERFLOW_CHECK */
};

#if MEMP_OVERFLOW_CHECK
/* if MEMP_OVERFLOW_CHECK is turned on, we reserve some bytes at the beginning
 * and at the end of each element, initialize them as 0xcd and check
 * them later. */
/* If MEMP_OVERFLOW_CHECK is >= 2, on every call to memp_malloc or memp_free,
 * every single element in each pool is checked!
 * This is VERY SLOW but also very helpful. */
/* MEMP_SANITY_REGION_BEFORE and MEMP_SANITY_REGION_AFTER can be overridden in
 * lwipopts.h to change the amount reserved for checking. */
#ifndef MEMP_SANITY_REGION_BEFORE
#define MEMP_SANITY_REGION_BEFORE  16
#endif /* MEMP_SANITY_REGION_BEFORE*/
#if MEMP_SANITY_REGION_BEFORE > 0
#define MEMP_SANITY_REGION_BEFORE_ALIGNED    LWIP_MEM_ALIGN_SIZE(MEMP_SANITY_REGION_BEFORE)
#else
#define MEMP_SANITY_REGION_BEFORE_ALIGNED    0
#endif /* MEMP_SANITY_REGION_BEFORE*/
#ifndef MEMP_SANITY_REGION_AFTER
#define MEMP_SANITY_REGION_AFTER   16
#endif /* MEMP_SANITY_REGION_AFTER*/
#if MEMP_SANITY_REGION_AFTER > 0
#define MEMP_SANITY_REGION_AFTER_ALIGNED     LWIP_MEM_ALIGN_SIZE(MEMP_SANITY_REGION_AFTER)
#else
#define MEMP_SANITY_REGION_AFTER_ALIGNED     0
#endif /* MEMP_SANITY_REGION_AFTER*/

/* MEMP_SIZE: save space for struct memp and for sanity check */
#define MEMP_SIZE          (LWIP_MEM_ALIGN_SIZE(sizeof(struct memp)) + MEMP_SANITY_REGION_BEFORE_ALIGNED)
#define MEMP_ALIGN_SIZE(x) (LWIP_MEM_ALIGN_SIZE(x) + MEMP_SANITY_REGION_AFTER_ALIGNED)

#else /* MEMP_OVERFLOW_CHECK */

/* No sanity checks
 * We don't need to preserve the struct memp while not allocated, so we
 * can save a little space and set MEMP_SIZE to 0.
 */
#define MEMP_SIZE           0
#define MEMP_ALIGN_SIZE(x) (LWIP_MEM_ALIGN_SIZE(x))

#endif /* MEMP_OVERFLOW_CHECK */

/** This array holds the first free element of each pool.
 *  Elements form a linked list. */
/* 指向每类pool中第一个pool */
static struct memp *memp_tab[MEMP_MAX];

#else /* MEMP_MEM_MALLOC */

#define MEMP_ALIGN_SIZE(x) (LWIP_MEM_ALIGN_SIZE(x))

#endif /* MEMP_MEM_MALLOC */

/** This array holds the element sizes of each pool. */
/* 定义每种类型POOL的大小 
 * LWIP_MEM_ALIGN_SIZE(sizeof(struct raw_pcb))
 * LWIP_MEM_ALIGN_SIZE(sizeof(struct udp_pcb))
 * ...
 */
#if !MEM_USE_POOLS && !MEMP_MEM_MALLOC
static
#endif
const u16_t memp_sizes[MEMP_MAX] = {
#define LWIP_MEMPOOL(name,num,size,desc)  LWIP_MEM_ALIGN_SIZE(size),
#include "lwip/memp_std.h"
};

#if !MEMP_MEM_MALLOC /* don't build if not configured for use in lwipopts.h */

/** This array holds the number of elements in each pool. */
/* 定义每种POOL的个数
 * MEMP_NUM_RAW_PCB
 * MEMP_NUM_UDP_PCB
 * MEMP_NUM_TCP_PCB
 * ...
 */
static const u16_t memp_num[MEMP_MAX] = {
#define LWIP_MEMPOOL(name,num,size,desc)  (num),
#include "lwip/memp_std.h"
};

/** This array holds a textual description of each pool. */
/* 定义每种POOL的描述字符串
 * "RAW_PCB" 
 * "UDP_PCB"
 * "TCP_PCB"
 * ...
 */
#ifdef LWIP_DEBUG
static const char *memp_desc[MEMP_MAX] = {
#define LWIP_MEMPOOL(name,num,size,desc)  (desc),
#include "lwip/memp_std.h"
};
#endif /* LWIP_DEBUG */

/** This is the actual memory used by the pools. */
/* 真正的内存池区域，在内存中开辟空间
 * memp_memory[MEM_ALIGNMENT - 1 + () + () + () + ...]
 * MEMP_SIZE是在每种内存池头部预留的空间，设置为0
 */
static u8_t memp_memory[MEM_ALIGNMENT - 1 
#define LWIP_MEMPOOL(name,num,size,desc) + ( (num) * (MEMP_SIZE + MEMP_ALIGN_SIZE(size) ) )
#include "lwip/memp_std.h"
];

#if MEMP_SANITY_CHECK
/**
 * Check that memp-lists don't form a circle
 */
static int
memp_sanity(void)
{
  s16_t i, c;
  struct memp *m, *n;

  for (i = 0; i < MEMP_MAX; i++) {
    for (m = memp_tab[i]; m != NULL; m = m->next) {
      c = 1;
      for (n = memp_tab[i]; n != NULL; n = n->next) {
        if (n == m && --c < 0) {
          return 0;
        }
      }
    }
  }
  return 1;
}
#endif /* MEMP_SANITY_CHECK*/
#if MEMP_OVERFLOW_CHECK
/**
 * Check if a memp element was victim of an overflow
 * (e.g. the restricted area after it has been altered)
 *
 * @param p the memp element to check
 * @param memp_size the element size of the pool p comes from
 */
static void
memp_overflow_check_element(struct memp *p, u16_t memp_size)
{
  u16_t k;
  u8_t *m;
#if MEMP_SANITY_REGION_BEFORE_ALIGNED > 0
  m = (u8_t*)p + MEMP_SIZE - MEMP_SANITY_REGION_BEFORE_ALIGNED;
  for (k = 0; k < MEMP_SANITY_REGION_BEFORE_ALIGNED; k++) {
    if (m[k] != 0xcd) {
      LWIP_ASSERT("detected memp underflow!", 0);
    }
  }
#endif
#if MEMP_SANITY_REGION_AFTER_ALIGNED > 0
  m = (u8_t*)p + MEMP_SIZE + memp_size;
  for (k = 0; k < MEMP_SANITY_REGION_AFTER_ALIGNED; k++) {
    if (m[k] != 0xcd) {
      LWIP_ASSERT("detected memp overflow!", 0);
    }
  }
#endif
}

/**
 * Do an overflow check for all elements in every pool.
 *
 * @see memp_overflow_check_element for a description of the check
 */
static void
memp_overflow_check_all(void)
{
  u16_t i, j;
  struct memp *p;

  p = LWIP_MEM_ALIGN(memp_memory);
  for (i = 0; i < MEMP_MAX; ++i) {
    p = p;
    for (j = 0; j < memp_num[i]; ++j) {
      memp_overflow_check_element(p, memp_sizes[i]);
      p = (struct memp*)((u8_t*)p + MEMP_SIZE + memp_sizes[i] + MEMP_SANITY_REGION_AFTER_ALIGNED);
    }
  }
}

/**
 * Initialize the restricted areas of all memp elements in every pool.
 */
static void
memp_overflow_init(void)
{
  u16_t i, j;
  struct memp *p;
  u8_t *m;

  p = LWIP_MEM_ALIGN(memp_memory);
  for (i = 0; i < MEMP_MAX; ++i) {
    p = p;
    for (j = 0; j < memp_num[i]; ++j) {
#if MEMP_SANITY_REGION_BEFORE_ALIGNED > 0
      m = (u8_t*)p + MEMP_SIZE - MEMP_SANITY_REGION_BEFORE_ALIGNED;
      memset(m, 0xcd, MEMP_SANITY_REGION_BEFORE_ALIGNED);
#endif
#if MEMP_SANITY_REGION_AFTER_ALIGNED > 0
      m = (u8_t*)p + MEMP_SIZE + memp_sizes[i];
      memset(m, 0xcd, MEMP_SANITY_REGION_AFTER_ALIGNED);
#endif
      p = (struct memp*)((u8_t*)p + MEMP_SIZE + memp_sizes[i] + MEMP_SANITY_REGION_AFTER_ALIGNED);
    }
  }
}
#endif /* MEMP_OVERFLOW_CHECK */

/**
 * Initialize this module.
 * 
 * Carves out memp_memory into linked lists for each pool-type.
 */
void
memp_init(void)
{
  struct memp *memp;
  u16_t i, j;

  for (i = 0; i < MEMP_MAX; ++i) {
    MEMP_STATS_AVAIL(used, i, 0);
    MEMP_STATS_AVAIL(max, i, 0);
    MEMP_STATS_AVAIL(err, i, 0);
    MEMP_STATS_AVAIL(avail, i, memp_num[i]);
  }

	/* 把内存池起始地址向上对齐 */
  memp = LWIP_MEM_ALIGN(memp_memory);
  /* for every pool: */
	/* 把每种POOL内部的所有POOL链接成链表
	 * 每种POOL的链表头节点用memp_tab[]指示
	 */
  for (i = 0; i < MEMP_MAX; ++i) {
    memp_tab[i] = NULL;
    /* create a linked list of memp elements */
		/* 把该类所有POOL链接成链表 */
    for (j = 0; j < memp_num[i]; ++j) {
      memp->next = memp_tab[i];
			/* memp_tab[i]在内部循环完成后，表示该类POOL中的第一个POOL地址 */
      memp_tab[i] = memp;
			/* 该类POOL中的下一个POOL */
      memp = (struct memp *)((u8_t *)memp + MEMP_SIZE + memp_sizes[i]
#if MEMP_OVERFLOW_CHECK
        + MEMP_SANITY_REGION_AFTER_ALIGNED
#endif
      );
    }
  }
#if MEMP_OVERFLOW_CHECK
  memp_overflow_init();
  /* check everything a first time to see if it worked */
  memp_overflow_check_all();
#endif /* MEMP_OVERFLOW_CHECK */
}

/**
 * Get an element from a specific pool.
 *
 * @param type the pool to get an element from
 *
 * the debug version has two more parameters:
 * @param file file name calling this function
 * @param line number of line where this function is called
 *
 * @return a pointer to the allocated memory or a NULL pointer on error
 */
/* 从内存池type中申请一个内存块 */
void *
#if !MEMP_OVERFLOW_CHECK
memp_malloc(memp_t type)
#else
memp_malloc_fn(memp_t type, const char* file, const int line)
#endif
{
  struct memp *memp;
  SYS_ARCH_DECL_PROTECT(old_level);
 
  LWIP_ERROR("memp_malloc: type < MEMP_MAX", (type < MEMP_MAX), return NULL;);

	/* 进入临界区 */
  SYS_ARCH_PROTECT(old_level);
#if MEMP_OVERFLOW_CHECK >= 2
  memp_overflow_check_all();
#endif /* MEMP_OVERFLOW_CHECK >= 2 */

	/* 获取该类内存池中空闲块的链表头 */
  memp = memp_tab[type];
  
  if (memp != NULL) {
		/* 调整该类内存池空闲块的链表头 */
    memp_tab[type] = memp->next;
#if MEMP_OVERFLOW_CHECK
    memp->next = NULL;
    memp->file = file;
    memp->line = line;
#endif /* MEMP_OVERFLOW_CHECK */
		/* 统计该类POOL使用情况 */
    MEMP_STATS_INC_USED(used, type);
    LWIP_ASSERT("memp_malloc: memp properly aligned",
                ((mem_ptr_t)memp % MEM_ALIGNMENT) == 0);
		/* 申请到的内存池的首地址 */
    memp = (struct memp*)((u8_t*)memp + MEMP_SIZE);
  } else {
    LWIP_DEBUGF(MEMP_DEBUG | LWIP_DBG_LEVEL_SERIOUS, ("memp_malloc: out of memory in pool %s\n", memp_desc[type]));
    MEMP_STATS_INC(err, type);
  }

	/* 退出临界区 */
  SYS_ARCH_UNPROTECT(old_level);

  return memp;
}

/**
 * Put an element back into its pool.
 *
 * @param type the pool where to put mem
 * @param mem the memp element to free
 */
void
memp_free(memp_t type, void *mem)
{
  struct memp *memp;
  SYS_ARCH_DECL_PROTECT(old_level);

  if (mem == NULL) {
    return;
  }
  LWIP_ASSERT("memp_free: mem properly aligned",
                ((mem_ptr_t)mem % MEM_ALIGNMENT) == 0);

	/* 得到待释放的内存块的首地址 */
  memp = (struct memp *)((u8_t*)mem - MEMP_SIZE);

	/* 进入临界区 */
  SYS_ARCH_PROTECT(old_level);
#if MEMP_OVERFLOW_CHECK
#if MEMP_OVERFLOW_CHECK >= 2
  memp_overflow_check_all();
#else
  memp_overflow_check_element(memp, memp_sizes[type]);
#endif /* MEMP_OVERFLOW_CHECK >= 2 */
#endif /* MEMP_OVERFLOW_CHECK */

	/* 减少该类型POOL使用统计量 */
  MEMP_STATS_DEC(used, type); 
  
	/* 把POOL链接入该类POOL空闲块链表 */
  memp->next = memp_tab[type]; 
  memp_tab[type] = memp;

#if MEMP_SANITY_CHECK
  LWIP_ASSERT("memp sanity", memp_sanity());
#endif /* MEMP_SANITY_CHECK */

	/* 退出临界区 */
  SYS_ARCH_UNPROTECT(old_level);
}

#endif /* MEMP_MEM_MALLOC */
