#ifndef __ETHERNETIF_H
#define __ETHERNETIF_H	 


#include "lwip/opt.h"
#include <lwip/stats.h>

extern const u8 mymac[6];	//MACµÿ÷∑

void ethernetif_input(struct netif *netif);
err_t ethernetif_init(struct netif *netif);

#endif

