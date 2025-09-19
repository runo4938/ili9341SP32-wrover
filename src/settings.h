#ifndef SETTINGS_H
#define SETTINGS_H
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <EEPROM.h>
#include <EncButton.h>
#include <Audio.h>
#include <HTTPClient.h>

#include <Update.h>
#define I2S_DOUT 27 // 27 // 18 // DIN connection
#define I2S_BCLK 26 // 26// Bit clock
#define I2S_LRC 25  //  25// Left Right Clock
                    // end audio

// encoder
#define CLK 32 // 35 //
#define DT 33  //  //
#define SW 35  //  //

extern const char *PARAM_INPUT;
extern String sliderValue;
extern Audio audio;
extern bool volUpdate;
extern File file;
extern String listRadio; // радиостанции на странице

#define U_PART U_SPIFFS

static unsigned long lastUpdate = 0;
const unsigned long frameInterval = 30; // ~33 FPS
// int16_t spriteX = -250;        // Начинаем за пределами слева
enum State
{
  MOVING_TO_LEFT_EDGE,
  WAITING_AT_LEFT,
  MOVING_OFF_LEFT
};


const unsigned long waitDuration = 3000; // 3 секунды ожидания
const int screenWidth = 320;
const int spriteWidth = 250;
const int speed = 4;    // Скорость движения (пикс/кадр)
const int spriteY = 64; // Y-позиция спрайта на экране

   // for initial values


void stationDisplay(int st);
void deleteFile(fs::FS &fs, const String &path);
void onMenu();
void onMenuOff();
// void handleDoUpdate(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final);
void newrelease();
void onMenuOn();
void notFound(AsyncWebServerRequest *request);
void handleDoUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
void setupRoutes(AsyncWebServer &server);
String processor_update(const String &var);

#endif
