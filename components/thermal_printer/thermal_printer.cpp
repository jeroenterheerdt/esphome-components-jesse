#include "thermal_printer.h"
#include "esphome_bitmap.h"
#include <cinttypes>

/* Commands (* means done)
03 - horizontal tab ~ is just \t in text
04 - set tab locations ~ kind of done but issue in display.py when adding the templ.
*09 - set row spacing
*10 - alignment
*11 - set double width mode (also settable through 16)
*12 - cancel double width mode (also settable through 16)
*16 - set print mode (
*  font //implemented correctly but not seeing any difference on my printer!
*  inverse (also settable through 18)
*  upsidedown (also settable though 24)
*  bold (also settable through 22)
*  doubleheight (settable through 17)
*  doublewidth (also settable through 11/12)
*  strikethrough (implemented but don't see diff on my printer)
*  )
*17 - font size
*18 - inverse (also settable through 16)
*19 - 90 degree rotation (
*22 - bold (also settable through 16)
*24 - upside down (also settable through 16, keep in mind that this also inverts the meaning of the l/r alignment
*25 - underline (can't be combined with 90 degree rotation. if both set, underline is ignored)
36 - print bitmap ~ wip
*46 - print barcode
*XX - print qr code (in basic and advanced datasheet)
*XX - cut (full/partial) - from advanced datasheet - didn't work for my printer
*XX - partial cut
XX - check printer has paper
XX - print test page
# basic datasheet: https://dfimg.dfrobot.com/nobody/wiki/0c0a789684349c93a55e754f49bdea18.pdf
# advanced datasheet: https://wiki.dfrobot.com/Embedded%20Thermal%20Printer%20-%20TTL%20Serial%20SKU%3A%20DFR0503-EN
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
static const uint8_t SET_BOLD_CMD[] = {ESC, 0x47};              // watch out, gets (maybe) overriden with esc, 0x21!
static const uint8_t SET_DOUBLE_WIDTH_ON_CMD[] = {ESC, 0x0E};   // watch out, gets overriden with esc, 0x21!
static const uint8_t SET_DOUBLE_WIDTH_OFF_CMD[] = {ESC, 0x14};  // watch out, gets overriden with esc, 0x21!
static const uint8_t SET_FONT_SIZE_CMD[] = {GS, 0x21};
static const uint8_t SET_PRINT_MODE_CMD[] = {ESC, 0x21};  // conflicts with bold, double width on and double width off?
static const uint8_t SET_TAB_POSITIONS_CMD[] = {ESC, 0x44};  // ESC D
static const uint8_t SET_ROW_SPACING_CMD[] = {ESC, 0x33};    // ESC 3

// Barcodes commands
static const uint8_t BARCODE_TEXT_POSITION_CMD[] = {GS, 0x48};  // GS H
static const uint8_t BARCODE_HEIGHT_CMD[] = {GS, 0x68};         // GS h
static const uint8_t BARCODE_WIDTH_CMD[] = {GS, 0x77};          // GS w
static const uint8_t PRINT_BARCODE_CMD[] = {GS, 0x6B};          // GS k

// QRCode commands
static const uint8_t QR_FN = 0x28;
static const uint8_t QR_MOD = 0x6B;
static const uint8_t QR_CODE_MODEL_CMD[] = {GS, QR_FN, QR_MOD, 0x04, 0x00, 0x31, 0x41};             // GS ( k
static const uint8_t QR_CODE_SIZE_CMD[] = {GS, QR_FN, QR_MOD, 0x03, 0x00, 0x31, 0x43};              // GS ( k
static const uint8_t QR_CODE_ERROR_CORRECTION_CMD[] = {GS, QR_FN, QR_MOD, 0x03, 0x00, 0x31, 0x45};  // GS ( k
static const uint8_t QR_CODE_DATA_CMD[] = {GS, QR_FN, QR_MOD};                                      // GS ( k
static const uint8_t QR_CODE_PRINT_CMD[] = {GS, QR_FN, QR_MOD, 0x03, 0x00, 0x31, 0x51, 0x30};       // GS ( Q

// Cut commands
static const uint8_t CUT_FULL_CMD[] = {ESC, 0x69};     // ESC i
static const uint8_t CUT_PARTIAL_CMD[] = {ESC, 0x6D};  // ESC m

// Bitmap print commands
static const uint8_t LOAD_BITMAP_CMD[] = {ESC, 0x2A, 0x20};  // ESC * 32
static const uint8_t PRINT_BITMAP_CMD[] = {GS, 0x2F};        // GS /

// Get status command
static const uint8_t GET_STATUS_CMD[] = {ESC, 0x76, 0x00};  // ESC v 0

// Other
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
                                       uint8_t underline_weight, bool updown, bool bold, bool double_width,
                                       bool double_height, std::string font, bool strikethrough) {
  uint8_t font_width = 2;
  uint8_t font_height = 2;
  this->print_text(text, align, inverse, ninety_degree, underline_weight, updown, bold, font_width, font_height, font,
                   strikethrough);
}
void ThermalPrinterDisplay::print_text(std::string text, std::string align, bool inverse, bool ninety_degree,
                                       uint8_t underline_weight, bool updown, bool bold, uint8_t font_width,
                                       uint8_t font_height, std::string font, bool strikethrough) {
  this->init_();
  const char *tag = "print_text";

  ESP_LOGD(tag, "text: %s", text.c_str());
  ESP_LOGD(tag, "align: %s", align.c_str());
  ESP_LOGD(tag, "inverse: %s", inverse ? "true" : "false");
  ESP_LOGD(tag, "ninety_degree: %s", ninety_degree ? "true" : "false");
  ESP_LOGD(tag, "underline_weight: %d", underline_weight);
  ESP_LOGD(tag, "updown: %s", updown ? "true" : "false");
  ESP_LOGD(tag, "bold: %s", bold ? "true" : "false");
  ESP_LOGD(tag, "font_width: %d", font_width);
  ESP_LOGD(tag, "font_height: %d", font_height);
  ESP_LOGD(tag, "font: %s", font.c_str());
  ESP_LOGD(tag, "strikethrough: %s", strikethrough ? "true" : "false");

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
  // bold
  if (bold) {
    this->write_array(SET_BOLD_CMD, sizeof(SET_BOLD_CMD));
    this->write_byte(0x01);  // Bold
  } else {
    this->write_array(SET_BOLD_CMD, sizeof(SET_BOLD_CMD));
    this->write_byte(0x00);  // Normal
  }
  /*// double width
  if (double_width) {
    this->write_array(SET_DOUBLE_WIDTH_ON_CMD, sizeof(SET_DOUBLE_WIDTH_ON_CMD));
  } else {
    this->write_array(SET_DOUBLE_WIDTH_OFF_CMD, sizeof(SET_DOUBLE_WIDTH_OFF_CMD));
  }*/
  // font_width and font_height
  font_width = clamp<uint8_t>(font_width, 1, 8);
  font_height = clamp<uint8_t>(font_height, 1, 8);
  uint8_t width_bits = (font_width - 1) << 4;
  uint8_t height_bits = (font_height - 1) & 0x0F;
  uint8_t wn = width_bits | height_bits;
  this->write_array(SET_FONT_SIZE_CMD, sizeof(SET_FONT_SIZE_CMD));
  this->write_byte(wn);  // Font size

  // font and strikethrough
  uint8_t n = 0x00;
  font = this->toUpperCase(font)[0];
  if (font == "B") {
    n |= 0x01;  // Font B
  } else if (font == "A") {
    n |= 0x00;  // Font A
  } else {
    ESP_LOGW(tag, "Invalid font: %s", font.c_str());
  }
  if (strikethrough) {
    n |= 0x40;  // Strikethrough
  }
  ESP_LOGD(tag, "font and strikethough byte: %d", n);
  this->write_array(SET_PRINT_MODE_CMD, sizeof(SET_PRINT_MODE_CMD));
  this->write_byte(n);  // Print mode

  // row spacing
  this->write_array(SET_ROW_SPACING_CMD, sizeof(SET_ROW_SPACING_CMD));
  this->write_byte(this->spacing);  // Row spacing

  // tab positions
  this->write_array(SET_TAB_POSITIONS_CMD, sizeof(SET_TAB_POSITIONS_CMD));
  this->write_array(this->tab_positions.data(), this->tab_positions.size());

  ESP_LOGD(tag, "printing now!");
  this->write_str(text.c_str());
}

