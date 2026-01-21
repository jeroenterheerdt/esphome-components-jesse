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

  /**
   * Cut paper (full cut)
   * Requires printer with cutting mechanism
   */
  void cut_paper();

  /**
   * Cut paper with specified mode
   * @param mode 0=full cut, 1=partial cut, 66=cut and feed
   * @param feed_lines Additional lines to feed before cutting (mode 66 only)
   */
  void cut_paper(uint8_t mode, uint8_t feed_lines = 0);

  /**
   * Set text alignment
   * @param alignment 0=left, 1=center, 2=right
   */
  void set_text_alignment(uint8_t alignment);

  /**
   * Set line spacing
   * @param spacing Line spacing in dots (0-255)
   */
  void set_line_spacing(uint8_t spacing);

  /**
   * Set print density and break time
   * @param density Print density (0-31, affects darkness)
   * @param break_time Break time between dots (0-7)
   */
  void set_print_density(uint8_t density, uint8_t break_time = 0);

  /**
   * Set text style modes
   * @param bold Enable bold/emphasized text
   * @param underline Underline mode (0=off, 1=1dot, 2=2dot)
   * @param double_width Enable double width text
   * @param upside_down Enable upside down text
   */
  void set_text_style(bool bold = false, uint8_t underline = 0, bool double_width = false, bool upside_down = false);

  /**
   * Print test page
   * Useful for checking printer functionality
   */
  void print_test_page();

  /**
   * Check printer status
   * @return Status byte with paper/voltage/temperature flags
   */
  uint8_t get_printer_status();

  /**
   * Enter sleep mode after timeout
   * @param timeout_seconds Time to wait before sleep (0=disable)
   * Useful for battery-powered applications
   */
  void set_sleep_mode(uint16_t timeout_seconds);

  /**
   * Set 90-degree clockwise rotation mode
   * @param enable True to enable rotation, false to disable
   */
  void set_90_degree_rotation(bool enable);

  /**
   * Set white/black inverse printing mode
   * @param enable True to enable inverse (white text on black), false for normal
   */
  void set_inverse_printing(bool enable);

  /**
   * Enable Chinese/Japanese character mode
   * When enabled, processes multi-byte characters for Asian text
   * @param enable True to enable Asian character mode, false for ASCII mode
   */
  void set_chinese_mode(bool enable);

  /**
   * Run demo/debug function to showcase all printer capabilities
   * Features Hitchhiker's Guide to the Galaxy themed content
   * @param show_qr_code Print QR code demo (default: false, true when no params)
   * @param show_barcode Print barcode demo (default: false, true when no params)
   * @param show_text_styles Print various text formatting demos (default: false, true when no params)
   * @param show_inverse Print inverse text demo (default: false, true when no params)
   * @param show_rotation Print 90-degree rotated text demo (default: false, true when no params)
   */
  void run_demo(bool show_qr_code = false, bool show_barcode = false,
                bool show_text_styles = false, bool show_inverse = false, bool show_rotation = false);

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

  void play(const Ts &...x) override { this->parent_->print_text(this->text_.value(x...), this->font_size_.value(x...)); }
};

template<typename... Ts>
class M5StackPrinterPrintQRCodeAction : public Action<Ts...>, public Parented<M5StackPrinterDisplay> {
 public:
  TEMPLATABLE_VALUE(std::string, qrcode)

  void play(const Ts &...x) override { this->parent_->print_qrcode(this->qrcode_.value(x...)); }
};

template<typename... Ts>
class M5StackPrinterPrintBarcodeAction : public Action<Ts...>, public Parented<M5StackPrinterDisplay> {
 public:
  TEMPLATABLE_VALUE(std::string, barcode)
  TEMPLATABLE_VALUE(std::string, type)

  void play(Ts... x) override {
    // Convert string type to BarcodeType enum
    std::string type_str = this->type_.value(x...);
    BarcodeType barcode_type = CODE128; // Default
    if (type_str == "UPC_A") barcode_type = UPC_A;
    else if (type_str == "UPC_E") barcode_type = UPC_E;
    else if (type_str == "EAN13") barcode_type = EAN13;
    else if (type_str == "EAN8") barcode_type = EAN8;
    else if (type_str == "CODE39") barcode_type = CODE39;
    else if (type_str == "ITF") barcode_type = ITF;
    else if (type_str == "CODABAR") barcode_type = CODABAR;
    else if (type_str == "CODE93") barcode_type = CODE93;
    else if (type_str == "CODE128") barcode_type = CODE128;

    this->parent_->print_barcode(this->barcode_.value(x...), barcode_type);
  }
};

template<typename... Ts>
class M5StackPrinterNewLineAction : public Action<Ts...>, public Parented<M5StackPrinterDisplay> {
 public:
  TEMPLATABLE_VALUE(uint8_t, lines)

  void play(const Ts &...x) override { this->parent_->new_line(this->lines_.value(x...)); }
};

template<typename... Ts>
class M5StackPrinterCutPaperAction : public Action<Ts...>, public Parented<M5StackPrinterDisplay> {
 public:
  TEMPLATABLE_VALUE(uint8_t, cut_mode)
  TEMPLATABLE_VALUE(uint8_t, feed_lines)

  void play(Ts... x) override {
    if (this->cut_mode_.value(x...) == 0 && this->feed_lines_.value(x...) == 0) {
      this->parent_->cut_paper();
    } else {
      this->parent_->cut_paper(this->cut_mode_.value(x...), this->feed_lines_.value(x...));
    }
  }
};

