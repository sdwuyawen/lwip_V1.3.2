/**
 * @file
 * Dynamic memory manager
 *
 * This is a lightweight replacement for the standard C library malloc().
 *
 * If you want to use the standard C library malloc() instead, define
 * MEM_LIBC_MALLOC to 1 in your lwipopts.h
 *
 * To let mem_malloc() use pools (prevents fragmentation and is much faster than
 * a heap but might waste some memory), define MEM_USE_POOLS to 1, define
 * MEM_USE_CUSTOM_POOLS to 1 and create a file "lwippools.h" that includes a list
 * of pools like this (more pools can be added between _START and _END):
 *
 * Define three pools with sizes 256, 512, and 1512 bytes
 * LWIP_MALLOC_MEMPOOL_START
 * LWIP_MALLOC_MEMPOOL(20, 256)
 * LWIP_MALLOC_MEMPOOL(10, 512)
 * LWIP_MALLOC_MEMPOOL(5, 1512)
 * LWIP_MALLOC_MEMPOOL_END
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
 *         Simon Goldschmidt
 *
 */

#include "lwip/opt.h"

#if !MEM_LIBC_MALLOC /* don't build if not configured for use in lwipopts.h */

#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/sys.h"
#include "lwip/stats.h"

#include <string.h>

/* 如果选择用内存池实现内存堆分配 */
#if MEM_USE_POOLS
/* lwIP head implemented with different sized pools */

/**
 * Allocate memory: determine the smallest pool that is big enough
 * to contain an element of 'size' and get an element from that pool.
 *
 * @param size the size in bytes of the memory needed
 * @return a pointer to the allocated memory or NULL if the pool is empty
 */
void *
mem_malloc(mem_size_t size)
{
	/* 使用内存池分配内存堆时，在每个申请的POOL前
	 * 附加一个memp_malloc_helper类型的结构，标示该
	 * 内存属于哪种POOL
	 */
  struct memp_malloc_helper *element;
  memp_t poolnr;
	/* 修正申请空间 */
  mem_size_t required_size = size + sizeof(struct memp_malloc_helper);

	/* MEMP_POOL_FIRST记录了堆分配函数使用的POOL在memp_t中的起始编号 
	 * MEMP_POOL_LAST记录了堆分配函数使用的POOL在memp_t中的结束编号
	 */
  for (poolnr = MEMP_POOL_FIRST; poolnr <= MEMP_POOL_LAST; poolnr++) {
#if MEM_USE_POOLS_TRY_BIGGER_POOL
again:
#endif /* MEM_USE_POOLS_TRY_BIGGER_POOL */
    /* is this pool big enough to hold an element of the required size
       plus a struct memp_malloc_helper that saves the pool this element came from? */
    /* 查找能满足申请空间的POOL类型 */
		if (required_size <= memp_sizes[poolnr]) {
      break;
    }
  }
	/* 没有找到能满足申请空间的POOL，返回NULL */
  if (poolnr > MEMP_POOL_LAST) {
    LWIP_ASSERT("mem_malloc(): no pool is that big!", 0);
    return NULL;
  }
	/* 分配一个满足申请空间类型的POOL */
  element = (struct memp_malloc_helper*)memp_malloc(poolnr);
	/* POOL分配失败，返回NULL */
  if (element == NULL) {
    /* No need to DEBUGF or ASSERT: This error is already
       taken care of in memp.c */
#if MEM_USE_POOLS_TRY_BIGGER_POOL
    /** Try a bigger pool if this one is empty! */
    if (poolnr < MEMP_POOL_LAST) {
      poolnr++;
      goto again;
    }
#endif /* MEM_USE_POOLS_TRY_BIGGER_POOL */
    return NULL;
  }

	/* 在分配的空间中保存分配的POOL类型，用于free时使用 */
  /* save the pool number this element came from */
  element->poolnr = poolnr;
	/* 返回用户可用的空间首地址 */
  /* and return a pointer to the memory directly after the struct memp_malloc_helper */
  element++;

  return element;
}

/**
 * Free memory previously allocated by mem_malloc. Loads the pool number
 * and calls memp_free with that pool number to put the element back into
 * its pool
 *
 * @param rmem the memory element to free
 */
