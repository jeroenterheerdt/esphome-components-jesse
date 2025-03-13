from esphome import automation
import esphome.codegen as cg
from esphome.components import display, uart
import esphome.config_validation as cv
from esphome.const import CONF_HEIGHT, CONF_ID, CONF_LAMBDA

DEPENDENCIES = ["uart"]

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

CONF_FONT_SIZE = "font_size"
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
                cv.Required(CONF_LINES): cv.templatable(cg.uint8),
            }
        ), key=CONF_LINES,
    ),
)
async def m5stack_printer_new_line_action_to_code(
    config, action_id, template_arg, args
):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    templ = await cg.templatable(config[CONF_LINES], args, cg.uint8)
    cg.add(var.set_lines(templ))
    return 


@automation.register_action(
    "m5stack_printer.print_qr_code",
    M5StackPrinterPrintQRCodeAction,
    cv.maybe_simple_value(
        cv.Schema(
            {
                cv.GenerateID(): cv.use_id(M5StackPrinterDisplay),
                cv.Required(CONF_DATA): cv.templatable(cg.std_string),
            }
        ), key=CONF_DATA,
    ),
)
async def m5stack_printer_print_qr_code_action_to_code(
    config, action_id, template_arg, args
):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    templ = await cg.templatable(config[CONF_DATA], args, cg.std_string)
    cg.add(var.set_data(templ))
    return var


@automation.register_action(
    "m5stack_printer.print_bar_code",
    M5StackPrinterPrintBarCodeAction,
    cv.maybe_simple_value(
        cv.Schema(
            {
                cv.GenerateID(): cv.use_id(M5StackPrinterDisplay),
                cv.Required(CONF_BARCODE): cv.templatable(cg.std_string),
                cv.Required(CONF_BARCODE_TYPE): cv.templatable(cg.std_string),
            }
        ), key=CONF_BARCODE,
    ),
)
async def m5stack_printer_print_bar_code_action_to_code(
    config, action_id, template_arg, args
):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    templ = await cg.templatable(config[CONF_BARCODE], args, cg.std_string)
    cg.add(var.set_barcode(templ))
    templ = await cg.templatable(
        config[CONF_BARCODE_TYPE], args, cg.std_string
    )
    cg.add(var.set_type(templ))
    return var

