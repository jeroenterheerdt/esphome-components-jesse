import logging
from esphome import automation
import esphome.codegen as cg
from esphome.components import display, uart
import esphome.config_validation as cv
from esphome.const import CONF_HEIGHT, CONF_ID, CONF_LAMBDA

DEPENDENCIES = ["uart"]

_LOGGER = logging.getLogger(__name__)

# todo:
# add other abilities:
# doublewidth mode, bold etc (https://github.com/adafruit/Adafruit-Thermal-Printer-Library/tree/master)
# also check what else the printer can do, maybe cut paper as well? (see datasheet linked here: https://wiki.dfrobot.com/Embedded%20Thermal%20Printer%20-%20TTL%20Serial%20SKU%3A%20DFR0503-EN)
thermal_printer_ns = cg.esphome_ns.namespace("thermal_printer")

ThermalPrinterDisplay = thermal_printer_ns.class_(
    "ThermalPrinterDisplay", display.DisplayBuffer, uart.UARTDevice
)

ThermalPrinterTabAction = thermal_printer_ns.class_(
    "ThermalPrinterTabAction", automation.Action
)
ThermalPrinterSetLineHeightAction = thermal_printer_ns.class_(
    "ThermalPrinterSetLineHeightAction", automation.Action
)
ThermalPrinterPrintTextAction = thermal_printer_ns.class_(
    "ThermalPrinterPrintTextAction", automation.Action
)

ThermalPrinterNewLineAction = thermal_printer_ns.class_(
    "ThermalPrinterNewLineAction", automation.Action
)

ThermalPrinterPrintQRCodeAction = thermal_printer_ns.class_(
    "ThermalPrinterPrintQRCodeAction", automation.Action
)
ThermalPrinterPrintBarCodeAction = thermal_printer_ns.class_(
    "ThermalPrinterPrintBarCodeAction", automation.Action
)
ThermalPrinterBoldOffAction = thermal_printer_ns.class_(
    "ThermalPrinterBoldOffAction", automation.Action
)
ThermalPrinterBoldOnAction = thermal_printer_ns.class_(
    "ThermalPrinterBoldOnAction", automation.Action
)

BARCODETYPE = {
    "UPC_A": ThermalPrinterPrintBarCodeAction.UPC_A,
    "UPC_E": ThermalPrinterPrintBarCodeAction.UPC_E,
    "EAN13": ThermalPrinterPrintBarCodeAction.EAN13,
    "EAN8": ThermalPrinterPrintBarCodeAction.EAN8,
    "CODE39": ThermalPrinterPrintBarCodeAction.CODE39,
    "ITF": ThermalPrinterPrintBarCodeAction.ITF,
    "CODABAR": ThermalPrinterPrintBarCodeAction.CODABAR,
    "CODE93": ThermalPrinterPrintBarCodeAction.CODE93,
    "CODE128": ThermalPrinterPrintBarCodeAction.CODE128,
}

CONF_LINE_HEIGHT = "line_height"
CONF_FONT_SIZE = "font_size"
CONF_FONT_SIZE_FACTOR = "font_size_factor"
CONF_FIRMWARE = "firmware"
CONF_FONT = "font"
CONF_INVERSE = "inverse"
CONF_UPSIDE_DOWN = "upside_down"
CONF_BOLD = "bold"
CONF_DOUBLE_HEIGHT = "double_height"
CONF_DOUBLE_WIDTH = "double_width"
CONF_STRIKETHROUGH = "strikethrough"
CONF_NINETY_DEGREES = "ninety_degrees"
CONF_TEXT = "text"
CONF_SEND_WAKEUP = "send_wakeup"
CONF_LINES = "lines"
CONF_DATA = "qrcode"
CONF_BARCODE = "barcode"
CONF_BARCODE_TYPE = "type"

