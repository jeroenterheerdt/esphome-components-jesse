#pragma once

#include "esphome/core/helpers.h"

#include "esphome/components/display/display_buffer.h"
#include "esphome/components/uart/uart.h"

#include <cinttypes>
#include <queue>
#include <vector>

namespace esphome {
namespace thermal_printer {

enum BarcodeType { UPC_A = 0x41, UPC_E, EAN13, EAN8, CODE39, ITF, CODABAR, CODE93, CODE128, UNKNOWN };

class ThermalPrinterDisplay : public display::DisplayBuffer, public uart::UARTDevice {
 public:
  void setup() override;
  void loop() override;
  void update() override;
  bool can_proceed() override { return this->ready_; }

  void write_to_device_();

  // Display buffer
  int get_width_internal() override { return 8 * 58; };  // 58mm, 8 dots per mm
  int get_height_internal() override { return this->height_; };

  display::DisplayType get_display_type() override { return display::DisplayType::DISPLAY_TYPE_BINARY; }

  void print_text(std::string text);
  void new_line(uint8_t lines);

  void set_send_wakeup(bool send_wakeup) {
    ESP_LOGD("set_send_wakeup", "send_wakeup: %s", send_wakeup ? "true" : "false");
    this->send_wakeup_ = send_wakeup;
  }

  void set_font_size_factor(double font_size_factor) {
    ESP_LOGD("set_font_size_factor", "font_size_factor: %d", font_size_factor);
    this->font_size_factor_ = font_size_factor;
  }

 protected:
  size_t get_buffer_length_() { return size_t(this->get_width_internal()) * size_t(this->get_height_internal()) / 8; }
  void queue_data_(std::vector<uint8_t> data);
  void queue_data_(const uint8_t *data, size_t size);
  void init_();

  std::queue<std::vector<uint8_t>> queue_{};
  int height_{0};
  bool ready_{false};
  bool send_wakeup_{false};
  double font_size_factor_{1.0};

 private:
  std::string toUpperCase(const std::string &input);
};

template<typename... Ts>
class ThermalPrinterPrintTextAction : public Action<Ts...>, public Parented<ThermalPrinterDisplay> {
 public:
  TEMPLATABLE_VALUE(std::string, text)

  void play(Ts... x) override { this->parent_->print_text(this->text_.value(x...)); }
};

template<typename... Ts>
class ThermalPrinterNewLineAction : public Action<Ts...>, public Parented<ThermalPrinterDisplay> {
 public:
  TEMPLATABLE_VALUE(uint8_t, lines)

  void play(Ts... x) override { this->parent_->new_line(this->lines_.value(x...)); }
};

}  // namespace thermal_printer
}  // namespace esphome
