#include "thermal_printer.h"

#include <cinttypes>

/* Commands (* means done)
03 - horizontal tab
04 - set tab locations
09 - set row spacing
*10 - alignment
11 - set double width mode (also settable through 16)
12 - cancel double width mode (also settable through 16)
16 - set print mode (font/inverse/upsidedown/bold/doubleheight/doublewidth/strikethrough)
17 - font size (also settable through 16)
*18 - inverse (also settable through 16)
*19 - 90 degree rotation
22 - bold (also settable through 16)
24 - upside down (also settable through 16)
*25 - underline (can't be combined with 90 degree rotation. if both set, underline is ignored)
36 - print bitmap
46 - print barcode
?? - print qr code
*/
namespace esphome {
namespace thermal_printer {

static const char *const TAG = "thermal_printer";

static const uint8_t ESC = 0x1B;
static const uint8_t GS = 0x1D;
static const uint8_t TAB = '\t';  // Horizontal tab

static const uint8_t INIT_PRINTER_CMD[] = {ESC, 0x40};
static const uint8_t WAKEUP_CMD[] = {ESC, 0x38, 0, 0};
static const uint8_t SET_ALIGNMENT_CMD[] = {ESC, 0x61};
static const uint8_t SET_INVERSE_CMD[] = {GS, 0x42};
static const uint8_t SET_90_DEGREE_CMD[] = {ESC, 0x56};
static const uint8_t SET_UNDERLINE_CMD[] = {ESC, 0x2D};
static const uint8_t SET_UPDOWN_CMD[] = {ESC, 0x7B};
static const uint8_t BYTES_PER_LOOP = 120;

void ThermalPrinterDisplay::setup() {
  this->init_internal_(this->get_buffer_length_());

  this->set_timeout(500, [this]() {
    this->init_();
    this->ready_ = true;
  });
}

void ThermalPrinterDisplay::init_() {
  if (this->send_wakeup_) {
    this->write_byte(0xFF);
    delay(50);
    this->write_array(WAKEUP_CMD, sizeof(WAKEUP_CMD));
  }
  this->write_array(INIT_PRINTER_CMD, sizeof(INIT_PRINTER_CMD));
}

void ThermalPrinterDisplay::print_text(std::string text, std::string align, bool inverse, bool ninety_degree,
                                       uint8_t underline_weight, bool updown) {
  this->init_();

  ESP_LOGD("print_text", "text: %s", text.c_str());
  ESP_LOGD("print_text", "align: %s", align.c_str());
  ESP_LOGD("print_text", "inverse: %s", inverse ? "true" : "false");
  ESP_LOGD("print_text", "ninety_degree: %s", ninety_degree ? "true" : "false");
  ESP_LOGD("print_text", "underline_weight: %d", underline_weight);
  ESP_LOGD("print_text", "updown: %s", updown ? "true" : "false");

  // alignment
  //  Convert the alignment string to uppercase
  align = this->toUpperCase(align)[0];
  if (align == "C") {
    this->write_array(SET_ALIGNMENT_CMD, sizeof(SET_ALIGNMENT_CMD));
    this->write_byte(0x01);  // Center
  } else if (align == "R") {
    this->write_array(SET_ALIGNMENT_CMD, sizeof(SET_ALIGNMENT_CMD));
    this->write_byte(0x02);  // Right
  } else if (align == "L") {
    this->write_array(SET_ALIGNMENT_CMD, sizeof(SET_ALIGNMENT_CMD));
    this->write_byte(0x00);  // Left
  } else {
    ESP_LOGW(TAG, "Invalid alignment: %s", align.c_str());
  }
  // inverse
  if (inverse) {
    this->write_array(SET_INVERSE_CMD, sizeof(SET_INVERSE_CMD));
    this->write_byte(0x01);  // Inverse
  } else {
    this->write_array(SET_INVERSE_CMD, sizeof(SET_INVERSE_CMD));
    this->write_byte(0x00);  // Normal
  }

  // 90 degree
  if (ninety_degree) {
    this->write_array(SET_90_DEGREE_CMD, sizeof(SET_90_DEGREE_CMD));
    this->write_byte(0x01);  // 90 degree
  } else {
    this->write_array(SET_90_DEGREE_CMD, sizeof(SET_90_DEGREE_CMD));
    this->write_byte(0x00);  // Normal
  }
  // underline
  underline_weight = clamp<uint8_t>(underline_weight, 0, 2);
  this->write_array(SET_UNDERLINE_CMD, sizeof(SET_UNDERLINE_CMD));
  this->write_byte(underline_weight);  // Underline

  // updown
  if (updown) {
    this->write_array(SET_UPDOWN_CMD, sizeof(SET_UPDOWN_CMD));
    this->write_byte(0x01);  // Updown
  } else {
    this->write_array(SET_UPDOWN_CMD, sizeof(SET_UPDOWN_CMD));
    this->write_byte(0x00);  // Normal
  }
  ESP_LOGD("print_text", "printing now!");
  this->write_str(text.c_str());
}

void ThermalPrinterDisplay::new_line(uint8_t lines) {
  for (uint8_t i = 0; i < lines; i++) {
    this->write_byte('\n');
  }
}
void ThermalPrinterDisplay::queue_data_(std::vector<uint8_t> data) {
  for (size_t i = 0; i < data.size(); i += BYTES_PER_LOOP) {
    std::vector<uint8_t> chunk(data.begin() + i, data.begin() + std::min(i + BYTES_PER_LOOP, data.size()));
    this->queue_.push(chunk);
  }
}
void ThermalPrinterDisplay::queue_data_(const uint8_t *data, size_t size) {
  for (size_t i = 0; i < size; i += BYTES_PER_LOOP) {
    size_t chunk_size = std::min(i + BYTES_PER_LOOP, size) - i;
    std::vector<uint8_t> chunk(data + i, data + i + chunk_size);
    this->queue_.push(chunk);
  }
}

void ThermalPrinterDisplay::loop() {
  if (this->queue_.empty()) {
    return;
  }

  std::vector<uint8_t> data = this->queue_.front();
  this->queue_.pop();
  this->write_array(data.data(), data.size());
}

static uint16_t count = 0;

void ThermalPrinterDisplay::update() {
  this->do_update_();
  this->write_to_device_();
  ESP_LOGD(TAG, "count: %d;", count);
  count = 0;
}

void ThermalPrinterDisplay::write_to_device_() {
  if (this->buffer_ == nullptr) {
    return;
  }

  uint8_t header[] = {0x1D, 0x76, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00};

  uint16_t width = this->get_width() / 8;
  uint16_t height = this->get_height();

  header[3] = 0;  // Mode
  header[4] = width & 0xFF;
  header[5] = (width >> 8) & 0xFF;
  header[6] = height & 0xFF;
  header[7] = (height >> 8) & 0xFF;

  this->queue_data_(header, sizeof(header));
  this->queue_data_(this->buffer_, this->get_buffer_length_());
}

void ThermalPrinterDisplay::draw_absolute_pixel_internal(int x, int y, Color color) {
  if (this->buffer_ == nullptr) {
    ESP_LOGW(TAG, "Buffer is null");
    return;
  }
  if (x < 0 || y < 0 || x >= this->get_width_internal() || y >= this->get_height_internal()) {
    ESP_LOGW(TAG, "Invalid pixel: x=%d, y=%d", x, y);
    return;
  }
  uint8_t width = this->get_width_internal() / 8;
  uint16_t index = x / 8 + y * width;
  uint8_t bit = x % 8;
  if (color.is_on()) {
    this->buffer_[index] |= 1 << (7 - bit);
  } else {
    this->buffer_[index] &= ~(1 << (7 - bit));
  }
  count++;
}

// Method to convert a std::string to uppercase
std::string ThermalPrinterDisplay::toUpperCase(const std::string &input) {
  std::string result = input;  // Make a copy of the input string
  std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::toupper(c); });
  return result;  // Return the transformed string
}

}  // namespace thermal_printer
}  // namespace esphome
