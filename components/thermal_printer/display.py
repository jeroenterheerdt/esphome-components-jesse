import logging
from esphome import automation
import esphome.codegen as cg
from esphome.components import display, uart
import esphome.config_validation as cv
from esphome.const import CONF_HEIGHT, CONF_ID, CONF_LAMBDA

DEPENDENCIES = ["uart"]

_LOGGER = logging.getLogger(__name__)

thermal_printer_ns = cg.esphome_ns.namespace("thermal_printer")

ThermalPrinterDisplay = thermal_printer_ns.class_(
    "ThermalPrinterDisplay", display.DisplayBuffer, uart.UARTDevice
)
ThermalPrinterPrintTextActionDWDH = thermal_printer_ns.class_(
    "ThermalPrinterPrintTextActionDWDH", automation.Action
)
ThermalPrinterPrintTextActionFWFH = thermal_printer_ns.class_(
    "ThermalPrinterPrintTextActionFWFH", automation.Action
)
ThermalPrinterTabPositionsAction = thermal_printer_ns.class_(
    "ThermalPrinterTabPositionsAction", automation.Action
)
ThermalPrinterRowSpacingAction = thermal_printer_ns.class_(
    "ThermalPrinterRowSpacingAction", automation.Action
)
ThermalPrinterNewLineAction = thermal_printer_ns.class_(
    "ThermalPrinterNewLineAction", automation.Action
)
CONF_FONT_SIZE_FACTOR = "font_size_factor"
CONF_TEXT = "text"
CONF_SEND_WAKEUP = "send_wakeup"
CONF_LINES = "lines"
CONF_ALIGN = "align"
CONF_INVERSE = "inverse"
CONF_90_DEGREE = "ninety_degree"
CONF_UNDERLINE_WEIGHT = "underline_weight"
CONF_UPDOWN = "upside_down"
CONF_BOLD = "bold"
CONF_DOUBLE_WIDTH = "double_width"
CONF_DOUBLE_HEIGHT = "double_height"
CONF_FONT_WIDTH = "font_width"
CONF_FONT_HEIGHT = "font_height"
CONF_FONT = "font"
CONF_STRIKETHROUGH = "strikethrough"
CONF_TAB_POSITIONS = "tab_positions"
CONF_ROW_SPACING = "row_spacing"

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


# PRINT_TEXT() width double height and width bools
@automation.register_action(
    "thermal_printer.print_text",
    ThermalPrinterPrintTextActionDWDH,
    cv.maybe_simple_value(
        cv.Schema(
            {
                cv.GenerateID(): cv.use_id(ThermalPrinterDisplay),
                cv.Required(CONF_TEXT): cv.templatable(cv.string),
                cv.Optional(CONF_ALIGN, default="L"): cv.templatable(
                    cv.one_of("Left", "Center", "Right")
                ),
                cv.Optional(CONF_INVERSE, default=False): cv.templatable(cv.boolean),
                cv.Optional(CONF_90_DEGREE, default=False): cv.templatable(cv.boolean),
                cv.Optional(CONF_UNDERLINE_WEIGHT, default=0): cv.templatable(
                    cv.int_range(0, 2)
                ),
                cv.Optional(CONF_UPDOWN, default=False): cv.templatable(cv.boolean),
                cv.Optional(CONF_BOLD, default=False): cv.templatable(cv.boolean),
                cv.Optional(CONF_DOUBLE_WIDTH, default=False): cv.templatable(
                    cv.boolean
                ),
                cv.Optional(CONF_DOUBLE_HEIGHT, default=False): cv.templatable(
                    cv.boolean
                ),
                cv.Optional(CONF_FONT, default="A"): cv.templatable(
                    cv.one_of("A", "B")
                ),
                cv.Optional(CONF_STRIKETHROUGH, default=False): cv.templatable(
                    cv.boolean
                ),
            }
        ),
        key=CONF_TEXT,
    ),
)
async def thermal_printer_print_text_DW_DH_action_to_code(
    config, action_id, template_arg, args
):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    templ = await cg.templatable(config[CONF_TEXT], args, cg.std_string)
    cg.add(var.set_text(templ))
    templ = await cg.templatable(config[CONF_ALIGN], args, cg.std_string)
    cg.add(var.set_align(templ))
    templ = await cg.templatable(config[CONF_INVERSE], args, cg.bool_)
    cg.add(var.set_inverse(templ))
    templ = await cg.templatable(config[CONF_90_DEGREE], args, cg.bool_)
    cg.add(var.set_ninety_degree(templ))
    templ = await cg.templatable(config[CONF_UNDERLINE_WEIGHT], args, cg.uint8)
    cg.add(var.set_underline_weight(templ))
    templ = await cg.templatable(config[CONF_UPDOWN], args, cg.bool_)
    cg.add(var.set_updown(templ))
    templ = await cg.templatable(config[CONF_BOLD], args, cg.bool_)
    cg.add(var.set_bold(templ))
    templ = await cg.templatable(config[CONF_DOUBLE_WIDTH], args, cg.bool_)
    cg.add(var.set_double_width(templ))
    templ = await cg.templatable(config[CONF_DOUBLE_HEIGHT], args, cg.bool_)
    cg.add(var.set_double_height(templ))
    templ = await cg.templatable(config[CONF_FONT], args, cg.std_string)
    cg.add(var.set_font(templ))
    templ = await cg.templatable(config[CONF_STRIKETHROUGH], args, cg.bool_)
    cg.add(var.set_strikethrough(templ))

    return var


