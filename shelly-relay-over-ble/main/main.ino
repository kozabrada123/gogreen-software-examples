// Read https://www.instructables.com/Controlling-Shelly-Home-Automation-Relays-From-an-/
// You might need to use the MAC address (see line 21, use that instead of line 20) to distinguish your
// device from the others; to find it, look under Wi-Fi status in the web UI or use a bluetooth scanner app

// File: ESP32_Shelly_BLE_Controller_v28July25.ino
// Tested with ESP32 and ESP32S3.
// Tested with "Shelly 1 Mini" Gen3 & Gen4 with firmware v1.62 (see "Settings/Firmware")

// To factory-reset "Shelly 1 Mini" (resets hotspot etc): Press+Hold small-button (on back of Shelly) for 10seconds.
// To update Shelly-firmware (need to have internet): Connect to "Shelly-hotspot", Go-to: "http://192.168.33.1/", Select: "Settings/Firmware/Check-For-Updates".
// To connect-Shelly-to-internet: Select "Settings/Wi-Fi/Wi-Fi_1_settings", select SSID/pw & "Save_settings".  (You can delete this after the above firmware-update.)
// To add-a-password to the above "Shelly-hotspot", Select "Settings/Access_Point/Access_Point_settings", enter a pw and click  "Save_settings".

// Note that for the below appication we DON'T have to set any Shelly "Actions" or "Schedules" or "Scripts"
// To see Shelly-diagnostic-info: click on "Diagnostics" (click on "CLEAR CONSOLE" as required).
// NB. This demo code does NOT make use of the "Shelly-Authentication" Security-Settings

#include "BLEDevice.h"

const char BLE_Device_Advertised_Text_Wanted[] = "Shelly1MiniG";
// const char BLE_Device_Advertised_Text_Wanted[]="cc:xx:xx:xx:xx:xx";         // option_1 or
// const char BLE_Device_Advertised_Text_Wanted[]="Shelly1MiniG3-CCxxxxxxxxx"; // option_2 or ?

// BLE *** Device found: Name: Shelly1MiniG3-CCxxxxxxxxxx, Address: cc:xx:xx:xx:xx:xx, manufacturer data: a90xxx, rssi: -61

static BLEUUID serviceUUID("5f6d4f53-5f52-5043-5f53-56435f49445f");  // The "Shelly 1 Mini"
static BLEUUID charUUID("5f6d4f53-5f52-5043-5f74-785f63746c5f");     // WRITE // PC_CHAR_TX_CTL_UUID
static BLEUUID charUUID2("5f6d4f53-5f52-5043-5f64-6174615f5f5f");    // READ, WRITE // RPC_CHAR_DATA_UUID

char ble_command[160];

// NB. the BLE "pRemoteCharacteristic->writeValue()" requires "char arrays" to be /0 terminated!
uint8_t Command_BLE_Size[] = { 0, 0, 0, 0 };  // 4-byte big-endian
uint32_t ble_command_counter = 0;

char *Command_On_Off_Switch0(boolean turn_on) {
  snprintf(ble_command, sizeof(ble_command), "{\"id\":%u,\"src\":\"esp32\",\"method\":\"Switch.Set\",\"params\":{\"id\":0,\"on\":%s }}", ble_command_counter, turn_on ? "true" : "false");
  
  if (ble_command_counter == 9) ble_command_counter = 0;
  else ble_command_counter++;
  return ble_command;
}

volatile boolean do_Connect = false;
volatile boolean is_Connected = false;
static BLERemoteCharacteristic *pRemoteCharacteristic;
static BLERemoteCharacteristic *pRemoteCharacteristic2;
static BLEAdvertisedDevice *myDevice;
static BLEClient *pClient;

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient *pclient) {}
  void onDisconnect(BLEClient *pclient) {
    is_Connected = false;
    Serial.println("onDisconnect");
  }
};

bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());
  if (!pClient->connect(myDevice)) {
    Serial.printf("Failed to connect with the client!\n");
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Connected to server");
  pClient->setMTU(517);  //set client to request maximum MTU from server (default is 23 otherwise)
  BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.printf("Failed to find our service UUID=%s\n", serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our characteristic");
  pRemoteCharacteristic2 = pRemoteService->getCharacteristic(charUUID2);
  if (pRemoteCharacteristic2 == nullptr) {
    Serial.print("Failed to find our characteristic UUID2: ");
    Serial.println(charUUID2.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our characteristic2");
  is_Connected = true;
  return true;
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    const String p1 = advertisedDevice.toString().c_str();
    boolean my_device = p1.indexOf(BLE_Device_Advertised_Text_Wanted) >= 0;
    boolean got_UUID;
    if (my_device) {
      got_UUID = advertisedDevice.haveServiceUUID();
      if (got_UUID) got_UUID = advertisedDevice.isAdvertisingService(serviceUUID);
      else got_UUID = true;  // no ServiceUUID in Adverting-Packet available, but MAC is OK so continue
      // for some BLE devices the ServiceUUID is not sent in the initial-Advertising-Packet, but in a later "Scan Responce Packet"
      // However, I  cant find a way to access this in arduino (eg. with "BLESCANPARAM" or similar), so I'm using the above MAC-address etc instead
      if (got_UUID) {  // Found our server
        Serial.printf("BLE -MY-- Device found=%s\n", p1.c_str());  // p1 is usually coorrupted if printed later
        BLEDevice::getScan()->stop();
        delete myDevice;
        myDevice = new BLEAdvertisedDevice(advertisedDevice);
        do_Connect = true;
      }
    } else got_UUID = false;
    if (!got_UUID) Serial.printf("BLE OTHER Device found=%s\n", p1.c_str());
  };  // onResult
};

void setup() {
  Serial.begin(115200);
  delay(3000);
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");
  pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());
  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
}

void loop() {
  static boolean do_Scan = true;
  static uint32_t millis_next = 5000;  // wait for ~5s at start
  uint32_t millis_now = millis();

  if (millis_next <= millis_now) {
    millis_next = millis_now + 15000;  // then every 15s

    if (do_Scan) {
      do_Scan = false;
      Serial.println("Scan(8s)");
      BLEDevice::getScan()->start(8, false);  // scans/waits for 8s (unless target is found)
      delay(10);                              // allow time for [do_Connect] to be updated
      if (do_Connect == true) {
        if (connectToServer()) Serial.println("We are now connected to the BLE Server.");
        else Serial.println("We have failed to connect to the server; there is nothing more we will do.");
        do_Connect = false;
      }
    }

    delay(10);  // allow time for [is_Connected] to be updated

    if (is_Connected) {
      static boolean on_not_off = true;
      char* cmd = Command_On_Off_Switch0(on_not_off);
      uint8_t size = strlen(cmd);
      Command_BLE_Size[3] = size;  
      pRemoteCharacteristic->writeValue(Command_BLE_Size, 4);
      pRemoteCharacteristic2->writeValue(cmd, size);
      Serial.printf("Sent %s\n", on_not_off ? "ON" : "OFF");
      on_not_off = !on_not_off;
    } else do_Scan = true;
  }
}
