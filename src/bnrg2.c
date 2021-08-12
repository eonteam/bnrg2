#include "bnrg2.h"
#include "bluenrg1_aci.h"
#include "bluenrg1_events.h"
#include "bluenrg1_hci_le.h"
#include "hci.h"
#include "hci_tl_interface.h"
#include "impl_bnrg2_evt_rx.h"

// ===============================================================
// Debug macros
// ===============================================================

#ifdef BNRG2_DEBUG
#define DEBUG_PRINTF pc_printf
#else
#define DEBUG_PRINTF(fmt, ...)
#endif

// ===============================================================
// Static data
// ===============================================================

#define MAX_LOCAL_NAME_AD_LEN ((uint8_t) 21) // 20 for name and 1 byte for ad type
#define DISCOVERABLE_MODE_STARTED ((uint8_t) (0x00))
#define DISCOVERABLE_MODE_STOPPED ((uint8_t) (0x01))

static struct {
  ble_error_t error;
  volatile ble_conn_t conn_handle;
  volatile uint8_t is_connected;
  volatile uint8_t is_tx_buffer_full;
  volatile uint8_t discoverable_mode; // internal flag to know the discoveral mode of the device
  uint8_t connectable_mode_enabled;   // check if user enable connectable mode
  uint8_t mtu_exchanged;
  uint8_t mtu_exchanged_wait;
  uint8_t local_name_AD[MAX_LOCAL_NAME_AD_LEN];
  uint8_t local_name_AD_len;
} ble_state = {
    .error = BLE_ERROR_NONE,
    .conn_handle = 0,
    .is_connected = false,
    .connectable_mode_enabled = false,
    .discoverable_mode = DISCOVERABLE_MODE_STOPPED,
    .mtu_exchanged = 0,
    .mtu_exchanged_wait = 0,
    .local_name_AD = {AD_TYPE_COMPLETE_LOCAL_NAME, 'E', 'O', 'N', 'B', 'L', 'E'},
    .local_name_AD_len = 7,
};

// ===============================================================
// Privates
// ===============================================================

__STATIC_INLINE void setError(ble_error_t error) { ble_state.error = error; }

static tBleStatus setup_public_address(const uint8_t *addr) {
  uint8_t bdaddr[6];

  if (addr == NULL) {
    // get a random number from BlueNRG
    uint8_t random_number[8];
    tBleStatus ret = hci_le_rand(random_number);
    if (ret != BLE_STATUS_SUCCESS) {
      DEBUG_PRINTF("Error while resetting: 0x%x\n", ret);
      return ret;
    }
    // setup last 3 bytes of public address with random number
    bdaddr[0] = (uint8_t) (random_number[0]);
    bdaddr[1] = (uint8_t) (random_number[3]);
    bdaddr[2] = (uint8_t) (random_number[6]);
    bdaddr[3] = 0xE1;
    bdaddr[4] = 0x80;
    bdaddr[5] = 0x02;
  } else {
    for (uint8_t i = 0; i < 6; i++) {
      bdaddr[i] = addr[5 - i];
    }
  }

  return aci_hal_write_config_data(CONFIG_DATA_PUBADDR_OFFSET, CONFIG_DATA_PUBADDR_LEN, bdaddr);
}

// Hex digit to decimal digit
static uint8_t _hexDigitToDec(char hexDigit) {
  if ((hexDigit >= '0') && (hexDigit <= '9')) { return (hexDigit - '0'); }
  if ((hexDigit >= 'A') && (hexDigit <= 'F')) { return (hexDigit - 'A' + 10); }
  if ((hexDigit >= 'a') && (hexDigit <= 'f')) { return (hexDigit - 'a' + 10); }
  return 0;
}

// Converts uuid string to object type
static bool _uuidStringToObjType(UUID_t *uuid, uint8_t *uuidType, const char *uuidString) {
  setError(BLE_ERROR_NONE);
  uint8_t uuidLen = strlen(uuidString);
  if (uuidLen == 32) { // 128 bits UUID
    uint8_t k = 0;
    for (uint8_t i = 0; i < 16; i++) {
      uuid->UUID_128[15 - i] = (_hexDigitToDec(uuidString[k]) << 4) | (_hexDigitToDec(uuidString[k + 1]));
      k += 2;
    }
    *uuidType = UUID_TYPE_128;
    return true;
  }
  if (uuidLen == 4) { // 16 bits UUID
    const uint8_t upper = (_hexDigitToDec(uuidString[0]) << 4) | (_hexDigitToDec(uuidString[1]));
    const uint8_t lower = (_hexDigitToDec(uuidString[2]) << 4) | (_hexDigitToDec(uuidString[3]));
    uuid->UUID_16 = (uint16_t) (upper << 8) | (uint16_t) (lower);
    *uuidType = UUID_TYPE_16;
    return true;
  }
  setError(BLE_STATUS_ERROR);
  return false;
}

