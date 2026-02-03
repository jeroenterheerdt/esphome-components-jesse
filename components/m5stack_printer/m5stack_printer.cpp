#include "m5stack_printer.h"

#include <cinttypes>
#include <sstream>

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
static const uint8_t UPSIDE_DOWN_CMD[] = {ESC, '{'}; // ESC { n - upside down mode

// Printer control commands
static const uint8_t TEST_PAGE_CMD[] = {0x12, 'T'}; // DC2 T - print test page
static const uint8_t STATUS_REQUEST_CMD[] = {ESC, 'v', 0}; // ESC v 0 - request status
static const uint8_t SLEEP_MODE_CMD[] = {ESC, '8'}; // ESC 8 n1 n2 - sleep mode

// Additional text formatting commands
static const uint8_t ROTATE_90_CMD[] = {ESC, 'R'}; // ESC R n - 90 degree rotation (alternative)
static const uint8_t INVERSE_PRINT_CMD[] = {ESC, 'B'}; // ESC B n - white/black inverse (alternative)
// Chinese mode commands removed - they interfere with character encoding
// Use charset/codepage commands instead for proper character support

// Character set and code page commands  
static const uint8_t CHARSET_CMD[] = {ESC, 'R'}; // ESC R n - select character set (0-15)
static const uint8_t CODEPAGE_CMD[] = {ESC, 't'}; // ESC t n - select code page (0-47)

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
  if (this->send_wakeup_) {
    ESP_LOGD(TAG, "Sending printer initialization command (ESC @)");
    this->write_array(INIT_PRINTER_CMD, sizeof(INIT_PRINTER_CMD));
    delay(100); // Give printer time to initialize
    ESP_LOGD(TAG, "Printer initialization command sent");
  } else {
    ESP_LOGD(TAG, "Skipping printer initialization (send_wakeup: false)");
  }
}

void M5StackPrinterDisplay::print_text(std::string text, uint8_t font_width, uint8_t font_height, uint8_t font_type) {
  ESP_LOGD(TAG, "Text printing: '%s' with font_width=%d, font_height=%d, font_type=%d", text.c_str(), font_width, font_height, font_type);
  
  // Apply font size if not default (1x1)
  if (font_width > 1 || font_height > 1) {
    // Clamp values to valid range (1-8)
    font_width = clamp<uint8_t>(font_width, 1, 8);
    font_height = clamp<uint8_t>(font_height, 1, 8);
    
    // Convert to ESC/POS format: width bits 0-2, height bits 4-6
    // Subtract 1 because ESC/POS uses 0-7 for 1x-8x
    uint8_t escpos_value = (font_width - 1) | ((font_height - 1) << 4);
    
    uint8_t size_cmd[] = {0x1D, 0x21, escpos_value};
    this->write_array(size_cmd, sizeof(size_cmd));
    ESP_LOGD(TAG, "Applied font size: %dx%d (ESC/POS value: %d)", font_width, font_height, escpos_value);
  }

  // Apply font type selection (Font A/B) if not default (Font A)
  if (font_type > 0) {
    uint8_t clamped_font_type = clamp<uint8_t>(font_type, 0, 1);
    uint8_t font_type_cmd[] = {0x1B, 0x21, clamped_font_type}; // ESC ! n
    this->write_array(font_type_cmd, sizeof(font_type_cmd));
    ESP_LOGD(TAG, "Applied font type: %s (%d)", (clamped_font_type == 0) ? "Font A (12x24)" : "Font B (9x17)", clamped_font_type);
  }
  
  // Add indentation spaces if configured
  for (uint8_t i = 0; i < this->text_indentation_; i++) {
    this->write_byte(' ');
  }
  
  // Send the text characters directly
  for (char c : text) {
    this->write_byte(c);
  }
  this->write_byte('\n');  // Simple newline
  
  // Reset font size to default after printing if it was changed
  if (font_width > 1 || font_height > 1) {
    uint8_t reset_cmd[] = {0x1D, 0x21, 0x00};
    this->write_array(reset_cmd, sizeof(reset_cmd));
    ESP_LOGD(TAG, "Reset font size to default (1x1)");
  }

  // Reset font type to Font A if it was changed
  if (font_type > 0) {
    uint8_t font_type_reset_cmd[] = {0x1B, 0x21, 0x00}; // ESC ! 0 (Font A)
    this->write_array(font_type_reset_cmd, sizeof(font_type_reset_cmd));
    ESP_LOGD(TAG, "Reset font type to Font A (12x24)");
  }
  
  ESP_LOGD(TAG, "Text printing complete");
}

