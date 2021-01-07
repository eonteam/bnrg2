/**
 ******************************************************************************
 * File Name bluenrg_conf.h
 * @author   SRA
 * @version  V1.0.0
 * @date     Oct-2019
 * @brief
 ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef BLUENRG_CONF_H
#define BLUENRG_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "eonOS.h"
#include <string.h>

/*---------- Number of Bytes reserved for HCI Read Packet -----------*/
#define HCI_READ_PACKET_SIZE 128
/*---------- Number of Bytes reserved for HCI Max Payload -----------*/
#define HCI_MAX_PAYLOAD_SIZE 128
/*---------- Number of incoming packets added to the list of packets to read -----------*/
#define HCI_READ_PACKET_NUM_MAX 10
/*---------- Scan Interval: time interval from when the Controller started its last scan until it begins the subsequent scan (for a number N, Time = N x 0.625 msec) -----------*/
#define SCAN_P 16384
/*---------- Scan Window: amount of time for the duration of the LE scan (for a number N, Time = N x 0.625 msec) -----------*/
#define SCAN_L 16384
/*---------- Supervision Timeout for the LE Link (for a number N, Time = N x 10 msec) -----------*/
#define SUPERV_TIMEOUT 60
/*---------- Minimum Connection Period (for a number N, Time = N x 1.25 msec) -----------*/
#define CONN_P1 40
/*---------- Maximum Connection Period (for a number N, Time = N x 1.25 msec) -----------*/
#define CONN_P2 40
/*---------- Minimum Connection Length (for a number N, Time = N x 0.625 msec) -----------*/
#define CONN_L1 2000
/*---------- Maximum Connection Length (for a number N, Time = N x 0.625 msec) -----------*/
#define CONN_L2 2000
/*---------- Advertising Type -----------*/
#define ADV_DATA_TYPE ADV_IND
/*---------- Minimum Advertising Interval (for a number N, Time = N x 0.625 msec) -----------*/
#define ADV_INTERV_MIN 0x0030 // 30 ms
/*---------- Maximum Advertising Interval (for a number N, Time = N x 0.625 msec) -----------*/
#define ADV_INTERV_MAX 0x0640 // 1000 ms
/*---------- Minimum Connection Event Interval (for a number N, Time = N x 1.25 msec) -----------*/
#define L2CAP_INTERV_MIN 9
/*---------- Maximum Connection Event Interval (for a number N, Time = N x 1.25 msec) -----------*/
#define L2CAP_INTERV_MAX 20
/*---------- Timeout Multiplier (for a number N, Time = N x 10 msec) -----------*/
#define L2CAP_TIMEOUT_MULTIPLIER 600
/*---------- HCI Default Timeout -----------*/
#define HCI_DEFAULT_TIMEOUT_MS 1000

#define BLUENRG_memcpy memcpy
#define BLUENRG_memset memset
#define BLUENRG_memcmp memcmp

#if defined(BNRG2_LL_DEBUG) && defined(PC_WRITE)
/*---------- Print messages from BLE2 files at middleware level -----------*/
#define BLUENRG_PRINTF(...) pc_printf(__VA_ARGS__)
#else
#define BLUENRG_PRINTF(...)
#endif

#ifdef __cplusplus
}
#endif
#endif /* BLUENRG_CONF_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