void ThermalPrinterDisplay::set_tab_positions(std::vector<int> tab_positions) {
  std::vector<uint8_t> cmd;
  for (int pos : tab_positions) {
    if (pos >= 1 && pos <= 255)
      cmd.push_back((uint8_t) pos);
  }

  cmd.push_back(0x00);  // End with NUL
  this->tab_positions = cmd;
}

// default is 32. range 0<=n<=255
void ThermalPrinterDisplay::set_row_spacing(uint8_t spacing) {
  spacing = clamp<uint8_t>(spacing, 0, 255);
  this->spacing = spacing;
}

void ThermalPrinterDisplay::new_line(uint8_t lines) {
  for (uint8_t i = 0; i < lines; i++) {
    this->write_byte('\n');
  }
}

void ThermalPrinterDisplay::print_barcode(std::string text,  // BarcodeType type
                                          std::string type, uint8_t height, uint8_t width,
                                          // BarcodeTextPosition pos,
                                          std::string pos,  // BarcodeAlignment align
                                          std::string align) {
  this->init_();
  const char *tag = "print_barcode";
  ESP_LOGD(tag, "text: %s", text.c_str());
  ESP_LOGD(tag, "type: %s", type.c_str());
  ESP_LOGD(tag, "height: %d", height);
  ESP_LOGD(tag, "width: %d", width);
  ESP_LOGD(tag, "pos: %s", pos.c_str());
  ESP_LOGD(tag, "align: %s", align.c_str());
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
  // hri text position
  // Convert the position string to uppercase
  pos = this->toUpperCase(pos);
  if (pos == "BELOW") {
    this->write_array(BARCODE_TEXT_POSITION_CMD, sizeof(BARCODE_TEXT_POSITION_CMD));
    this->write_byte(BarcodeTextPosition::BELOW);  // HRI text position
  } else if (pos == "ABOVE") {
    this->write_array(BARCODE_TEXT_POSITION_CMD, sizeof(BARCODE_TEXT_POSITION_CMD));
    this->write_byte(BarcodeTextPosition::ABOVE);  // HRI text position
  } else if (pos == "BOTH") {
    this->write_array(BARCODE_TEXT_POSITION_CMD, sizeof(BARCODE_TEXT_POSITION_CMD));
    this->write_byte(BarcodeTextPosition::BOTH);  // HRI text position
  } else if (pos == "NONE") {
    this->write_array(BARCODE_TEXT_POSITION_CMD, sizeof(BARCODE_TEXT_POSITION_CMD));
    this->write_byte(BarcodeTextPosition::NONE);  // HRI text position
  } else {
    ESP_LOGW(TAG, "Invalid barcode text position: %s", pos.c_str());
  }
  this->write_array(BARCODE_TEXT_POSITION_CMD, sizeof(BARCODE_TEXT_POSITION_CMD));
  // barcode height
  height = clamp<uint8_t>(height, 1, 255);
  this->write_array(BARCODE_HEIGHT_CMD, sizeof(BARCODE_HEIGHT_CMD));
  this->write_byte(height);  // Barcode height
  // barcode width
  width = clamp<uint8_t>(width, 2, 6);
  this->write_array(BARCODE_WIDTH_CMD, sizeof(BARCODE_WIDTH_CMD));
  this->write_byte(width);  // Barcode width
  // barcode type
  // Convert the type string to uppercase
  type = this->toUpperCase(type);
  uint8_t btype = 0x00;
  if (type == "EAN_13") {
    btype = BarcodeType::EAN13;
  } else if (type == "CODE_39") {
    btype = BarcodeType::CODE39;
  } else if (type == "ITF") {
    btype = BarcodeType::ITF;
  } else if (type == "UPC_A") {
    btype = BarcodeType::UPC_A;
  } else if (type == "UPC_E") {
    btype = BarcodeType::UPC_E;
  } else if (type == "EAN_8") {
    btype = BarcodeType::EAN8;
  } else if (type == "CODABAR") {
    btype = BarcodeType::CODABAR;
  } else {
    ESP_LOGW(TAG, "Invalid barcode type: %s", type.c_str());
  }
  this->write_array(PRINT_BARCODE_CMD, sizeof(PRINT_BARCODE_CMD));
  this->write_byte(btype);  // Barcode type
  for (char c : text) {
    this->write_byte(c);  // Barcode data
  }
  this->write_byte(0x00);  // End with NUL
}

