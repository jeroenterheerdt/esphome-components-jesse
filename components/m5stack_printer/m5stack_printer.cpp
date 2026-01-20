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

// Paper cutting commands based on datasheet
static const uint8_t PAPER_CUT_FULL_CMD[] = {ESC, 'i'}; // Full cut - ESC i
static const uint8_t PAPER_CUT_PARTIAL_CMD[] = {GS, 'V', 1}; // Partial cut - GS V 1
static const uint8_t PAPER_CUT_FEED_PREFIX[] = {GS, 'V'}; // GS V m [n] for cut with feed

// Text formatting commands
static const uint8_t TEXT_ALIGN_CMD[] = {ESC, 'a'}; // ESC a n - text alignment
static const uint8_t LINE_SPACING_CMD[] = {ESC, '3'}; // ESC 3 n - line spacing
static const uint8_t DEFAULT_LINE_SPACING_CMD[] = {ESC, '2'}; // ESC 2 - default spacing
static const uint8_t PRINT_DENSITY_CMD[] = {0x12, '#'}; // DC2 # n - print density

// Text style commands
static const uint8_t BOLD_ON_CMD[] = {ESC, 'E', 1}; // ESC E 1 - bold on
static const uint8_t BOLD_OFF_CMD[] = {ESC, 'E', 0}; // ESC E 0 - bold off
static const uint8_t UNDERLINE_CMD[] = {ESC, '-'}; // ESC - n - underline
static const uint8_t DOUBLE_WIDTH_ON_CMD[] = {ESC, 0x0E}; // ESC SO - double width on
static const uint8_t DOUBLE_WIDTH_OFF_CMD[] = {ESC, 0x14}; // ESC DC4 - double width off
static const uint8_t UPSIDE_DOWN_CMD[] = {ESC, '{'}; // ESC { n - upside down mode

// Printer control commands
static const uint8_t TEST_PAGE_CMD[] = {0x12, 'T'}; // DC2 T - print test page
static const uint8_t STATUS_REQUEST_CMD[] = {ESC, 'v', 0}; // ESC v 0 - request status
static const uint8_t SLEEP_MODE_CMD[] = {ESC, '8'}; // ESC 8 n1 n2 - sleep mode

// Paper cutting commands based on datasheet
static const uint8_t PAPER_CUT_FULL_CMD[] = {ESC, 'i'}; // Full cut - ESC i
static const uint8_t PAPER_CUT_PARTIAL_CMD[] = {GS, 'V', 1}; // Partial cut - GS V 1
static const uint8_t PAPER_CUT_FEED_PREFIX[] = {GS, 'V'}; // GS V m [n] for cut with feed

// Text formatting commands
static const uint8_t TEXT_ALIGN_CMD[] = {ESC, 'a'}; // ESC a n - text alignment
static const uint8_t LINE_SPACING_CMD[] = {ESC, '3'}; // ESC 3 n - line spacing
static const uint8_t DEFAULT_LINE_SPACING_CMD[] = {ESC, '2'}; // ESC 2 - default spacing
static const uint8_t PRINT_DENSITY_CMD[] = {0x12, '#'}; // DC2 # n - print density

// Text style commands
static const uint8_t BOLD_ON_CMD[] = {ESC, 'E', 1}; // ESC E 1 - bold on
static const uint8_t BOLD_OFF_CMD[] = {ESC, 'E', 0}; // ESC E 0 - bold off
static const uint8_t UNDERLINE_CMD[] = {ESC, '-'}; // ESC - n - underline
static const uint8_t DOUBLE_WIDTH_ON_CMD[] = {ESC, 0x0E}; // ESC SO - double width on
static const uint8_t DOUBLE_WIDTH_OFF_CMD[] = {ESC, 0x14}; // ESC DC4 - double width off
static const uint8_t UPSIDE_DOWN_CMD[] = {ESC, '{'}; // ESC { n - upside down mode

