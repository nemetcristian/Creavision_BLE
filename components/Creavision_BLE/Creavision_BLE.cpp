#include <cstdio>
#include <utility>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLESecurity.h>
#include <string>
#include "esphome/components/wifi/wifi_component.h"

#include "esphome/core/application.h"
#include "esphome/core/log.h"

#include <esp_bt_main.h>
#include <esp32-hal-bt.h>

#include "Creavision_BLE.h"
#include "automation.h"


#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a9"


namespace esphome {
namespace Creavision_BLE {

static const char *TAG = "esp32_ble_server";

ESP32BLEController::ESP32BLEController() {}

void ESP32BLEController::set_component_source(const std::string& source) {
}

void ESP32BLEController::add_on_show_pass_key_callback(std::function<void(string)>&& trigger_function) {
  can_show_pass_key = true;
  on_show_pass_key_callbacks.add(std::move(trigger_function));
}

void ESP32BLEController::add_on_authentication_complete_callback(std::function<void(bool)>&& trigger_function) {
  on_authentication_complete_callbacks.add(std::move(trigger_function));
}

void ESP32BLEController::add_on_connected_callback(std::function<void()>&& trigger_function) {
  on_connected_callbacks.add(std::move(trigger_function));
}

void ESP32BLEController::add_on_disconnected_callback(std::function<void()>&& trigger_function) {
  on_disconnected_callbacks.add(std::move(trigger_function));

}
// void ESP32BLEController::add_on_receive_data_callback(std::function<void()>&& trigger_function) {
//   on_receive_data_callback.add(std::move(trigger_function));

// }

BLEMaintenanceMode set_feature(BLEMaintenanceMode current_mode, BLEMaintenanceMode feature, bool is_set) {
  uint8_t new_mode = static_cast<uint8_t>(current_mode) & (~static_cast<uint8_t>(feature));
  if (is_set) {
    new_mode |= static_cast<uint8_t>(feature);
  }
  return static_cast<BLEMaintenanceMode>(new_mode);
}


void ESP32BLEController::set_maintenance_service_exposed_after_flash(bool exposed) {
  initial_ble_mode_after_flashing = set_feature(initial_ble_mode_after_flashing, BLEMaintenanceMode::MAINTENANCE_SERVICE, exposed);    
}

void ESP32BLEController::set_security_enabled(bool enabled) {
  set_security_mode(BLESecurityMode::SECURE);
}

void ESP32BLEController::setup() {
  ESP_LOGCONFIG(TAG, "Setting up BLE controller ...");

  initialize_ble_mode();

  if (ble_mode == BLEMaintenanceMode::NONE) {
    ESP_LOGCONFIG(TAG, "BLE inactive");
    return;
  }

//   #ifdef USE_WIFI
//   wifi_configuration_handler.setup();
//   #endif

  if (!setup_ble()) {
    return;
  }

  if (global_ble_controller == nullptr) {
    global_ble_controller = this;
  } else {
    ESP_LOGE(TAG, "Already have an instance of the BLE controller");
  }

  // Create the BLE Device
  BLEDevice::init(App.get_name());

  configure_ble_security();

  setup_ble_server_and_services();

  BLEDevice::startAdvertising(); 
}

void ESP32BLEController::init_service(BLEServer* ble_server) {
    BLEService *Service = ble_server->createService(SERVICE_UUID);
    if (!Service) {
        // Serial.println("Failed to create BLE service");
        ESP_LOGCONFIG(TAG, "Failed to create BLE service");
        return;
    }

    CreaVisionCharacteristic = Service->createCharacteristic(CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_WRITE);
    if (!CreaVisionCharacteristic) {
        // Serial.println("Failed to create BLE characteristic");
        ESP_LOGCONFIG(TAG, "Failed to create BLE characteristic");
        return;
    }

    esp_gatt_perm_t access_permissions;
    if (global_ble_controller->get_security_enabled()) {
        access_permissions = ESP_GATT_PERM_READ_ENC_MITM | ESP_GATT_PERM_WRITE_ENC_MITM;
    } else {
        access_permissions = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE;
    }
    CreaVisionCharacteristic->setAccessPermissions(access_permissions);

    BLEDescriptor* descriptor_2901 = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
    if (!descriptor_2901) {
        // Serial.println("Failed to create BLE descriptor");
        ESP_LOGCONFIG(TAG, "Failed to create BLE descriptor");
        return;
    }
    descriptor_2901->setAccessPermissions(access_permissions);
    // Set descriptor value if needed
    // descriptor_2901->setValue(description);
    CreaVisionCharacteristic->addDescriptor(descriptor_2901);

    // Set characteristic callbacks if needed
    CreaVisionCharacteristic->setCallbacks(this);

    Service->start();

    ESP_LOGCONFIG(TAG, "BLE service initialized successfully");
}

void ESP32BLEController::setup_ble_server_and_services() {
  
  ble_server = BLEDevice::createServer();
  ble_server->setCallbacks(this);

  init_service(ble_server);

}


bool ESP32BLEController::setup_ble() {
  if (btStarted()) {
    ESP_LOGI(TAG, "BLE already started");
    return true;
  }

  ESP_LOGI(TAG, "  Setting up BLE ...");

  esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);

