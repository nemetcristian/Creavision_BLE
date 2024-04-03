#pragma once

#include <memory>

#include "esphome/core/automation.h"
#include "esphome/core/optional.h"

#include "Creavision_BLE.h"

using std::shared_ptr;

namespace esphome {
namespace Creavision_BLE {

// authentication ///////////////////////////////////////////////////////////////////////////////////////////////

// / Trigger for showing the pass key during authentication with a client.
class BLEControllerShowPassKeyTrigger : public Trigger<std::string> {
public:
  BLEControllerShowPassKeyTrigger(ESP32BLEController* controller) {
    controller->add_on_show_pass_key_callback([this](string pass_key) {
      this->trigger(pass_key);
    });
  }
};

/// Trigger that is fired when authentication with a client is completed (either with success or failure).
class BLEControllerAuthenticationCompleteTrigger : public Trigger<bool> {
public:
  BLEControllerAuthenticationCompleteTrigger(ESP32BLEController* controller) {
    controller->add_on_authentication_complete_callback([this](bool success) {
      this->trigger(success);
    });
  }
};

// connect & disconnect ///////////////////////////////////////////////////////////////////////////////////////////////

/// Trigger that is fired when the BLE server has connected to a client.
class BLEControllerServerConnectedTrigger : public Trigger<> {
public:
  BLEControllerServerConnectedTrigger(ESP32BLEController* controller) {
    controller->add_on_connected_callback([this]() {
      this->trigger();
    });
  }
};

/// Trigger that is fired when the BLE server has disconnected from a client.
class BLEControllerServerDisconnectedTrigger : public Trigger<> {
public:
  BLEControllerServerDisconnectedTrigger(ESP32BLEController* controller) {
    controller->add_on_disconnected_callback([this]() {
      this->trigger();
    });
  }
};

// class BLEControllerServerShowReceiveDataTrigger : public Trigger<> {
// public:
//   BLEControllerServerShowReceiveDataTrigger(ESP32BLEController* controller) {
//     controller->add_on_receive_data_callback([this]() {
//       this->trigger(received_data);
//     });
//   }
// };
// class BLEControllerServerShowReceiveDataTrigger : public Trigger<std::string> {
// public:
//   BLEControllerServerShowReceiveDataTrigger(ESP32BLEController* controller) {
//     controller->add_on_receive_data_callback([this](string receive_data) {
//       this->trigger(receive_data);
//     });
//   }
// };

// custom command execution ///////////////////////////////////////////////////////////////////////////////////////////////

// class BLECustomCommandResultSender {
// public:
//   void operator=(const string& result) { global_ble_controller->send_command_result(result); }
// };

// /// Trigger that is fired when a custom command is executed.
// class BLEControllerCustomCommandExecutionTrigger : public Trigger<std::vector<std::string>, BLECustomCommandResultSender> {
// public:
//   BLEControllerCustomCommandExecutionTrigger(ESP32BLEController* controller) {}
// };

// // actions for BLE maintenance service ////////////////////////////////////////////////////////////////////////////////////

// template<typename... Ts> class TurnOnMaintenanceServiceAction : public Action<Ts...> {
// public:
//   void play(Ts... x) override { global_ble_controller->switch_maintenance_service_exposed(true); }
// };

// template<typename... Ts> class TurnOffMaintenanceServiceAction : public Action<Ts...> {
// public:
//   void play(Ts... x) override { global_ble_controller->switch_maintenance_service_exposed(false); }
// };

// template<typename... Ts> class ToggleMaintenanceServiceAction : public Action<Ts...> {
// public:
//   void play(Ts... x) override {
//     const bool exposed = global_ble_controller->get_maintenance_service_exposed();
//     global_ble_controller->switch_maintenance_service_exposed(!exposed); 
//   }
// };

} // namespace Creavision_BLE
} // namespace esphome
