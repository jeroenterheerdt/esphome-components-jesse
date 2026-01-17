#pragma once

#include "esphome/core/helpers.h"

#include "esphome/components/display/display_buffer.h"
#include "esphome/components/uart/uart.h"

#include <cinttypes>
#include <queue>
#include <vector>

namespace esphome {
namespace m5stack_printer {

// Barcode types according to CSN-A2 datasheet GS k command table
// These values correspond to format 1 (m=0-6) from the datasheet
enum BarcodeType {
  UPC_A = 0,     // 11-12 characters, digits only
  UPC_E = 1,     // 11-12 characters, digits only  
  EAN13 = 2,     // 12-13 characters, digits only (was JAN13)
  EAN8 = 3,      // 7-8 characters, digits only (was JAN8)
  CODE39 = 4,    // Variable length, alphanumeric + special chars
  ITF = 5,       // Variable even length, digits only
  CODABAR = 6,   // Variable length, digits + A-D + special chars
  CODE93 = 7,    // Variable length, full ASCII (uses format 2, m=72)
  CODE128 = 8,   // Variable length, full ASCII (uses format 2, m=73)
};

/**
 * M5Stack Thermal Printer Component
 * 
 * This component provides ESC/POS compatible thermal printing functionality
 * for the M5Stack thermal printer module (CSN-A2 based).
 * 
 * Supported features:
 * - Text printing with font size control (0-7)
 * - QR code printing with error correction
 * - 1D barcode printing (UPC, EAN, CODE39, CODE128, etc.)
 * - Raster image printing via display buffer
 * 
 * Communication: UART (TTL/RS232) at 9600 baud by default
 * Print width: 58mm (384 dots), 8 dots/mm resolution
 */
class M5StackPrinterDisplay : public display::DisplayBuffer, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void update() override;

  void write_to_device_();

  // Display buffer
  int get_width_internal() override { return 8 * 58; };  // 58mm, 8 dots per mm
  int get_height_internal() override { return this->height_; };

  void set_height(int height) { this->height_ = height; }

  display::DisplayType get_display_type() override { return display::DisplayType::DISPLAY_TYPE_BINARY; }

  /**
   * Print text with specified font size
   * @param text Text string to print
   * @param font_size Font size multiplier (0-7), affects both width and height
   */
  void print_text(std::string text, uint8_t font_size = 0);
  
  /**
   * Print newlines to advance paper
   * @param lines Number of lines to advance (1-255)
   */
  void new_line(uint8_t lines);
  
  /**
   * Print QR code with error correction level L
   * @param data String data to encode (max ~100 characters recommended)
   */
  void print_qrcode(std::string data);

  /**
   * Print 1D barcode with validation
   * @param barcode Barcode data string
   * @param type Barcode format (UPC_A, EAN13, CODE128, etc.)
   */
  void print_barcode(std::string barcode, BarcodeType type);

 protected:
  void draw_absolute_pixel_internal(int x, int y, Color color) override;
  size_t get_buffer_length_() { return size_t(this->get_width_internal()) * size_t(this->get_height_internal()) / 8; }
  void queue_data_(std::vector<uint8_t> data);
  void queue_data_(const uint8_t *data, size_t size);
  void init_();

  std::queue<std::vector<uint8_t>> queue_{};
  int height_{0};
};

template<typename... Ts>
class M5StackPrinterPrintTextAction : public Action<Ts...>, public Parented<M5StackPrinterDisplay> {
 public:
  TEMPLATABLE_VALUE(std::string, text)
  TEMPLATABLE_VALUE(uint8_t, font_size)

  void play(Ts... x) override { this->parent_->print_text(this->text_.value(x...), this->font_size_.value(x...)); }
};

}  // namespace m5stack_printer
}  // namespace esphome