void ThermalPrinterDisplay::print_qr_code(std::string text, std::string model, std::string error_correction_level,
                                          uint8_t size) {
  this->init_();
  const char *tag = "print_qr_code";
  ESP_LOGD(tag, "text: %s", text.c_str());
  ESP_LOGD(tag, "model: %s", model.c_str());
  ESP_LOGD(tag, "error_correction_level: %s", error_correction_level.c_str());
  ESP_LOGD(tag, "size: %d", size);
  // model
  // Convert the model string to uppercase
  model = this->toUpperCase(model);
  uint8_t qmodel = 0x00;
  if (model == "MODEL_1") {
    qmodel = QRCodeModel::MODEL_1;
  } else if (model == "MODEL_2") {
    qmodel = QRCodeModel::MODEL_2;
  } else {
    ESP_LOGW(TAG, "Invalid QR code model: %s", model.c_str());
  }
  this->write_array(QR_CODE_MODEL_CMD, sizeof(QR_CODE_MODEL_CMD));
  this->write_byte(qmodel);  // QR code model
  // error correction level
  // Convert the error correction level string to uppercase
  error_correction_level = this->toUpperCase(error_correction_level);
  uint8_t qeclevel = 0x00;
  if (error_correction_level == "LEVEL_L") {
    qeclevel = QRCodeErrorCorrectionLevel::LEVEL_L;
  } else if (error_correction_level == "LEVEL_M") {
    qeclevel = QRCodeErrorCorrectionLevel::LEVEL_M;
  } else if (error_correction_level == "LEVEL_Q") {
    qeclevel = QRCodeErrorCorrectionLevel::LEVEL_Q;
  } else if (error_correction_level == "LEVEL_H") {
    qeclevel = QRCodeErrorCorrectionLevel::LEVEL_H;
  } else {
    ESP_LOGW(TAG, "Invalid QR code error correction level: %s", error_correction_level.c_str());
  }
  this->write_array(QR_CODE_ERROR_CORRECTION_CMD, sizeof(QR_CODE_ERROR_CORRECTION_CMD));
  this->write_byte(qeclevel);  // QR code error correction level
  // size
  size = clamp<uint8_t>(size, 1, 16);
  this->write_array(QR_CODE_SIZE_CMD, sizeof(QR_CODE_SIZE_CMD));
  this->write_byte(size);  // QR code size
  // data
  uint16_t len = text.length() + 3;
  uint8_t pL = len & 0xFF;
  uint8_t pH = (len >> 8) & 0xFF;

  this->write_array(QR_CODE_DATA_CMD, sizeof(QR_CODE_DATA_CMD));
  this->write_byte(pL);
  this->write_byte(pH);
  this->write_byte(0x31);  // Mode
  this->write_byte(0x50);  // Character count
  this->write_byte(0x30);
  for (char c : text) {
    this->write_byte(c);  // QRCode data
  }
  // print the qr code
  this->write_array(QR_CODE_PRINT_CMD, sizeof(QR_CODE_PRINT_CMD));
}

