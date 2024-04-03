
import re
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_TRIGGER_ID
from esphome.core import coroutine, Lambda
from esphome.components import esp32_ble_server
from esphome.cpp_generator import MockObj
from esphome.automation import LambdaAction
from esphome import automation


# from esphome.core import coroutine_with_priority, coroutine, Lambda
# from esphome.cpp_generator import MockObj
# from esphome.components import ble, sensor


Creavision_BLE_ns = cg.esphome_ns.namespace('Creavision_BLE')
CreavisionBLE = Creavision_BLE_ns.class_('ESP32BLEController', cg.Component, cg.Controller)


BLEControllerCustomCommandExecutionTrigger = Creavision_BLE_ns.class_('BLEControllerCustomCommandExecutionTrigger', automation.Trigger.template())


CONF_SECURITY_MODE = 'security_mode'
BLESecurityMode = Creavision_BLE_ns.enum("BLESecurityMode", is_class = True)
CONF_SECURITY_MODE_NONE = 'none' # no security, no bonding
CONF_SECURITY_MODE_BOND = 'bond' # no secure connection, no man-in-the-middle protection, just bonding (pairing)
CONF_SECURITY_MODE_SECURE = 'secure' # default: bonding, secure connection, MITM protection
SECURTY_MODE_OPTIONS = {
    CONF_SECURITY_MODE_NONE: BLESecurityMode.NONE,
    CONF_SECURITY_MODE_BOND: BLESecurityMode.BOND,
    CONF_SECURITY_MODE_SECURE: BLESecurityMode.SECURE,
}


CONF_ON_SHOW_PASS_KEY = "on_show_pass_key"
BLEControllerShowPassKeyTrigger = Creavision_BLE_ns.class_('BLEControllerShowPassKeyTrigger', automation.Trigger.template())

CONF_ON_AUTHENTICATION_COMPLETE = "on_authentication_complete"
BLEControllerAuthenticationCompleteTrigger = Creavision_BLE_ns.class_('BLEControllerAuthenticationCompleteTrigger', automation.Trigger.template())

# CONF_RECEIVE_DATA = "receive_data"
# BLEControllerServerShowReceiveDataTrigger = Creavision_BLE_ns.class_('BLEControllerServerShowReceiveDataTrigger', automation.Trigger.template())

def forbid_config_setting_for_automation(automation_id, setting_key, forbidden_setting_value, config):
    """Validates that a given automation is only present if a given setting does not have a given value."""
    if automation_id in config and config[setting_key] == forbidden_setting_value:
        raise cv.Invalid("Automation '" + automation_id + "' not available if " + setting_key + " = " + forbidden_setting_value)

def automations_available(config):
    """Validates that the security related automations are only present if the security mode is not none."""
    forbid_config_setting_for_automation(CONF_ON_SHOW_PASS_KEY, CONF_SECURITY_MODE, CONF_SECURITY_MODE_NONE, config)
    forbid_config_setting_for_automation(CONF_ON_AUTHENTICATION_COMPLETE, CONF_SECURITY_MODE, CONF_SECURITY_MODE_NONE, config)
    # forbid_config_setting_for_automation(CONF_RECEIVE_DATA, CONF_SECURITY_MODE, CONF_SECURITY_MODE_NONE, config)
    return config

def require_automation_for_config_setting(automation_id, setting_key, requiring_setting_value, config):
    """Validates that a given automation is only present if a given setting does not have a given value."""
    if config[setting_key] == requiring_setting_value and not automation_id in config:
        raise cv.Invalid("Automation '" + automation_id + "' required if " + setting_key + " = " + requiring_setting_value)

def required_automations_present(config):
    """Validates that the pass key related automation is present if the security mode is set to secure."""
    require_automation_for_config_setting(CONF_ON_SHOW_PASS_KEY, CONF_SECURITY_MODE, CONF_SECURITY_MODE_SECURE, config)
    # require_automation_for_config_setting(CONF_RECEIVE_DATA, CONF_SECURITY_MODE, CONF_SECURITY_MODE_SECURE, config)
    return config

CONF_ON_SERVER_CONNECTED = "on_connected"
BLEControllerServerConnectedTrigger = Creavision_BLE_ns.class_('BLEControllerServerConnectedTrigger', automation.Trigger.template())

CONF_ON_SERVER_DISCONNECTED = "on_disconnected"
BLEControllerServerDisconnectedTrigger = Creavision_BLE_ns.class_('BLEControllerServerDisconnectedTrigger', automation.Trigger.template())


CONFIG_SCHEMA = cv.All(cv.only_on_esp32, cv.only_with_arduino, cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(CreavisionBLE),
            cv.Optional(CONF_SECURITY_MODE, default=CONF_SECURITY_MODE_SECURE): cv.enum(SECURTY_MODE_OPTIONS),
            cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(BLEControllerCustomCommandExecutionTrigger),
            
            cv.Optional(CONF_SECURITY_MODE, default=CONF_SECURITY_MODE_SECURE): cv.enum(SECURTY_MODE_OPTIONS),
            
            cv.Optional(CONF_ON_SHOW_PASS_KEY): automation.validate_automation({
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(BLEControllerShowPassKeyTrigger),
            }),

            cv.Optional(CONF_ON_AUTHENTICATION_COMPLETE): automation.validate_automation({
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(BLEControllerAuthenticationCompleteTrigger),
            }),

            cv.Optional(CONF_ON_SERVER_CONNECTED): automation.validate_automation({
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(BLEControllerServerConnectedTrigger),
            }),

            cv.Optional(CONF_ON_SERVER_DISCONNECTED): automation.validate_automation({
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(BLEControllerServerDisconnectedTrigger),
            }),

            # cv.Optional(CONF_RECEIVE_DATA): automation.validate_automation({
            #     cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(BLEControllerServerShowReceiveDataTrigger),
            # }),
        }
    ), automations_available, required_automations_present 
)



# @coroutine
def to_code(config):
    
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)

    security_enabled = SECURTY_MODE_OPTIONS[config[CONF_SECURITY_MODE]]
    cg.add(var.set_security_mode(config[CONF_SECURITY_MODE]))

    for conf in config.get(CONF_ON_SHOW_PASS_KEY, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        yield automation.build_automation(trigger, [(cg.std_string, 'pass_key')], conf)

    # for conf in config.get(CONF_RECEIVE_DATA, []):
    #     trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
    #     yield automation.build_automation(trigger, [(cg.std_string, 'received_data')], conf)

    for conf in config.get(CONF_ON_AUTHENTICATION_COMPLETE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        yield automation.build_automation(trigger, [(cg.bool_, 'success')], conf)

    for conf in config.get(CONF_ON_SERVER_CONNECTED, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        yield automation.build_automation(trigger, [], conf)

    for conf in config.get(CONF_ON_SERVER_DISCONNECTED, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        yield automation.build_automation(trigger, [], conf)
  
 

    cg.add_library("ESP32 BLE Arduino", "2.0.0")