void
mem_free(void *rmem)
{
  struct memp_malloc_helper *hmem = (struct memp_malloc_helper*)rmem;

  LWIP_ASSERT("rmem != NULL", (rmem != NULL));
	/* 检查内存地址对齐 */
  LWIP_ASSERT("rmem == MEM_ALIGN(rmem)", (rmem == LWIP_MEM_ALIGN(rmem)));

  /* get the original struct memp_malloc_helper */
  /* 得到memp_malloc_helper的地址 */
	hmem--;

  LWIP_ASSERT("hmem != NULL", (hmem != NULL));
  LWIP_ASSERT("hmem == MEM_ALIGN(hmem)", (hmem == LWIP_MEM_ALIGN(hmem)));
  LWIP_ASSERT("hmem->poolnr < MEMP_MAX", (hmem->poolnr < MEMP_MAX));

  /* and put it in the pool we saved earlier */
	/* 根据POOL的类型，释放该内存空间 */
  memp_free(hmem->poolnr, hmem);
}

#else /* MEM_USE_POOLS */
/* lwIP replacement for your libc malloc() */

/**
 * The heap is made up as a list of structs of this type.
 * This does not have to be aligned since for getting its size,
 * we only use the macro SIZEOF_STRUCT_MEM, which automatically alignes.
 */
struct mem {
  /** index (-> ram[next]) of the next struct */
  mem_size_t next;
  /** index (-> ram[next]) of the next struct */
  mem_size_t prev;
  /** 1: this area is used; 0: this area is unused */
  u8_t used;
};

/** All allocated blocks will be MIN_SIZE bytes big, at least!
 * MIN_SIZE can be overridden to suit your needs. Smaller values save space,
 * larger values could prevent too small blocks to fragment the RAM too much. */
#ifndef MIN_SIZE
#define MIN_SIZE             12
#endif /* MIN_SIZE */
/* some alignment macros: we define them here for better source code layout */
#define MIN_SIZE_ALIGNED     LWIP_MEM_ALIGN_SIZE(MIN_SIZE)
#define SIZEOF_STRUCT_MEM    LWIP_MEM_ALIGN_SIZE(sizeof(struct mem))
#define MEM_SIZE_ALIGNED     LWIP_MEM_ALIGN_SIZE(MEM_SIZE)

/** the heap. we need one struct mem at the end and some room for alignment */
/* 内存堆实际使用的内存空间 
 * 附加MEM_ALIGNMENT字节的空间是为了ram起始地址按MEM_ALIGNMENT对齐后，仍然能保证
 * MEM_SIZE_ALIGNED + (2*SIZEOF_STRUCT_MEM)大小的空间
 */
static u8_t ram_heap[MEM_SIZE_ALIGNED + (2*SIZEOF_STRUCT_MEM) + MEM_ALIGNMENT];
/** pointer to the heap (ram_heap): for alignment, ram is now a pointer instead of an array */
/* 获取对齐后的堆起始地址 */
static u8_t *ram;
/** the last entry, always unused! */
/* 最后一个内存块指针，一直标记为占用 */
static struct mem *ram_end;
/** pointer to the lowest free block, this is used for faster search */
/* 最低地址的空闲块指针 */
static struct mem *lfree;

/** concurrent access protection */
static sys_sem_t mem_sem;

#if LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT

static volatile u8_t mem_free_count;

/* Allow mem_free from other (e.g. interrupt) context */
#define LWIP_MEM_FREE_DECL_PROTECT()  SYS_ARCH_DECL_PROTECT(lev_free)
#define LWIP_MEM_FREE_PROTECT()       SYS_ARCH_PROTECT(lev_free)
#define LWIP_MEM_FREE_UNPROTECT()     SYS_ARCH_UNPROTECT(lev_free)
#define LWIP_MEM_ALLOC_DECL_PROTECT() SYS_ARCH_DECL_PROTECT(lev_alloc)
#define LWIP_MEM_ALLOC_PROTECT()      SYS_ARCH_PROTECT(lev_alloc)
#define LWIP_MEM_ALLOC_UNPROTECT()    SYS_ARCH_UNPROTECT(lev_alloc)

#else /* LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT */

