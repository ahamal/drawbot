#pragma once
#include <Arduino.h>

void   wifi_setup_init();   // blocks until connected (or AP-mode timeout reboots)
bool   wifi_connected();
String wifi_ip();
