# M5Stack Printer ESPHome Component

An enhanced ESPHome component for controlling thermal printers compatible with standard ESC/POS protocol, including printers like the CSN-A2 and DFRobot models.

## Features

- **Text Printing**: Print text with various font sizes
- **Formatted Text**: Bold, underline, inverse, and text alignment support  
- **QR Code Printing**: Generate QR codes with configurable error correction and module size
- **Barcode Printing**: Support for multiple barcode types using standard ESC/POS commands
- **Paper Control**: Cut paper (where supported), feed lines
- **Image Printing**: Print bitmap images using raster graphics
- **Character Encoding**: Multiple character sets and code pages
- **Status Monitoring**: Basic printer control and reset functionality

## Important Compatibility Notes

**This component has been updated to use standard ESC/POS commands**, which should work with most thermal printers including:

- **CSN-A2 Thermal Printer** (Adafruit)
- **DFRobot Thermal Printers** 
- **Standard ESC/POS Compatible Printers**

**Previous Issues Fixed:**
- Removed non-standard baud rate change commands
- Fixed incorrect QR code command format
- Updated barcode commands to use standard ESC/POS format
- Corrected image printing header format
- Fixed font size reset command

**Known Limitations:**
- Some advanced features (like paper cutting) may not be supported by all printer models
- QR code and advanced barcode features require printers with full ESC/POS support
- Always test with your specific printer model as ESC/POS implementations can vary

## Configuration

```yaml
m5stack_printer:
  - platform: m5stack_printer
    id: my_printer
    uart_id: printer_uart
    height: 200
    send_wakeup: false
```

### Parameters

- **id** (Required): The ID for this printer component
- **uart_id** (Required): The UART component ID
- **height** (Required): The height of the display buffer in pixels
- **send_wakeup** (Optional, default: false): Send wakeup command on initialization

## Actions

### Print Text

```yaml
m5stack_printer.print_text:
  id: my_printer
  text: "Hello World!"
  font_size: 1
  font_size_factor: 1.0
```

### Print Formatted Text

```yaml
script:
  - m5stack_printer.set_bold:
      id: my_printer
      bold: true
  - m5stack_printer.set_align:
      id: my_printer
      align: center
  - m5stack_printer.print_text:
      id: my_printer
      text: "Bold Centered Text"
  - m5stack_printer.set_bold:
      id: my_printer
      bold: false
  - m5stack_printer.set_align:
      id: my_printer
      align: left
```

### Print QR Code

```yaml
m5stack_printer.print_qr_code:
  id: my_printer
  qrcode: "https://esphome.io"
```

### Print Barcode

```yaml
m5stack_printer.print_bar_code:
  id: my_printer
  barcode: "123456789"
  type: "CODE128"
```

### Paper Control

```yaml
# Cut paper (full cut)
m5stack_printer.cut_paper:
  id: my_printer
  partial: false

# Feed lines
m5stack_printer.feed_lines:
  id: my_printer
  lines: 3

# Add line breaks
m5stack_printer.new_line:
  id: my_printer
  lines: 2
```

### Printer Control

```yaml
# Reset printer
m5stack_printer.reset:
  id: my_printer
```

## Advanced Usage

### Complete Receipt Example

```yaml
script:
  - id: print_receipt
    then:
      # Reset printer
      - m5stack_printer.reset:
          id: my_printer
      
      # Header
      - m5stack_printer.set_align:
          id: my_printer
          align: center
      - m5stack_printer.set_bold:
          id: my_printer
          bold: true
      - m5stack_printer.print_text:
          id: my_printer
          text: "STORE NAME"
          font_size: 2
      - m5stack_printer.new_line:
          id: my_printer
          lines: 1
      
      # Reset formatting
      - m5stack_printer.set_bold:
          id: my_printer
          bold: false
      - m5stack_printer.set_align:
          id: my_printer
          align: left
      
      # Receipt content
      - m5stack_printer.print_text:
          id: my_printer
          text: "Item 1: $10.00"
      - m5stack_printer.new_line:
          id: my_printer
          lines: 1
      - m5stack_printer.print_text:
          id: my_printer
          text: "Item 2: $15.00"
      - m5stack_printer.new_line:
          id: my_printer
          lines: 1
      
      # Total
      - m5stack_printer.set_bold:
          id: my_printer
          bold: true
      - m5stack_printer.print_text:
          id: my_printer
          text: "Total: $25.00"
      - m5stack_printer.set_bold:
          id: my_printer
          bold: false
      - m5stack_printer.new_line:
          id: my_printer
          lines: 2
      
      # QR Code
      - m5stack_printer.set_align:
          id: my_printer
          align: center
      - m5stack_printer.print_qr_code:
          id: my_printer
          qrcode: "Receipt-ID-12345"
      
      # Cut paper
      - m5stack_printer.feed_lines:
          id: my_printer
          lines: 2
      - m5stack_printer.cut_paper:
          id: my_printer
          partial: false
```