void ThermalPrinterDisplay::cut(std::string cut_type) {
  this->init_();
  const char *tag = "cut";
  ESP_LOGD(tag, "cut_type: %s", cut_type.c_str());
  // Convert the cut type string to uppercase
  cut_type = this->toUpperCase(cut_type);
  if (cut_type == "FULL") {
    this->write_array(CUT_FULL_CMD, sizeof(CUT_FULL_CMD));
  } else if (cut_type == "PARTIAL") {
    this->write_array(CUT_PARTIAL_CMD, sizeof(CUT_PARTIAL_CMD));
  } else {
    ESP_LOGW(TAG, "Invalid cut type: %s", cut_type.c_str());
  }
}

void ThermalPrinterDisplay::print_image(std::string image, int width) {
  // this->init_();
  const char *tag = "print_image";
  // use image2cpp!!
  this->write_array(show, sizeof(show));
  this->write_byte('\n');
}

bool ThermalPrinterDisplay::has_paper() {
  /*this->write_array(GET_STATUS_CMD, sizeof(GET_STATUS_CMD));

  uint8_t status = -1;
  uint8_t *data = nullptr;
  for (uint8_t i = 0; i < 10; i++) {
    this->read_byte(data);
    if (data != nullptr) {
      status = static_cast<int>(*data);
      break;
    }

    delay(100);
  }
  ESP_LOGD(TAG, "status: %d", status & 0b00000100);
  return !(status & 0b00000100);*/
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
  this->haspaper_ = this->has_paper();
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