// Printer control commands
static const uint8_t TEST_PAGE_CMD[] = {0x12, 'T'}; // DC2 T - print test page
static const uint8_t STATUS_REQUEST_CMD[] = {ESC, 'v', 0}; // ESC v 0 - request status
static const uint8_t SLEEP_MODE_CMD[] = {ESC, '8'}; // ESC 8 n1 n2 - sleep mode

// Additional text formatting commands  
static const uint8_t ROTATE_90_CMD[] = {ESC, 'V'}; // ESC V n - 90 degree rotation
static const uint8_t INVERSE_PRINT_CMD[] = {GS, 'B'}; // GS B n - white/black inverse
static const uint8_t CHINESE_MODE_ON_CMD[] = {0x1C, 0x26}; // FS & - select Chinese/Japanese mode
static const uint8_t CHINESE_MODE_OFF_CMD[] = {0x1C, 0x2E}; // FS . - cancel Chinese/Japanese mode

static const uint8_t BYTES_PER_LOOP = 120;

void M5StackPrinterDisplay::setup() {
  ESP_LOGD(TAG, "Setting up M5Stack Printer Display");
  
  this->init_internal_(this->get_buffer_length_());
  ESP_LOGD(TAG, "Buffer length: %d bytes", this->get_buffer_length_());
  
  // Initialize printer with proper sequence
  this->init_();
  ESP_LOGD(TAG, "M5Stack Printer initialized successfully");
  
  // Note: Commented baud rate change - keep default 9600 baud for stability
  // Some printer modules don't handle baud rate changes reliably
  // this->write_array(BAUD_RATE_115200_CMD, sizeof(BAUD_RATE_115200_CMD));
  // delay(10);
  // this->parent_->set_baud_rate(115200);
  // this->parent_->load_settings();
  // delay(10);
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
  if (lines == 0) {
    ESP_LOGW(TAG, "new_line called with 0 lines, ignoring");
    return;
  }
  
  if (lines > 10) {
    ESP_LOGW(TAG, "new_line called with %d lines (>10), clamping to 10", lines);
    lines = 10;
  }
  
  ESP_LOGD(TAG, "Adding %d new lines", lines);
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

void M5StackPrinterDisplay::cut_paper() {
  ESP_LOGD(TAG, "Cutting paper (full cut)");
  this->write_array(PAPER_CUT_FULL_CMD, sizeof(PAPER_CUT_FULL_CMD));
}

void M5StackPrinterDisplay::cut_paper(uint8_t mode, uint8_t feed_lines) {
  ESP_LOGD(TAG, "Cutting paper with mode %d, feed lines %d", mode, feed_lines);
  
  if (mode == 0) {
    // Full cut
    this->write_array(PAPER_CUT_FULL_CMD, sizeof(PAPER_CUT_FULL_CMD));
  } else if (mode == 1) {
    // Partial cut
    this->write_array(PAPER_CUT_PARTIAL_CMD, sizeof(PAPER_CUT_PARTIAL_CMD));
  } else if (mode == 66) {
    // Cut with feed - GS V 66 n
    this->write_array(PAPER_CUT_FEED_PREFIX, sizeof(PAPER_CUT_FEED_PREFIX));
    this->write_byte(66);
    this->write_byte(feed_lines);
  } else {
    ESP_LOGW(TAG, "Invalid cut mode: %d", mode);
  }
}

void M5StackPrinterDisplay::set_text_alignment(uint8_t alignment) {
  if (alignment > 2) {
    ESP_LOGW(TAG, "Invalid alignment %d, clamping to 2", alignment);
    alignment = 2;
  }
  
  ESP_LOGD(TAG, "Setting text alignment to %d (0=left, 1=center, 2=right)", alignment);
  this->write_array(TEXT_ALIGN_CMD, sizeof(TEXT_ALIGN_CMD));
  this->write_byte(alignment);
}

void M5StackPrinterDisplay::set_line_spacing(uint8_t spacing) {
  if (spacing == 0) {
    // Reset to default line spacing
    ESP_LOGD(TAG, "Resetting to default line spacing");
    this->write_array(DEFAULT_LINE_SPACING_CMD, sizeof(DEFAULT_LINE_SPACING_CMD));
  } else {
    ESP_LOGD(TAG, "Setting line spacing to %d dots", spacing);
    this->write_array(LINE_SPACING_CMD, sizeof(LINE_SPACING_CMD));
    this->write_byte(spacing);
  }
}

void M5StackPrinterDisplay::set_print_density(uint8_t density, uint8_t break_time) {
  // Validate ranges according to datasheet
  density = clamp<uint8_t>(density, 0, 31); // D4-D0 bits
  break_time = clamp<uint8_t>(break_time, 0, 7); // D7-D5 bits
  
  uint8_t density_byte = density | (break_time << 5);
  ESP_LOGD(TAG, "Setting print density %d, break time %d (combined byte: 0x%02X)", 
           density, break_time, density_byte);
  
  this->write_array(PRINT_DENSITY_CMD, sizeof(PRINT_DENSITY_CMD));
  this->write_byte(density_byte);
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
  
  ESP_LOGV(TAG, "Writing %d bytes from queue (%d items remaining)", 
           data.size(), this->queue_.size());
  this->write_array(data.data(), data.size());
}

static uint16_t count = 0;

void M5StackPrinterDisplay::update() {
  ESP_LOGD(TAG, "Display update triggered");
  this->do_update_();
  this->write_to_device_();
  ESP_LOGD(TAG, "Display update complete, pixels drawn: %d", count);
  count = 0;
}

void M5StackPrinterDisplay::write_to_device_() {
  if (this->buffer_ == nullptr) {
    ESP_LOGW(TAG, "Buffer is null, cannot write to device");
    return;
  }

  ESP_LOGD(TAG, "Writing raster image to device: %dx%d pixels", this->get_width(), this->get_height());
  
  // Raster bit-image command: GS v 0 m xL xH yL yH [data]
  // Based on datasheet: GS v 30 00 xL xH yL yH data
  uint8_t header[] = {0x1D, 0x76, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00};

  uint16_t width_bytes = this->get_width() / 8;  // Convert pixels to bytes
  uint16_t height = this->get_height();
  
  // Validate dimensions
  if (width_bytes == 0 || height == 0) {
    ESP_LOGW(TAG, "Invalid image dimensions: %d bytes width, %d height", width_bytes, height);
    return;
  }

  header[3] = 0;  // Mode (0 = normal)
  header[4] = width_bytes & 0xFF;        // xL
  header[5] = (width_bytes >> 8) & 0xFF; // xH  
  header[6] = height & 0xFF;             // yL
  header[7] = (height >> 8) & 0xFF;      // yH

  ESP_LOGD(TAG, "Raster header: mode=%d, width_bytes=%d, height=%d", 
           header[3], width_bytes, height);

  this->queue_data_(header, sizeof(header));
  this->queue_data_(this->buffer_, this->get_buffer_length_());
  
  ESP_LOGD(TAG, "Queued %d bytes of raster data", this->get_buffer_length_());
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

void M5StackPrinterDisplay::set_90_degree_rotation(bool enable) {
  ESP_LOGD(TAG, "Setting 90-degree rotation: %s", enable ? "enabled" : "disabled");
  
  this->write_array(ROTATE_90_CMD, sizeof(ROTATE_90_CMD));
  this->write_byte(enable ? 1 : 0);
}

void M5StackPrinterDisplay::set_inverse_printing(bool enable) {
  ESP_LOGD(TAG, "Setting inverse printing: %s", enable ? "enabled" : "disabled");
  
  this->write_array(INVERSE_PRINT_CMD, sizeof(INVERSE_PRINT_CMD));
  this->write_byte(enable ? 1 : 0);
}

void M5StackPrinterDisplay::set_chinese_mode(bool enable) {
  ESP_LOGD(TAG, "Setting Chinese/Japanese character mode: %s", enable ? "enabled" : "disabled");
  
  if (enable) {
    this->write_array(CHINESE_MODE_ON_CMD, sizeof(CHINESE_MODE_ON_CMD));
  } else {
    this->write_array(CHINESE_MODE_OFF_CMD, sizeof(CHINESE_MODE_OFF_CMD));
  }
}

void M5StackPrinterDisplay::run_demo(bool show_qr_code, bool show_barcode, 
                                     bool show_text_styles, bool show_inverse, bool show_rotation) {
  ESP_LOGD(TAG, "Running printer demo with flags: QR=%s, barcode=%s, text=%s, inverse=%s, rotation=%s",
           show_qr_code ? "yes" : "no", show_barcode ? "yes" : "no", 
           show_text_styles ? "yes" : "no", show_inverse ? "yes" : "no", show_rotation ? "yes" : "no");

  this->init_();
  
  // Reset printer to default state
  this->write_array(PRINT_MODE_RESET_CMD, sizeof(PRINT_MODE_RESET_CMD));
  this->set_text_alignment(1); // Center align for header
  this->new_line(1);
  
  // Header with title
  this->set_text_style(true, 0, true, false); // Bold, double width
  this->print_text("DON'T PANIC", 2);
  this->new_line(1);
  this->set_text_style(false, 1, false, false); // Normal, underlined
  this->print_text("M5Stack Printer Demo", 1);
  this->new_line(2);
  
  // Reset formatting and left align
  this->set_text_style(false, 0, false, false);
  this->set_text_alignment(0);
  
  // Display what features will be tested
  this->print_text("The Answer to the Ultimate");
  this->new_line(1);
  this->print_text("Question of Thermal Printing:");
  this->new_line(1);
  this->set_text_style(true, 0, false, false); // Bold
  this->print_text("42 functions tested below", 1);
  this->set_text_style(false, 0, false, false); // Reset
  this->new_line(2);
  
  if (show_text_styles) {
    ESP_LOGD(TAG, "Demo: Text styles");
    
    this->set_text_alignment(1); // Center
    this->set_text_style(true, 0, false, false); // Bold
    this->print_text("=== TEXT STYLES ===", 1);
    this->set_text_style(false, 0, false, false);
    this->set_text_alignment(0); // Left
    this->new_line(1);
    
    // Font sizes demo
    this->print_text("Font sizes (Zaphod approved):");
    this->new_line(1);
    for (uint8_t size = 0; size <= 3; size++) {
      char size_text[32];
      snprintf(size_text, sizeof(size_text), "Size %d: Hoopy!", size);
      this->print_text(size_text, size);
      this->new_line(1);
    }
    this->new_line(1);
    
    // Style combinations
    this->print_text("Text styles:");
    this->new_line(1);
    
    this->set_text_style(true, 0, false, false); // Bold
    this->print_text("Bold: Vogon poetry");
    this->new_line(1);
    
    this->set_text_style(false, 2, false, false); // Underlined (2 dots)
    this->print_text("Underlined: Babel fish");
    this->new_line(1);
    
    this->set_text_style(false, 0, true, false); // Double width
    this->print_text("Wide: Heart of Gold");
    this->new_line(1);
    
    this->set_text_style(true, 1, true, false); // Bold + underline + wide
    this->print_text("All styles: Infinite");
    this->new_line(1);
    
    this->set_text_style(false, 0, false, false); // Reset
    this->new_line(1);
    
    // Alignment demo
    this->print_text("Text alignment test:");
    this->new_line(1);
    this->set_text_alignment(0); // Left
    this->print_text("Left: Earth (mostly harmless)");
    this->new_line(1);
    this->set_text_alignment(1); // Center
    this->print_text("Center: Restaurant");
    this->new_line(1);
    this->set_text_alignment(2); // Right
    this->print_text("Right: Magrathea");
    this->new_line(1);
    this->set_text_alignment(0); // Reset to left
    this->new_line(1);
  }
  
  if (show_inverse) {
    ESP_LOGD(TAG, "Demo: Inverse printing");
    
    this->set_text_alignment(1); // Center
    this->set_text_style(true, 0, false, false);
    this->print_text("=== INVERSE MODE ===", 1);
    this->set_text_style(false, 0, false, false);
    this->set_text_alignment(0);
    this->new_line(1);
    
    this->print_text("Normal: So long and thanks");
    this->new_line(1);
    
    this->set_inverse_printing(true);
    this->print_text("Inverse: for all the fish!");
    this->new_line(1);
    this->print_text("Black background mode");
    this->new_line(1);
    this->set_inverse_printing(false);
    
    this->print_text("Back to normal printing");
    this->new_line(2);
  }
  
  if (show_rotation) {
    ESP_LOGD(TAG, "Demo: 90-degree rotation");
    
    this->set_text_alignment(1); // Center
    this->set_text_style(true, 0, false, false);
    this->print_text("=== ROTATION ===", 1);
    this->set_text_style(false, 0, false, false);
    this->set_text_alignment(0);
    this->new_line(1);
    
    this->print_text("Normal orientation:");
    this->new_line(1);
    this->print_text("Hitchhiker's Guide");
    this->new_line(1);
    
    this->set_90_degree_rotation(true);
    this->print_text("90 deg: Mostly Harmless");
    this->new_line(1);
    this->print_text("Rotated text mode");
    this->new_line(1);
    this->set_90_degree_rotation(false);
    
    this->print_text("Rotation disabled");
    this->new_line(2);
  }
  
  if (show_qr_code) {
    ESP_LOGD(TAG, "Demo: QR codes");
    
    this->set_text_alignment(1); // Center  
    this->set_text_style(true, 0, false, false);
    this->print_text("=== QR CODES ===", 1);
    this->set_text_style(false, 0, false, false);
    this->new_line(1);
    
    this->print_text("The Ultimate Answer:");
    this->new_line(1);
    
    // QR code with the answer to everything
    this->print_qrcode("42");
    this->new_line(1);
    
    this->print_text("Scan this for the Answer!");
    this->new_line(1);
    
    // QR with a longer message
    this->print_text("Hitchhiker's wisdom:");
    this->new_line(1);
    this->print_qrcode("Don't Panic! Always carry a towel.");
    this->new_line(1);
    
    this->print_text("Essential travel advice");
    this->new_line(2);
  }
  
  if (show_barcode) {
    ESP_LOGD(TAG, "Demo: Barcodes");
    
    this->set_text_alignment(1); // Center
    this->set_text_style(true, 0, false, false);
    this->print_text("=== BARCODES ===", 1);
    this->set_text_style(false, 0, false, false);
    this->new_line(1);
    
    this->print_text("Universal Product Codes:");
    this->new_line(1);
    
    // CODE128 barcode - most versatile
    this->print_text("CODE128: Towel SKU");
    this->new_line(1);
    this->print_barcode("TOWEL42", CODE128);
    this->new_line(1);
    
    // CODE39 for alphanumeric
    this->print_text("CODE39: Babel Fish ID");  
    this->new_line(1);
    this->print_barcode("BABEL42", CODE39);
    this->new_line(1);
    
    // EAN13 for numeric (needs 12-13 digits)
    this->print_text("EAN13: Guide Edition");
    this->new_line(1);
    this->print_barcode("424242424242", EAN13);
    this->new_line(2);
  }
  
  // Footer with completion message
  this->set_text_alignment(1); // Center
  this->set_text_style(true, 0, true, false); // Bold + double width
  this->print_text("Demo Complete!", 2);
  this->new_line(1);
  this->set_text_style(false, 0, false, false);
  
  this->print_text("Thank you for printing");
  this->new_line(1);
  this->print_text("with M5Stack Printer!");
  this->new_line(1);
  
  this->set_text_style(false, 1, false, false); // Underlined
  this->print_text("github.com/jesserockz/");
  this->new_line(1);
  this->print_text("esphome-components");
  this->new_line(3);
  
  // Reset all settings to defaults
  this->set_text_style(false, 0, false, false);
  this->set_text_alignment(0);
  this->set_inverse_printing(false);
  this->set_90_degree_rotation(false);
  
  ESP_LOGD(TAG, "Demo completed successfully");
}

}  // namespace m5stack_printer
}  // namespace esphome