/* Protect the heap only by using a semaphore */
#define LWIP_MEM_FREE_DECL_PROTECT()
#define LWIP_MEM_FREE_PROTECT()    sys_arch_sem_wait(mem_sem, 0)
#define LWIP_MEM_FREE_UNPROTECT()  sys_sem_signal(mem_sem)
/* mem_malloc is protected using semaphore AND LWIP_MEM_ALLOC_PROTECT */
#define LWIP_MEM_ALLOC_DECL_PROTECT()
#define LWIP_MEM_ALLOC_PROTECT()
#define LWIP_MEM_ALLOC_UNPROTECT()

#endif /* LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT */


/**
 * "Plug holes" by combining adjacent empty struct mems.
 * After this function is through, there should not exist
 * one empty struct mem pointing to another empty struct mem.
 *
 * @param mem this points to a struct mem which just has been freed
 * @internal this function is only called by mem_free() and mem_realloc()
 *
 * This assumes access to the heap is protected by the calling function
 * already.
 */
static void
plug_holes(struct mem *mem)
{
	/* 下一个内存块 */
  struct mem *nmem;
	/* 上一个内存块 */
  struct mem *pmem;

  LWIP_ASSERT("plug_holes: mem >= ram", (u8_t *)mem >= ram);
  LWIP_ASSERT("plug_holes: mem < ram_end", (u8_t *)mem < (u8_t *)ram_end);
  LWIP_ASSERT("plug_holes: mem->used == 0", mem->used == 0);

  /* plug hole forward */
  LWIP_ASSERT("plug_holes: mem->next <= MEM_SIZE_ALIGNED", mem->next <= MEM_SIZE_ALIGNED);

	/* 相邻的下一个内存块首地址 */
  nmem = (struct mem *)&ram[mem->next];
	/* 下一个内存块存在
	 * 且下一个相邻内存块空闲
	 * 待合并的内存块不是最后一个内存块
	 */
  if (mem != nmem && nmem->used == 0 && (u8_t *)nmem != (u8_t *)ram_end) {
    /* if mem->next is unused and not end of ram, combine mem and mem->next */
    /* 如果lfree指向下一个相邻内存块，则修改lfree
		 * 使它指向当前内存块
		 * 因为合并后，nmem将消失
		 */
		if (lfree == nmem) {
      lfree = mem;
    }
		/* 删除链表中的表示下一个相邻内存块的节点，需调整2个指针 */
    mem->next = nmem->next;
		/* .->next->next->prev = 当前内存块的偏移地址 
		 * 即删除.->next节点
		 */
    ((struct mem *)&ram[nmem->next])->prev = (u8_t *)mem - ram;
  }

  /* plug hole backward */
	/* 尝试合并相邻的上一个内存块 */
  pmem = (struct mem *)&ram[mem->prev];
	/* 上一个内存块存在 且 上一个内存块空闲 */
  if (pmem != mem && pmem->used == 0) {
    /* if mem->prev is unused, combine mem and mem->prev */
    /* 如果lfree指向mem，则指向pmem。
		 * 因为合并后，mem将消失
		 */
		if (lfree == mem) {
      lfree = pmem;
    }
		/* 删除链表中的mem节点，需要调整2个指针 */
    pmem->next = mem->next;
    ((struct mem *)&ram[mem->next])->prev = (u8_t *)pmem - ram;
  }
}

/**
 * Zero the heap and initialize start, end and lowest-free
 */
void
mem_init(void)
{
  struct mem *mem;

	/* 检查sizeof(struct mem)是否按照MEM_ALIGNMENT要求的大小对齐 */
  LWIP_ASSERT("Sanity check alignment",
    (SIZEOF_STRUCT_MEM & (MEM_ALIGNMENT-1)) == 0);

  /* align the heap */
	/* 对齐堆起始地址 */
  ram = LWIP_MEM_ALIGN(ram_heap);
  /* initialize the start of the heap */
	/* 在ram起始处初始化一个struct mem结构体 */
  mem = (struct mem *)ram;
	/* 下一个内存块偏移量为MEM_SIZE_ALIGNED，即
	 * 附加的第二个struct mem
	 */
  mem->next = MEM_SIZE_ALIGNED;
  mem->prev = 0;
  mem->used = 0;
	
  /* initialize the end of the heap */
	/* 最后一个内存块地址，即附加的
	 * 第二个struct mem
	 * 标记为已经使用，prev和next指针都指向自己
	 */
  ram_end = (struct mem *)&ram[MEM_SIZE_ALIGNED];
  ram_end->used = 1;
  ram_end->next = MEM_SIZE_ALIGNED;
  ram_end->prev = MEM_SIZE_ALIGNED;

	/* 创建一个在内存堆分配时使用的互斥信号量
	 * 无操作系统模拟层时，该函数为空
	 */
  mem_sem = sys_sem_new(1);

  /* initialize the lowest-free pointer to the start of the heap */
	/* 最低地址的空闲块指针 */
  lfree = (struct mem *)ram;

	/* 修改统计量 */
  MEM_STATS_AVAIL(avail, MEM_SIZE_ALIGNED);
}

