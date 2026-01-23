you are helping me write a esphome component and esphome yaml configuration for an esp32 to communicate with a thermal printer.
The datasheets folder contains markdown versions of datasheets that I think are for this printer.
Also, I found this library (https://github.com/adafruit/Adafruit-Thermal-Printer-Library) that contains Arduino code that seems to work for most functionalities with this printer, so make sure to review that as well.

Don't run compilations or uploads to the device, I'd like to run it myself.

In the end, I want to support all possible features that this printer offers, and I want to have services exposed to home assistant for easy interaction with the printer.

I think we should focus on:
- printing text, which includes formatting such as
    - alignment (left,center, right)
    - font size
    - double width - or is that just font size?
    - double height - or is that just font size?
    - font A / B
    - strikethrough
    - inverse
    - upside down
    - 90 degrees
    - bold
    - underline (all types)
    - character sets
- other features, such as:
    - bitmaps - I think this already works in the code that's there
    - qr codes - I think this already works in the code.
    - cutting paper
    - beep?
    - whatever else you find in the datasheets
- in the end I want also a method that provides a demo. I think we already have a method for it but it needs to be expanded with nerdy examples with references to Hitchhikers guide to the galaxy for any feature that ends up working.