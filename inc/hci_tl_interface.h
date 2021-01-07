#ifndef __HCI_TL_INTERFACE_H_
#define __HCI_TL_INTERFACE_H_

#include "bnrg2_types.h"

int32_t HCI_TL_SPI_Init(void *pConf);
int32_t HCI_TL_SPI_DeInit(void);
int32_t HCI_TL_SPI_Receive(uint8_t *buffer, uint16_t size);
int32_t HCI_TL_SPI_Send(uint8_t *buffer, uint16_t size);
int32_t HCI_TL_SPI_Reset(void);
int32_t HCI_TL_GetTick(void);

// Bridge to eon OS
void hci_eon_brige(const bnrg2_hw_t *hw);

// Register hci_tl_interface IO bus services
void hci_tl_lowlevel_init(void);

// HCI Transport Layer Low Level Interrupt Service Routine
void hci_tl_lowlevel_isr(void);

#endif