/**
 * Put a struct mem back on the heap
 *
 * @param rmem is the data portion of a struct mem as returned by a previous
 *             call to mem_malloc()
 */
void
mem_free(void *rmem)
{
  struct mem *mem;
  LWIP_MEM_FREE_DECL_PROTECT();

	/* 释放地址为空 */
  if (rmem == NULL) {
    LWIP_DEBUGF(MEM_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS, ("mem_free(p == NULL) was called.\n"));
    return;
  }
  LWIP_ASSERT("mem_free: sanity check alignment", (((mem_ptr_t)rmem) & (MEM_ALIGNMENT-1)) == 0);

  LWIP_ASSERT("mem_free: legal memory", (u8_t *)rmem >= (u8_t *)ram &&
    (u8_t *)rmem < (u8_t *)ram_end);

	/* 判断待释放的地址是否在内存堆范围内 */
  if ((u8_t *)rmem < (u8_t *)ram || (u8_t *)rmem >= (u8_t *)ram_end) {
    SYS_ARCH_DECL_PROTECT(lev);
    LWIP_DEBUGF(MEM_DEBUG | LWIP_DBG_LEVEL_SEVERE, ("mem_free: illegal memory\n"));
    /* protect mem stats from concurrent access */
    SYS_ARCH_PROTECT(lev);
		/* 修改统计量 */
    MEM_STATS_INC(illegal);
    SYS_ARCH_UNPROTECT(lev);
    return;
  }
  /* protect the heap from concurrent access */
  LWIP_MEM_FREE_PROTECT();
  /* Get the corresponding struct mem ... */
	/* 地址合法，则找到待释放内存块的struct mem结构体 */
  mem = (struct mem *)((u8_t *)rmem - SIZEOF_STRUCT_MEM);
  /* ... which has to be in a used state ... */
  LWIP_ASSERT("mem_free: mem->used", mem->used);
  /* ... and is now unused. */
	/* 内存块标记为未使用 */
  mem->used = 0;

	/* 如果释放的地址比lfree低，则更新lfree */
  if (mem < lfree) {
    /* the newly freed struct is now the lowest */
    lfree = mem;
  }

	/* 更新统计量 */
  MEM_STATS_DEC_USED(used, mem->next - ((u8_t *)mem - ram));

  /* finally, see if prev or next are free also */
  /* 检查并执行相邻内存块合并 */
	plug_holes(mem);
#if LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT
  mem_free_count = 1;
#endif /* LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT */
  LWIP_MEM_FREE_UNPROTECT();
}

/**
 * In contrast to its name, mem_realloc can only shrink memory, not expand it.
 * Since the only use (for now) is in pbuf_realloc (which also can only shrink),
 * this shouldn't be a problem!
 *
 * @param rmem pointer to memory allocated by mem_malloc the is to be shrinked
 * @param newsize required size after shrinking (needs to be smaller than or
 *                equal to the previous size)
 * @return for compatibility reasons: is always == rmem, at the moment
 *         or NULL if newsize is > old size, in which case rmem is NOT touched
 *         or freed!
 */
