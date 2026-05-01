#include "RMaker.h"
#include "WiFi.h"
#include "WiFiProv.h"

// Service details
const char *service_name = "Rudra Home";
const char *pop = "1234";
char nodeName[] = "ESP32_Home";

// Device Names
char deviceName_1[] = "LED";
char deviceName_2[] = "Small LED";
char deviceName_3[] = "Night Lamp";
char deviceName_4[] = "Smart Light";
char deviceName_5[] = "Charging Port";
char deviceName_6[] = "Shelf Light";
char deviceName_7[] = "Relay7";
char deviceName_8[] = "Home Theatre";

// Relay Pins
uint8_t RelayPin[] = {23,22,21,19,18,5,25,26};

// Switch Pins
uint8_t SwitchPin[] = {13,12,14,27,33,32,15,4};

uint8_t wifiLed = 2;
uint8_t gpio_reset = 0;

// Relay States
bool toggleState[8] = {0};
bool switchState[8] = {0};

// RainMaker Switch Devices
Switch my_switch1(deviceName_1, &RelayPin[0]);
Switch my_switch2(deviceName_2, &RelayPin[1]);
Switch my_switch3(deviceName_3, &RelayPin[2]);
Switch my_switch4(deviceName_4, &RelayPin[3]);
Switch my_switch5(deviceName_5, &RelayPin[4]);
Switch my_switch6(deviceName_6, &RelayPin[5]);
Switch my_switch7(deviceName_7, &RelayPin[6]);
Switch my_switch8(deviceName_8, &RelayPin[7]);

Switch* switchArray[] = {
  &my_switch1,&my_switch2,&my_switch3,&my_switch4,
  &my_switch5,&my_switch6,&my_switch7,&my_switch8
};

void sysProvEvent(arduino_event_t *sys_event) {
  if (sys_event->event_id == ARDUINO_EVENT_WIFI_STA_CONNECTED) {
    digitalWrite(wifiLed, HIGH);
  }
}

void write_callback(Device *device, Param *param,
                    const param_val_t val,
                    void *priv_data, write_ctx_t *ctx) {

  for(int i=0;i<8;i++){
    if(strcmp(device->getDeviceName(), switchArray[i]->getDeviceName()) == 0){
      toggleState[i] = val.val.b;
      digitalWrite(RelayPin[i], toggleState[i] ? LOW : HIGH);
      param->updateAndReport(val);
    }
  }
}

void manual_control() {
  for(int i=0;i<8;i++){
    if (digitalRead(SwitchPin[i]) == LOW && switchState[i] == LOW) {
      digitalWrite(RelayPin[i], LOW);
      toggleState[i] = 1;
      switchState[i] = HIGH;
      switchArray[i]->updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, true);
    }
    if (digitalRead(SwitchPin[i]) == HIGH && switchState[i] == HIGH) {
      digitalWrite(RelayPin[i], HIGH);
      toggleState[i] = 0;
      switchState[i] = LOW;
      switchArray[i]->updateAndReportParam(ESP_RMAKER_DEF_POWER_NAME, false);
    }
  }
}

void setup() {

  Serial.begin(115200);

  for(int i=0;i<8;i++){
    pinMode(RelayPin[i], OUTPUT);
    pinMode(SwitchPin[i], INPUT_PULLUP);
    digitalWrite(RelayPin[i], HIGH);
  }

  pinMode(wifiLed, OUTPUT);
  pinMode(gpio_reset, INPUT);

  Node my_node = RMaker.initNode(nodeName);

  for(int i=0;i<8;i++){
    switchArray[i]->addCb(write_callback);
    my_node.addDevice(*switchArray[i]);
  }

  RMaker.enableOTA(OTA_USING_PARAMS);
  RMaker.enableTZService();
  RMaker.enableSchedule();

  RMaker.start();

  WiFi.onEvent(sysProvEvent);

  WiFiProv.beginProvision(
    WIFI_PROV_SCHEME_BLE,
    WIFI_PROV_SCHEME_HANDLER_FREE_BTDM,
    WIFI_PROV_SECURITY_1,
    pop,
    service_name
  );

  for(int i=0;i<8;i++){
    switchArray[i]->updateAndReportParam(
      ESP_RMAKER_DEF_POWER_NAME, false);
  }
}

void loop() {

  // Reset Button
  if(digitalRead(gpio_reset) == LOW){
    delay(100);
    int startTime = millis();
    while(digitalRead(gpio_reset) == LOW) delay(50);
    int endTime = millis();

    if ((endTime - startTime) > 10000) {
      RMakerFactoryReset(2);
    } 
    else if ((endTime - startTime) > 3000) {
      RMakerWiFiReset(2);
    }
  }

  if (WiFi.status() == WL_CONNECTED)
    digitalWrite(wifiLed, HIGH);
  else
    digitalWrite(wifiLed, LOW);

  manual_control();
}