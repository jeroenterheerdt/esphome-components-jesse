#pragma once

#include "esphome/core/helpers.h"

#include "esphome/components/display/display_buffer.h"
#include "esphome/components/uart/uart.h"

#include <cinttypes>
#include <queue>
#include <vector>

namespace esphome {
namespace thermal_printer {

// Barcodes
enum BarcodeType : uint8_t { UPC_A = 0, UPC_E = 1, EAN13 = 2, EAN8 = 3, CODE39 = 4, ITF = 5, CODABAR = 6 };
enum BarcodeTextPosition : uint8_t { HRI_NONE = 0, HRI_ABOVE = 1, HRI_BELOW = 2, HRI_BOTH = 3 };
enum BarcodeAlignment : uint8_t { ALIGN_LEFT = 0, ALIGN_CENTER = 1, ALIGN_RIGHT = 2 };

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

  void set_height(int height) { this->height_ = height; }
  display::DisplayType get_display_type() override { return display::DisplayType::DISPLAY_TYPE_BINARY; }

  void print_text(std::string text, std::string align = "L", bool inverse = false, bool ninety_degree = false,
                  uint8_t underline_weight = 0, bool updown = false, bool bold = false, bool double_width = false,
                  bool double_height = false, std::string font = "A", bool strikethrough = false);
  void print_text(std::string text, std::string align = "L", bool inverse = false, bool ninety_degree = false,
                  uint8_t underline_weight = 0, bool updown = false, bool bold = false, uint8_t font_width = 0,
                  uint8_t font_height = 0, std::string font = "A", bool strikethrough = false);
  void set_tab_positions(std::vector<int> tab_positions);
  void set_row_spacing(uint8_t spacing);
  void new_line(uint8_t lines);
  void print_barcode(std::string text, BarcodeType type = BarcodeType::EAN13, uint8_t height = 162, uint8_t width = 2,
                     BarcodeTextPosition pos = BarcodeTextPosition::HRI_BELOW,
                     BarcodeAlignment align = BarcodeAlignment::ALIGN_CENTER);

  void set_send_wakeup(bool send_wakeup) {
    ESP_LOGD("set_send_wakeup", "send_wakeup: %s", send_wakeup ? "true" : "false");
    this->send_wakeup_ = send_wakeup;
  }

  void set_font_size_factor(double font_size_factor) {
    ESP_LOGD("set_font_size_factor", "font_size_factor: %d", font_size_factor);
    this->font_size_factor_ = font_size_factor;
  }

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
  uint8_t spacing{32};
  std::vector<uint8_t> tab_positions{};

 private:
  std::string toUpperCase(const std::string &input);
};

template<typename... Ts>
class ThermalPrinterPrintTextActionDWDH : public Action<Ts...>, public Parented<ThermalPrinterDisplay> {
 public:
  TEMPLATABLE_VALUE(std::string, text)
  TEMPLATABLE_VALUE(std::string, align)
  TEMPLATABLE_VALUE(bool, inverse)
  TEMPLATABLE_VALUE(bool, ninety_degree)
  TEMPLATABLE_VALUE(uint8_t, underline_weight)
  TEMPLATABLE_VALUE(bool, updown)
  TEMPLATABLE_VALUE(bool, bold)
  TEMPLATABLE_VALUE(bool, double_width)
  TEMPLATABLE_VALUE(bool, double_height)
  TEMPLATABLE_VALUE(std::string, font)
  TEMPLATABLE_VALUE(bool, strikethrough)

  void play(Ts... x) override {
    this->parent_->print_text(this->text_.value(x...), this->align_.value(x...), this->inverse_.value(x...),
                              this->ninety_degree_.value(x...), this->underline_weight_.value(x...),
                              this->updown_.value(x...), this->bold_.value(x...), this->double_width_.value(x...),
                              this->double_height_.value(x...), this->font_.value(x...),
                              this->strikethrough_.value(x...));
  }
};

template<typename... Ts>
class ThermalPrinterPrintTextActionFWFH : public Action<Ts...>, public Parented<ThermalPrinterDisplay> {
 public:
  TEMPLATABLE_VALUE(std::string, text)
  TEMPLATABLE_VALUE(std::string, align)
  TEMPLATABLE_VALUE(bool, inverse)
  TEMPLATABLE_VALUE(bool, ninety_degree)
  TEMPLATABLE_VALUE(uint8_t, underline_weight)
  TEMPLATABLE_VALUE(bool, updown)
  TEMPLATABLE_VALUE(bool, bold)
  TEMPLATABLE_VALUE(uint8_t, font_width)
  TEMPLATABLE_VALUE(uint8_t, font_height)
  TEMPLATABLE_VALUE(std::string, font)
  TEMPLATABLE_VALUE(bool, strikethrough)

  void play(Ts... x) override {
    this->parent_->print_text(this->text_.value(x...), this->align_.value(x...), this->inverse_.value(x...),
                              this->ninety_degree_.value(x...), this->underline_weight_.value(x...),
                              this->updown_.value(x...), this->bold_.value(x...), this->font_width_.value(x...),
                              this->font_height_.value(x...), this->font_.value(x...),
                              this->strikethrough_.value(x...));
  }
};

template<typename... Ts>
class ThermalPrinterTabPositionsAction : public Action<Ts...>, public Parented<ThermalPrinterDisplay> {
 public:
  TEMPLATABLE_VALUE(std::vector<int>, tabs)

  void play(Ts... x) override { this->parent_->set_tab_positions(this->tabs_.value(x...)); }
};

template<typename... Ts>
class ThermalPrinterRowSpacingAction : public Action<Ts...>, public Parented<ThermalPrinterDisplay> {
 public:
  TEMPLATABLE_VALUE(uint8_t, spacing)

  void play(Ts... x) override { this->parent_->set_row_spacing(this->spacing_.value(x...)); }
};
template<typename... Ts>
class ThermalPrinterNewLineAction : public Action<Ts...>, public Parented<ThermalPrinterDisplay> {
 public:
  TEMPLATABLE_VALUE(uint8_t, lines)

  void play(Ts... x) override { this->parent_->new_line(this->lines_.value(x...)); }
};

template<typename... Ts>
class ThermalPrinterBarcodeAction : public Action<Ts...>, public Parented<ThermalPrinterDisplay> {
 public:
  TEMPLATABLE_VALUE(std::string, text)
  TEMPLATABLE_VALUE(BarcodeType, type)
  TEMPLATABLE_VALUE(uint8_t, height)
  TEMPLATABLE_VALUE(uint8_t, width)
  TEMPLATABLE_VALUE(BarcodeTextPosition, pos)
  TEMPLATABLE_VALUE(BarcodeAlignment, align)

  void play(Ts... x) override {
    this->parent_->print_barcode(this->text_.value(x...), this->type_.value(x...), this->height_.value(x...),
                                 this->width_.value(x...), this->pos_.value(x...), this->align_.value(x...));
  }
};

}  // namespace thermal_printer
}  // namespace esphome