void *
mem_realloc(void *rmem, mem_size_t newsize)
{
  mem_size_t size;
  mem_size_t ptr, ptr2;
  struct mem *mem, *mem2;
  /* use the FREE_PROTECT here: it protects with sem OR SYS_ARCH_PROTECT */
  LWIP_MEM_FREE_DECL_PROTECT();

  /* Expand the size of the allocated memory region so that we can
     adjust for alignment. */
  newsize = LWIP_MEM_ALIGN_SIZE(newsize);

  if(newsize < MIN_SIZE_ALIGNED) {
    /* every data block must be at least MIN_SIZE_ALIGNED long */
    newsize = MIN_SIZE_ALIGNED;
  }

  if (newsize > MEM_SIZE_ALIGNED) {
    return NULL;
  }

  LWIP_ASSERT("mem_realloc: legal memory", (u8_t *)rmem >= (u8_t *)ram &&
   (u8_t *)rmem < (u8_t *)ram_end);

  if ((u8_t *)rmem < (u8_t *)ram || (u8_t *)rmem >= (u8_t *)ram_end) {
    SYS_ARCH_DECL_PROTECT(lev);
    LWIP_DEBUGF(MEM_DEBUG | LWIP_DBG_LEVEL_SEVERE, ("mem_realloc: illegal memory\n"));
    /* protect mem stats from concurrent access */
    SYS_ARCH_PROTECT(lev);
    MEM_STATS_INC(illegal);
    SYS_ARCH_UNPROTECT(lev);
    return rmem;
  }
  /* Get the corresponding struct mem ... */
  mem = (struct mem *)((u8_t *)rmem - SIZEOF_STRUCT_MEM);
  /* ... and its offset pointer */
  ptr = (u8_t *)mem - ram;

  size = mem->next - ptr - SIZEOF_STRUCT_MEM;
  LWIP_ASSERT("mem_realloc can only shrink memory", newsize <= size);
  if (newsize > size) {
    /* not supported */
    return NULL;
  }
  if (newsize == size) {
    /* No change in size, simply return */
    return rmem;
  }

  /* protect the heap from concurrent access */
  LWIP_MEM_FREE_PROTECT();

  MEM_STATS_DEC_USED(used, (size - newsize));

  mem2 = (struct mem *)&ram[mem->next];
  if(mem2->used == 0) {
    /* The next struct is unused, we can simply move it at little */
    mem_size_t next;
    /* remember the old next pointer */
    next = mem2->next;
    /* create new struct mem which is moved directly after the shrinked mem */
    ptr2 = ptr + SIZEOF_STRUCT_MEM + newsize;
    if (lfree == mem2) {
      lfree = (struct mem *)&ram[ptr2];
    }
    mem2 = (struct mem *)&ram[ptr2];
    mem2->used = 0;
    /* restore the next pointer */
    mem2->next = next;
    /* link it back to mem */
    mem2->prev = ptr;
    /* link mem to it */
    mem->next = ptr2;
    /* last thing to restore linked list: as we have moved mem2,
     * let 'mem2->next->prev' point to mem2 again. but only if mem2->next is not
     * the end of the heap */
    if (mem2->next != MEM_SIZE_ALIGNED) {
      ((struct mem *)&ram[mem2->next])->prev = ptr2;
    }
    /* no need to plug holes, we've already done that */
  } else if (newsize + SIZEOF_STRUCT_MEM + MIN_SIZE_ALIGNED <= size) {
    /* Next struct is used but there's room for another struct mem with
     * at least MIN_SIZE_ALIGNED of data.
     * Old size ('size') must be big enough to contain at least 'newsize' plus a struct mem
     * ('SIZEOF_STRUCT_MEM') with some data ('MIN_SIZE_ALIGNED').
     * @todo we could leave out MIN_SIZE_ALIGNED. We would create an empty
     *       region that couldn't hold data, but when mem->next gets freed,
     *       the 2 regions would be combined, resulting in more free memory */
    ptr2 = ptr + SIZEOF_STRUCT_MEM + newsize;
    mem2 = (struct mem *)&ram[ptr2];
    if (mem2 < lfree) {
      lfree = mem2;
    }
    mem2->used = 0;
    mem2->next = mem->next;
    mem2->prev = ptr;
    mem->next = ptr2;
    if (mem2->next != MEM_SIZE_ALIGNED) {
      ((struct mem *)&ram[mem2->next])->prev = ptr2;
    }
    /* the original mem->next is used, so no need to plug holes! */
  }
  /* else {
    next struct mem is used but size between mem and mem2 is not big enough
    to create another struct mem
    -> don't do anyhting. 
    -> the remaining space stays unused since it is too small
  } */
#if LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT
  mem_free_count = 1;
#endif /* LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT */
  LWIP_MEM_FREE_UNPROTECT();
  return rmem;
}

