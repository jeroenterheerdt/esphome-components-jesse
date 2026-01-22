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

M5StackPrinterPrintQRCodeAction = m5stack_printer_ns.class_(
    "M5StackPrinterPrintQRCodeAction", automation.Action
)

M5StackPrinterPrintBarcodeAction = m5stack_printer_ns.class_(
    "M5StackPrinterPrintBarcodeAction", automation.Action
)

M5StackPrinterNewLineAction = m5stack_printer_ns.class_(
    "M5StackPrinterNewLineAction", automation.Action
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

M5StackPrinterSet90DegreeRotationAction = m5stack_printer_ns.class_(
    "M5StackPrinterSet90DegreeRotationAction", automation.Action
)

M5StackPrinterSetInversePrintingAction = m5stack_printer_ns.class_(
    "M5StackPrinterSetInversePrintingAction", automation.Action
)

M5StackPrinterSetStrikethroughAction = m5stack_printer_ns.class_(
    "M5StackPrinterSetStrikethroughAction", automation.Action
)

M5StackPrinterSetChineseModeAction = m5stack_printer_ns.class_(
    "M5StackPrinterSetChineseModeAction", automation.Action
)

M5StackPrinterSetLineSpacingAction = m5stack_printer_ns.class_(
    "M5StackPrinterSetLineSpacingAction", automation.Action
)

M5StackPrinterSetPrintDensityAction = m5stack_printer_ns.class_(
    "M5StackPrinterSetPrintDensityAction", automation.Action
)

M5StackPrinterSetTabPositionsAction = m5stack_printer_ns.class_(
    "M5StackPrinterSetTabPositionsAction", automation.Action
)

M5StackPrinterHorizontalTabAction = m5stack_printer_ns.class_(
    "M5StackPrinterHorizontalTabAction", automation.Action
)

M5StackPrinterSetHorizontalPositionAction = m5stack_printer_ns.class_(
    "M5StackPrinterSetHorizontalPositionAction", automation.Action
)

M5StackPrinterPrintTestPageAction = m5stack_printer_ns.class_(
    "M5StackPrinterPrintTestPageAction", automation.Action
)

M5StackPrinterRunDemoAction = m5stack_printer_ns.class_(
    "M5StackPrinterRunDemoAction", automation.Action
)

M5StackPrinterSendRawCommandAction = m5stack_printer_ns.class_(
    "M5StackPrinterSendRawCommandAction", automation.Action
)

CONF_FONT_SIZE = "font_size"
CONF_TEXT = "text"
CONF_QRCODE = "qrcode"
CONF_BARCODE = "barcode"
CONF_TYPE = "type"
CONF_LINES = "lines"
CONF_CUT_MODE = "cut_mode"
CONF_FEED_LINES = "feed_lines"
CONF_ALIGNMENT = "alignment"
CONF_BOLD = "bold"
CONF_UNDERLINE = "underline"
CONF_DOUBLE_WIDTH = "double_width"
CONF_UPSIDE_DOWN = "upside_down"
CONF_ENABLE = "enable"
CONF_SPACING = "spacing"
CONF_DENSITY = "density"
CONF_BREAK_TIME = "break_time"
CONF_POSITIONS = "positions"
CONF_POSITION = "position"
CONF_SHOW_QR_CODE = "show_qr_code"
CONF_SHOW_BARCODE = "show_barcode"
CONF_SHOW_TEXT_STYLES = "show_text_styles"
CONF_SHOW_INVERSE = "show_inverse"
CONF_SHOW_ROTATION = "show_rotation"

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
    "m5stack_printer.print_qrcode",
    M5StackPrinterPrintQRCodeAction,
    cv.maybe_simple_value(
        cv.Schema(
            {
                cv.GenerateID(): cv.use_id(M5StackPrinterDisplay),
                cv.Required(CONF_QRCODE): cv.templatable(cv.string),
            }
        ),
        key=CONF_QRCODE,
    ),
)
async def m5stack_printer_print_qrcode_action_to_code(
    config, action_id, template_arg, args
):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    templ = await cg.templatable(config[CONF_QRCODE], args, cg.std_string)
    cg.add(var.set_qrcode(templ))
    return var


