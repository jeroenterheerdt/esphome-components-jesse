from esphome import automation
import esphome.codegen as cg
from esphome.components import display, uart
import esphome.config_validation as cv
from esphome.const import CONF_HEIGHT, CONF_ID, CONF_LAMBDA

DEPENDENCIES = ["uart"]

# todo:
# fix font size factor
# add other abilities such as
# doublewidth mode, bold etc (https://github.com/adafruit/Adafruit-Thermal-Printer-Library/tree/master)
# also check what else the printer can do, maybe cut paper as well? (see datasheet linked here: https://wiki.dfrobot.com/Embedded%20Thermal%20Printer%20-%20TTL%20Serial%20SKU%3A%20DFR0503-EN)
m5stack_printer_ns = cg.esphome_ns.namespace("m5stack_printer")

M5StackPrinterDisplay = m5stack_printer_ns.class_(
    "M5StackPrinterDisplay", display.DisplayBuffer, uart.UARTDevice
)

M5StackPrinterPrintTextAction = m5stack_printer_ns.class_(
    "M5StackPrinterPrintTextAction", automation.Action
)

M5StackPrinterNewLineAction = m5stack_printer_ns.class_(
    "M5StackPrinterNewLineAction", automation.Action
)

M5StackPrinterPrintQRCodeAction = m5stack_printer_ns.class_(
    "M5StackPrinterPrintQRCodeAction", automation.Action
)
M5StackPrinterPrintBarCodeAction = m5stack_printer_ns.class_(
    "M5StackPrinterPrintBarCodeAction", automation.Action
)
M5StackPrinterBoldOffAction = m5stack_printer_ns.class_(
    "M5StackPrinterBoldOffAction", automation.Action
)
M5StackPrinterBoldOnAction = m5stack_printer_ns.class_(
    "M5StackPrinterBoldOnAction", automation.Action
)

BARCODETYPE = {
    "UPC_A": M5StackPrinterPrintBarCodeAction.UPC_A,
    "UPC_E": M5StackPrinterPrintBarCodeAction.UPC_E,
    "EAN13": M5StackPrinterPrintBarCodeAction.EAN13,
    "EAN8": M5StackPrinterPrintBarCodeAction.EAN8,
    "CODE39": M5StackPrinterPrintBarCodeAction.CODE39,
    "ITF": M5StackPrinterPrintBarCodeAction.ITF,
    "CODABAR": M5StackPrinterPrintBarCodeAction.CODABAR,
    "CODE93": M5StackPrinterPrintBarCodeAction.CODE93,
    "CODE128": M5StackPrinterPrintBarCodeAction.CODE128,
}
CONF_FONT_SIZE = "font_size"
CONF_FONT_SIZE_FACTOR = "font_size_factor"
CONF_TEXT = "text"
CONF_SEND_WAKEUP = "send_wakeup"
CONF_LINES = "lines"
CONF_DATA = "qrcode"
CONF_BARCODE = "barcode"
CONF_BARCODE_TYPE = "type"

CONFIG_SCHEMA = (
    display.FULL_DISPLAY_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(M5StackPrinterDisplay),
            cv.Required(CONF_HEIGHT): cv.uint16_t,
            cv.Optional(CONF_SEND_WAKEUP, default=False): cv.boolean,
            cv.Optional(CONF_FONT_SIZE_FACTOR, default=1.0): cv.float_,
        }
    )
    .extend(
        cv.polling_component_schema("never")
    )  # This component should always be manually updated with actions
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await display.register_display(var, config)
    await uart.register_uart_device(var, config)

    cg.add(var.set_height(config[CONF_HEIGHT]))
    cg.add(var.set_send_wakeup(config[CONF_SEND_WAKEUP]))
    cg.add(var.set_font_size_factor(config[CONF_FONT_SIZE_FACTOR]))

    if lambda_config := config.get(CONF_LAMBDA):
        lambda_ = await cg.process_lambda(
            lambda_config, [(display.DisplayRef, "it")], return_type=cg.void
        )
        cg.add(var.set_writer(lambda_))