/**
 * Adam's mem_malloc() plus solution for bug #17922
 * Allocate a block of memory with a minimum of 'size' bytes.
 *
 * @param size is the minimum size of the requested block in bytes.
 * @return pointer to allocated memory or NULL if no free memory was found.
 *
 * Note that the returned value will always be aligned (as defined by MEM_ALIGNMENT).
 */
void *
mem_malloc(mem_size_t size)
{
  mem_size_t ptr, ptr2;
  struct mem *mem, *mem2;
#if LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT
  u8_t local_mem_free_count = 0;
#endif /* LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT */
  LWIP_MEM_ALLOC_DECL_PROTECT();

	/* 申请0字节，返回NULL */
  if (size == 0) {
    return NULL;
  }

  /* Expand the size of the allocated memory region so that we can
     adjust for alignment. */
	/* 把size进行字节对齐 */
  size = LWIP_MEM_ALIGN_SIZE(size);

	/* 申请空间至少为MIN_SIZE_ALIGNED字节，以防止过多的内存碎片 */
  if(size < MIN_SIZE_ALIGNED) {
    /* every data block must be at least MIN_SIZE_ALIGNED long */
    size = MIN_SIZE_ALIGNED;
  }
	
	/* 若申请长度大于内存堆总大小，返回错误 */
  if (size > MEM_SIZE_ALIGNED) {
    return NULL;
  }

  /* protect the heap from concurrent access */
	/* 获得互斥信号量，进行线程安全保护 */
  sys_arch_sem_wait(mem_sem, 0);
  LWIP_MEM_ALLOC_PROTECT();
#if LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT
  /* run as long as a mem_free disturbed mem_malloc */
  do {
    local_mem_free_count = 0;
#endif /* LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT */

    /* Scan through the heap searching for a free block that is big enough,
     * beginning with the lowest free block.
     */
		/* 遍历所有内存块，找到第一个不小于申请空间的空闲内存块 */
    for (ptr = (u8_t *)lfree - ram; ptr < MEM_SIZE_ALIGNED - size;
         ptr = ((struct mem *)&ram[ptr])->next) {
      mem = (struct mem *)&ram[ptr];
#if LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT
      mem_free_count = 0;
      LWIP_MEM_ALLOC_UNPROTECT();
      /* allow mem_free to run */
      LWIP_MEM_ALLOC_PROTECT();
      if (mem_free_count != 0) {
        local_mem_free_count = mem_free_count;
      }
      mem_free_count = 0;
#endif /* LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT */

			/* 该内存块未使用
			 * mem->next - (ptr + SIZEOF_STRUCT_MEM)表示该内存块除了struct mem使用的空间外
			 * 可以分配给用户的空间不小于申请的空间
			 */
      if ((!mem->used) &&
          (mem->next - (ptr + SIZEOF_STRUCT_MEM)) >= size) {
        /* mem is not used and at least perfect fit is possible:
         * mem->next - (ptr + SIZEOF_STRUCT_MEM) gives us the 'user data size' of mem */

				/* 判断是把该内存块所有空间全部分配给用户
				 * 还是截取一部分给用户，剩余部分分割成另一个内存块
				 * 判断标准为剩余空间不小于最小内存块大小+struct mem大小
				 */
				/* 剩余空间足够分割成一个新内存块 */
        if (mem->next - (ptr + SIZEOF_STRUCT_MEM) >= (size + SIZEOF_STRUCT_MEM + MIN_SIZE_ALIGNED)) {
          /* (in addition to the above, we test if another struct mem (SIZEOF_STRUCT_MEM) containing
           * at least MIN_SIZE_ALIGNED of data also fits in the 'user data space' of 'mem')
           * -> split large block, create empty remainder,
           * remainder must be large enough to contain MIN_SIZE_ALIGNED data: if
           * mem->next - (ptr + (2*SIZEOF_STRUCT_MEM)) == size,
           * struct mem would fit in but no data between mem2 and mem2->next
           * @todo we could leave out MIN_SIZE_ALIGNED. We would create an empty
           *       region that couldn't hold data, but when mem->next gets freed,
           *       the 2 regions would be combined, resulting in more free memory
           */
					/* 剩余空间起始地址偏移量 */
          ptr2 = ptr + SIZEOF_STRUCT_MEM + size;
          /* create mem2 struct */
					/* 构建新内存块的struct mem结构 */
          mem2 = (struct mem *)&ram[ptr2];
					/* 新内存块标记为未使用 */
          mem2->used = 0;
					/* 新内存块插入内存块双向链表，调整4个指针 */
          mem2->next = mem->next;
          mem2->prev = ptr;
          /* and insert it between mem and mem->next */
          mem->next = ptr2;
					/* 分配出去的内存块标记为已使用 */
          mem->used = 1;

					/* 调整.->next->prev指针 */
          if (mem2->next != MEM_SIZE_ALIGNED) {
            ((struct mem *)&ram[mem2->next])->prev = ptr2;
          }
					
					/* 统计量 */
          MEM_STATS_INC_USED(used, (size + SIZEOF_STRUCT_MEM));
        } else {		/* 直接分配，不截取 */
          /* (a mem2 struct does no fit into the user data space of mem and mem->next will always
           * be used at this point: if not we have 2 unused structs in a row, plug_holes should have
           * take care of this).
           * -> near fit or excact fit: do not split, no mem2 creation
           * also can't move mem->next directly behind mem, since mem->next
           * will always be used at this point!
           */
					/* 分配的内存块标记为已使用 */
          mem->used = 1;
          MEM_STATS_INC_USED(used, mem->next - ((u8_t *)mem - ram));
        }

				/* 只有被分配出去的内存块是lfree，才调整lfree指针 */
        if (mem == lfree) {
          /* Find next free block after mem and update lowest free pointer */
					/* 找到下一个空闲的内存块 */
          while (lfree->used && lfree != ram_end) {
            LWIP_MEM_ALLOC_UNPROTECT();
            /* prevent high interrupt latency... */
            LWIP_MEM_ALLOC_PROTECT();
						/* 调整lfree指针 */
            lfree = (struct mem *)&ram[lfree->next];
          }
          LWIP_ASSERT("mem_malloc: !lfree->used", ((lfree == ram_end) || (!lfree->used)));
        }
        LWIP_MEM_ALLOC_UNPROTECT();
				/* 释放互斥信号量 */
        sys_sem_signal(mem_sem);
        LWIP_ASSERT("mem_malloc: allocated memory not above ram_end.",
         (mem_ptr_t)mem + SIZEOF_STRUCT_MEM + size <= (mem_ptr_t)ram_end);
        LWIP_ASSERT("mem_malloc: allocated memory properly aligned.",
         ((mem_ptr_t)mem + SIZEOF_STRUCT_MEM) % MEM_ALIGNMENT == 0);
        LWIP_ASSERT("mem_malloc: sanity check alignment",
          (((mem_ptr_t)mem) & (MEM_ALIGNMENT-1)) == 0);

				/* 分配成功返回用户数据区首地址 */
        return (u8_t *)mem + SIZEOF_STRUCT_MEM;
      }
    }
#if LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT
    /* if we got interrupted by a mem_free, try again */
  } while(local_mem_free_count != 0);
#endif /* LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT */
	
	/* 执行到这里，说明所有内存块都不能满足要求 */
  LWIP_DEBUGF(MEM_DEBUG | LWIP_DBG_LEVEL_SERIOUS, ("mem_malloc: could not allocate %"S16_F" bytes\n", (s16_t)size));
	/* 修改统计量 */
  MEM_STATS_INC(err);
  LWIP_MEM_ALLOC_UNPROTECT();
	/* 释放互斥信号量 */
  sys_sem_signal(mem_sem);
	/* 分配失败，返回空指针 */
  return NULL;
}

#endif /* MEM_USE_POOLS */
/**
 * Contiguously allocates enough space for count objects that are size bytes
 * of memory each and returns a pointer to the allocated memory.
 *
 * The allocated memory is filled with bytes of value zero.
 *
 * @param count number of objects to allocate
 * @param size size of the objects to allocate
 * @return pointer to allocated memory / NULL pointer if there is an error
 */
void *mem_calloc(mem_size_t count, mem_size_t size)
{
  void *p;

  /* allocate 'count' objects of size 'size' */
  p = mem_malloc(count * size);
  if (p) {
    /* zero the memory */
    memset(p, 0, count * size);
  }
  return p;
}

#endif /* !MEM_LIBC_MALLOC */
