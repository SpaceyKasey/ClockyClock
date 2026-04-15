#pragma once

bool wifiConnect(const char* ssid, const char* password, int timeoutSec = 15);
void wifiDisconnect();
bool wifiIsConnected();

bool ntpSync();
