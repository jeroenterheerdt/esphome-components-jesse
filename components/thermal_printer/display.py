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
ThermalPrinterPrintTextAction = thermal_printer_ns.class_(
    "ThermalPrinterPrintTextAction", automation.Action
)

ThermalPrinterNewLineAction = thermal_printer_ns.class_(
    "ThermalPrinterNewLineAction", automation.Action
)
CONF_FONT_SIZE_FACTOR = "font_size_factor"
CONF_TEXT = "text"
CONF_SEND_WAKEUP = "send_wakeup"
CONF_LINES = "lines"

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


# PRINT_TEXT()
@automation.register_action(
    "thermal_printer.print_text",
    ThermalPrinterPrintTextAction,
    cv.maybe_simple_value(
        cv.Schema(
            {
                cv.GenerateID(): cv.use_id(ThermalPrinterDisplay),
                cv.Required(CONF_TEXT): cv.templatable(cv.string),
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