void M5StackPrinterDisplay::thermal_print_text_with_formatting(
    const std::string &text, uint8_t font_width, uint8_t font_height, uint8_t font_type, bool bold, bool double_strike, uint8_t underline,
    bool upside_down, bool rotation,
    bool inverse, bool chinese_mode, uint8_t alignment, uint8_t charset, uint8_t codepage, uint8_t character_spacing) {
  ESP_LOGD(TAG, "thermal_print_text_with_formatting: text='%s'", text.c_str());
  ESP_LOGD(TAG, "Formatting: font=%dx%d, type=%d, bold=%d, double_strike=%d, underline=%d, align=%d, charset=%d, codepage=%d, spacing=%d", font_width, font_height, font_type, bold, double_strike, underline, alignment, charset, codepage, character_spacing);
  
  // Check if any formatting is actually requested (non-default values)
  bool has_formatting = (font_width > 1 || font_height > 1 || font_type > 0 || bold || double_strike || underline > 0 || upside_down || 
                        rotation || inverse || chinese_mode ||
                        alignment > 0 || charset > 0 || codepage > 0 || character_spacing > 0);
  
  if (!has_formatting) {
    // No formatting needed - use simple text printing with default font size
    ESP_LOGD(TAG, "No formatting requested, using simple text printing");
    this->print_text(text, 1, 1);  // 1x1 = default font size
  } else {
    // Formatting requested - implement safe formatting features one by one
    ESP_LOGD(TAG, "Applying safe formatting features");
    
    // Apply font size first if specified
    if (font_width > 1 || font_height > 1) {
      // Clamp values to valid range (1-8)
      uint8_t clamped_width = clamp<uint8_t>(font_width, 1, 8);
      uint8_t clamped_height = clamp<uint8_t>(font_height, 1, 8);
      
      // Convert to ESC/POS format
      uint8_t escpos_value = (clamped_width - 1) | ((clamped_height - 1) << 4);
      
      uint8_t size_cmd[] = {0x1D, 0x21, escpos_value};
      this->write_array(size_cmd, sizeof(size_cmd));
      ESP_LOGD(TAG, "Applied font size: %dx%d (ESC/POS: %d)", clamped_width, clamped_height, escpos_value);
    }

    // Apply font type selection (Font A/B) if specified
    if (font_type > 0) {
      // Clamp to valid range (0=Font A, 1=Font B)
      uint8_t clamped_font_type = clamp<uint8_t>(font_type, 0, 1);
      
      // ESC ! n command - bit 0 controls font type
      // 0 = Font A (12x24), 1 = Font B (9x17)
      uint8_t font_type_cmd[] = {0x1B, 0x21, clamped_font_type};
      this->write_array(font_type_cmd, sizeof(font_type_cmd));
      ESP_LOGD(TAG, "Applied font type: %s (%d)", (clamped_font_type == 0) ? "Font A (12x24)" : "Font B (9x17)", clamped_font_type);
    }

    // Apply character set (ESC R n) if specified
    if (charset > 0) {
      uint8_t clamped_charset = clamp<uint8_t>(charset, 0, 15);
      uint8_t charset_cmd[] = {0x1B, 0x52, clamped_charset}; // ESC R n
      this->write_array(charset_cmd, sizeof(charset_cmd));
      ESP_LOGD(TAG, "Applied character set: %d (0=USA, 1=France, 2=Germany, etc.)", clamped_charset);
    }

    // Apply code page (ESC t n) if specified  
    if (codepage > 0) {
      uint8_t clamped_codepage = clamp<uint8_t>(codepage, 0, 47);
      uint8_t codepage_cmd[] = {0x1B, 0x74, clamped_codepage}; // ESC t n
      this->write_array(codepage_cmd, sizeof(codepage_cmd));
      ESP_LOGD(TAG, "Applied code page: %d (0=CP437, 1=Katakana, 2=CP850, etc.)", clamped_codepage);
    }

    // Apply character spacing (ESC SP n) if specified
    if (character_spacing > 0) {
      uint8_t clamped_spacing = clamp<uint8_t>(character_spacing, 0, 255);
      uint8_t spacing_cmd[] = {0x1B, 0x20, clamped_spacing}; // ESC SP n
      this->write_array(spacing_cmd, sizeof(spacing_cmd));
      ESP_LOGD(TAG, "Applied character spacing: %d (in 0.125mm units = %.2fmm)", clamped_spacing, clamped_spacing * 0.125f);
    }
    
    // Apply alignment - generally safe and commonly used
    if (alignment > 0) {
      uint8_t align_cmd[] = {0x1B, 0x61, (uint8_t)alignment};
      this->write_array(align_cmd, sizeof(align_cmd));
      ESP_LOGD(TAG, "Applied text alignment: %d", alignment);
    }
    
    // Add bold formatting - test if this works without character corruption
    if (bold) {
      uint8_t bold_cmd[] = {0x1B, 0x45, 0x01};
      this->write_array(bold_cmd, sizeof(bold_cmd));
      ESP_LOGD(TAG, "Applied bold formatting");
    }
    
    // Add double-strike formatting (ESC G n) - overlapping dots for darker text
    if (double_strike) {
      uint8_t double_strike_cmd[] = {0x1B, 0x47, 0x01}; // ESC G 1
      this->write_array(double_strike_cmd, sizeof(double_strike_cmd));
      ESP_LOGD(TAG, "Applied double-strike formatting (overlapping dots)");
    }
    
    // Add underline formatting
    if (underline > 0) {
      uint8_t underline_cmd[] = {0x1B, 0x2D, underline};
      this->write_array(underline_cmd, sizeof(underline_cmd));
      ESP_LOGD(TAG, "Applied underline formatting: %d", underline);
    }
    
    // Add inverse printing formatting
    if (inverse) {
      uint8_t inverse_cmd[] = {0x1D, 0x42, 0x01}; // GS B 1 (inverse on)
      this->write_array(inverse_cmd, sizeof(inverse_cmd));
      ESP_LOGD(TAG, "Applied inverse formatting");
    }
    
    // Add upside down formatting
    if (upside_down) {
      uint8_t upside_down_cmd[] = {0x1B, 0x7B, 0x01}; // ESC { 1 (upside down on)
      this->write_array(upside_down_cmd, sizeof(upside_down_cmd));
      ESP_LOGD(TAG, "Applied upside down formatting");
    }
    
    // Add 90-degree rotation formatting
    if (rotation) {
      uint8_t rotation_cmd[] = {0x1B, 0x56, 0x01}; // ESC V 1 (90-degree clockwise rotation)
      this->write_array(rotation_cmd, sizeof(rotation_cmd));
      ESP_LOGD(TAG, "Applied 90-degree rotation formatting");
    }
    
    // Print the text directly (font size already applied above)
    for (char c : text) {
      this->write_byte(c);
    }
    this->write_byte('\n');  // Simple newline
    
    // Reset formatting after printing to avoid affecting subsequent prints
    if (font_width > 1 || font_height > 1) {
      uint8_t size_reset_cmd[] = {0x1D, 0x21, 0x00};
      this->write_array(size_reset_cmd, sizeof(size_reset_cmd));
      ESP_LOGD(TAG, "Reset font size to default (1x1)");
    }

    // Reset font type to Font A if it was changed
    if (font_type > 0) {
      uint8_t font_type_reset_cmd[] = {0x1B, 0x21, 0x00}; // ESC ! 0 (Font A)
      this->write_array(font_type_reset_cmd, sizeof(font_type_reset_cmd));
      ESP_LOGD(TAG, "Reset font type to Font A (12x24)");
    }

    // Reset character set to USA if it was changed
    if (charset > 0) {
      uint8_t charset_reset_cmd[] = {0x1B, 0x52, 0x00}; // ESC R 0 (USA)
      this->write_array(charset_reset_cmd, sizeof(charset_reset_cmd));
      ESP_LOGD(TAG, "Reset character set to USA");
    }

    // Reset code page to CP437 if it was changed
    if (codepage > 0) {
      uint8_t codepage_reset_cmd[] = {0x1B, 0x74, 0x00}; // ESC t 0 (CP437)
      this->write_array(codepage_reset_cmd, sizeof(codepage_reset_cmd));
      ESP_LOGD(TAG, "Reset code page to CP437");
    }

    // Reset character spacing to default if it was changed
    if (character_spacing > 0) {
      uint8_t spacing_reset_cmd[] = {0x1B, 0x20, 0x00}; // ESC SP 0
      this->write_array(spacing_reset_cmd, sizeof(spacing_reset_cmd));
      ESP_LOGD(TAG, "Reset character spacing to default (0)");
    }
    
    // Reset bold if it was applied
    if (bold) {
      uint8_t bold_reset_cmd[] = {0x1B, 0x45, 0x00};
      this->write_array(bold_reset_cmd, sizeof(bold_reset_cmd));
      ESP_LOGD(TAG, "Reset bold formatting");
    }
    
    // Reset double-strike if it was applied
    if (double_strike) {
      uint8_t double_strike_reset_cmd[] = {0x1B, 0x47, 0x00}; // ESC G 0
      this->write_array(double_strike_reset_cmd, sizeof(double_strike_reset_cmd));
      ESP_LOGD(TAG, "Reset double-strike formatting");
    }
    
    // Reset underline if it was applied
    if (underline > 0) {
      uint8_t underline_reset_cmd[] = {0x1B, 0x2D, 0x00};
      this->write_array(underline_reset_cmd, sizeof(underline_reset_cmd));
      ESP_LOGD(TAG, "Reset underline formatting");
    }
    
    // Reset inverse if it was applied
    if (inverse) {
      uint8_t inverse_reset_cmd[] = {0x1D, 0x42, 0x00}; // GS B 0 (inverse off)
      this->write_array(inverse_reset_cmd, sizeof(inverse_reset_cmd));
      ESP_LOGD(TAG, "Reset inverse formatting");
    }
    
    // Reset upside down if it was applied
    if (upside_down) {
      uint8_t upside_down_reset_cmd[] = {0x1B, 0x7B, 0x00}; // ESC { 0 (upside down off)
      this->write_array(upside_down_reset_cmd, sizeof(upside_down_reset_cmd));
      ESP_LOGD(TAG, "Reset upside down formatting");
    }
    
    // Reset rotation if it was applied
    if (rotation) {
      uint8_t rotation_reset_cmd[] = {0x1B, 0x56, 0x00}; // ESC V 0 (normal orientation)
      this->write_array(rotation_reset_cmd, sizeof(rotation_reset_cmd));
      ESP_LOGD(TAG, "Reset rotation formatting");
    }
    
    // Reset alignment to left if it was changed
    if (alignment > 0) {
      uint8_t align_reset_cmd[] = {0x1B, 0x61, 0x00};
      this->write_array(align_reset_cmd, sizeof(align_reset_cmd));
      ESP_LOGD(TAG, "Reset alignment to left");
    }
    
    if (bold) {
      uint8_t bold_off_cmd[] = {0x1B, 0x45, 0x00};
      this->write_array(bold_off_cmd, sizeof(bold_off_cmd));
      ESP_LOGD(TAG, "Reset bold formatting");
    }
    
    if (alignment > 0) {
      uint8_t align_reset_cmd[] = {0x1B, 0x61, 0x00};  // Reset to left align
      this->write_array(align_reset_cmd, sizeof(align_reset_cmd));
      ESP_LOGD(TAG, "Reset text alignment");
    }
  }
  
  ESP_LOGD(TAG, "thermal_print_text_with_formatting complete");
}