// ===============================================================
// Error getter
// ===============================================================

ble_error_t bnrg2_getError(void) { return ble_state.error; }

// ===============================================================
// Functions
// ===============================================================

// Initialize BlueNRG-2 hardware. Passing a null public address, will
// generate a real random public address.
bool bnrg2_init(const bnrg2_hw_t *hw, const uint8_t *pubaddr) {
  uint8_t ret;
  setError(BLE_ERROR_NONE);
  hci_eon_brige(hw);
  hci_init(bnrg2_event_rx, NULL);
  ret = hci_reset(); // Sw reset of the device
  if (ret != BLE_ERROR_NONE) {
    DEBUG_PRINTF("Error while resetting: 0x%x\n", ret);
    setError(ret);
    return false;
  }
  delay(2000);                         // wait to the initialization is done
  ret = setup_public_address(pubaddr); // set public address
  if (ret != BLE_ERROR_NONE) {
    DEBUG_PRINTF("Error while setting device address: 0x%x\n", ret);
    setError(ret);
    return false;
  }
  return true;
}

// Set transmission power.
//
bool bnrg2_setTxPower(bool high_power, uint8_t pa_level) {
  uint8_t ret = aci_hal_set_tx_power_level(high_power, pa_level);
  if (ret != BLE_ERROR_NONE) {
    DEBUG_PRINTF("Error while setting tx power level: 0x%x\n", ret);
    setError(ret);
    return false;
  }
  return true;
}

// Initialize ble stack ( GATT and GAP ).
//
bool bnrg2_stackInit(void) {
  uint8_t ret;
  uint16_t service_handle, dev_name_char_handle, appearance_char_handle;
  setError(BLE_ERROR_NONE);
  // GATT Init
  ret = aci_gatt_init();
  if (ret != BLE_STATUS_SUCCESS) {
    DEBUG_PRINTF("GATT_Init failed: 0x%x\r\n", ret);
    setError(ret);
    return false;
  }
  // GAP Init: disable privacy (0x00), and characteristics name length = 0x07
  ret = aci_gap_init(GAP_PERIPHERAL_ROLE, 0x0, 0x07, &service_handle,
                     &dev_name_char_handle, &appearance_char_handle);
  if (ret != BLE_STATUS_SUCCESS) {
    DEBUG_PRINTF("GAP_Init failed: 0x%x\r\n", ret);
    setError(ret);
    return false;
  }
  return true;
}

// Add a ble service.
//
bool bnrg2_addService(ble_service_t *s, const char *uuid, uint8_t nbOfCharacteristics) {
  uint8_t ret;
  Service_UUID_t service_uuid;
  uint8_t uuidType;
  setError(BLE_ERROR_NONE);
  if (!_uuidStringToObjType((UUID_t *) &service_uuid, &uuidType, uuid)) {
    DEBUG_PRINTF("Invalid service uuid");
    return false;
  }
  // 1 for service attribute, every characteristic can have 3 attributes:
  // declaration, value and descriptor (Client Characteristic Configuration Descriptor -
  // for notifications and indications), other descriptors are not supported in
  // this library.
  const uint8_t max_attribute_records = 1 + 3 * nbOfCharacteristics;
  ret = aci_gatt_add_service(uuidType, &service_uuid, PRIMARY_SERVICE,
                             max_attribute_records, &s->_service_handle);
  if (ret != BLE_ERROR_NONE) {
    setError(ret);
    DEBUG_PRINTF("Error while adding the ble service: 0x%x", ret);
    return false;
  }
  return true;
}

