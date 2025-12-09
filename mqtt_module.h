#pragma once
#include <Arduino.h>

typedef void (*MqttCommandCallback)(const String &cmdJson);

void mqttInit();
void mqttLoop();
void mqttSetCommandCallback(MqttCommandCallback cb);

void mqttPublishState(const String &json);
void mqttPublishDiag(const String &json);
void mqttPublishCfgRequest();
