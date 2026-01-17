import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.components import display, uart
from esphome.const import CONF_HEIGHT, CONF_ID, CONF_LAMBDA
from esphome import automation

DEPENDENCIES = ["uart"]

m5stack_printer_ns = cg.esphome_ns.namespace("m5stack_printer")

M5StackPrinterDisplay = m5stack_printer_ns.class_(
    "M5StackPrinterDisplay", display.DisplayBuffer, uart.UARTDevice
)

M5StackPrinterPrintTextAction = m5stack_printer_ns.class_(
    "M5StackPrinterPrintTextAction", automation.Action
)

# Additional action classes for new features
M5StackPrinterCutPaperAction = m5stack_printer_ns.class_(
    "M5StackPrinterCutPaperAction", automation.Action
)

M5StackPrinterSetAlignmentAction = m5stack_printer_ns.class_(
    "M5StackPrinterSetAlignmentAction", automation.Action
)

M5StackPrinterSetStyleAction = m5stack_printer_ns.class_(
    "M5StackPrinterSetStyleAction", automation.Action
)

M5StackPrinterPrintTestPageAction = m5stack_printer_ns.class_(
    "M5StackPrinterPrintTestPageAction", automation.Action
)

CONF_FONT_SIZE = "font_size"
CONF_TEXT = "text"
CONF_CUT_MODE = "cut_mode"
CONF_FEED_LINES = "feed_lines" 
CONF_ALIGNMENT = "alignment"
CONF_BOLD = "bold"
CONF_UNDERLINE = "underline"
CONF_DOUBLE_WIDTH = "double_width"
CONF_UPSIDE_DOWN = "upside_down"

CONFIG_SCHEMA = (
    display.FULL_DISPLAY_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(M5StackPrinterDisplay),
            cv.Required(CONF_HEIGHT): cv.uint16_t,
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
                cv.Optional(CONF_FONT_SIZE, default=0): cv.templatable(
                    cv.int_range(min=0, max=7)  # Font size range according to datasheet
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
    "m5stack_printer.cut_paper",
    M5StackPrinterCutPaperAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(M5StackPrinterDisplay),
            cv.Optional(CONF_CUT_MODE, default=0): cv.templatable(cv.int_range(min=0, max=66)),
            cv.Optional(CONF_FEED_LINES, default=0): cv.templatable(cv.int_range(min=0, max=255)),
        }
    ),
)
async def m5stack_printer_cut_paper_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    templ = await cg.templatable(config[CONF_CUT_MODE], args, cg.uint8)
    cg.add(var.set_cut_mode(templ))
    templ = await cg.templatable(config[CONF_FEED_LINES], args, cg.uint8)
    cg.add(var.set_feed_lines(templ))
    return var


@automation.register_action(
    "m5stack_printer.set_alignment",
    M5StackPrinterSetAlignmentAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(M5StackPrinterDisplay),
            cv.Required(CONF_ALIGNMENT): cv.templatable(cv.int_range(min=0, max=2)),  # 0=left, 1=center, 2=right
        }
    ),
)
async def m5stack_printer_set_alignment_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    templ = await cg.templatable(config[CONF_ALIGNMENT], args, cg.uint8)
    cg.add(var.set_alignment(templ))
    return var


@automation.register_action(
    "m5stack_printer.set_text_style",
    M5StackPrinterSetStyleAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(M5StackPrinterDisplay),
            cv.Optional(CONF_BOLD, default=False): cv.templatable(cv.boolean),
            cv.Optional(CONF_UNDERLINE, default=0): cv.templatable(cv.int_range(min=0, max=2)),
            cv.Optional(CONF_DOUBLE_WIDTH, default=False): cv.templatable(cv.boolean),
            cv.Optional(CONF_UPSIDE_DOWN, default=False): cv.templatable(cv.boolean),
        }
    ),
)
async def m5stack_printer_set_style_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    templ = await cg.templatable(config[CONF_BOLD], args, cg.bool_)
    cg.add(var.set_bold(templ))
    templ = await cg.templatable(config[CONF_UNDERLINE], args, cg.uint8)
    cg.add(var.set_underline(templ))
    templ = await cg.templatable(config[CONF_DOUBLE_WIDTH], args, cg.bool_)
    cg.add(var.set_double_width(templ))
    templ = await cg.templatable(config[CONF_UPSIDE_DOWN], args, cg.bool_)
    cg.add(var.set_upside_down(templ))
    return var


@automation.register_action(
    "m5stack_printer.print_test_page",
    M5StackPrinterPrintTestPageAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(M5StackPrinterDisplay),
        }
    ),
)
async def m5stack_printer_print_test_page_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var