@automation.register_action(
    "m5stack_printer.print_text",
    M5StackPrinterPrintTextAction,
    cv.maybe_simple_value(
        cv.Schema(
            {
                cv.GenerateID(): cv.use_id(M5StackPrinterDisplay),
                cv.Required(CONF_TEXT): cv.templatable(cv.string),
                cv.Optional(CONF_FONT_SIZE, default=1): cv.templatable(
                    cv.int_range(min=0, max=7)
                ),
            }
        ),
        key=CONF_TEXT,
    ),
)
async def m5stack_printer_print_text_action_to_code(
    config, action_id, template_arg, args
):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    templ = await cg.templatable(config[CONF_TEXT], args, cg.std_string)
    cg.add(var.set_text(templ))
    templ = await cg.templatable(config[CONF_FONT_SIZE], args, cg.uint8)
    cg.add(var.set_font_size(templ))
    return var


@automation.register_action(
    "m5stack_printer.new_line",
    M5StackPrinterNewLineAction,
    cv.maybe_simple_value(
        cv.Schema(
            {
                cv.GenerateID(): cv.use_id(M5StackPrinterDisplay),
                cv.Required(CONF_LINES): cv.templatable(cv.int_),
            }
        ),
        key=CONF_LINES,
    ),
)
async def m5stack_printer_new_line_action_to_code(
    config, action_id, template_arg, args
):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    templ = await cg.templatable(config[CONF_LINES], args, cg.uint8)
    cg.add(var.set_lines(templ))
    return var


@automation.register_action(
    "m5stack_printer.print_qr_code",
    M5StackPrinterPrintQRCodeAction,
    cv.maybe_simple_value(
        cv.Schema(
            {
                cv.GenerateID(): cv.use_id(M5StackPrinterDisplay),
                cv.Required(CONF_DATA): cv.templatable(cg.std_string),
            }
        ),
        key=CONF_DATA,
    ),
)
async def m5stack_printer_print_qr_code_action_to_code(
    config, action_id, template_arg, args
):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    templ = await cg.templatable(config[CONF_DATA], args, cg.std_string)
    cg.add(var.set_qrcode(templ))
    return var


@automation.register_action(
    "m5stack_printer.print_bar_code",
    M5StackPrinterPrintBarCodeAction,
    cv.maybe_simple_value(
        cv.Schema(
            {
                cv.GenerateID(): cv.use_id(M5StackPrinterDisplay),
                cv.Required(CONF_BARCODE): cv.templatable(cg.std_string),
                cv.Optional(CONF_BARCODE_TYPE, default="UPC_A"): cv.templatable(
                    cv.enum(BARCODETYPE, upper=True)
                ),
            }
        ),
        key=CONF_BARCODE,
    ),
)
async def m5stack_printer_print_bar_code_action_to_code(
    config, action_id, template_arg, args
):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    templ = await cg.templatable(config[CONF_BARCODE], args, cg.std_string)
    cg.add(var.set_barcode(templ))
    templ = await cg.templatable(config[CONF_BARCODE_TYPE], args, cg.std_string)
    cg.add(var.set_type(templ))
    return var


@automation.register_action(
    "m5stack_printer.bold_off",
    M5StackPrinterBoldOffAction,
    cv.maybe_simple_value(
        cv.Schema(
            {
                cv.GenerateID(): cv.use_id(M5StackPrinterDisplay),
            }
        ),
    ),
)
async def m5stack_printer_bold_off(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var


@automation.register_action(
    "m5stack_printer.bold_on",
    M5StackPrinterBoldOnAction,
    cv.maybe_simple_value(
        cv.Schema(
            {
                cv.GenerateID(): cv.use_id(M5StackPrinterDisplay),
            }
        ),
    ),
)
async def m5stack_printer_bold_on(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var