void M5StackPrinterDisplay::set_printer_settings(uint8_t line_spacing, uint8_t print_density, uint8_t break_time) {
  ESP_LOGD(TAG, "Setting printer settings: line_spacing=%d, density=%d, break_time=%d", 
           line_spacing, print_density, break_time);
  
  // Set line spacing (ESC 3 n)
  uint8_t spacing_cmd[] = {0x1B, 0x33, line_spacing};
  this->write_array(spacing_cmd, sizeof(spacing_cmd));
  
  // Set print density and break time (ESC 7 n1 n2)
  uint8_t density_cmd[] = {0x1B, 0x37, print_density, break_time};
  this->write_array(density_cmd, sizeof(density_cmd));
  
  ESP_LOGD(TAG, "Printer settings applied");
}

void M5StackPrinterDisplay::reset_printer_settings() {
  ESP_LOGD(TAG, "Resetting printer settings to defaults");
  
  // Reset to default values: line_spacing=30, density=10, break_time=4
  this->set_printer_settings(30, 10, 4);
  
  // Send ESC @ to reset printer to default state
  uint8_t reset_cmd[] = {0x1B, 0x40};
  this->write_array(reset_cmd, sizeof(reset_cmd));
  
  ESP_LOGD(TAG, "Printer settings reset complete");
}

