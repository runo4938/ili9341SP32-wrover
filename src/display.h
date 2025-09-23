#ifndef DISPLAY_H
#define DISOLAY_H
#include <Arduino.h>
//WiFi_ssid
uint16_t x_wifi_ssid = 135;
uint16_t y_wifi_ssid = 225;
uint16_t x_WiFi_localIP = 135; 
uint16_t y_WiFi_localIP = 225;
//print StName
uint16_t x_stName = 7;
uint16_t y_stName = 0;
//codec botrate
uint16_t x_codec = 194;
uint16_t y_codec = 182;
uint16_t x_bitrate = 250;
uint16_t y_bitrate = 182;
//WiFi
uint16_t x_wifi = 90; 
uint16_t y_wifi = 182;
//Current Data нижняя строка
uint16_t x_data = 5;
uint16_t y_data =238;
bool title_flag = true;
bool show_title = false; // если не титров
String before;
String after;
#endif