#include "hci_tl.h"

// ===============================================================
// Private structure
// ===============================================================

static bnrg2_hw_t ble_hw;

// ===============================================================
// Definitions
// ===============================================================

#define HEADER_SIZE 5U
#define MAX_BUFFER_SIZE 255U
#define TIMEOUT_DURATION 15U

// ===============================================================
// Privates
// ===============================================================

// Enable SPI IRQ.
static void HCI_TL_SPI_Enable_IRQ(void) { NVIC_EnableIRQ(ble_hw.exti_irqn); }

// Disable SPI IRQ.
static void HCI_TL_SPI_Disable_IRQ(void) { NVIC_DisableIRQ(ble_hw.exti_irqn); }

// Reports if the BlueNRG has data for the host micro (1 if data are present, 0 otherwise).
static int32_t IsDataAvailable(void) { return gpio_read(ble_hw.exti_irq_pin); }

// ===============================================================
// EON Bridge
// ===============================================================

void hci_eon_brige(const bnrg2_hw_t *hw) {
  ble_hw.SPIx = hw->SPIx;
  ble_hw.cs_pin = hw->cs_pin;
  ble_hw.rst_pin = hw->rst_pin;
  ble_hw.exti_irq_pin = hw->exti_irq_pin;
  ble_hw.exti_irqn = hw->exti_irqn;
}

// ===============================================================
// IO Operation and BUS services
// ===============================================================

/**
 * @brief  Initializes the peripherals communication with the BlueNRG
 *         Expansion Board (via SPI, I2C, USART, ...)
 *
 * @param  void* Pointer to configuration struct
 * @retval int32_t Status
 */
int32_t HCI_TL_SPI_Init(void *pConf) {

  // Configure RESET Line
  gpio_mode(ble_hw.rst_pin, OUTPUT_PP, NOPULL, SPEED_LOW);

  // Configure CS
  gpio_mode(ble_hw.cs_pin, OUTPUT_PP, NOPULL, SPEED_LOW);

  // Deselect CS PIN for BlueNRG at startup to avoid spurious commands
  gpio_set(ble_hw.cs_pin);

  return 0;
}

/**
 * @brief  DeInitializes the peripherals communication with the BlueNRG
 *         Expansion Board (via SPI, I2C, USART, ...)
 *
 * @param  None
 * @retval int32_t 0
 */
int32_t HCI_TL_SPI_DeInit(void) {
  exti_detach(ble_hw.exti_irq_pin);
  gpio_mode(ble_hw.exti_irq_pin, ANALOG, NOPULL, SPEED_LOW);
  gpio_mode(ble_hw.cs_pin, ANALOG, NOPULL, SPEED_LOW);
  gpio_mode(ble_hw.rst_pin, ANALOG, NOPULL, SPEED_LOW);
  return 0;
}

/**
 * @brief Reset BlueNRG module.
 *
 * @param  None
 * @retval int32_t 0
 */
int32_t HCI_TL_SPI_Reset(void) {
  // Deselect CS PIN for BlueNRG to avoid spurious commands
  gpio_set(ble_hw.cs_pin);

  gpio_reset(ble_hw.rst_pin);
  delay(5);
  gpio_set(ble_hw.rst_pin);
  delay(5);

  return 0;
}

/**
 * @brief  Reads from BlueNRG SPI buffer and store data into local buffer.
 *
 * @param  buffer : Buffer where data from SPI are stored
 * @param  size   : Buffer size
 * @retval int32_t: Number of read bytes
 */
