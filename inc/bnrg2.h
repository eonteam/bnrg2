#ifndef __BLUENRG2_H_
#define __BLUENRG2_H_

#include "bnrg2_types.h"
#include "hci.h"
#include "hci_tl.h"

// ===============================================================================
// Macros
// ===============================================================================

// BLE Errors
typedef uint8_t ble_error_t;
#define BLE_ERROR_NONE ((ble_error_t)(BLE_STATUS_SUCCESS))
// other errors are defined in ST-Middleware/BlueNRG-2/includes/ble_status.h

// ===============================================================
// Error getter
// ===============================================================

ble_error_t bnrg2_getError(void);

// ===============================================================
// Functions
// ===============================================================

/**
 * @brief Initialize BlueNRG-2 hardware.
 * 
 * @param hw Hardware structure pointer.
 * @param pubaddr Public address must be of length 6.
 * @return true if success, false if failed.
 */
bool bnrg2_init(const bnrg2_hw_t *hw, const uint8_t *pubaddr);

/**
 * @brief Set transmission power.
 * 
 * @param high_power True if you need to enable high power.
 * @param pa_level Power amplifier output level (Values: 0x00 ... 0x31)
 * @return true if success, false if failed.
 */
bool bnrg2_setTxPower(bool high_power, uint8_t pa_level);

/**
 * @brief Initialize ble stack ( GATT and GAP ).
 * 
 * @return true if success, false if failed.
 */
bool bnrg2_stackInit(void);

/**
 * @brief Add a ble service.
 * 
 * @param s BLE Service object.
 * @param uuid must be a 16 bit or 128 bit UUID in a hex string.
 * @param nbOfCharacteristics Number of characteristics this service will handle.
 * @return true if success, false if failed.
 */
bool bnrg2_addService(ble_service_t *s, const char *uuid, uint8_t nbOfCharacteristics);

/**
 * @brief Add a characteristic to a service.
 * 
 * @param s Ble service to add the characteristic.
 * @param charact Characteristic object.
 * @param uuid Characteristic UUID.
 * @param max_value_len Max characteristic value length.
 * @param is_variable_len True if the length is variable.
 * @param char_properties Characteristic properties.
 * @param gatt_evt_mask This value must be: GATT_DONT_NOTIFY_EVENTS, GATT_NOTIFY_ATTRIBUTE_WRITE, 
 * GATT_NOTIFY_WRITE_REQ_AND_WAIT_FOR_APPL_RESP, GATT_NOTIFY_READ_REQ_AND_WAIT_FOR_APPL_RESP.
 * @note Description for gatt_evt_mask values:
 *    GATT_DONT_NOTIFY_EVENTS
 *      |---> do not notify
 *    GATT_NOTIFY_ATTRIBUTE_WRITE
 *      |---> triggers aci_gatt_attribute_modified_event()
 *    GATT_NOTIFY_WRITE_REQ_AND_WAIT_FOR_APPL_RESP
 *      |---> triggers aci_gatt_write_permit_req_event() and check if write is allowed and
 *      |     call aci_gatt_write_resp().
 *    GATT_NOTIFY_READ_REQ_AND_WAIT_FOR_APPL_RESP
 *      |---> triggers aci_gatt_read_permit_req_event() and need to call aci_gatt_allow_read()
 *      |     if the read is allowed.
 * @return true if success, false if failed.
 */
bool bnrg2_addCharacteristic(const ble_service_t *s, ble_char_t *charact,
                             const char *uuid, uint16_t max_value_len,
                             uint8_t is_variable_len, uint8_t char_properties,
                             uint8_t gatt_evt_mask);

/**
 * @brief Update a characteristic value while in a ble connection.
 * 
 * @param conn Connection handle.
 * @param charact Characteristic object.
 * @param update_type Update type. Must be one of ...
 * @param value Buffer to be written.
 * @param value_len Length of the value buffer.
 * @return true if success, false if failed.
 */
bool bnrg2_updateCharValue(ble_conn_t conn, const ble_char_t *charact,
                           const uint8_t *value, uint8_t value_len);

/**
 * @brief Verify if the passed attribute handle is the characteristic value handle of the
 * characteristic specified.
 * 
 * @param charact The characteristic of the value we want to verify.
 * @param attr_handle The attribute handle.
 * @return true if it is the CharValueHandle, false if not.
 */
__STATIC_INLINE bool bnrg2_isCharValueHandle(const ble_char_t *charact, uint16_t attr_handle) {
  return charact->_char_val_handle == attr_handle;
}

/**
 * @brief Verify if the passed attribute handle is the Client Characteristic Configuration Descriptor 
 * handle of the characteristic specified.
 * 
 * @param charact The characteristic of the CCCD we want to verify.
 * @param attr_handle The attribute handle.
 * @return true if it is the CCCD handle, false if not.
 */
__STATIC_INLINE bool bnrg2_isCharCCCDHandle(const ble_char_t *charact, uint16_t attr_handle) {
  return charact->_char_desc_cccd_handle == attr_handle;
}

/**
 * @brief Verify if the CCCD data buffer passed means Notification Enabled.
 * 
 * @param cccd_data CCCD data buffer.
 * @param cccd_data_len CCCD data buffer length.
 * @return true if Notification is enabled, false if not.
 */
__STATIC_INLINE bool bnrg2_isCCCDNotiEnabled(const uint8_t *cccd_data, uint8_t cccd_data_len) {
  if (cccd_data_len != 2) return false;
  if (cccd_data[0] != 0x01) return false;
  return true;
}

/**
 * @brief Verify if the CCCD data buffer passed means Indication Enabled.
 * 
 * @param cccd_data CCCD data buffer.
 * @param cccd_data_len CCCD data buffer length.
 * @return true if Indication is enabled, false if failed.
 */
__STATIC_INLINE bool bnrg2_isCCCDIndEnabled(const uint8_t *cccd_data, uint8_t cccd_data_len) {
  if (cccd_data_len != 2) return false;
  if (cccd_data[1] != 0x01) return false;
  return true;
}

/**
 * @brief Set the device Complete Local Name.
 * 
 * @param local_name Local name buffer.
 * @param local_name_len Local name buffer length.
 */
void bnrg2_setLocalName(const uint8_t *local_name, uint8_t local_name_len);

/**
 * @brief Enable or disable connectable mode.
 * 
 * @param en True if you want to enable, false if you want to disable.
 */
void bnrg2_setConnectableMode(bool en);

/**
 * @brief Execute bluenrg2 processes (must be called always in the loop).
 * 
 */
void bnrg2_process(void);

/**
 * @brief Returns the connection handle if any, if not returns 0.
 * 
 * @return 0 if no connection, otherwise connection handle
 */
ble_conn_t bnrg2_getConnHandle(void);

// ========================================================================
// Event handlers
// ========================================================================

void __bnrg_on_connect(ble_conn_t conn);
#define BNRG_EVT_ON_CONNECT(conn) void __bnrg_on_connect(ble_conn_t conn)

void __bnrg_on_disconnect(ble_conn_t conn);
#define BNRG_EVT_ON_DISCONNECT(conn) void __bnrg_on_disconnect(ble_conn_t conn)

#define BNRG_EVT_ON_ATTR_MODIFIED(conn, attr_handle, attr_data, attr_data_len) \
  void aci_gatt_attribute_modified_event(uint16_t conn,                        \
                                         uint16_t attr_handle,                 \
                                         uint16_t offset,                      \
                                         uint16_t attr_data_len,               \
                                         uint8_t attr_data[])

// ===============================================================
// EXTI IRQ Handler Function
// ===============================================================

void bnrg2_exti_irq_handler(void);

#endif