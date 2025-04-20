import esphome.codegen as cg

CODEOWNERS = ["@jeroenterheerdt"]
DEPENDENCIES = ["uart", "image"]
MULTI_CONF = True

thermal_printer = cg.esphome_ns.namespace("thermal_printer")
ThermalPrinter = thermal_printer.class_("ThermalPrinter", cg.Component)