  // Initialize the bluetooth controller with the default configuration
  if (!btStart()) {
    ESP_LOGE(TAG, "btStart failed: %d", esp_bt_controller_get_status());
    // mark_failed();
    return false;
  }

  esp_err_t err = esp_bluedroid_init();
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_bluedroid_init failed: %d", err);
    // mark_failed();
    return false;
  }

  err = esp_bluedroid_enable();
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_bluedroid_enable failed: %d", err);
    // mark_failed();
    return false;
  }

  return true;
}

void ESP32BLEController::initialize_ble_mode() {
  // Note: We include the compilation time to force a reset after flashing new firmware
  ble_mode_preference = global_preferences->make_preference<uint8_t>(fnv1_hash("ble-mode#" + App.get_compilation_time()));

  if (!ble_mode_preference.load(&ble_mode)) {
    ble_mode = initial_ble_mode_after_flashing;
  }

  ESP_LOGCONFIG(TAG, "BLE mode: %d", static_cast<uint8_t>(ble_mode));
}

vector<string> get_bonded_devices() {
  vector<string> paired_devices;

  int dev_num = esp_ble_get_bond_device_num();

  esp_ble_bond_dev_t *dev_list = (esp_ble_bond_dev_t*) malloc(sizeof(esp_ble_bond_dev_t) * dev_num);
  esp_ble_get_bond_device_list(&dev_num, dev_list);

  for (int i = 0; i < dev_num; i++) {
    char bd_address_str[18];
    esp_bd_addr_t& bd_address = dev_list[i].bd_addr;
    snprintf(bd_address_str, sizeof(bd_address_str), "%X:%X:%X:%X:%X:%X", bd_address[0], bd_address[1], bd_address[2], bd_address[3], bd_address[4], bd_address[5]);
    paired_devices.push_back(bd_address_str);
  }

  free(dev_list);

  return paired_devices;
}

void ESP32BLEController::dump_config() {
  if (ble_mode == BLEMaintenanceMode::NONE) {
    return;
  }
  
  ESP_LOGCONFIG(TAG, "Bluetooth Low Energy Controller:");
  ESP_LOGCONFIG(TAG, "  BLE device address: %s", BLEDevice::getAddress().toString().c_str());
  ESP_LOGCONFIG(TAG, "  BLE mode: %d", (uint8_t) ble_mode);

  if (get_security_mode() != BLESecurityMode::NONE) {
    if (get_security_mode() == BLESecurityMode::BOND) {
      ESP_LOGCONFIG(TAG, "  only bonding enabled, no real security");
    } else {
      ESP_LOGCONFIG(TAG, "  security enabled (secure connections, MITM protection)");
    }

    vector<string> bonded_devices = get_bonded_devices();
    if (bonded_devices.empty()) {
      ESP_LOGCONFIG(TAG, "  no bonded BLE devices");
    } else {
      ESP_LOGCONFIG(TAG, "  bonded BLE devices (%d):", bonded_devices.size());
      int i = 0;
      for (const auto& bd_address : bonded_devices) {
        ESP_LOGCONFIG(TAG, "    %d) BD address %s", ++i, bd_address.c_str());
      }
    }
  } else {
    ESP_LOGCONFIG(TAG, "  security disabled");
  }
}

void ESP32BLEController::execute_in_loop(std::function<void()>&& deferred_function) {
  bool ok = deferred_functions_for_loop.push(std::move(deferred_function));
  if (!ok) {
    ESP_LOGCONFIG(TAG, "Deferred functions queue full");
  }
}