@automation.register_action(
    "m5stack_printer.print_barcode",
    M5StackPrinterPrintBarcodeAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(M5StackPrinterDisplay),
            cv.Required(CONF_BARCODE): cv.templatable(cv.string),
            cv.Required(CONF_TYPE): cv.templatable(cv.string),
        }
    ),
)
async def m5stack_printer_print_barcode_action_to_code(
    config, action_id, template_arg, args
):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    templ = await cg.templatable(config[CONF_BARCODE], args, cg.std_string)
    cg.add(var.set_barcode(templ))
    templ = await cg.templatable(config[CONF_TYPE], args, cg.std_string)
    cg.add(var.set_type(templ))
    return var


@automation.register_action(
    "m5stack_printer.new_line",
    M5StackPrinterNewLineAction,
    cv.maybe_simple_value(
        cv.Schema(
            {
                cv.GenerateID(): cv.use_id(M5StackPrinterDisplay),
                cv.Required(CONF_LINES): cv.templatable(cv.int_range(min=1, max=255)),
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
    "m5stack_printer.set_90_degree_rotation",
    M5StackPrinterSet90DegreeRotationAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(M5StackPrinterDisplay),
            cv.Required(CONF_ENABLE): cv.templatable(cv.boolean),
        }
    ),
)
async def m5stack_printer_set_90_degree_rotation_action_to_code(
    config, action_id, template_arg, args
):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    templ = await cg.templatable(config[CONF_ENABLE], args, cg.bool_)
    cg.add(var.set_enable(templ))
    return var


@automation.register_action(
    "m5stack_printer.set_inverse_printing",
    M5StackPrinterSetInversePrintingAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(M5StackPrinterDisplay),
            cv.Required(CONF_ENABLE): cv.templatable(cv.boolean),
        }
    ),
)
async def m5stack_printer_set_inverse_printing_action_to_code(
    config, action_id, template_arg, args
):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    templ = await cg.templatable(config[CONF_ENABLE], args, cg.bool_)
    cg.add(var.set_enable(templ))
    return var


@automation.register_action(
    "m5stack_printer.set_strikethrough",
    M5StackPrinterSetStrikethroughAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(M5StackPrinterDisplay),
            cv.Required(CONF_ENABLE): cv.templatable(cv.boolean),
        }
    ),
)
async def m5stack_printer_set_strikethrough_to_code(
    config, action_id, template_arg, args
):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    templ = await cg.templatable(config[CONF_ENABLE], args, cg.bool_)
    cg.add(var.set_enable(templ))
    return var


# Raw command action
M5StackPrinterSendRawCommandAction = m5stack_printer_ns.class_(
    "M5StackPrinterSendRawCommandAction", automation.Action
)


@automation.register_action(
    "m5stack_printer.send_raw_command",
    M5StackPrinterSendRawCommandAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(M5StackPrinterDisplay),
            cv.Required("command"): cv.templatable(cv.string),
        }
    ),
)
async def m5stack_printer_send_raw_command_to_code(
    config, action_id, template_arg, args
):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])

    # Set the templatable command string
    command_template = await cg.templatable(config["command"], args, cg.std_string)
    cg.add(var.set_command(command_template))

    return var


@automation.register_action(
    "m5stack_printer.set_chinese_mode",
    M5StackPrinterSetChineseModeAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(M5StackPrinterDisplay),
            cv.Required(CONF_ENABLE): cv.templatable(cv.boolean),
        }
    ),
)
async def m5stack_printer_set_chinese_mode_action_to_code(
    config, action_id, template_arg, args
):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    templ = await cg.templatable(config[CONF_ENABLE], args, cg.bool_)
    cg.add(var.set_enable(templ))
    return var


@automation.register_action(
    "m5stack_printer.set_line_spacing",
    M5StackPrinterSetLineSpacingAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(M5StackPrinterDisplay),
            cv.Required(CONF_SPACING): cv.templatable(cv.int_range(min=0, max=255)),
        }
    ),
)
async def m5stack_printer_set_line_spacing_action_to_code(
    config, action_id, template_arg, args
):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    templ = await cg.templatable(config[CONF_SPACING], args, cg.uint8)
    cg.add(var.set_spacing(templ))
    return var


@automation.register_action(
    "m5stack_printer.set_print_density",
    M5StackPrinterSetPrintDensityAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(M5StackPrinterDisplay),
            cv.Required(CONF_DENSITY): cv.templatable(cv.int_range(min=0, max=31)),
            cv.Optional(CONF_BREAK_TIME, default=0): cv.templatable(
                cv.int_range(min=0, max=7)
            ),
        }
    ),
)
async def m5stack_printer_set_print_density_action_to_code(
    config, action_id, template_arg, args
):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    templ = await cg.templatable(config[CONF_DENSITY], args, cg.uint8)
    cg.add(var.set_density(templ))
    templ = await cg.templatable(config[CONF_BREAK_TIME], args, cg.uint8)
    cg.add(var.set_break_time(templ))
    return var


