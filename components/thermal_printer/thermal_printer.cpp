#include "thermal_printer.h"

#include <cinttypes>

namespace esphome {
namespace thermal_printer {

static const char *const TAG = "thermal_printer";

static const uint8_t ESC = 0x1B;
static const uint8_t GS = 0x1D;
static const uint8_t TAB = '\t';  // Horizontal tab

static const uint8_t INIT_PRINTER_CMD[] = {ESC, 0x40};
static const uint8_t WAKEUP_CMD[] = {ESC, 0x38, 0, 0};
static const uint8_t BAUD_RATE_9600_CMD[] = {ESC, '#', '#', 'S', 'B', 'D', 'R', 0x80, 0x25, 0x00, 0x00};
static const uint8_t BAUD_RATE_115200_CMD[] = {ESC, '#', '#', 'S', 'B', 'D', 'R', 0x00, 0xC2, 0x01, 0x00};

static const uint8_t SET_TABS_CMD[] = {ESC, 0x44};       //'D'
static const uint8_t CLEAR_TABS_CMD[] = {ESC, 0x44, 0};  //'D'
static const uint8_t FONT_SIZE_CMD[] = {GS, 0x21};       //'!'
static const uint8_t FONT_SIZE_RESET_CMD[] = {ESC, 0x14};
static const uint8_t UNDERLINE_RESET_CMD[] = {ESC, 0x2D, 0x00};

static const uint8_t QR_CODE_SET_CMD[] = {GS, 0x28, 0x6B, 0x00, 0x00, 0x31, 0x50, 0x30};
static const uint8_t QR_CODE_PRINT_CMD[] = {GS, 0x28, 0x6B, 0x03, 0x00, 0x31, 0x51, 0x30, 0x00};

static const uint8_t BARCODE_ENABLE_CMD[] = {GS, 0x45, 0x43, 0x01};
static const uint8_t BARCODE_DISABLE_CMD[] = {GS, 0x45, 0x43, 0x00};
static const uint8_t BARCODE_PRINT_CMD[] = {GS, 0x6B};

static const uint8_t INVERSE_ON_CMD[] = {GS, 'B', 0x01};
static const uint8_t INVERSE_OFF_CMD[] = {GS, 'B', 0x00};

static const uint8_t UPDOWN_ON_CMD[] = {ESC, '{', 0x01};
static const uint8_t UPDOWN_OFF_CMD[] = {ESC, '{', 0x00};

static const uint8_t NINETY_DEGREES_ROTATION_ON_CMD[] = {ESC, 0x56, 0x01};
static const uint8_t NINETY_DEGREES_ROTATION_OFF_CMD[] = {ESC, 0x56, 0x00};

// === Character commands ===
#define FONT_MASK (1 << 0)           //!< Select character font A or B
#define BOLD_MASK (1 << 3)           //!< Turn on/off bold printing mode
#define DOUBLE_HEIGHT_MASK (1 << 4)  //!< Turn on/off double-height printing mode
#define DOUBLE_WIDTH_MASK (1 << 5)   //!< Turn on/off double-width printing mode
#define STRIKE_MASK (1 << 6)         //!< Turn on/off deleteline mode

static const uint8_t BYTES_PER_LOOP = 120;

void ThermalPrinterDisplay::setup() {
  this->init_internal_(this->get_buffer_length_());

  this->set_timeout(500, [this]() {
    this->init_();
    this->ready_ = true;
  });
  // this->write_array(BAUD_RATE_115200_CMD, sizeof(BAUD_RATE_115200_CMD));
  // delay(10);
  // this->parent_->set_baud_rate(115200);
  // this->parent_->load_settings();
  // delay(10);

  // this->write_array(INIT_PRINTER_CMD, sizeof(INIT_PRINTER_CMD));
}

void ThermalPrinterDisplay::init_() {
  if (this->send_wakeup_) {
    this->write_byte(0xFF);
    delay(50);
    this->write_array(WAKEUP_CMD, sizeof(WAKEUP_CMD));
  }
  this->write_array(INIT_PRINTER_CMD, sizeof(INIT_PRINTER_CMD));

  this->reset();
}

void ThermalPrinterDisplay::reset() {
  printMode = 0;
  /*charHeight = 24;
  maxColumn = 32;*/
}

// expects a list of tabs in sequential order, for example: {5, 10, 15, 20, 25}
void ThermalPrinterDisplay::setTabs(std::vector<uint8_t> tab) {
  this->init_();
  this->tabsAmount = 0;
  this->tabs[this->tabsAmount] = 0;

  this->write_array(SET_TABS_CMD, sizeof(SET_TABS_CMD));  // from datasheet - set tab stops
  for (uint8_t i = 0; i < sizeof(tab); i++) {
    if (tab[i] < this->widthMax && tab[i] > (tabs[this->tabsAmount] / this->charWidth) && this->tabsAmount < 32) {
      this->write_byte(tab[i]);
      this->tabs[this->tabsAmount++] = tab[i] * this->charWidth;
    }
  }
  this->write_byte(0);  // from datasheet - end of list
  this->cursor = 0;
}
void ThermalPrinterDisplay::tab() {
  this->init_();
  for (uint8_t i = 0; i < this->tabsAmount; i++) {
    if (this->tabs[i] > this->cursor) {
      cursor = this->tabs[i];
      break;
    }
  }
  this->write_byte(TAB);
  if ((this->widthInDots - this->cursor) < this->charWidth) {
    this->cursor = 0;  // printer go newline
  }
}

void ThermalPrinterDisplay::clearTabs() {
  this->init_();
  this->write_array(CLEAR_TABS_CMD, sizeof(CLEAR_TABS_CMD));  // from datasheet - set tab stops
  this->tabs[0] = 0;
}
// default / min is 24
void ThermalPrinterDisplay::setLineHeight(uint8_t height) {
  this->init_();
  if (height < 0) {
    height = 0;
  }
  if (height > 255) {
    height = 255;
  }
  static const uint8_t lineHeightCMD[] = {ESC, 0x33, height};
  ESP_LOGD("setLineHeight", "height: %d", height);
  this->write_array(lineHeightCMD, sizeof(lineHeightCMD));
}

// default is L
/*void ThermalPrinterDisplay::justify(std::string value) {
  this->init_();
  uint8_t set = 0;
  if (value[0] == 'C') {
    set = 1;
  } else if (value[0] == 'R') {
    set = 2;
  }
  // ESP_LOGD("justify", "value: %s", value.c_str());
  // ESP_LOGD("justify", "value[0]: %s", value[0]);
  ESP_LOGD("justify", "set: %d", set);
  // static const uint8_t justifyCMD[] = {ESC, 'a', set};
  // this->write_array(justifyCMD, sizeof(justifyCMD));
  this->write_byte(ESC);
  this->write_byte('a');
  this->write_byte(set);
}*/

/*void ThermalPrinterDisplay::print_text(std::string text, std::string font, bool inverse, bool updown, bool bold,
                                       bool double_height, bool double_width, bool strike, bool ninety_degrees,
                                       uint8_t underline_weight, std::string justify) {
  this->print_text(text, -1, font, inverse, updown, bold, double_height, double_width, strike, ninety_degrees,
                   underline_weight, justify);
}*/
void ThermalPrinterDisplay::print_text(std::string text, uint8_t font_size, std::string font, bool inverse, bool updown,
                                       bool bold, bool double_height, bool double_width, bool strike,
                                       bool ninety_degrees, uint8_t underline_weight, std::string justify) {
  this->init_();

  ESP_LOGD("print_text", "text: %s", text.c_str());
  ESP_LOGD("print_text", "font_size: %d", font_size);
  ESP_LOGD("print_text", "font_size_factor: %f", this->font_size_factor_);
  ESP_LOGD("print_text", "font: %s", font.c_str());
  ESP_LOGD("print_text", "inverse: %s", inverse ? "true" : "false");
  ESP_LOGD("print_text", "updown: %s", updown ? "true" : "false");
  ESP_LOGD("print_text", "bold: %s", bold ? "true" : "false");
  ESP_LOGD("print_text", "double_height: %s", double_height ? "true" : "false");
  ESP_LOGD("print_text", "double_width: %s", double_width ? "true" : "false");
  ESP_LOGD("print_text", "strike: %s", strike ? "true" : "false");
  ESP_LOGD("print_text", "ninety_degrees: %s", ninety_degrees ? "true" : "false");
  ESP_LOGD("print_text", "underline_weight: %d", underline_weight);
  ESP_LOGD("print_text", "justify: %s", justify.c_str());

  // doesn't work on my printer, but it's in the datasheet
  font = this->toUpperCase(font);
  if (font == "B") {
    this->setPrintMode(FONT_MASK);
  } else {
    this->unsetPrintMode(FONT_MASK);
  }
  if (inverse) {
    this->write_array(INVERSE_ON_CMD, sizeof(INVERSE_ON_CMD));
  }
  if (updown) {
    this->write_array(UPDOWN_ON_CMD, sizeof(UPDOWN_ON_CMD));
  }
  // doesn't work on my printer, but it's in the datasheet
  if (bold) {
    ESP_LOGD("print_text", "turning on bold");
    this->setPrintMode(BOLD_MASK);
  }
  if (double_height) {
    ESP_LOGD("print_text", "turning on double_height");
    this->setPrintMode(DOUBLE_HEIGHT_MASK);
  }
  if (double_width) {
    ESP_LOGD("print_text", "turning on double_width");
    this->setPrintMode(DOUBLE_WIDTH_MASK);
  }
  // doesn't work on my printer, but it's in the datasheet
  if (strike) {
    ESP_LOGD("print_text", "turning on strike");
    this->setPrintMode(STRIKE_MASK);
  }
  if (ninety_degrees) {
    ESP_LOGD("print_text", "turning on ninety_degrees");
    this->write_array(NINETY_DEGREES_ROTATION_ON_CMD, sizeof(NINETY_DEGREES_ROTATION_ON_CMD));
  }

  // underline
  underline_weight = clamp<uint8_t>(underline_weight, 0, 2);
  ESP_LOGD("print_text", "setting underline");
  this->write_byte(ESC);
  this->write_byte(0x2D);
  this->write_byte(underline_weight);

  // justify
  // make uppercase
  justify = this->toUpperCase(justify);
  uint8_t justify_set = 0;
  if (justify[0] == 'C') {
    justify_set = 1;
  } else if (justify[0] == 'R') {
    justify_set = 2;
  }
  ESP_LOGD("print_text", "setting justify: %d", justify_set);
  this->write_byte(ESC);
  this->write_byte(0x61);  //'a'
  this->write_byte(justify_set);

  // font_size
  // if >=0, provide it to the printer.
  // if ==255 ignore it (for overloading)
  if (font_size != 255) {
    font_size = clamp<uint8_t>(font_size, 0, 7);
    if (font_size > 0) {
      font_size = font_size * this->font_size_factor_;
      this->write_array(FONT_SIZE_CMD, sizeof(FONT_SIZE_CMD));
      this->write_byte(font_size | (font_size << 4));
    }
  }

  ESP_LOGD("print_text", "printing now!");
  this->write_str(text.c_str());

  // turn settings off if they were on
  if (inverse) {
    this->write_array(INVERSE_OFF_CMD, sizeof(INVERSE_OFF_CMD));
  }
  if (updown) {
    this->write_array(UPDOWN_OFF_CMD, sizeof(UPDOWN_OFF_CMD));
  }
  if (bold) {
    ESP_LOGD("print_text", "turning off bold");
    this->unsetPrintMode(BOLD_MASK);
  }
  if (double_height) {
    ESP_LOGD("print_text", "turning off double_height");
    this->unsetPrintMode(DOUBLE_HEIGHT_MASK);
  }
  if (strike) {
    ESP_LOGD("print_text", "turning off strike");
    this->unsetPrintMode(STRIKE_MASK);
  }
  if (double_width) {
    ESP_LOGD("print_text", "turning off double_width");
    this->unsetPrintMode(DOUBLE_WIDTH_MASK);
  }
  if (ninety_degrees) {
    ESP_LOGD("print_text", "turning off ninety_degrees");
    this->write_array(NINETY_DEGREES_ROTATION_OFF_CMD, sizeof(NINETY_DEGREES_ROTATION_OFF_CMD));
  }
  if (justify_set > 0) {
    ESP_LOGD("print_text", "turning off justify");
    this->write_byte(ESC);
    this->write_byte(0x61);  //'a'
    this->write_byte(0);
  }
  // underline
  this->write_array(UNDERLINE_RESET_CMD, sizeof(UNDERLINE_RESET_CMD));
  // reset font_size
  this->write_array(FONT_SIZE_RESET_CMD, sizeof(FONT_SIZE_RESET_CMD));
}

void ThermalPrinterDisplay::new_line(uint8_t lines) {
  for (uint8_t i = 0; i < lines; i++) {
    this->write_byte('\n');
  }
}

void ThermalPrinterDisplay::print_qrcode(std::string data) {
  this->init_();

  size_t len;
  uint8_t len_low, len_high;
  len = data.length() + 3;
  len_low = len & 0xFF;
  len_high = len >> 8;

  uint8_t qr_code_cmd[sizeof(QR_CODE_SET_CMD)];
  memcpy(qr_code_cmd, QR_CODE_SET_CMD, sizeof(QR_CODE_SET_CMD));
  qr_code_cmd[3] = len_low;
  qr_code_cmd[4] = len_high;
  this->write_array(qr_code_cmd, sizeof(qr_code_cmd));
  this->write_str(data.c_str());
  this->write_byte(0x00);

  this->write_array(QR_CODE_PRINT_CMD, sizeof(QR_CODE_PRINT_CMD));
}

BarcodeType stringToBarcodeType(const std::string &typeStr) {
  if (typeStr == "UPC_A") {
    return BarcodeType::UPC_A;
  } else if (typeStr == "UPC_E") {
    return BarcodeType::UPC_E;
  } else if (typeStr == "EAN13") {
    return BarcodeType::EAN13;
  } else if (typeStr == "EAN8") {
    return BarcodeType::EAN8;
  } else if (typeStr == "CODE39") {
    return BarcodeType::CODE39;
  } else if (typeStr == "ITF") {
    return BarcodeType::ITF;
  } else if (typeStr == "CODABAR") {
    return BarcodeType::CODABAR;
  } else if (typeStr == "CODE93") {
    return BarcodeType::CODE93;
  } else if (typeStr == "CODE128") {
    return BarcodeType::CODE128;
  }
  return BarcodeType::UNKNOWN;
}
void ThermalPrinterDisplay::print_barcode(std::string barcode, std::string type) {
  this->init_();

  this->write_array(BARCODE_ENABLE_CMD, sizeof(BARCODE_ENABLE_CMD));

  this->write_array(BARCODE_PRINT_CMD, sizeof(BARCODE_PRINT_CMD));
  BarcodeType barcode_type = stringToBarcodeType(type);
  if (barcode_type == BarcodeType::UNKNOWN) {
    ESP_LOGE("Unable to print barcode", "Unknown barcode type: %s", type.c_str());
  } else {
    this->write_byte(barcode_type);
    this->write_byte(barcode.length());
    this->write_str(barcode.c_str());
    this->write_byte(0x00);
  }
  this->write_array(BARCODE_DISABLE_CMD, sizeof(BARCODE_DISABLE_CMD));
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

void ThermalPrinterDisplay::writePrintMode() {
  // ESP_LOGD("writePrintMode", "writing printMode: %d", printMode);
  static const uint8_t printModeCMD[] = {ESC, 0x21, printMode};
  this->write_array(printModeCMD, sizeof(printModeCMD));
}

void ThermalPrinterDisplay::adjustCharValues(uint8_t printMode) {
  uint8_t charWidth;
  if (printMode & FONT_MASK) {
    // FontB
    charHeight = 17;
    charWidth = 9;
  } else {
    // FontA
    charHeight = 24;
    charWidth = 12;
  }
  // Double Width Mode
  if (printMode & DOUBLE_WIDTH_MASK) {
    maxColumn /= 2;
    charWidth *= 2;
  }
  // Double Height Mode
  if (printMode & DOUBLE_HEIGHT_MASK) {
    charHeight *= 2;
  }
  maxColumn = (384 / charWidth);
}
void ThermalPrinterDisplay::unsetPrintMode(uint8_t mask) {
  // ESP_LOGD("unsetPrintMode", "mask: %d", mask);
  // ESP_LOGD("unsetPrintMode", "printMode before: %d", printMode);
  printMode &= ~mask;
  // ESP_LOGD("unsetPrintMode", "printMode after: %d", printMode);
  this->writePrintMode();
  this->adjustCharValues(printMode);
}

void ThermalPrinterDisplay::setPrintMode(uint8_t mask) {
  // ESP_LOGD("setPrintMode", "mask: %d", mask);
  // ESP_LOGD("setPrintMode", "printMode before: %d", printMode);
  printMode |= mask;
  // ESP_LOGD("setPrintMode", "printMode after: %d", printMode);
  this->writePrintMode();
  this->adjustCharValues(printMode);
}

// Method to convert a std::string to uppercase
std::string ThermalPrinterDisplay::toUpperCase(const std::string &input) {
  std::string result = input;  // Make a copy of the input string
  std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::toupper(c); });
  return result;  // Return the transformed string
}

}  // namespace thermal_printer
}  // namespace esphome
