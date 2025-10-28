#ifndef SETTINGS_H
#define SETTINGS_H
#ifdef BOARD_VS1053
#include "SPI.h"
#include "vs1053_ext.h"
#endif

#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <EEPROM.h>
#include <EncButton.h>
#include <HTTPClient.h>
#include <Update.h>

#ifdef BOARD_PCM5102
#include <Audio.h>
#define I2S_DOUT 27 // 27 // 18 // DIN connection
#define I2S_BCLK 26 // 26// Bit clock
#define I2S_LRC 25  //  25// Left Right Clock
#elif BOARD_VS1053
#define VS1053_MOSI   23
#define VS1053_MISO   19
#define VS1053_SCK    18
#define VS1053_CS      27
#define VS1053_DCS     25
#define VS1053_DREQ   26
extern VS1053 audio;
#endif

// end audio
#define LED_BRIGHTNESS 200 // яркость дисплея при старте
#define LED_BUILT 22       // управление яркостью дисплея

// encoder

#ifdef BOARD_ILI9341_PLYWOOD
#define CLK 35 // 32 // 35 //
#define DT 32  // 33  // 32 //
#define SW 33  // 35  //  33//
#elif defined(BOARD_ILI9341_PLASTIC)
#define CLK 32 // 35 //
#define DT 33  // 32 //
#define SW 35  //  33//
#else
#error "Board type not defined!"
#endif

extern const char *PARAM_INPUT;
extern String sliderValue;
#ifdef BOARD_PCM5102
extern Audio audio;
#endif
extern bool volUpdate;
extern File myFile;

extern unsigned long currentMillis;

extern uint8_t NEWStation;
extern int numbStations; // количество радиостанций
// new banch
#define U_PART U_SPIFFS

static unsigned long lastUpdate = 0;
static unsigned long lastUpdateForRight = 0;

const unsigned long frameInterval = 30; // ~33 FPS
// int16_t spriteX = -250;        // Начинаем за пределами слева
enum State
{
  MOVING_TO_LEFT_EDGE,
  WAITING_AT_LEFT,
  MOVING_OFF_LEFT,
  MOVING_TO_LEFT, //-----------
  WAITING_AT_RIGHT,
  MOVING_TO_RIGHT,
  WAITING_TO_RIGHT,
  WAITING_TO_LEFT
};

// #define BOARD_ILI9341_PLASTIC  // раскомментируйте, если нужно

const unsigned long waitDuration = 3000; // 3 секунды ожидания
const int screenWidth = 320;
const int spriteWidth = 250;
const int speed = 4;    // Скорость движения (пикс/кадр)
const int spriteY = 64; // Y-позиция спрайта на экране

void stationDisplay(int st);
void deleteFile(fs::FS &fs, const String &path);
// void onMenu();
void onMenuNext();
void audioVolume();
void newrelease();
void onMenuPrev();
void notFound(AsyncWebServerRequest *request);
void handleDoUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
void initSpiffs();
void menuStation();
void listStaton();
void notifyWebClients();
void setupRoutes(AsyncWebServer &server);
String processor_update(const String &var);
extern AsyncWebSocket ws; // WebSocket эндпоинт
#endif