void M5StackPrinterDisplay::build_formatting_prefix_(std::vector<uint8_t> &prefix) {
  // Build ESC/POS command sequence based on current formatting state

  ESP_LOGD(TAG, "Building format prefix - states: align=%d bold=%d underline=%d ud=%d inv=%d rot=%d",
           this->alignment_state_, this->bold_state_, this->underline_state_,
           this->upside_down_state_,
           this->inverse_state_, this->rotation_state_);

  // Text alignment (must come first)
  if (this->alignment_state_ != 0) {
    prefix.insert(prefix.end(), {0x1B, 0x61, this->alignment_state_}); // ESC a n
    ESP_LOGD(TAG, "Added alignment command: ESC a %d", this->alignment_state_);
  }

  // Bold/emphasized text
  if (this->bold_state_) {
    prefix.insert(prefix.end(), {0x1B, 0x45, 0x01}); // ESC E 1
    ESP_LOGD(TAG, "Added bold command: ESC E 1");
  }

  // Underline
  if (this->underline_state_ > 0) {
    prefix.insert(prefix.end(), {0x1B, 0x2D, this->underline_state_}); // ESC - n
    ESP_LOGD(TAG, "Added underline command: ESC - %d", this->underline_state_);
  }

  // Upside down (character rotation)
  if (this->upside_down_state_) {
    prefix.insert(prefix.end(), {0x1B, 0x7B, 0x01}); // ESC { 1
    ESP_LOGD(TAG, "Added upside down command: ESC { 1");
  }

  // Inverse printing
  if (this->inverse_state_) {
    prefix.insert(prefix.end(), {0x1D, 0x42, 0x01}); // GS B 1
    ESP_LOGD(TAG, "Added inverse command: GS B 1");
  }

  // 90-degree rotation
  if (this->rotation_state_) {
    prefix.insert(prefix.end(), {0x1B, 0x56, 0x01}); // ESC V 1
    ESP_LOGD(TAG, "Added rotation command: ESC V 1");
  }

  ESP_LOGD(TAG, "Format prefix complete, %d bytes added", prefix.size());
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

  // Track alignment state for next text print (line-buffered printer)
  this->alignment_state_ = alignment;
  ESP_LOGD(TAG, "Set text alignment: %d (0=left, 1=center, 2=right)", alignment);
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
  // Process print queue
  if (!this->queue_.empty()) {
    std::vector<uint8_t> data = this->queue_.front();
    this->queue_.pop();

    ESP_LOGV(TAG, "Writing %d bytes from queue (%d items remaining)",
             data.size(), this->queue_.size());
    this->write_array(data.data(), data.size());
  }
  
  // Status polling completely disabled - this printer hardware doesn't support
  // reliable ESC/POS status commands (both DLE EOT and ESC v return random data)
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
  // Track rotation state for next text print (line-buffered printer)
  this->rotation_state_ = enable;
  ESP_LOGD(TAG, "Set 90-degree rotation: %s", enable ? "enabled" : "disabled");
}

