#include "RMaker.h"
#include "WiFi.h"
#include "WiFiProv.h"

// ------------------- SERVICE DETAILS -------------------
const char *service_name = "RudraHome";
const char *pop = "12345678";

char nodeName[] = "ESP32_Home";

// ------------------- DEVICE NAMES -------------------
char deviceName_1[] = "LED";
char deviceName_2[] = "Small LED";
char deviceName_3[] = "Night Lamp";
char deviceName_4[] = "Smart Light";
char deviceName_5[] = "Charging Port";
char deviceName_6[] = "Shelf Light";
char deviceName_7[] = "Relay7";
char deviceName_8[] = "Home Theatre";

// ------------------- RELAY PINS -------------------
uint8_t RelayPin[] = {23, 22, 21, 19, 18, 5, 25, 26};

// ------------------- SWITCH PINS -------------------
uint8_t SwitchPin[] = {13, 12, 14, 27, 33, 32, 15, 4};

// ------------------- STATUS LED -------------------
uint8_t wifiLed = 2;

// ------------------- RESET BUTTON -------------------
uint8_t gpio_reset = 0;

// ------------------- STATES -------------------
bool toggleState[8] = {0};
bool switchState[8] = {0};

// ------------------- RAINMAKER SWITCHES -------------------
Switch my_switch1(deviceName_1, &RelayPin[0]);
Switch my_switch2(deviceName_2, &RelayPin[1]);
Switch my_switch3(deviceName_3, &RelayPin[2]);
Switch my_switch4(deviceName_4, &RelayPin[3]);
Switch my_switch5(deviceName_5, &RelayPin[4]);
Switch my_switch6(deviceName_6, &RelayPin[5]);
Switch my_switch7(deviceName_7, &RelayPin[6]);
Switch my_switch8(deviceName_8, &RelayPin[7]);

Switch *switchArray[] = {
  &my_switch1,
  &my_switch2,
  &my_switch3,
  &my_switch4,
  &my_switch5,
  &my_switch6,
  &my_switch7,
  &my_switch8
};

// =====================================================
// WRITE CALLBACK
// =====================================================
void write_callback(Device *device,
                    Param *param,
                    const param_val_t val,
                    void *priv_data,
                    write_ctx_t *ctx)
{

  for (int i = 0; i < 8; i++) {

    if (strcmp(device->getDeviceName(),
               switchArray[i]->getDeviceName()) == 0) {

      toggleState[i] = val.val.b;

      digitalWrite(RelayPin[i],
                   toggleState[i] ? LOW : HIGH);

      param->updateAndReport(val);
    }
  }
}

// =====================================================
// MANUAL SWITCH CONTROL
// =====================================================
void manual_control()
{
  for (int i = 0; i < 8; i++) {

    // SWITCH PRESSED
    if (digitalRead(SwitchPin[i]) == LOW &&
        switchState[i] == LOW) {

      digitalWrite(RelayPin[i], LOW);

      toggleState[i] = 1;
      switchState[i] = HIGH;

      switchArray[i]->updateAndReportParam(
        ESP_RMAKER_DEF_POWER_NAME,
        true
      );
    }

    // SWITCH RELEASED
    if (digitalRead(SwitchPin[i]) == HIGH &&
        switchState[i] == HIGH) {

      digitalWrite(RelayPin[i], HIGH);

      toggleState[i] = 0;
      switchState[i] = LOW;

      switchArray[i]->updateAndReportParam(
        ESP_RMAKER_DEF_POWER_NAME,
        false
      );
    }
  }
}

// =====================================================
// WIFI EVENT
// =====================================================
void sysProvEvent(arduino_event_t *sys_event)
{
  switch (sys_event->event_id) {

    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.println("WiFi Connected");
      digitalWrite(wifiLed, HIGH);
      break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println("WiFi Disconnected");
      digitalWrite(wifiLed, LOW);
      break;

    default:
      break;
  }
}

// =====================================================
// SETUP
// =====================================================
void setup()
{
  Serial.begin(115200);

  // RELAY SETUP
  for (int i = 0; i < 8; i++) {

    pinMode(RelayPin[i], OUTPUT);

    digitalWrite(RelayPin[i], HIGH);

    pinMode(SwitchPin[i], INPUT_PULLUP);
  }

  // LED
  pinMode(wifiLed, OUTPUT);

  // RESET BUTTON
  pinMode(gpio_reset, INPUT);

  // =====================================================
  // NODE INITIALIZATION
  // =====================================================
  Node my_node = RMaker.initNode(nodeName);

  // ADD DEVICES
  for (int i = 0; i < 8; i++) {

    switchArray[i]->addCb(write_callback);

    my_node.addDevice(*switchArray[i]);
  }

  // OPTIONAL FEATURES
  RMaker.enableOTA(OTA_USING_PARAMS);

  // START RAINMAKER
  RMaker.start();

  // WIFI EVENT
  WiFi.onEvent(sysProvEvent);

  // =====================================================
  // START PROVISIONING
  // =====================================================
  WiFiProv.beginProvision(
    WIFI_PROV_SCHEME_BLE,
    WIFI_PROV_SCHEME_HANDLER_NONE,
    WIFI_PROV_SECURITY_1,
    pop,
    service_name
  );

  Serial.println("BLE Provisioning Started");
WiFiProv.printQR(service_name, pop, "ble");
  // INITIAL STATE UPDATE
  for (int i = 0; i < 8; i++) {

    switchArray[i]->updateAndReportParam(
      ESP_RMAKER_DEF_POWER_NAME,
      false
    );
  }
}

// =====================================================
// LOOP
// =====================================================
void loop()
{
  // =====================================================
  // RESET BUTTON
  // =====================================================
  if (digitalRead(gpio_reset) == LOW) {

    delay(100);

    int startTime = millis();

    while (digitalRead(gpio_reset) == LOW) {
      delay(50);
    }

    int endTime = millis();

    // FACTORY RESET
    if ((endTime - startTime) > 10000) {

      Serial.println("Factory Reset");

      RMakerFactoryReset(2);
    }

    // WIFI RESET
    else if ((endTime - startTime) > 3000) {

      Serial.println("WiFi Reset");

      RMakerWiFiReset(2);
    }
  }

  // WIFI LED STATUS
  if (WiFi.status() == WL_CONNECTED)
    digitalWrite(wifiLed, HIGH);
  else
    digitalWrite(wifiLed, LOW);

  // MANUAL CONTROL
  manual_control();
}