void ESP32BLEController::loop() {
  std::function<void()> deferred_function;
  while (deferred_functions_for_loop.take(deferred_function)) {
    deferred_function();
  }
}

void ESP32BLEController::configure_ble_security() {
  
  if (get_security_mode() == BLESecurityMode::NONE) {
    return;
  }

  ESP_LOGD(TAG, "  Setting up BLE security");

  BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT_MITM);
  BLEDevice::setSecurityCallbacks(this);

  // see https://github.com/espressif/esp-idf/blob/b0150615dff529662772a60dcb57d5b559f480e2/examples/bluetooth/bluedroid/ble/gatt_security_server/tutorial/Gatt_Security_Server_Example_Walkthrough.md
  BLESecurity security;
  security.setAuthenticationMode(get_security_mode() == BLESecurityMode::BOND ? ESP_LE_AUTH_BOND : ESP_LE_AUTH_REQ_SC_MITM_BOND);
  security.setCapability(can_show_pass_key ? ESP_IO_CAP_OUT : ESP_IO_CAP_NONE);
  security.setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
  security.setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
  security.setKeySize(16);

  uint8_t auth_option = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_ENABLE;
  esp_ble_gap_set_security_param(ESP_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH, &auth_option, sizeof(uint8_t));
}

void ESP32BLEController::onAuthenticationComplete(esp_ble_auth_cmpl_t result) {
  auto& callbacks = on_authentication_complete_callbacks;
  bool success=result.success;
  global_ble_controller->execute_in_loop([&callbacks, success](){
    if (success) {
      ESP_LOGD(TAG, "BLE authentication - completed succesfully");
    } else {
      ESP_LOGD(TAG, "BLE authentication - failed");
    }
    callbacks.call(success);
  });
}

uint32_t ESP32BLEController::onPassKeyRequest() {
  global_ble_controller->execute_in_loop([](){ ESP_LOGD(TAG, "onPassKeyRequest"); });
  return 123456;
}

bool ESP32BLEController::onSecurityRequest() {
  global_ble_controller->execute_in_loop([](){ ESP_LOGD(TAG, "onSecurityRequest"); });
  return true;
}

bool ESP32BLEController::onConfirmPIN(uint32_t pin) {
  global_ble_controller->execute_in_loop([](){ ESP_LOGD(TAG, "onConfirmPIN"); });
  return true;
}


void ESP32BLEController::onPassKeyNotify(uint32_t pass_key) {
  char pass_key_digits[6 + 1];

  snprintf(pass_key_digits, sizeof(pass_key_digits), "%06d", pass_key);
  string pass_key_str(pass_key_digits);

  auto& callbacks = on_show_pass_key_callbacks;
  global_ble_controller->execute_in_loop([&callbacks, pass_key_str](){ 
    ESP_LOGI(TAG, "BLE authentication - pass received");
    callbacks.call(pass_key_str);
  });
}

void ESP32BLEController::ESP32BLEController::onConnect(BLEServer* server) {
  auto& callbacks = on_connected_callbacks;
  global_ble_controller->execute_in_loop([&callbacks](){ 
    ESP_LOGD(TAG, "BLE server - connected");
    callbacks.call();
  });
}

void ESP32BLEController::ESP32BLEController::onDisconnect(BLEServer* server) {
  auto& callbacks = on_disconnected_callbacks;
  global_ble_controller->execute_in_loop([&callbacks, this](){ 
    ESP_LOGD(TAG, "BLE server - disconnected");

    // after 500ms start advertising again
    const uint32_t delay_millis = 500;
    App.scheduler.set_timeout(this, "", delay_millis, []{ BLEDevice::startAdvertising(); });

    callbacks.call(); 
  });
}


void ESP32BLEController::onWrite(BLECharacteristic *characteristic) {
  if (characteristic == CreaVisionCharacteristic) {
    global_ble_controller->execute_in_loop([this](){ on_command_written(); });
  } else {
    ESP_LOGD(TAG, "Unknown characteristic written!");
  }
}

void ESP32BLEController ::on_command_written() {
 received_data = CreaVisionCharacteristic->getValue();
  ESP_LOGD(TAG, "Received BLE command: %s", received_data.c_str());
}


ESP32BLEController* global_ble_controller = nullptr;


} // namespace Creavision_BLE
} // namespace esphome2