void M5StackPrinterDisplay::set_inverse_printing(bool enable) {
  // Track inverse state for next text print (line-buffered printer)
  this->inverse_state_ = enable;
  ESP_LOGD(TAG, "Set inverse printing: %s", enable ? "enabled" : "disabled");
}

void M5StackPrinterDisplay::set_chinese_mode(bool enable) {
  ESP_LOGD(TAG, "Chinese/Japanese character mode state: %s (no commands sent)", enable ? "enabled" : "disabled");
  
  // Only track the state - don't send FS commands that interfere with character encoding
  // Character encoding is handled by charset (ESC R n) and codepage (ESC t n) commands
  this->chinese_mode_state_ = enable;
}

void M5StackPrinterDisplay::set_charset(uint8_t charset) {
  if (charset > 15) {
    ESP_LOGW(TAG, "Invalid charset %d, clamping to 15", charset);
    charset = 15;
  }
  
  ESP_LOGD(TAG, "Character set %d requested (0=USA) - using default initialization", charset);
  // Don't send charset commands as they interfere with text printing on this printer
  // The printer defaults to USA charset which is sufficient for ASCII text
}

void M5StackPrinterDisplay::set_codepage(uint8_t codepage) {
  if (codepage > 47) {
    ESP_LOGW(TAG, "Invalid codepage %d, clamping to 47", codepage);
    codepage = 47;
  }
  
  ESP_LOGD(TAG, "Code page %d requested (0=CP437) - using default initialization", codepage);
  // Don't send codepage commands as they interfere with text printing on this printer  
  // The printer defaults to CP437 which is sufficient for ASCII text
}