int32_t HCI_TL_SPI_Receive(uint8_t *buffer, uint16_t size) {
  uint16_t byte_count;
  uint8_t len = 0;
  uint8_t char_00 = 0x00;
  volatile uint8_t read_char;

  uint8_t header_master[HEADER_SIZE] = {0x0b, 0x00, 0x00, 0x00, 0x00};
  uint8_t header_slave[HEADER_SIZE];

  HCI_TL_SPI_Disable_IRQ();

  // CS reset
  gpio_reset(ble_hw.cs_pin);

  // Read the header
  spi_writeMultiple8(ble_hw.SPIx, header_master, header_slave, HEADER_SIZE);

  // device is ready
  byte_count = (header_slave[4] << 8) | header_slave[3];

  if (byte_count > 0) {

    // avoid to read more data than the size of the buffer
    if (byte_count > size) {
      byte_count = size;
    }

    for (len = 0; len < byte_count; len++) {
      read_char = spi_write8(ble_hw.SPIx, char_00);
      buffer[len] = read_char;
    }
  }

  // Release CS line
  gpio_set(ble_hw.cs_pin);

  HCI_TL_SPI_Enable_IRQ();

  return len;
}

/**
 * @brief  Writes data from local buffer to SPI.
 *
 * @param  buffer : data buffer to be written
 * @param  size   : size of first data buffer to be written
 * @retval int32_t: Number of read bytes
 */
int32_t HCI_TL_SPI_Send(uint8_t *buffer, uint16_t size) {
  int32_t result;
  uint16_t rx_bytes;

  uint8_t header_master[HEADER_SIZE] = {0x0a, 0x00, 0x00, 0x00, 0x00};
  uint8_t header_slave[HEADER_SIZE];

  static uint8_t read_char_buf[MAX_BUFFER_SIZE];
  uint32_t tickstart = millis();

  HCI_TL_SPI_Disable_IRQ();

  do {
    uint32_t tickstart_data_available = millis();

    result = 0;

    // CS reset
    gpio_reset(ble_hw.cs_pin);

    // Wait until BlueNRG-2 is ready.
    // When ready it will raise the IRQ pin.
    while (!IsDataAvailable()) {
      if ((millis() - tickstart_data_available) > TIMEOUT_DURATION) {
        result = -3;
        break;
      }
    }
    if (result == -3) {
      // The break causes the exiting from the "while", so the CS line must be released
      gpio_set(ble_hw.cs_pin);
      break;
    }

    // Read header
    spi_writeMultiple8(ble_hw.SPIx, header_master, header_slave, HEADER_SIZE);

    rx_bytes = (((uint16_t) header_slave[2]) << 8) | ((uint16_t) header_slave[1]);

    if (rx_bytes >= size) {
      // Buffer is big enough
      spi_writeMultiple8(ble_hw.SPIx, buffer, read_char_buf, size);
    } else {
      // Buffer is too small
      result = -2;
    }

    // Release CS line
    gpio_set(ble_hw.cs_pin);

    if ((millis() - tickstart) > TIMEOUT_DURATION) {
      result = -3;
      break;
    }
  } while (result < 0);

  HCI_TL_SPI_Enable_IRQ();

  return result;
}

/**
 * @brief  Return the current milliseconds of the system.
 *
 * @param  None
 * @retval int32_t: Return the current milliseconds of the system
 */
int32_t HCI_TL_GetTick(void) {
  return (int32_t) millis();
}

// ===============================================================
// hci_tl_interface main functions
// ===============================================================

// Register hci_tl_interface IO bus services
void hci_tl_lowlevel_init(void) {
  tHciIO fops;

  // Register IO bus services
  fops.Init = HCI_TL_SPI_Init;
  fops.DeInit = HCI_TL_SPI_DeInit;
  fops.Send = HCI_TL_SPI_Send;
  fops.Receive = HCI_TL_SPI_Receive;
  fops.Reset = HCI_TL_SPI_Reset;
  fops.GetTick = HCI_TL_GetTick;

  hci_register_io_bus(&fops);

  // Initialize event irq
  // REVIEW MODE
  exti_attach(ble_hw.exti_irq_pin, NOPULL, MODE_CHANGE);
}

// HCI Transport Layer Low Level Interrupt Service Routine
void hci_tl_lowlevel_isr(void) {
  // Call hci_notify_asynch_evt()
  while (IsDataAvailable()) {
    hci_notify_asynch_evt(NULL);
  }
}