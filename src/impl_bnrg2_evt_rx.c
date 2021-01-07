#include "impl_bnrg2_evt_rx.h"
#include "hci.h"

void bnrg2_event_rx(void *pData) {
  uint32_t i;

  hci_spi_pckt *hci_pckt = (hci_spi_pckt *) pData;

  if (hci_pckt->type == HCI_EVENT_PKT) {
    hci_event_pckt *event_pckt = (hci_event_pckt *) hci_pckt->data;

    if (event_pckt->evt == EVT_LE_META_EVENT) {
      evt_le_meta_event *evt = (void *) event_pckt->data;

      for (i = 0; i < (sizeof(hci_le_meta_events_table) / sizeof(hci_le_meta_events_table_type)); i++) {
        if (evt->subevent == hci_le_meta_events_table[i].evt_code) {
          hci_le_meta_events_table[i].process((void *) evt->data);
        }
      }
    } else if (event_pckt->evt == EVT_VENDOR) {
      evt_blue_aci *blue_evt = (void *) event_pckt->data;

      for (i = 0; i < (sizeof(hci_vendor_specific_events_table) / sizeof(hci_vendor_specific_events_table_type)); i++) {
        if (blue_evt->ecode == hci_vendor_specific_events_table[i].evt_code) {
          hci_vendor_specific_events_table[i].process((void *) blue_evt->data);
        }
      }
    } else {
      for (i = 0; i < (sizeof(hci_events_table) / sizeof(hci_events_table_type)); i++) {
        if (event_pckt->evt == hci_events_table[i].evt_code) {
          hci_events_table[i].process((void *) event_pckt->data);
        }
      }
    }
  }
}