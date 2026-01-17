#include "m5stack_printer.h"

#include <cinttypes>

namespace esphome {
namespace m5stack_printer {

static const char *const TAG = "m5stack_printer";

static const uint8_t ESC = 0x1B;
static const uint8_t GS = 0x1D;

static const uint8_t INIT_PRINTER_CMD[] = {ESC, 0x40};
static const uint8_t BAUD_RATE_9600_CMD[] = {ESC, '#', '#', 'S', 'B', 'D', 'R', 0x80, 0x25, 0x00, 0x00};
static const uint8_t BAUD_RATE_115200_CMD[] = {ESC, '#', '#', 'S', 'B', 'D', 'R', 0x00, 0xC2, 0x01, 0x00};

// Font size commands based on GS ! n format from datasheet
static const uint8_t FONT_SIZE_CMD[] = {GS, '!'};
// Reset to default print mode using ESC @ (initialization)
static const uint8_t PRINT_MODE_RESET_CMD[] = {ESC, '@'};

// QR Code commands based on datasheet GS (k pL pH cn fn m format
// Set QR code module size (3x3 points) - GS ( k 03 00 31 43 03
static const uint8_t QR_CODE_MODULE_SIZE_CMD[] = {GS, 0x28, 0x6B, 0x03, 0x00, 0x31, 0x43, 0x03};
// Set QR code error correction level L - GS ( k 03 00 31 45 30
static const uint8_t QR_CODE_ERROR_CORRECTION_CMD[] = {GS, 0x28, 0x6B, 0x03, 0x00, 0x31, 0x45, 0x30};
// Store QR code data prefix - GS ( k pL pH 31 50 30 [data]
static const uint8_t QR_CODE_STORE_PREFIX[] = {GS, 0x28, 0x6B}; // pL pH will be calculated
// Print QR code - GS ( k 03 00 31 51 30
static const uint8_t QR_CODE_PRINT_CMD[] = {GS, 0x28, 0x6B, 0x03, 0x00, 0x31, 0x51, 0x30};

// Barcode commands based on datasheet - GS k m format
static const uint8_t BARCODE_HEIGHT_CMD[] = {GS, 'h'}; // Set barcode height
static const uint8_t BARCODE_WIDTH_CMD[] = {GS, 'w'}; // Set barcode width  
static const uint8_t BARCODE_HRI_POS_CMD[] = {GS, 'H'}; // Set HRI character position
static const uint8_t BARCODE_PRINT_CMD[] = {GS, 'k'}; // Print barcode command
// Default barcode settings
static const uint8_t DEFAULT_BARCODE_HEIGHT = 162; // Default height in dots
static const uint8_t DEFAULT_BARCODE_WIDTH = 3; // Default width multiplier
static const uint8_t DEFAULT_HRI_POSITION = 0; // No HRI characters

static const uint8_t BYTES_PER_LOOP = 120;

void M5StackPrinterDisplay::setup() {
  this->init_internal_(this->get_buffer_length_());

  this->init_();
  // this->write_array(BAUD_RATE_115200_CMD, sizeof(BAUD_RATE_115200_CMD));
  // delay(10);
  // this->parent_->set_baud_rate(115200);
  // this->parent_->load_settings();
  // delay(10);

  // this->write_array(INIT_PRINTER_CMD, sizeof(INIT_PRINTER_CMD));
}

void M5StackPrinterDisplay::init_() { 
  // Initialize printer according to datasheet ESC @ command
  this->write_array(INIT_PRINTER_CMD, sizeof(INIT_PRINTER_CMD));
  // Small delay to ensure initialization is complete
  delay(50);
}

void M5StackPrinterDisplay::print_text(std::string text, uint8_t font_size) {
  this->init_();
  
  // Font size range validation according to datasheet
  font_size = clamp<uint8_t>(font_size, 0, 7);
  
  // Set font size using GS ! n command format
  // Bits 0-3: character width (0=normal, 1=double width, etc.)
  // Bits 4-7: character height (0=normal, 1=double height, etc.)
  this->write_array(FONT_SIZE_CMD, sizeof(FONT_SIZE_CMD));
  this->write_byte(font_size | (font_size << 4));

  // Print the text
  this->write_str(text.c_str());

  // Reset to default print mode
  this->write_array(PRINT_MODE_RESET_CMD, sizeof(PRINT_MODE_RESET_CMD));
}

void M5StackPrinterDisplay::new_line(uint8_t lines) {
  for (uint8_t i = 0; i < lines; i++) {
    this->write_byte('\n');
  }
}

void M5StackPrinterDisplay::print_qrcode(std::string data) {
  this->init_();

  // QR code sequence based on datasheet:
  // 1. Set QR code module size (3x3 dots)
  this->write_array(QR_CODE_MODULE_SIZE_CMD, sizeof(QR_CODE_MODULE_SIZE_CMD));
  
  // 2. Set error correction level L
  this->write_array(QR_CODE_ERROR_CORRECTION_CMD, sizeof(QR_CODE_ERROR_CORRECTION_CMD));
  
  // 3. Store QR code data - GS ( k pL pH 31 50 30 [data]
  size_t data_len = data.length() + 3; // +3 for (31 50 30) prefix
  uint8_t len_low = data_len & 0xFF;
  uint8_t len_high = (data_len >> 8) & 0xFF;
  
  this->write_array(QR_CODE_STORE_PREFIX, sizeof(QR_CODE_STORE_PREFIX));
  this->write_byte(len_low);
  this->write_byte(len_high);
  this->write_byte(0x31); // cn
  this->write_byte(0x50); // fn (Store data function)
  this->write_byte(0x30); // m (model)
  this->write_str(data.c_str());
  
  // 4. Print the QR code
  this->write_array(QR_CODE_PRINT_CMD, sizeof(QR_CODE_PRINT_CMD));
}

void M5StackPrinterDisplay::print_barcode(std::string barcode, BarcodeType type) {
  this->init_();
  
  // Validate barcode data length based on type
  size_t data_len = barcode.length();
  bool valid_length = false;
  
  switch (type) {
    case UPC_A:
      valid_length = (data_len >= 11 && data_len <= 12);
      break;
    case UPC_E:
      valid_length = (data_len >= 11 && data_len <= 12);
      break;
    case EAN13:
      valid_length = (data_len >= 12 && data_len <= 13);
      break;
    case EAN8:
      valid_length = (data_len >= 7 && data_len <= 8);
      break;
    case CODE39:
    case CODE93:
    case CODE128:
    case CODABAR:
      valid_length = (data_len >= 1 && data_len <= 255);
      break;
    case ITF:
      valid_length = (data_len >= 1 && data_len <= 255 && (data_len % 2) == 0);
      break;
    default:
      ESP_LOGE(TAG, "Invalid barcode type: %d", type);
      return;
  }
  
  if (!valid_length) {
    ESP_LOGE(TAG, "Invalid barcode data length %d for type %d", data_len, type);
    return;
  }

  // Set barcode height (default 162 dots according to datasheet)
  this->write_array(BARCODE_HEIGHT_CMD, sizeof(BARCODE_HEIGHT_CMD));
  this->write_byte(DEFAULT_BARCODE_HEIGHT);
  
  // Set barcode width (default 3 according to datasheet)
  this->write_array(BARCODE_WIDTH_CMD, sizeof(BARCODE_WIDTH_CMD));
  this->write_byte(DEFAULT_BARCODE_WIDTH);
  
  // Set HRI character position (default: no HRI)
  this->write_array(BARCODE_HRI_POS_CMD, sizeof(BARCODE_HRI_POS_CMD));
  this->write_byte(DEFAULT_HRI_POSITION);

  // Print barcode using GS k m n d1...dn format (format 2 from datasheet)
  this->write_array(BARCODE_PRINT_CMD, sizeof(BARCODE_PRINT_CMD));
  this->write_byte(type + 65); // Use format 2 (m = 65-73) for length prefix
  this->write_byte(data_len); // Number of data bytes
  this->write_str(barcode.c_str());
}

void M5StackPrinterDisplay::queue_data_(std::vector<uint8_t> data) {
  for (size_t i = 0; i < data.size(); i += BYTES_PER_LOOP) {
    std::vector<uint8_t> chunk(data.begin() + i, data.begin() + std::min(i + BYTES_PER_LOOP, data.size()));
    this->queue_.push(chunk);
  }
}
void M5StackPrinterDisplay::queue_data_(const uint8_t *data, size_t size) {
  for (size_t i = 0; i < size; i += BYTES_PER_LOOP) {
    size_t chunk_size = std::min(i + BYTES_PER_LOOP, size) - i;
    std::vector<uint8_t> chunk(data + i, data + i + chunk_size);
    this->queue_.push(chunk);
  }
}

void M5StackPrinterDisplay::loop() {
  if (this->queue_.empty()) {
    return;
  }

  std::vector<uint8_t> data = this->queue_.front();
  this->queue_.pop();
  this->write_array(data.data(), data.size());
}

static uint16_t count = 0;

void M5StackPrinterDisplay::update() {
  this->do_update_();
  this->write_to_device_();
  ESP_LOGD(TAG, "count: %d;", count);
  count = 0;
}

void M5StackPrinterDisplay::write_to_device_() {
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

void M5StackPrinterDisplay::draw_absolute_pixel_internal(int x, int y, Color color) {
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

}  // namespace m5stack_printer
}  // namespace esphome