template<typename... Ts>
class M5StackPrinterSetAlignmentAction : public Action<Ts...>, public Parented<M5StackPrinterDisplay> {
 public:
  TEMPLATABLE_VALUE(uint8_t, alignment)

  void play(const Ts &...x) override { this->parent_->set_text_alignment(this->alignment_.value(x...)); }
};

template<typename... Ts>
class M5StackPrinterSetStyleAction : public Action<Ts...>, public Parented<M5StackPrinterDisplay> {
 public:
  TEMPLATABLE_VALUE(bool, bold)
  TEMPLATABLE_VALUE(uint8_t, underline)
  TEMPLATABLE_VALUE(bool, double_width)
  TEMPLATABLE_VALUE(bool, upside_down)

  void play(Ts... x) override {
    this->parent_->set_text_style(
      this->bold_.value(x...),
      this->underline_.value(x...),
      this->double_width_.value(x...),
      this->upside_down_.value(x...)
    );
  }
};

template<typename... Ts>
class M5StackPrinterSet90DegreeRotationAction : public Action<Ts...>, public Parented<M5StackPrinterDisplay> {
 public:
  TEMPLATABLE_VALUE(bool, enable)

  void play(const Ts &...x) override { this->parent_->set_90_degree_rotation(this->enable_.value(x...)); }
};

template<typename... Ts>
class M5StackPrinterSetInversePrintingAction : public Action<Ts...>, public Parented<M5StackPrinterDisplay> {
 public:
  TEMPLATABLE_VALUE(bool, enable)

  void play(const Ts &...x) override { this->parent_->set_inverse_printing(this->enable_.value(x...)); }
};

template<typename... Ts>
class M5StackPrinterSetChineseModeAction : public Action<Ts...>, public Parented<M5StackPrinterDisplay> {
 public:
  TEMPLATABLE_VALUE(bool, enable)

  void play(const Ts &...x) override { this->parent_->set_chinese_mode(this->enable_.value(x...)); }
};

template<typename... Ts>
class M5StackPrinterSetLineSpacingAction : public Action<Ts...>, public Parented<M5StackPrinterDisplay> {
 public:
  TEMPLATABLE_VALUE(uint8_t, spacing)

  void play(const Ts &...x) override { this->parent_->set_line_spacing(this->spacing_.value(x...)); }
};

template<typename... Ts>
class M5StackPrinterSetPrintDensityAction : public Action<Ts...>, public Parented<M5StackPrinterDisplay> {
 public:
  TEMPLATABLE_VALUE(uint8_t, density)
  TEMPLATABLE_VALUE(uint8_t, break_time)

  void play(const Ts &...x) override {
    this->parent_->set_print_density(this->density_.value(x...), this->break_time_.value(x...));
  }
};

template<typename... Ts>
class M5StackPrinterSetTabPositionsAction : public Action<Ts...>, public Parented<M5StackPrinterDisplay> {
 public:
  TEMPLATABLE_VALUE(std::string, positions)

  void play(Ts... x) override {
    // Parse comma or space separated positions and call set_tab_positions
    // For now, just log - the actual implementation would parse the string
    ESP_LOGD("m5stack_printer", "Set tab positions: %s", this->positions_.value(x...).c_str());
    // TODO: Implement tab position parsing and setting
  }
};

template<typename... Ts>
class M5StackPrinterHorizontalTabAction : public Action<Ts...>, public Parented<M5StackPrinterDisplay> {
 public:
  void play(const Ts &...x) override {
    // Send horizontal tab character (0x09)
    this->parent_->write_byte(0x09);
  }
};

template<typename... Ts>
class M5StackPrinterSetHorizontalPositionAction : public Action<Ts...>, public Parented<M5StackPrinterDisplay> {
 public:
  TEMPLATABLE_VALUE(uint16_t, position)

  void play(const Ts &...x) override {
    // Send ESC $ nL nH command to set absolute horizontal position
    uint16_t pos = this->position_.value(x...);
    uint8_t cmd[] = {0x1B, '$', static_cast<uint8_t>(pos & 0xFF), static_cast<uint8_t>(pos >> 8)};
    this->parent_->write_array(cmd, 4);
  }
};

template<typename... Ts>
class M5StackPrinterPrintTestPageAction : public Action<Ts...>, public Parented<M5StackPrinterDisplay> {
 public:
  void play(const Ts &...x) override { this->parent_->print_test_page(); }
};

template<typename... Ts>
class M5StackPrinterRunDemoAction : public Action<Ts...>, public Parented<M5StackPrinterDisplay> {
 public:
  TEMPLATABLE_VALUE(bool, show_qr_code)
  TEMPLATABLE_VALUE(bool, show_barcode)
  TEMPLATABLE_VALUE(bool, show_text_styles)
  TEMPLATABLE_VALUE(bool, show_inverse)
  TEMPLATABLE_VALUE(bool, show_rotation)

  void play(const Ts &...x) override {
    bool qr = this->show_qr_code_.value(x...);
    bool bc = this->show_barcode_.value(x...);
    bool txt = this->show_text_styles_.value(x...);
    bool inv = this->show_inverse_.value(x...);
    bool rot = this->show_rotation_.value(x...);

    // If no specific flags set, run full demo
    if (!qr && !bc && !txt && !inv && !rot) {
      this->parent_->run_demo(true, true, true, true, true);
    } else {
      this->parent_->run_demo(qr, bc, txt, inv, rot);
    }
  }
};

}  // namespace m5stack_printer
}  // namespace esphome