// Add characteristic to a service.
//
bool bnrg2_addCharacteristic(const ble_service_t *s, ble_char_t *charact,
                             const char *uuid, uint16_t max_value_len,
                             uint8_t is_variable_len, uint8_t char_properties,
                             uint8_t gatt_evt_mask) {
  uint8_t ret;
  Char_UUID_t char_uuid;
  uint8_t uuidType;
  setError(BLE_ERROR_NONE);
  if (!_uuidStringToObjType((UUID_t *) &char_uuid, &uuidType, uuid)) {
    DEBUG_PRINTF("Invalid characteristic uuid");
    return false;
  }
  ret = aci_gatt_add_char(s->_service_handle, uuidType, &char_uuid, max_value_len,
                          char_properties, ATTR_PERMISSION_NONE, gatt_evt_mask,
                          16, is_variable_len, &charact->_char_decl_handle);
  if (ret != BLE_ERROR_NONE) {
    setError(ret);
    DEBUG_PRINTF("Error while adding the ble characteristic: 0x%x", ret);
    return false;
  }
  charact->_service_handle = s->_service_handle;
  charact->_char_val_handle = charact->_char_decl_handle + 1;
  charact->_char_desc_cccd_handle = charact->_char_decl_handle + 2;
  charact->_char_props = char_properties;
  charact->_max_value_len = max_value_len;
  charact->_is_variable_len = is_variable_len;
  return true;
}

// Update a characteristic value in a ble connection.
//
bool bnrg2_updateCharValue(ble_conn_t conn, const ble_char_t *charact,
                           const uint8_t *value, uint8_t value_len) {

  uint8_t update_type = 0x00; // GATT_LOCAL_UPDATE
  uint8_t ret = BLE_STATUS_INSUFFICIENT_RESOURCES;
  setError(BLE_ERROR_NONE);
  if ((charact->_char_props & CHAR_PROP_NOTIFY) != 0x00) {
    update_type |= 0x01; // GATT_NOTIFICATION
  }
  if ((charact->_char_props & CHAR_PROP_INDICATE) != 0x00) {
    update_type |= 0x02; // GATT_INDICATION
  }
  // REVIEW we do not support GATT_DISABLE_RETRANSMISSIONS (0x04)
  uint32_t tickstart = millis();
  while (ret == BLE_STATUS_INSUFFICIENT_RESOURCES) {
    ret = aci_gatt_update_char_value_ext(conn, charact->_service_handle,
                                         charact->_char_decl_handle, update_type, value_len,
                                         0, value_len, (uint8_t *) value); // offset = 0
    if (ret != BLE_STATUS_INSUFFICIENT_RESOURCES) { break; }
    ble_state.is_tx_buffer_full = true;
    while (ble_state.is_tx_buffer_full) {
      hci_user_evt_proc();
      // Radio is busy (buffer full).
      if ((millis() - tickstart) > (10 * HCI_DEFAULT_TIMEOUT_MS)) {
        setError(BLE_STATUS_TIMEOUT);
        DEBUG_PRINTF("Failed to update characteristic: TIMEOUT\n");
        return false;
      }
    }
  }
  if (ret != BLE_ERROR_NONE) {
    setError(ret);
    DEBUG_PRINTF("Failed to update characteristic: 0x%x\n", ret);
    return false;
  }
  return true;
}

// Set the device Complete Local Name.
//
void bnrg2_setLocalName(const uint8_t *local_name, uint8_t local_name_len) {
  if (local_name_len > (MAX_LOCAL_NAME_AD_LEN - 1)) {
    local_name_len = MAX_LOCAL_NAME_AD_LEN - 1;
  }
  ble_state.local_name_AD_len = local_name_len + 1;
  ble_state.local_name_AD[0] = AD_TYPE_COMPLETE_LOCAL_NAME;
  for (uint8_t i = 0; i < local_name_len; i++) {
    ble_state.local_name_AD[i + 1] = local_name[i];
  }
}

// Enable or disable connectable mode.
//
void bnrg2_setConnectableMode(bool en) {
  ble_state.connectable_mode_enabled = en;
}

