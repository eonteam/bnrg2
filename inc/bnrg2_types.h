#ifndef __BNRG2_TYPES_H_
#define __BNRG2_TYPES_H_

#include "eonOS.h"

// ===============================================================
// Hardware struct
// ===============================================================

typedef struct {
  SPI_TypeDef *SPIx;
  pin_t cs_pin;
  pin_t rst_pin;
  pin_t exti_irq_pin;
  IRQn_Type exti_irqn;
} bnrg2_hw_t;

typedef uint16_t ble_conn_t;

typedef struct {
  uint16_t _service_handle;
} ble_service_t;

typedef struct {
  uint16_t _service_handle;
  uint16_t _char_decl_handle;      // charactetistic declaration handle (handle)
  uint16_t _char_val_handle;       // charactetistic value handle (handle + 1)
  uint16_t _char_desc_cccd_handle; // charactetistic descriptor Client Characteristic Descriptor handle (handle + 2)
  uint16_t _max_value_len;
  uint8_t _char_props;
  uint8_t _is_variable_len;
} ble_char_t;

#endif
