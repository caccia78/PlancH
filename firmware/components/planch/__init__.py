# PlancH - external component with the menu model (menu.h), used by planch/core.yaml.
# A component instead of "esphome: includes" so that the firmware also works as a remote package
# (dashboard_import): includes are resolved next to the user's file, components are not.
# ESPHome adds menu.h to esphome.h, so the lambdas of the YAML files see planch::menu().
import esphome.config_validation as cv

CODEOWNERS = ["@caccia78"]
AUTO_LOAD = ["json"]   # ArduinoJson

CONFIG_SCHEMA = cv.Schema({})


async def to_code(config):
    pass
