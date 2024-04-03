
// #ifndef CREAVISION_BLE_H
// #define CREAVISION_BLE_H
#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <BLEServer.h>
#include <BLEDevice.h>

#include "esphome/components/wifi/wifi_component.h"

#include "esphome/core/entity_base.h"
#include "esphome/core/controller.h"
#include "esphome/core/defines.h"
#include "esphome/core/preferences.h"
#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/core/optional.h"

#include "thread_safe_bounded_queue.h"
// #include "automation.h"



using std::string;
using std::unordered_map;
using std::vector;

namespace esphome
{
  namespace Creavision_BLE
  {
    enum class BLESecurityMode : uint8_t { NONE, SECURE, BOND };

    enum class BLEMaintenanceMode : uint8_t
    {
      MAINTENANCE_SERVICE = 1,
      COMPONENT_SERVICES = 2,
      NONE = 0,
      ALL = 3
    };

    class BLEControllerCustomCommandExecutionTrigger;


    vector<string> get_bonded_devices();


    class ESP32BLEController : public Component, private BLESecurityCallbacks, public BLEServerCallbacks, public BLECharacteristicCallbacks {
    public:

      ESP32BLEController();
      virtual ~ESP32BLEController() {}

      void set_component_source(const std::string& source);

      void add_on_show_pass_key_callback(std::function<void(string)>&& trigger_function);
      void add_on_authentication_complete_callback(std::function<void(bool)>&& trigger_function);
      void add_on_connected_callback(std::function<void()>&& trigger_function);
      void add_on_disconnected_callback(std::function<void()>&& trigger_function);
      // void add_on_receive_data_callback(std::function<void()>&& trigger_function);

      void set_maintenance_service_exposed_after_flash(bool exposed);

      void set_security_mode(BLESecurityMode mode) { security_mode = mode; }
      inline BLESecurityMode get_security_mode() const { return security_mode; }

      void set_security_enabled(bool enabled);
      inline bool get_security_enabled() const { return security_mode != BLESecurityMode::NONE; }


      void setup() override;
      void dump_config() override;
      void loop() override;

      inline BLEMaintenanceMode get_ble_mode() const { return ble_mode; }
      bool get_maintenance_service_exposed() const { return static_cast<uint8_t>(ble_mode) & static_cast<uint8_t>(BLEMaintenanceMode::MAINTENANCE_SERVICE); }
      bool get_component_services_exposed() const { return static_cast<uint8_t>(ble_mode) & static_cast<uint8_t>(BLEMaintenanceMode::COMPONENT_SERVICES); }
      void switch_ble_mode(BLEMaintenanceMode mode);

      void execute_in_loop(std::function<void()> &&deferred_function);


    // public:
      void init_service(BLEServer* ble_server) ; 
      void initialize_ble_mode();
      bool setup_ble();
      void setup_ble_server_and_services();

      void configure_ble_security();
      virtual uint32_t onPassKeyRequest(); // inherited from BLESecurityCallbacks
      virtual void onPassKeyNotify(uint32_t pass_key); // inherited from BLESecurityCallbacks
      virtual bool onSecurityRequest(); // inherited from BLESecurityCallbacks
      virtual void onAuthenticationComplete(esp_ble_auth_cmpl_t); // inherited from BLESecurityCallbacks
      virtual bool onConfirmPIN(uint32_t pin); // inherited from BLESecurityCallbacks


      virtual void onConnect(BLEServer* server); // inherited from BLEServerCallbacks
      virtual void onDisconnect(BLEServer* server); // inherited from BLEServerCallbacks
      // virtual void onRead(BLECharacteristic *characteristic); 
      void onWrite(BLECharacteristic *characteristic) ;
      void on_command_written();



    public:
      BLEServer *ble_server;
      BLECharacteristic* CreaVisionCharacteristic;
      string received_data;

      BLEMaintenanceMode initial_ble_mode_after_flashing{BLEMaintenanceMode::ALL};
      BLEMaintenanceMode ble_mode;
      ESPPreferenceObject ble_mode_preference;

      BLESecurityMode security_mode{BLESecurityMode::SECURE};
      bool can_show_pass_key{false};

    
      ThreadSafeBoundedQueue<std::function<void()>> deferred_functions_for_loop{16};
    
      CallbackManager<void(string)> on_show_pass_key_callbacks;
      CallbackManager<void(bool)>   on_authentication_complete_callbacks;
      CallbackManager<void()>       on_connected_callbacks;
      CallbackManager<void()>       on_disconnected_callbacks;
      // CallbackManager<void()>       on_receive_data_callback;

    };

    extern ESP32BLEController *global_ble_controller;

    


  } // namespace Creavision_BLE
} // namespace esphome

// #endif // CREAVISION_BLE_H