CONFIG_SCHEMA = (
    display.FULL_DISPLAY_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(ThermalPrinterDisplay),
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
    _LOGGER.debug("wakeup: %s", config[CONF_SEND_WAKEUP])
    cg.add(var.set_send_wakeup(config[CONF_SEND_WAKEUP]))
    # _LOGGER.debug("font size factor: %s", config[CONF_FONT_SIZE_FACTOR])
    # cg.add(var.set_font_size_factor(config[CONF_FONT_SIZE_FACTOR]))

    if lambda_config := config.get(CONF_LAMBDA):
        lambda_ = await cg.process_lambda(
            lambda_config, [(display.DisplayRef, "it")], return_type=cg.void
        )
        cg.add(var.set_writer(lambda_))


# TAB()
@automation.register_action(
    "thermal_printer.tab",
    ThermalPrinterTabAction,
    cv.maybe_simple_value(
        cv.Schema(
            {
                cv.GenerateID(): cv.use_id(ThermalPrinterDisplay),
            }
        ),
        key=cv.GenerateID(),
    ),
)
async def thermal_printer_tab_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var


# SET_LINE_HEIGHT()
@automation.register_action(
    "thermal_printer.set_line_height",
    ThermalPrinterSetLineHeightAction,
    cv.maybe_simple_value(
        cv.Schema(
            {
                cv.GenerateID(): cv.use_id(ThermalPrinterDisplay),
                cv.Optional(CONF_LINE_HEIGHT, default=24): cv.templatable(
                    cv.int_range(min=0, max=24)
                ),
            }
        ),
        key=CONF_LINE_HEIGHT,
    ),
)
async def thermal_printer_set_line_height_action_to_code(
    config, action_id, template_arg, args
):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    templ = await cg.templatable(config[CONF_LINE_HEIGHT], args, cg.uint8)
    cg.add(var.set_line_height(templ))
    return var


# PRINT_TEXT()
@automation.register_action(
    "thermal_printer.print_text",
    ThermalPrinterPrintTextAction,
    cv.maybe_simple_value(
        cv.Schema(
            {
                cv.GenerateID(): cv.use_id(ThermalPrinterDisplay),
                cv.Required(CONF_TEXT): cv.templatable(cv.string),
                # cv.Optional(CONF_FONT_SIZE, default=1): cv.templatable(
                #    cv.int_range(min=0, max=7)
                # ),
                # cv.Optional(CONF_FONT_SIZE_FACTOR): cv.templatable(cv.float_),
                # cv.Optional(CONF_FONT, default="A"): cv.templatable(cv.string),
                # cv.Optional(CONF_INVERSE, default=False): cv.templatable(cv.boolean),
                # cv.Optional(CONF_UPSIDE_DOWN, default=False): cv.templatable(
                #    cv.boolean
                # ),
                # cv.Optional(CONF_BOLD, default=False): cv.templatable(cv.boolean),
                # cv.Optional(CONF_DOUBLE_HEIGHT, default=False): cv.templatable(
                #    cv.boolean
                # ),
                # cv.Optional(CONF_DOUBLE_WIDTH, default=False): cv.templatable(
                #    cv.boolean
                # ),
                # cv.Optional(CONF_STRIKETHROUGH, default=False): cv.templatable(
                #    cv.boolean
                # ),
                # cv.Optional(CONF_NINETY_DEGREES, default=False): cv.templatable(
                #    cv.boolean
                # ),
            }
        ),
        key=CONF_TEXT,
    ),
)
async def thermal_printer_print_text_action_to_code(
    config, action_id, template_arg, args
):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    templ = await cg.templatable(config[CONF_TEXT], args, cg.std_string)
    cg.add(var.set_text(templ))
    # templ = await cg.templatable(config[CONF_FONT_SIZE], args, cg.uint8)
    # cg.add(var.set_font_size(templ))
    # templ = await cg.templatable(config[CONF_FONT], args, cg.std_string)
    # cg.add(var.set_font(templ))
    # templ = await cg.templatable(config[CONF_INVERSE], args, cg.bool_)
    # cg.add(var.set_inverse(templ))
    # templ = await cg.templatable(config[CONF_UPSIDE_DOWN], args, cg.bool_)
    # cg.add(var.set_updown(templ))
    # templ = await cg.templatable(config[CONF_BOLD], args, cg.bool_)
    # cg.add(var.set_bold(templ))
    # templ = await cg.templatable(config[CONF_DOUBLE_HEIGHT], args, cg.bool_)
    # cg.add(var.set_double_height(templ))
    # templ = await cg.templatable(config[CONF_DOUBLE_WIDTH], args, cg.bool_)
    # cg.add(var.set_double_width(templ))
    # templ = await cg.templatable(config[CONF_STRIKETHROUGH], args, cg.bool_)
    # cg.add(var.set_strike(templ))
    # templ = await cg.templatable(config[CONF_NINETY_DEGREES], args, cg.bool_)
    # cg.add(var.set_ninety_degrees(templ))
    return var


# NEW_LINE()
@automation.register_action(
    "thermal_printer.new_line",
    ThermalPrinterNewLineAction,
    cv.maybe_simple_value(
        cv.Schema(
            {
                cv.GenerateID(): cv.use_id(ThermalPrinterDisplay),
                cv.Required(CONF_LINES): cv.templatable(cv.int_),
            }
        ),
        key=CONF_LINES,
    ),
)
async def thermal_printer_new_line_action_to_code(
    config, action_id, template_arg, args
):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    templ = await cg.templatable(config[CONF_LINES], args, cg.uint8)
    cg.add(var.set_lines(templ))
    return var


# PRINT_QR_CODE()
@automation.register_action(
    "thermal_printer.print_qr_code",
    ThermalPrinterPrintQRCodeAction,
    cv.maybe_simple_value(
        cv.Schema(
            {
                cv.GenerateID(): cv.use_id(ThermalPrinterDisplay),
                cv.Required(CONF_DATA): cv.templatable(cg.std_string),
            }
        ),
        key=CONF_DATA,
    ),
)
async def thermal_printer_print_qr_code_action_to_code(
    config, action_id, template_arg, args
):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    templ = await cg.templatable(config[CONF_DATA], args, cg.std_string)
    cg.add(var.set_qrcode(templ))
    return var


# PRINT_BAR_CODE()
@automation.register_action(
    "thermal_printer.print_bar_code",
    ThermalPrinterPrintBarCodeAction,
    cv.maybe_simple_value(
        cv.Schema(
            {
                cv.GenerateID(): cv.use_id(ThermalPrinterDisplay),
                cv.Required(CONF_BARCODE): cv.templatable(cg.std_string),
                cv.Optional(CONF_BARCODE_TYPE, default="UPC_A"): cv.templatable(
                    cv.enum(BARCODETYPE, upper=True)
                ),
            }
        ),
        key=CONF_BARCODE,
    ),
)
async def thermal_printer_print_bar_code_action_to_code(
    config, action_id, template_arg, args
):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    templ = await cg.templatable(config[CONF_BARCODE], args, cg.std_string)
    cg.add(var.set_barcode(templ))
    templ = await cg.templatable(config[CONF_BARCODE_TYPE], args, cg.std_string)
    cg.add(var.set_type(templ))
    return var