@automation.register_action(
    "m5stack_printer.set_tab_positions",
    M5StackPrinterSetTabPositionsAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(M5StackPrinterDisplay),
            cv.Required(CONF_POSITIONS): cv.templatable(cv.string),
        }
    ),
)
async def m5stack_printer_set_tab_positions_action_to_code(
    config, action_id, template_arg, args
):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    templ = await cg.templatable(config[CONF_POSITIONS], args, cg.std_string)
    cg.add(var.set_positions(templ))
    return var


@automation.register_action(
    "m5stack_printer.horizontal_tab",
    M5StackPrinterHorizontalTabAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(M5StackPrinterDisplay),
        }
    ),
)
async def m5stack_printer_horizontal_tab_action_to_code(
    config, action_id, template_arg, args
):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var


@automation.register_action(
    "m5stack_printer.set_horizontal_position",
    M5StackPrinterSetHorizontalPositionAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(M5StackPrinterDisplay),
            cv.Required(CONF_POSITION): cv.templatable(cv.int_range(min=0, max=383)),
        }
    ),
)
async def m5stack_printer_set_horizontal_position_action_to_code(
    config, action_id, template_arg, args
):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    templ = await cg.templatable(config[CONF_POSITION], args, cg.uint16)
    cg.add(var.set_position(templ))
    return var


@automation.register_action(
    "m5stack_printer.cut_paper",
    M5StackPrinterCutPaperAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(M5StackPrinterDisplay),
            cv.Optional(CONF_CUT_MODE, default=0): cv.templatable(
                cv.int_range(min=0, max=66)
            ),
            cv.Optional(CONF_FEED_LINES, default=0): cv.templatable(
                cv.int_range(min=0, max=255)
            ),
        }
    ),
)
async def m5stack_printer_cut_paper_action_to_code(
    config, action_id, template_arg, args
):
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
            cv.Required(CONF_ALIGNMENT): cv.templatable(
                cv.int_range(min=0, max=2)
            ),  # 0=left, 1=center, 2=right
        }
    ),
)
async def m5stack_printer_set_alignment_action_to_code(
    config, action_id, template_arg, args
):
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
            cv.Optional(CONF_UNDERLINE, default=0): cv.templatable(
                cv.int_range(min=0, max=2)
            ),
            cv.Optional(CONF_DOUBLE_WIDTH, default=False): cv.templatable(cv.boolean),
            cv.Optional(CONF_UPSIDE_DOWN, default=False): cv.templatable(cv.boolean),
        }
    ),
)
async def m5stack_printer_set_style_action_to_code(
    config, action_id, template_arg, args
):
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
async def m5stack_printer_print_test_page_action_to_code(
    config, action_id, template_arg, args
):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var


@automation.register_action(
    "m5stack_printer.run_demo",
    M5StackPrinterRunDemoAction,
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(M5StackPrinterDisplay),
            cv.Optional(CONF_SHOW_QR_CODE, default=False): cv.templatable(cv.boolean),
            cv.Optional(CONF_SHOW_BARCODE, default=False): cv.templatable(cv.boolean),
            cv.Optional(CONF_SHOW_TEXT_STYLES, default=False): cv.templatable(
                cv.boolean
            ),
            cv.Optional(CONF_SHOW_INVERSE, default=False): cv.templatable(cv.boolean),
            cv.Optional(CONF_SHOW_ROTATION, default=False): cv.templatable(cv.boolean),
        }
    ),
)
async def m5stack_printer_run_demo_action_to_code(
    config, action_id, template_arg, args
):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])

    templ = await cg.templatable(config[CONF_SHOW_QR_CODE], args, cg.bool_)
    cg.add(var.set_show_qr_code(templ))
    templ = await cg.templatable(config[CONF_SHOW_BARCODE], args, cg.bool_)
    cg.add(var.set_show_barcode(templ))
    templ = await cg.templatable(config[CONF_SHOW_TEXT_STYLES], args, cg.bool_)
    cg.add(var.set_show_text_styles(templ))
    templ = await cg.templatable(config[CONF_SHOW_INVERSE], args, cg.bool_)
    cg.add(var.set_show_inverse(templ))
    templ = await cg.templatable(config[CONF_SHOW_ROTATION], args, cg.bool_)
    cg.add(var.set_show_rotation(templ))

    return var
