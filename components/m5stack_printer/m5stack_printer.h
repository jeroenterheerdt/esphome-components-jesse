#pragma once

#include "esphome/core/helpers.h"

#include "esphome/components/display/display_buffer.h"
#include "esphome/components/uart/uart.h"

#include <cinttypes>
#include <queue>
#include <vector>

namespace esphome {
namespace m5stack_printer {

enum BarcodeType { UPC_A = 0x41, UPC_E, EAN13, EAN8, CODE39, ITF, CODABAR, CODE93, CODE128, UNKNOWN };

class M5StackPrinterDisplay : public display::DisplayBuffer, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void update() override;
  bool can_proceed() override { return this->ready_; }

  void write_to_device_();

  // Display buffer
  int get_width_internal() override { return 8 * 58; };  // 58mm, 8 dots per mm
  int get_height_internal() override { return this->height_; };

  void set_height(int height) { this->height_ = height; }

  display::DisplayType get_display_type() override { return display::DisplayType::DISPLAY_TYPE_BINARY; }

  void reset();
  void print_text(std::string text, uint8_t font_size = 0, std::string font = "A", bool inverse = false,
                  bool updown = false, bool bold = false, bool double_height = false, bool double_width = false,
                  bool strike = false, bool ninety_degrees = false);
  void new_line(uint8_t lines);
  void print_qrcode(std::string data);

  void print_barcode(std::string barcode, std::string type);

  void set_send_wakeup(bool send_wakeup) {
    ESP_LOGD("set_send_wakeup", "send_wakeup: %s", send_wakeup ? "true" : "false");
    this->send_wakeup_ = send_wakeup;
  }

  void set_font_size_factor(double font_size_factor) {
    ESP_LOGD("set_font_size_factor", "font_size_factor: %d", font_size_factor);
    this->font_size_factor_ = font_size_factor;
  }

  void set_firmware(std::string fw);

 protected:
  void draw_absolute_pixel_internal(int x, int y, Color color) override;
  size_t get_buffer_length_() { return size_t(this->get_width_internal()) * size_t(this->get_height_internal()) / 8; }
  void queue_data_(std::vector<uint8_t> data);
  void queue_data_(const uint8_t *data, size_t size);
  void init_();

  std::queue<std::vector<uint8_t>> queue_{};
  int height_{0};
  bool ready_{false};
  bool send_wakeup_{false};
  double font_size_factor_{1.0};
  int firmware_{268};

 private:
  uint8_t printMode, charHeight, maxColumn;
  void unsetPrintMode(uint8_t mask), setPrintMode(uint8_t mask), writePrintMode(), adjustCharValues(uint8_t mask);
};

template<typename... Ts>
class M5StackPrinterPrintTextAction : public Action<Ts...>, public Parented<M5StackPrinterDisplay> {
 public:
  TEMPLATABLE_VALUE(std::string, text)
  TEMPLATABLE_VALUE(uint8_t, font_size)
  TEMPLATABLE_VALUE(std::string, font)
  TEMPLATABLE_VALUE(bool, inverse)
  TEMPLATABLE_VALUE(bool, updown)
  TEMPLATABLE_VALUE(bool, bold)
  TEMPLATABLE_VALUE(bool, double_height)
  TEMPLATABLE_VALUE(bool, double_width)
  TEMPLATABLE_VALUE(bool, strike)
  TEMPLATABLE_VALUE(bool, ninety_degrees)

  void play(Ts... x) override {
    this->parent_->print_text(this->text_.value(x...), this->font_size_.value(x...), this->font_.value(x...),
                              this->inverse_.value(x...), this->updown_.value(x...), this->bold_.value(x...),
                              this->double_height_.value(x...), this->double_width_.value(x...),
                              this->strike_.value(x...)),
        this->ninety_degrees_.value(x...);
  }
};

template<typename... Ts>
class M5StackPrinterNewLineAction : public Action<Ts...>, public Parented<M5StackPrinterDisplay> {
 public:
  TEMPLATABLE_VALUE(uint8_t, lines)

  void play(Ts... x) override { this->parent_->new_line(this->lines_.value(x...)); }
};

template<typename... Ts>
class M5StackPrinterPrintQRCodeAction : public Action<Ts...>, public Parented<M5StackPrinterDisplay> {
 public:
  TEMPLATABLE_VALUE(std::string, qrcode)

  void play(Ts... x) override { this->parent_->print_qrcode(this->qrcode_.value(x...)); }
};

template<typename... Ts>
class M5StackPrinterPrintBarCodeAction : public Action<Ts...>, public Parented<M5StackPrinterDisplay> {
 public:
  TEMPLATABLE_VALUE(std::string, barcode)
  TEMPLATABLE_VALUE(std::string, type)

  void play(Ts... x) override { this->parent_->print_barcode(this->barcode_.value(x...), this->type_.value(x...)); }
};

}  // namespace m5stack_printer
}  // namespace esphome