void M5StackPrinterDisplay::send_raw_command(const std::vector<uint8_t> &command) {
  if (!command.empty()) {
    this->write_array(command.data(), command.size());
    ESP_LOGD(TAG, "Sent raw command: %d bytes", command.size());

    // Log the command bytes for debugging
    std::string cmd_str = "Command bytes: ";
    for (size_t i = 0; i < command.size(); i++) {
      cmd_str += std::to_string(command[i]);
      if (i < command.size() - 1) cmd_str += ", ";
    }
    ESP_LOGD(TAG, "%s", cmd_str.c_str());
  } else {
    ESP_LOGW(TAG, "Attempted to send empty raw command");
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
  this->set_text_style(true, 0, false); // Bold
  this->print_text("DON'T PANIC", 1);  // Reduced from 2 to 1
  this->new_line(1);
  this->set_text_style(false, 1, false); // Normal, underlined
  this->print_text("M5Stack Printer Demo", 0);  // Reduced from 1 to 0
  this->new_line(2);

  // Reset formatting and left align
  this->set_text_style(false, 0, false);
  this->set_text_alignment(0);

  // Display what features will be tested
  this->print_text("The Answer to the Ultimate");
  this->new_line(1);
  this->print_text("Question of Thermal Printing:");
  this->new_line(1);
  this->set_text_style(true, 0, false); // Bold
  this->print_text("42 functions tested below", 0);  // Reduced from 1 to 0
  this->set_text_style(false, 0, false); // Reset
  this->new_line(2);

  if (show_text_styles) {
    ESP_LOGD(TAG, "Demo: Text styles");

    this->set_text_alignment(1); // Center
    this->set_text_style(true, 0, false); // Bold
    this->print_text("=== TEXT STYLES ===", 0);  // Reduced from 1 to 0
    this->set_text_style(false, 0, false);
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

    this->set_text_style(true, 0, false); // Bold
    this->print_text("Bold: Vogon poetry");
    this->new_line(1);

    this->set_text_style(false, 2, false); // Underlined (2 dots)
    this->print_text("Underlined: Babel fish");
    this->new_line(1);

    this->print_text("Wide: Heart of Gold", 2, 1); // Use font_width instead
    this->new_line(1);

    this->set_text_style(true, 1, false); // Bold + underline
    this->print_text("All styles: Infinite", 2, 1); // Wide using font_width
    this->new_line(1);

    this->set_text_style(false, 0, false); // Reset
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
    this->set_text_style(true, 0, false);
    this->print_text("=== INVERSE MODE ===", 0);  // Reduced from 1 to 0
    this->set_text_style(false, 0, false);
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
    this->set_text_style(true, 0, false);
    this->print_text("=== ROTATION ===", 0);  // Reduced from 1 to 0
    this->set_text_style(false, 0, false);
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
    this->set_text_style(true, 0, false);
    this->print_text("=== QR CODES ===", 0);  // Reduced from 1 to 0
    this->set_text_style(false, 0, false);
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
    this->set_text_style(true, 0, false);
    this->print_text("=== BARCODES ===", 0);  // Reduced from 1 to 0
    this->set_text_style(false, 0, false);
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
  this->set_text_style(true, 0, false); // Bold
  this->print_text("Demo Complete!", 2, 1);  // Bold + wide using font_width
  this->new_line(1);
  this->set_text_style(false, 0, false);

  this->print_text("Thank you for printing");
  this->new_line(1);
  this->print_text("with M5Stack Printer!");
  this->new_line(1);

  this->set_text_style(false, 1, false); // Underlined
  this->print_text("github.com/jesserockz/");
  this->new_line(1);
  this->print_text("esphome-components");
  this->new_line(3);

  // Reset all settings to defaults
  this->set_text_style(false, 0, false);
  this->set_text_alignment(0);
  this->set_inverse_printing(false);
  this->set_90_degree_rotation(false);

  ESP_LOGD(TAG, "Demo completed successfully");
}

void M5StackPrinterDisplay::set_text_style(bool bold, uint8_t underline, bool upside_down, uint8_t font_type) {
  // Track formatting state for next text print (line-buffered printer)
  this->bold_state_ = bold;
  this->underline_state_ = underline;
  this->upside_down_state_ = upside_down;
  this->font_type_state_ = font_type;

  ESP_LOGD(TAG, "Set text style - bold:%s, underline:%d, upside_down:%s, font_type:%d",
           bold ? "true" : "false", underline, upside_down ? "true" : "false", font_type);
}

void M5StackPrinterDisplay::print_test_page() {
  ESP_LOGD(TAG, "Printing test page");

  // Print header
  this->set_text_style(true, 0, false); // Bold
  this->print_text("=== TEST PAGE ===\n", 0);
  this->set_text_style(false, 0, false); // Reset

  // Test basic text
  this->print_text("Basic text printing test\n", 0);

  // Test font sizes
  for (uint8_t size = 0; size <= 7; size++) {
    this->print_text("Font size " + std::to_string(size) + "\n", size);
  }

  // Test text styles
  this->set_text_style(true, 0, false); // Bold
  this->print_text("Bold text\n", 0);

  this->set_text_style(false, 1, false); // Underline
  this->print_text("Underlined text\n", 0);

  this->print_text("Double width\n", 2, 1); // Use font_width instead

  this->set_text_style(true, 1, false); // Bold + underline
  this->print_text("Bold+Under+Wide\n", 2, 1); // Wide using font_width

  // Reset styles
  this->set_text_style(false, 0, false);

  // Test alignment
  this->set_text_alignment(0); // Left
  this->print_text("Left aligned\n", 0);

  this->set_text_alignment(1); // Center
  this->print_text("Center aligned\n", 0);

  this->set_text_alignment(2); // Right
  this->print_text("Right aligned\n", 0);

  this->set_text_alignment(0); // Reset to left

  // Test QR code (small)
  this->print_text("\nQR Test:\n", 0);
  this->print_qrcode("ESP32");

  // End of test page
  this->print_text("\n=== END TEST ===\n", 0);
  this->new_line(3); // Feed paper for clean tear

  ESP_LOGD(TAG, "Test page completed");
}

// Status functions completely removed - hardware doesn't support reliable status reporting
// Both DLE EOT and ESC v commands return random/garbage values instead of actual status

void M5StackPrinterDisplay::set_sleep_mode(uint16_t timeout_seconds) {
  // ESC 8 n m - Set sleep mode timeout
  // Convert seconds to appropriate units for the printer
  uint8_t timeout_low = timeout_seconds & 0xFF;
  uint8_t timeout_high = (timeout_seconds >> 8) & 0xFF;

  uint8_t cmd[] = {0x1B, 0x38, timeout_low, timeout_high};
  this->write_array(cmd, sizeof(cmd));

  ESP_LOGD(TAG, "Sleep mode set to %d seconds", timeout_seconds);
}

void M5StackPrinterDisplay::wake_up() {
  ESP_LOGD(TAG, "Waking up printer from sleep mode");
  
  // Send any character to wake the printer (space character is common)
  this->write_byte(0x20);  // Space character
  delay(100);  // Give printer time to wake up
  
  // Send initialization command to ensure printer is ready
  this->write_array(INIT_PRINTER_CMD, sizeof(INIT_PRINTER_CMD));
  delay(100);  // Give printer time to initialize
  
  ESP_LOGD(TAG, "Printer wake-up complete");
}

void M5StackPrinterDisplay::set_tab_positions(const std::string &positions) {
  // ESC D n1 n2 ... nk NUL - Set horizontal tab positions
  std::vector<uint8_t> cmd;
  cmd.push_back(0x1B);  // ESC
  cmd.push_back(0x44);  // D

  // Simple parsing of comma or space separated positions
  std::string current_number;
  int position_count = 0;

  for (size_t i = 0; i < positions.length() && position_count < 32; i++) {
    char c = positions[i];

    if ((c >= '0' && c <= '9')) {
      current_number += c;
    } else if ((c == ',' || c == ' ' || c == '\t') && !current_number.empty()) {
      // Parse the number using simple conversion to avoid exceptions
      int pos = 0;
      bool valid = true;
      for (char digit : current_number) {
        if (digit >= '0' && digit <= '9') {
          pos = pos * 10 + (digit - '0');
        } else {
          valid = false;
          break;
        }
      }

      if (valid && pos >= 1 && pos <= 255) {
        cmd.push_back(static_cast<uint8_t>(pos));
        position_count++;
      }
      current_number.clear();
    }
  }

  // Handle the last number if exists
  if (!current_number.empty() && position_count < 32) {
    int pos = 0;
    bool valid = true;
    for (char digit : current_number) {
      if (digit >= '0' && digit <= '9') {
        pos = pos * 10 + (digit - '0');
      } else {
        valid = false;
        break;
      }
    }

    if (valid && pos >= 1 && pos <= 255) {
      cmd.push_back(static_cast<uint8_t>(pos));
      position_count++;
    }
  }

  cmd.push_back(0);  // NUL terminator

  this->write_array(cmd.data(), cmd.size());
  ESP_LOGD(TAG, "Set %d tab positions", position_count);
}

void M5StackPrinterDisplay::horizontal_tab() {
  // Send horizontal tab character (HT = 0x09)
  uint8_t cmd = 0x09;
  this->write_array(&cmd, 1);
}

void M5StackPrinterDisplay::set_horizontal_position(uint16_t position) {
  // ESC $ nL nH - Set absolute horizontal print position
  uint8_t pos_low = position & 0xFF;
  uint8_t pos_high = (position >> 8) & 0xFF;

  uint8_t cmd[] = {0x1B, 0x24, pos_low, pos_high};
  this->write_array(cmd, sizeof(cmd));

  ESP_LOGD(TAG, "Set horizontal position to %d", position);
}

void M5StackPrinterDisplay::set_text_indentation(uint8_t spaces) {
  // Store indentation as number of spaces to prepend to each print
  if (spaces > 50) {
    ESP_LOGE(TAG, "Invalid text indentation %d, must be 0-50 spaces", spaces);
    return;
  }
  
  this->text_indentation_ = spaces;
  ESP_LOGD(TAG, "Set text indentation to %d spaces", spaces);
}

void M5StackPrinterDisplay::reset_text_indentation() {
  // Reset indentation to default (0 spaces)
  this->text_indentation_ = 0;
  ESP_LOGD(TAG, "Reset text indentation to default");
}

void M5StackPrinterDisplay::feed_paper_dots(uint8_t dots) {
  // ESC J n - Feed paper n dots (0.125mm units)
  uint8_t cmd[] = {0x1B, 0x4A, dots};
  this->write_array(cmd, sizeof(cmd));
  
  ESP_LOGD(TAG, "Feed paper %d dots (%.2f mm)", dots, dots * 0.125f);
}

void M5StackPrinterDisplay::print_and_feed_lines(uint8_t lines) {
  // ESC d n - Print buffer data and feed n lines
  uint8_t cmd[] = {0x1B, 0x64, lines};
  this->write_array(cmd, sizeof(cmd));
  
  ESP_LOGD(TAG, "Print buffer and feed %d lines", lines);
}



}  // namespace m5stack_printer
}  // namespace esphome