# PRINT_TEXT() width font height and width ints
@automation.register_action(
    "thermal_printer.print_text",
    ThermalPrinterPrintTextActionFWFH,
    cv.maybe_simple_value(
        cv.Schema(
            {
                cv.GenerateID(): cv.use_id(ThermalPrinterDisplay),
                cv.Required(CONF_TEXT): cv.templatable(cv.string),
                cv.Optional(CONF_ALIGN, default="L"): cv.templatable(
                    cv.one_of("Left", "Center", "Right")
                ),
                cv.Optional(CONF_INVERSE, default=False): cv.templatable(cv.boolean),
                cv.Optional(CONF_90_DEGREE, default=False): cv.templatable(cv.boolean),
                cv.Optional(CONF_UNDERLINE_WEIGHT, default=0): cv.templatable(
                    cv.int_range(0, 2)
                ),
                cv.Optional(CONF_UPDOWN, default=False): cv.templatable(cv.boolean),
                cv.Optional(CONF_BOLD, default=False): cv.templatable(cv.boolean),
                cv.Optional(CONF_FONT_WIDTH, default=1): cv.templatable(
                    cv.int_range(1, 8)
                ),
                cv.Optional(CONF_FONT_HEIGHT, default=1): cv.templatable(
                    cv.int_range(1, 8)
                ),
                cv.Optional(CONF_FONT, default="A"): cv.templatable(
                    cv.one_of("A", "B")
                ),
                cv.Optional(CONF_STRIKETHROUGH, default=False): cv.templatable(
                    cv.boolean
                ),
            }
        ),
        key=CONF_TEXT,
    ),
)
async def thermal_printer_print_text_FW_FH_action_to_code(
    config, action_id, template_arg, args
):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    templ = await cg.templatable(config[CONF_TEXT], args, cg.std_string)
    cg.add(var.set_text(templ))
    templ = await cg.templatable(config[CONF_ALIGN], args, cg.std_string)
    cg.add(var.set_align(templ))
    templ = await cg.templatable(config[CONF_INVERSE], args, cg.bool_)
    cg.add(var.set_inverse(templ))
    templ = await cg.templatable(config[CONF_90_DEGREE], args, cg.bool_)
    cg.add(var.set_ninety_degree(templ))
    templ = await cg.templatable(config[CONF_UNDERLINE_WEIGHT], args, cg.uint8)
    cg.add(var.set_underline_weight(templ))
    templ = await cg.templatable(config[CONF_UPDOWN], args, cg.bool_)
    cg.add(var.set_updown(templ))
    templ = await cg.templatable(config[CONF_BOLD], args, cg.bool_)
    cg.add(var.set_bold(templ))
    templ = await cg.templatable(config[CONF_FONT_WIDTH], args, cg.uint8)
    cg.add(var.set_font_width(templ))
    templ = await cg.templatable(config[CONF_FONT_HEIGHT], args, cg.uint8)
    cg.add(var.set_font_height(templ))
    templ = await cg.templatable(config[CONF_FONT], args, cg.std_string)
    cg.add(var.set_font(templ))
    templ = await cg.templatable(config[CONF_STRIKETHROUGH], args, cg.bool_)
    cg.add(var.set_strikethrough(templ))

    return var


# SET TAB POSITIONS()
@automation.register_action(
    "thermal_printer.set_tab_positions",
    ThermalPrinterTabPositionsAction,
    cv.maybe_simple_value(
        cv.Schema(
            {
                cv.GenerateID(): cv.use_id(ThermalPrinterDisplay),
                cv.Required(CONF_TAB_POSITIONS): cv.templatable(
                    cv.ensure_list(cv.int_)
                ),
            }
        ),
        key=CONF_TAB_POSITIONS,
    ),
)
async def thermal_printer_set_tab_positions_action_to_code(
    config, action_id, template_arg, args
):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    templ = await cg.templatable(
        config[CONF_TAB_POSITIONS], args, cg.std_vector(cg.uint8)
    )
    # this fails, not sure why!
    # cg.add(var.set_tabs(templ))
    return var


# SET_ROW_SPACING()
@automation.register_action(
    "thermal_printer.set_row_spacing",
    ThermalPrinterRowSpacingAction,
    cv.maybe_simple_value(
        cv.Schema(
            {
                cv.GenerateID(): cv.use_id(ThermalPrinterDisplay),
                cv.Required(CONF_ROW_SPACING): cv.templatable(cv.int_),
            }
        ),
        key=CONF_LINES,
    ),
)
async def thermal_printer_set_row_spacing_action_to_code(
    config, action_id, template_arg, args
):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    templ = await cg.templatable(config[CONF_ROW_SPACING], args, cg.uint8)
    cg.add(var.set_lines(templ))
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
