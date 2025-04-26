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
enum BarcodeTextPosition : uint8_t { NONE = 0, ABOVE = 1, BELOW = 2, BOTH = 3 };

// QR Codes
enum QRCodeModel : uint8_t { MODEL_1 = 0, MODEL_2 = 1 };
enum QRCodeErrorCorrectionLevel : uint8_t {
  LEVEL_L = 0,  // 7% error correction
  LEVEL_M = 1,  // 15% error correction
  LEVEL_Q = 2,  // 25% error correction
  LEVEL_H = 3   // 30% error correction
};

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
  void print_barcode(std::string text,  // BarcodeType type = BarcodeType::EAN13
                     std::string type = "EAN13", uint8_t height = 162, uint8_t width = 2,
                     // BarcodeTextPosition pos = BarcodeTextPosition::BELOW,
                     std::string pos = "BELOW",
                     // BarcodeAlignment align = BarcodeAlignment::ALIGN_CENTER
                     std::string align = "C");
  void print_qr_code(std::string text,
                     // QRCodeModel model = QRCodeModel::MODEL_2,
                     std::string model = "MODEL_2",
                     // QRCodeErrorCorrectionLevel error_correction_level = QRCodeErrorCorrectionLevel::LEVEL_L,
                     std::string error_correction_level = "LEVEL_L", uint8_t size = 3);
  void cut(std::string cut_type = "Full");
  void print_image(std::string image, int height, int width);
  void demo();

  void set_send_wakeup(bool send_wakeup) {
    ESP_LOGD("set_send_wakeup", "send_wakeup: %s", send_wakeup ? "true" : "false");
    this->send_wakeup_ = send_wakeup;
  }

  void set_font_size_factor(double font_size_factor) {
    ESP_LOGD("set_font_size_factor", "font_size_factor: %d", font_size_factor);
    this->font_size_factor_ = font_size_factor;
  }

 protected:
  bool has_paper();
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
  bool paper_{true};
  uint8_t spacing{32};
  std::vector<uint8_t> tab_positions{};

 private:
  std::string toUpperCase(const std::string &input);
  void invertBitmapColors(uint8_t *bitmap);
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
  TEMPLATABLE_VALUE(std::string, barcode_text)
  // TEMPLATABLE_VALUE(BarcodeType, barcode_type)
  TEMPLATABLE_VALUE(std::string, barcode_type)
  TEMPLATABLE_VALUE(uint8_t, barcode_height)
  TEMPLATABLE_VALUE(uint8_t, barcode_width)
  // TEMPLATABLE_VALUE(BarcodeTextPosition, barcode_text_pos)
  TEMPLATABLE_VALUE(std::string, barcode_text_pos)
  // TEMPLATABLE_VALUE(BarcodeAlignment, align)
  TEMPLATABLE_VALUE(std::string, barcode_align)

  void play(Ts... x) override {
    this->parent_->print_barcode(this->barcode_text_.value(x...), this->barcode_type_.value(x...),
                                 this->barcode_height_.value(x...), this->barcode_width_.value(x...),
                                 this->barcode_text_pos_.value(x...), this->barcode_align_.value(x...));
  }
};

template<typename... Ts>
class ThermalPrinterQRCodeAction : public Action<Ts...>, public Parented<ThermalPrinterDisplay> {
 public:
  TEMPLATABLE_VALUE(std::string, qr_code_text)
  TEMPLATABLE_VALUE(std::string, qr_code_model)
  TEMPLATABLE_VALUE(std::string, qr_code_error_correction_level)
  TEMPLATABLE_VALUE(uint8_t, qr_code_size)

  void play(Ts... x) override {
    this->parent_->print_qr_code(this->qr_code_text_.value(x...), this->qr_code_model_.value(x...),
                                 this->qr_code_error_correction_level_.value(x...), this->qr_code_size_.value(x...));
  }
};
template<typename... Ts> class ThermalPrinterCutAction : public Action<Ts...>, public Parented<ThermalPrinterDisplay> {
 public:
  TEMPLATABLE_VALUE(std::string, cut_type)
  void play(Ts... x) override { this->parent_->cut(this->cut_type_.value(x...)); }
};

template<typename... Ts>
class ThermalPrinterPrintImageAction : public Action<Ts...>, public Parented<ThermalPrinterDisplay> {
 public:
  TEMPLATABLE_VALUE(std::string, image_data)
  TEMPLATABLE_VALUE(int, image_height)
  TEMPLATABLE_VALUE(int, image_width)

  void play(Ts... x) override {
    this->parent_->print_image(this->image_data_.value(x...), this->image_height_.value(x...),
                               this->image_width_.value(x...));
  }
};

template<typename... Ts> class ThermalPrinterDemoAction : public Action<Ts...>, public Parented<ThermalPrinterDisplay> {
 public:
  void play(Ts... x) override { this->parent_->demo(); }
};

}  // namespace display

}  // namespace thermal_printer
}  // namespace esphome