## UART Configuration

Make sure to configure the UART component properly:

```yaml
uart:
  - id: printer_uart
    tx_pin: GPIO17
    rx_pin: GPIO16
    baud_rate: 9600
    data_bits: 8
    parity: NONE
    stop_bits: 1
```

## Supported Printers

This component is designed to work with ESC/POS compatible thermal printers including:

- M5Stack Thermal Printer Unit
- Adafruit CSN-A2 Thermal Printer
- DFRobot Thermal Printers
- Other ESC/POS compatible thermal printers

## Font Sizes

- 0: Normal (default)
- 1: Double width
- 2: Double height
- 3: Double width and height
- 4-7: Various size combinations

## Text Alignment

- `left`: Left aligned (default)
- `center`: Center aligned
- `right`: Right aligned

## Barcode Types

- `UPC_A`: UPC-A
- `UPC_E`: UPC-E
- `EAN13`: EAN-13
- `EAN8`: EAN-8
- `CODE39`: Code 39
- `ITF`: Interleaved 2 of 5
- `CODABAR`: Codabar
- `CODE93`: Code 93
- `CODE128`: Code 128

## Error Correction Levels (QR Codes)

- 0: Level L (~7% correction)
- 1: Level M (~15% correction) - Default
- 2: Level Q (~25% correction)
- 3: Level H (~30% correction)

## Troubleshooting

### Common Issues and Solutions

1. **Printer not responding**
   - Check UART connections (TX/RX should be crossed)
   - Verify baud rate (try 9600, 19200, or 115200)
   - Ensure proper power supply (most thermal printers need 5V)
   - Try enabling `send_wakeup: true` in configuration

2. **Garbled or no output**
   - Verify correct baud rate
   - Check if printer is in the right mode
   - Try sending a reset command first: `m5stack_printer.reset`

3. **QR codes not working**
   - Not all thermal printers support QR codes
   - Try different error correction levels (0-3)
   - Reduce module size if QR code is too large
   - Some printers need specific initialization sequences

4. **Barcodes not printing correctly**
   - Verify your printer supports the barcode type you're using
   - Some printers only support basic barcode types (CODE39, CODE128)
   - Check barcode data format requirements

5. **Paper cutting not working**
   - Many thermal printers don't have a paper cutter
   - The cut commands are optional in ESC/POS and may be ignored
   - Use `feed_lines` instead to advance paper for manual cutting

6. **Images not printing**
   - Ensure image width matches printer width (usually 384 dots = 48mm)
   - Images must be in 1-bit black and white format
   - Check that image data is in the correct byte order

### Testing Your Printer

Start with basic text printing to verify connectivity:

```yaml
script:
  - id: test_printer
    then:
      - m5stack_printer.reset:
          id: my_printer
      - delay: 1s
      - m5stack_printer.print_text:
          id: my_printer
          text: "Test Print"
      - m5stack_printer.new_line:
          id: my_printer
          lines: 3
```

### Printer-Specific Notes

**CSN-A2 (Adafruit):**
- Use baud rate 19200
- QR codes and most barcodes supported
- No paper cutter

**DFRobot Thermal Printers:**
- Usually 9600 baud rate
- Basic ESC/POS support
- Check specific model documentation

**Generic ESC/POS Printers:**
- Start with baud rate 9600
- Test basic text first
- Advanced features may not be supported

## Hardware Connections

For M5Stack printers:
- VCC: 5V
- GND: Ground
- TX: Connect to ESP32 RX pin
- RX: Connect to ESP32 TX pin

For other thermal printers, consult the specific datasheet for pinout information.