// Execute bluenrg2 processes (must be called always in the loop).
//
void bnrg2_process(void) {
  uint8_t ret;
  hci_user_evt_proc();
  if (ble_state.is_connected == false) {
    // If Bluenrg2 is in discoverable mode stopped and the user enabled connectable mode,
    // then set discoverable mode.
    if (ble_state.discoverable_mode == DISCOVERABLE_MODE_STOPPED &&
        ble_state.connectable_mode_enabled) {
      // Put Peripheral device in discoverable mode
      // disable scan response
      hci_le_set_scan_response_data(0, NULL);
      ret = aci_gap_set_discoverable(ADV_DATA_TYPE, ADV_INTERV_MIN, ADV_INTERV_MAX, PUBLIC_ADDR,
                                     NO_WHITE_LIST_USE, ble_state.local_name_AD_len,
                                     ble_state.local_name_AD,
                                     0, NULL, 0x0, 0x0);
      if (ret != BLE_STATUS_SUCCESS) {
        DEBUG_PRINTF("aci_gap_set_discoverable() failed: 0x%x\r\n", ret);
      } else {
        DEBUG_PRINTF("discoverable mode started\n");
        ble_state.discoverable_mode = DISCOVERABLE_MODE_STARTED;
      }
    }
    // If Bluenrg2 has started the discoverable mode and the user disabled connectable mode,
    // then set non discoverable mode.
    if (ble_state.discoverable_mode == DISCOVERABLE_MODE_STARTED &&
        !ble_state.connectable_mode_enabled) {
      // Put Peripheral device in non-discoverable mode
      ret = aci_gap_set_non_discoverable();
      if (ret != BLE_STATUS_SUCCESS) {
        DEBUG_PRINTF("aci_gap_set_non_discoverable() failed: 0x%x\r\n", ret);
      } else {
        DEBUG_PRINTF("discoverable mode stopped\n");
        ble_state.discoverable_mode = DISCOVERABLE_MODE_STOPPED;
      }
    }
  } else {
    // Handle the connection and mtu exchanged
    if ((ble_state.mtu_exchanged == 0) && (ble_state.mtu_exchanged_wait == 0)) {
      ble_state.mtu_exchanged_wait = 1;
      uint8_t ret = aci_gatt_exchange_config(ble_state.conn_handle);
      if (ret != BLE_STATUS_SUCCESS) {
        DEBUG_PRINTF("aci_gatt_exchange_configuration() error: 0x%x\r\n", ret);
      }
    }
  }
}

// Returns the connection handle if any, if not returns 0.
//
ble_conn_t bnrg2_getConnHandle(void) {
  if (!ble_state.is_connected) { return 0; }
  return ble_state.conn_handle;
}

// ===============================================================
// Weak functions
// ===============================================================

__weak void __bnrg_on_connect(ble_conn_t conn);
__weak void __bnrg_on_disconnect(ble_conn_t conn);

// ===========================================================================
//  ***** Handle BlueNRG event functions declared in bluenrg1_events.h *****
// ===========================================================================

// The LE Connection Complete event indicates to both of the Hosts forming the
// connection that a new connection has been created.
//
void hci_le_connection_complete_event(uint8_t Status,
                                      uint16_t Connection_Handle,
                                      uint8_t Role,
                                      uint8_t Peer_Address_Type,
                                      uint8_t Peer_Address[6],
                                      uint16_t Conn_Interval,
                                      uint16_t Conn_Latency,
                                      uint16_t Supervision_Timeout,
                                      uint8_t Master_Clock_Accuracy)

{
  ble_state.is_connected = true;
  ble_state.conn_handle = Connection_Handle;
  __bnrg_on_connect(Connection_Handle);
#ifdef BNRG2_DEBUG
  DEBUG_PRINTF("Connection complete with peer address: ");
  for (uint8_t i = 5; i > 0; i--) {
    DEBUG_PRINTF("%x-", Peer_Address[i]);
  }
  DEBUG_PRINTF("%x\r\n", Peer_Address[0]);
#endif
}

// The Disconnection Complete event occurs when a connection is terminated.
//
void hci_disconnection_complete_event(uint8_t Status,
                                      uint16_t Connection_Handle,
                                      uint8_t Reason) {
  ble_state.is_connected = false;
  ble_state.discoverable_mode = DISCOVERABLE_MODE_STOPPED;
  ble_state.conn_handle = 0;
  ble_state.mtu_exchanged = 0;
  ble_state.mtu_exchanged_wait = 0;
  __bnrg_on_disconnect(Connection_Handle);
  DEBUG_PRINTF("Disconnection with reason: 0x%x\r\n", Reason);
}

// This event is generated in response to an Exchange MTU request (local or from the peer).
//
void aci_att_exchange_mtu_resp_event(uint16_t Connection_Handle,
                                     uint16_t Server_RX_MTU) {
  // TODO handle the rx mtu
  DEBUG_PRINTF("aci_att_exchange_mtu_resp_event: Server_RX_MTU=%d\r\n", Server_RX_MTU);
  if ((ble_state.mtu_exchanged_wait == 0) || ((ble_state.mtu_exchanged_wait == 1))) {
    // The aci_att_exchange_mtu_resp_event is received also if the
    // aci_gatt_exchange_config is called by the other peer.
    // Here we manage this case.
    if (ble_state.mtu_exchanged_wait == 0) { ble_state.mtu_exchanged_wait = 2; }
    ble_state.mtu_exchanged = 1;
  }
}

// ===============================================================
// EXTI IRQ Handler Function
// ===============================================================

void bnrg2_exti_irq_handler(void) {
  hci_tl_lowlevel_isr();
}