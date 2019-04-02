#ifndef __RES_DRV_H
#define	__RES_DRV_H

#include "stm32h7xx.h"


int RES_GetOffset(const char *res_name);
void RES_DevTest(void);
void RES_DevInit(void);
uint32_t RES_DevGetID(void);
int8_t RES_DevWrite(uint8_t *buf,uint32_t addr,uint32_t size);
int8_t RES_DevRead(uint8_t *buf,uint32_t addr,uint32_t size);
int RES_DevEraseSector(uint32_t addr);

#endif /* __RES_DRV_H */
