
#ifdef BOARD_VS1053
#include "SPI.h"
#include "vs1053_ext.h"
#endif

#include <TFT_eSPI.h>
#include <display.h>
#include <ESPmDNS.h>
#include <WiFiManager.h>
#include <GyverNTP.h>
#include <settings.h>
#include <routes.h>
#include <AsyncTCP.h>
#include <UnixTime.h>

#include "../lib/fonts.h"
#include "../lib/CourierCyr10.h"
#include "../lib/Bahamas18.h"
#define CODE &FreeMonoBold8pt8b
#include "../lib/CourierCyr12.h" //для меню станций
#include "../lib/Free_Fonts.h"
#include "../lib/DS_DIGI28pt7b.h"

#define RU12 &FreeSansBold10pt8b
#define RU10 &FreeSans18pt7b
#define RU8 &FreeSans9pt7b
#define BAHAMAS &Bahamas18pt8b
#define SAN &FreeSans18pt7b
#define DIG20 &DIG_Bold_20

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite txtSprite = TFT_eSprite(&tft); // Create Sprite
TFT_eSprite vuSprite = TFT_eSprite(&tft);  // Create Sprite
TFT_eSprite txtTrek = TFT_eSprite(&tft);   // Create Sprite

UnixTime stamp(3); // указать GMT (3 для Москвы)

#ifdef BOARD_PCM5102
Audio audio;
#elif BOARD_VS1053
VS1053 audio(VS1053_CS, VS1053_DCS, VS1053_DREQ, VSPI, VS1053_MOSI, VS1053_MISO, VS1053_SCK);
#endif

// Loop
uint16_t ind;
int audiovol = 15;
String newSt;

int yForInit = 40; // Для вывода на первый экран
bool initStart = true;
bool showBitrate = false;
//---------
WiFiManager wifiManager;
/* this info will be read by the python script */
#define FORMAT_SPIFFS_IF_FAILED true

TaskHandle_t myTaskHandle = NULL;
// TaskHandle_t LogoHandle = NULL;

uint16_t PL_0 = tft.color565(115, 115, 115);
uint16_t PL_1 = tft.color565(89, 89, 89);
uint16_t PL_2 = tft.color565(56, 56, 56);
uint16_t PL_3 = tft.color565(35, 35, 35);
uint16_t ST_BG = tft.color565(231, 211, 90);
uint16_t VU_MIN = tft.color565(135, 125, 123);
uint16_t VU_MAX = tft.color565(231, 211, 90);
uint16_t color_volume = tft.color565(165, 165, 132);
uint16_t color_clock = tft.color565(231, 211, 90);
uint16_t COLOR_SNG_TITLE_1 = tft.color565(165, 162, 132);
uint16_t COLOR_SNG_TITLE_2 = tft.color565(255, 255, 255);

int ypos = 190; // position title
int xpos = 0;

String bitrate;

// Radio
uint8_t NEWStation = 0;
uint8_t OLDStation = 1;

int currentStationIndex = 0; // глобальная переменная

const int MAX_STATIONS = 50; // Задайте достаточный размер
int numbStations = 0;        // количество радиостанций

String displayStations[9];         // Массив для станций на дисплее
String StationList[MAX_STATIONS];  // Реальные станции в массиве заполняются в iniSpiffs() имена и url
String nameStations[MAX_STATIONS]; // Наименования станций
bool getClock = true;              // Получать время только при запуске
bool first = true;                 // Вывести дату и день недели
bool volUpdate = true;
String listRadio; // радиостанции на странице
unsigned long lastTime = 0;
unsigned long lastTime_ssid = 0;
unsigned long timerDelay_ssid = 4000;
uint32_t vumetersShow = 250;
int16_t spriteX = 0;         // Начинаем справа
int16_t spriteXForRIGHT = 0; // Начинаем с 0 позиции
State currentState = MOVING_TO_LEFT_EDGE;
State currentStateForRight = MOVING_TO_LEFT;
unsigned long stateStartTime = 0;
unsigned long stateStartTimeForRight = 0;

bool textUpdated = false;
unsigned long currentMillis;   // To return from the menu after the time has expired
unsigned long intervalForMenu; // Для возврата из меню по истечении времении
bool f_startProgress = true;
bool showRadio = true;         // show radio or menu of station,
bool directionStations = true; // Направление движения по меню

EncButton enc1(CLK, DT, SW);
File myFile;

String sliderValue;
const char *PARAM_INPUT = "value";
bool opened = false;
const char *PARAM = "file";
size_t content_len;

String CurrentDate;
uint8_t CurrentWeek;
String days[8] = {"Воскресенье", "ПН", "ВТ", "СР", "ЧТ", "ПТ", "СБ", "ВС"};

// Scrolling
String MessageToScroll_1 = F("For scrolling text 23 23 23 45 2 - Для прокрутки влево ");

// int totalStations = 0; // ← ВОТ ОН — реальный счётчик заполненных элементов!
int16_t width_txt;
// int16_t width_txtW;

int x_scroll_L;
int x_scroll_R;

GyverNTP ntp(3);
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const char *host = "esp32";
//---------------------------------
TaskHandle_t Task1;
TaskHandle_t logoTaskHandle;

bool showLogo = true;
SemaphoreHandle_t logoTaskSemaphore;

void Task1code(void *pvParameters);
void logoTask(void *cparameter);

void printStation(uint8_t indexOfStation);
void printCodecAndBitrate();

void readEEprom();
void initWiFi();
void wifiLevel();
void myEncoder();

void nextStation(bool stepStation);
void clock_on_core0();
void vuMeter();
void lineondisp();

static void rebootEspWithReason(String reason);
void performUpdate(Stream &updateSource, size_t updateSize);
String trim(const String &str);
String make_str(String str);
String utf8rus(String source);
String readFile(fs::FS &fs, const char *path);
void listDir(fs::FS &fs, const char *dirname, uint8_t levels);
void printProgress(size_t prg, size_t sz);

String processor_playlst(const String &var);
String processor(const String &var);
// void newrelease();
void startWiFiManager();
void printConnectionInfo();
int8_t txtSpriteHight = 17;
int8_t txtTrekHight = 17;
uint8_t ssid_show = 1;
//--- START ---
void setup()
{
  pinMode(LED_BUILT, OUTPUT);
  analogWrite(LED_BUILT, LED_BRIGHTNESS); // первоначальная яркость дисплея

  Serial.begin(115200);

#ifdef BOARD_VS1053
  SPI.begin(VS1053_SCK, VS1053_MISO, VS1053_MOSI);
#endif
  tft.begin();

  // Создаем семафор для сигнала о завершении задачи логотипа
  logoTaskSemaphore = xSemaphoreCreateBinary();

  xTaskCreatePinnedToCore(
      logoTask,
      "LogoTask",
      4096,
      NULL,
      0,
      &logoTaskHandle,
      1 // Ядро 1 для logo
  );

#ifdef BOARD_ILI9341_PLYWOOD
  tft.setRotation(1);
#elif defined(BOARD_ILI9341_PLASTIC)
  tft.setRotation(3);
#else
#error "Board type not defined!"
#endif
  // tft.loadFont(DS_DIGI28pt7b);

  Serial.println();
  Serial.printf("Flash chip size: %u MB\n", ESP.getFlashChipSize() / (1024 * 1024));
  Serial.printf("PSRAM size: %u KB\n", ESP.getPsramSize() / 1024);

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(40, yForInit);
  tft.println("Starting Radio...");

  readEEprom();
  initSpiffs();
  initWiFi();

  // tft.fillScreen(TFT_BLACK);
  delay(1000);

#ifdef BOARD_PCM5102
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(EEPROM.read(6));
#elif BOARD_VS1053
  // Start SPI bus
  SPI.begin(VS1053_SCK, VS1053_MISO, VS1053_MOSI);
#endif
  // tft.fillScreen(TFT_BLACK);
  // firs raw
  if (!MDNS.begin(host))
  {
    Serial.println("Error setting up MDNS responder!");
    while (1)
    {
      delay(500);
    }
  }

  Serial.println("mDNS responder started");

  listDir(SPIFFS, "/", 0);

  setupRoutes(server);
  ntp.begin();
  ntp.setHost("time.google.com");
  server.onNotFound(notFound);
  server.begin();
  Update.onProgress(printProgress);

  showLogo = false;
  // Ждем, пока задача логотипа завершится и освободит семафор
  if (xSemaphoreTake(logoTaskSemaphore, portMAX_DELAY) == pdTRUE)
  {
    // Задача завершилась, можно удалять семафор? Но мы больше не используем.
    vSemaphoreDelete(logoTaskSemaphore);
    logoTaskHandle = NULL;
  }
  delay(500);

  // The first connection
  ind = StationList[NEWStation].indexOf('\t');
  newSt = StationList[NEWStation].substring(ind + 1, StationList[NEWStation].length());
  const char *sl = newSt.c_str();
  // audio.begin();
  audio.setVolume(EEPROM.read(6));
  audio.connecttohost(sl); // переключаем станцию

  // stream.setVolume(EEPROM.read(6));
  // stream.connecttohost(sl); // переключаем станцию

  Serial.printf("\n %s \n", sl);
  OLDStation = NEWStation; //
  tft.fillScreen(TFT_BLACK);
  printStation(NEWStation); // display the name of the station on the screen
  printCodecAndBitrate();
  lineondisp();

  // For CORE0
  xTaskCreatePinnedToCore(
      Task1code,     /* Функция задачи. */
      "Task1",       /* Ее имя. */
      10000,         /* Размер стека функции */
      NULL,          /* Параметры */
      1,             /* Приоритет */
      &myTaskHandle, /* Дескриптор задачи для отслеживания */
      0);            /* Указываем пин для данного ядра */
  delay(1);

  tft.setSwapBytes(true);
  txtSprite.createSprite(278, txtSpriteHight); // Ширина и высота спрайта
  txtSprite.setTextSize(1);
  txtSprite.setTextColor(COLOR_SNG_TITLE_1, TFT_BLACK);
  txtSprite.fillSprite(TFT_BLACK);
  txtSprite.setFreeFont(&CourierCyr10pt8b);
  txtSprite.setTextDatum(TL_DATUM); // Привязка к верхнему левому углу

  txtTrek.createSprite(278, txtTrekHight); // Название трека
  txtTrek.setTextSize(1);
  txtTrek.setTextColor(COLOR_SNG_TITLE_2, TFT_BLACK);
  txtTrek.fillSprite(TFT_BLACK);
  txtTrek.setFreeFont(&CourierCyr10pt8b);
  txtTrek.setTextDatum(TL_DATUM); // Привязка к верхнему левому углу

  vuSprite.createSprite(60, 140); // Ширина 60, высота 150

  tft.setFreeFont(&CourierCyr10pt8b);
  tft.setTextSize(1);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.fillRect(x_wifi_ssid, y_wifi_ssid + 5, 183, 30, TFT_BLACK);
  tft.drawString(WiFi.SSID(), x_wifi_ssid, y_wifi_ssid);
  tft.drawString(WiFi.localIP().toString(), 160, y_wifi);
  // vTaskGetInfo(logoTaskHandle);
} // End Setup

//----------------------------------------
//*** Task for core 0 ***
//--------------------------------------
void Task1code(void *pvParameters)
{
  Serial.print("Task1 running on core ");
  Serial.println(xPortGetCoreID());
  for (;;)
  {
    audio.loop();
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}
int xLogo = 30;
int yLogo = 210;
void logoTask(void *cparametr)
{
  tft.setTextColor(color_clock, TFT_BLACK);
  tft.setTextSize(2);
  // while (showLogo)
  // {
  int count = 30;
  int widthLogo = 270;
  int hightLogo = 10;
  int widthLprogress = 7;
  tft.drawRect(xLogo, yLogo, widthLogo, hightLogo, TFT_BLUE);
  for (int i = 0; i < count; i++)
  {
    tft.fillRect(xLogo, yLogo, widthLprogress, hightLogo, color_clock);
    xLogo += 9;
    if (!showLogo)
      break;
    vTaskDelay(300 / portTICK_PERIOD_MS);
  }
  // Освобождаем семафор, сигнализируя о завершении
  xSemaphoreGive(logoTaskSemaphore);
  // Самоудаление
  vTaskDelete(NULL);
}

void volemeUpdate()
{
  if (volUpdate)
  {
    audioVolume();
    volUpdate = false;
  }
}
// подготовка к показу названия трека
void trekPreparingShow()
{
  if (title_flag && showRadio && show_title)
  {
    title_flag = false;
    txtTrek.fillRect(0, 47, 255, txtTrekHight, TFT_BLACK);
    txtTrek.drawString("                                               ", 0, 0);
    txtTrek.pushSprite(0, 47);
    txtSprite.fillRect(0, 64, 255, txtSpriteHight, TFT_BLACK);
    txtSprite.drawString("                                             ", 0, 0);
    txtSprite.pushSprite(0, 64);

    String str = MessageToScroll_1;
    char delimiter = '-';
    int pos = str.indexOf(delimiter); // Находим позицию символа
    if (pos != -1)
    {                                 // Если символ найден
      before = str.substring(0, pos); // До символа: "Hello"
      after = str.substring(pos + 1); // После символа: "World!"
      before = utf8rus(before) + char(0x20);
      after = utf8rus(after) + char(0x20);
      tft.setTextSize(1);
      tft.setTextColor(COLOR_SNG_TITLE_1);
      tft.setFreeFont(&CourierCyr10pt8b);
      txtTrek.fillRect(0, 47, 255, txtTrekHight, TFT_BLACK);
      txtTrek.drawString(before, 0, 0);
      tft.setTextColor(COLOR_SNG_TITLE_2);
      txtSprite.drawString(after, 0, 0);
      Serial.println(after);
    }
  }
}
//---------------------------
// Возврат этрана с часами после истечения времени ожидания
void returnFromDisplayScreen()
{
  intervalForMenu = millis() - currentMillis;
  if (intervalForMenu > 10000 && showRadio == false) // если время истекло
  {
    // stations = false;
    tft.fillScreen(TFT_BLACK);
    NEWStation = OLDStation;
    printStation(NEWStation);
    wifiLevel();
    getClock = true; // получить время при переходе от меню станций
    showRadio = true;
    vuSprite.createSprite(60, 140);
    txtSprite.createSprite(278, txtSpriteHight);
    txtTrek.createSprite(278, txtTrekHight);
    tft.setFreeFont(&CourierCyr10pt8b);
    tft.setTextSize(1);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString(WiFi.localIP().toString(), 160, y_wifi);
    lineondisp();
    printCodecAndBitrate();
    first = true;
  }
}
//-----------------------
// Если титры не получены
void noTitle()
{
  if (!show_title) // если не получены титры
  {
    txtSprite.fillScreen(TFT_BLACK);
    txtTrek.fillScreen(TFT_BLACK);
    after = "";
    before = "";
    txtTrek.fillRect(0, 47, 255, txtTrekHight, TFT_BLACK);
    txtSprite.fillRect(0, 64, 255, txtSpriteHight, TFT_BLACK);
  }
}
// Scrolling
void scrolling()
{
  if (millis() - lastUpdate > frameInterval)
  {
    tft.setTextSize(1);
    tft.setTextColor(0x9772);
    tft.setFreeFont(&CourierCyr10pt8b);
    lastUpdate = millis();
    unsigned long now = millis();
    unsigned long nowRight_2 = millis();
    switch (currentState)
    {
    case MOVING_TO_LEFT_EDGE:
      spriteX = spriteX - speed; // Движение влево
      if (spriteX <= -(tft.textWidth(after) - 250))
      { // Достигли левого края
        // spriteX = 0;
        currentState = WAITING_AT_LEFT;
        stateStartTime = now;
      }
      break;
    case WAITING_AT_LEFT:
      if (now - stateStartTime >= 3000)
      { // Ждем 3 секунды
        currentState = MOVING_OFF_LEFT;
      }
      break;
    case MOVING_OFF_LEFT:
      spriteX += speed; // Продолжаем движение влево
      // int16_t width = 320 - tft.textWidth(after);
      if (spriteX >= 0) //<= -tft.textWidth(after) - width)
      {                 // Полностью ушел за левый край
        // spriteX = 320;                      // Появляемся с правого края
        currentState = WAITING_TO_LEFT; // Начинаем цикл заново
      }
      break;
    case WAITING_TO_LEFT:
      if (nowRight_2 - stateStartTime >= 7000)
      { // Ждем 3 секунды
        currentState = MOVING_TO_LEFT_EDGE;
      }
      break;
    }
    txtSprite.drawString(after, spriteX, 0);
    txtSprite.pushSprite(0, 64);
  }
  //---Scrolling RIGHT
  if (millis() - lastUpdateForRight > frameInterval)
  {
    tft.setTextSize(1);
    tft.setTextColor(0x9772);
    tft.setFreeFont(&CourierCyr10pt8b);
    lastUpdateForRight = millis();
    unsigned long nowRight = millis();
    switch (currentStateForRight)
    {
    case MOVING_TO_LEFT:
      spriteXForRIGHT = spriteXForRIGHT - speed; // Движение влево
      if (spriteXForRIGHT <= -(tft.textWidth(before) - 250))
      {
        currentStateForRight = WAITING_AT_RIGHT;
        stateStartTimeForRight = nowRight;
      }
      break;
    case WAITING_AT_RIGHT:
      if (nowRight - stateStartTimeForRight >= 3000)
      { // Ждем 3 секунды
        currentStateForRight = MOVING_TO_RIGHT;
      }
      break;
    case MOVING_TO_RIGHT:
      spriteXForRIGHT += speed; // Двигаемс вправо
                                // int16_t width = 320 - tft.textWidth(after);
      if (spriteXForRIGHT >= 0)
      { // Дошли до левого края
        // spriteX = 320;
        currentStateForRight = WAITING_TO_RIGHT; // Начинаем цикл заново
      }
      break;
    case WAITING_TO_RIGHT:
      if (nowRight - stateStartTimeForRight >= 7000)
      { // Ждем 3 секунды
        currentStateForRight = MOVING_TO_LEFT;
      }
      break;
    }
    txtTrek.drawString(before, spriteXForRIGHT, 0);
    txtTrek.pushSprite(0, 47);
  }
}
//--------------
// show IP and SSID
// void showIPAndSSID()
// {
//   if ((millis() - lastTime_ssid) > timerDelay_ssid)
//   {
//     printCodecAndBitrate();
//   }
// }
//-----------------------
//*******************************
// START loop
//*******************************
unsigned long timer_prev = 0;
int timer_interval = 3000;
int timer_interval_W = 4000;
void loop()
{
  // обновить прогресс громкости на экране
  volemeUpdate();
  // подготовка для названия трека
  trekPreparingShow();
  // Опрос энкодера
  if (enc1.tick())
    myEncoder();
  // для возврата из меню по истечении времени
  returnFromDisplayScreen();

  if (showRadio)
  {
    clock_on_core0();
    // Титры не получены - очистить
    noTitle();
    //-------------
    // Scrolling
    scrolling();
    // выввод даты после меню станций
    if (first && CurrentDate != "Not sync" && CurrentDate != "20.02.1611")
    {
      tft.setTextSize(1);
      tft.setTextColor(0x9772);
      tft.setFreeFont(&CourierCyr12pt8b);
      tft.setCursor(285, 152);
      tft.print(utf8rus(days[CurrentWeek]));
      tft.setTextSize(1);
      tft.setFreeFont(RU8);
      tft.setTextColor(color_clock);
      tft.setCursor(x_data, y_data);
      tft.fillRect(x_data, y_data, 130, 30, TFT_BLACK);
      tft.drawString(CurrentDate, x_data, y_data);
      printStation(NEWStation);
      wifiLevel();
      printCodecAndBitrate();
      first = false;
    }
    if (NEWStation != OLDStation)
    {
      ind = StationList[NEWStation].indexOf('\t');
      newSt = StationList[NEWStation].substring(ind + 1, StationList[NEWStation].length());
      const char *sl = newSt.c_str();
      #ifdef BOARD_PCM5102
      audio.stopSong();
      #elif BOARD_VS1053
      audio.stop_mp3client();
      #endif
      printStation(NEWStation);
      delay(100);
      audio.setVolume(EEPROM.read(6));
      audio.connecttohost(sl); // новая станция
      OLDStation = NEWStation;
    }
    //-----vumeter
    if (vumetersShow < millis())
    {
      vuMeter();
      vumetersShow = millis() + 25;
    }
    // WiFi level
    unsigned long timer_curr = millis();
    if (timer_curr - timer_prev >= timer_interval) // 2 sec
    {
      timer_prev = timer_curr;
      wifiLevel();
    }
    // show IP and SSID
    if ((millis() - lastTime_ssid) > timerDelay_ssid)
    {
      printCodecAndBitrate();
      lastTime_ssid = millis();
    }
  }
} // end LOOP

//-------------------
// Показать VUmeter
//-------------------
void vuMeter()
{
  int x_show = 0;
  int width = 25;         // ширина
  int space = 3;          // расстояние между каналами
  int total_height = 140; // Высота VU-метра
  int y_offset = 80;      // сдиг сверху
  // Получаем текущие уровни (замените на ваши реальные значения)
  uint16_t vulevel = audio.getVUlevel();
  uint8_t y1_lev = (vulevel >> 8) & 0xFF; // Левый канал
  uint8_t y2_lev = vulevel & 0xFF;        // Правый канал

  int segment_height = 8;
  for (int y = 0; y < 150; y += segment_height)
  {
    uint16_t color = (y < 50) ? VU_MAX : (y < 100) ? TFT_CYAN
                                                   : VU_MIN;
    vuSprite.fillRect(0, y, 25, segment_height - 2, color);  // левый канал
    vuSprite.fillRect(28, y, 25, segment_height - 2, color); // правый канал
  }
  // уровни каналов
  vuSprite.fillRect(0, 0, 25, total_height - y1_lev, TFT_BLACK);
  vuSprite.fillRect(28, 0, 25, total_height - y2_lev, TFT_BLACK);

  // Выводим готовый спрайт на экран БЕЗ моргания
  vuSprite.pushSprite(8, y_offset);
}
//---------------------
//  Clock
//--------------------
byte omm = 99, oss = 99;
uint32_t targetTime_clock = 0; // update clock every second
byte xcolon = 0, xsecs_clock = 0;
uint8_t hh, mm, ss; // for new clock
// Получаем и выводим время
void clock_on_core0()
{
  if (targetTime_clock < millis())
  {
    tft.setTextColor(color_clock, TFT_BLACK);
    tft.setFreeFont(DIG20); //
    tft.setTextSize(3);     // 3
    ntp.tick();
    if (ntp.status() != 0)
    {
      Serial.println("Что-то с UPD сервером....");
      ntp.tick();
    }
    // vTaskDelay(20);
    hh = ntp.hour();
    mm = ntp.minute();
    ss = ntp.second();
    CurrentDate = ntp.dateString();
    CurrentWeek = ntp.dayWeek();

    // Set next update for 1 second later
    targetTime_clock = millis() + 1000;
    getClock = true;
    // Adjust the time values by adding 1 second
    ss++; // Advance second
    if (ss == 60)
    {           // Check for roll-over
      ss = 0;   // Reset seconds to zero
      omm = mm; // Save last minute time for display update
      mm++;     // Advance minute
      if (mm > 59)
      { // Check for roll-over
        mm = 0;
        hh++; // Advance hour
        if (hh > 23)
        {         // Check for 24hr roll-over (could roll-over on 13)
          hh = 0; // 0 for 24 hour clock, set to 1 for 12 hour clock
        }
      }
    }
    // Update digital time
    int xpos_clock = 65;
    int ypos_clock = 95; // Top left corner ot clock text, about half way down
    int ysecs_clock = ypos_clock;
    if (omm != mm || getClock == true)
    { // Redraw hours and minutes time every minute
      omm = mm;
      // Draw hours and minutes
      if (hh < 10)
        xpos_clock += tft.drawNumber(0, xpos_clock, ypos_clock); // Add hours leading zero for 24 hr clock
      xpos_clock += tft.drawNumber(hh, xpos_clock, ypos_clock);  // Draw hours
      xcolon = xpos_clock;                                       // Save colon coord for later to flash on/off later
      xpos_clock += tft.drawChar(':', xpos_clock, ypos_clock + 60);
      if (mm < 10)
        xpos_clock += tft.drawNumber(0, xpos_clock, ypos_clock); // Add minutes leading zero
      xpos_clock += tft.drawNumber(mm, xpos_clock, ypos_clock);  // Draw minutes
      xsecs_clock = xpos_clock;                                  // Sae seconds 'x' position for later display updates
    }
    if (oss != ss || getClock == true)
    { // Redraw seconds time every second
      oss = ss;
      xpos_clock = xsecs_clock;
      tft.setTextSize(3);
      if (ss % 2)
      {                                             // Flash the colons on/off
        tft.setTextColor(0x39C4, TFT_BLACK);        // Set colour to grey to dim colon
        tft.drawChar(':', xcolon, ypos_clock + 60); // Hour:minute colon
                                                    //  xpos += tft.drawChar(':', xsecs, ysecs); // Seconds colon
        tft.setTextColor(0x9772, TFT_BLACK);        // Set colour back to yellow
      }
      else
      {
        tft.drawChar(':', xcolon, ypos_clock + 60); // Hour:minute colon
                                                    // xpos += tft.drawChar(':', xsecs, ysecs); // Seconds colon
      }
      // Draw seconds
      tft.setTextSize(1);
      if (ss < 10)
        xpos_clock += tft.drawNumber(0, xpos_clock + 256, ysecs_clock + 3); // Add leading zero
      tft.drawNumber(ss, xpos_clock + 256, ysecs_clock + 3);                // Draw seconds
    }
  }
  getClock = false;
}
//------------------
//
//------------------
void notifyWebClients()
{
  String json = "{\"currentStationIndex\":" + String(NEWStation) + "}";
  ws.textAll(json);
}

//-------------------
// Encoder
//-------------------
void myEncoder()
{
  // enc1.tick();
  if (enc1.right())
  {
    if (showRadio)
    {
      nextStation(!directionStations);
      notifyWebClients();
    }
    if (!showRadio)
    {
      nextStation(!directionStations);
      stationDisplay(NEWStation);
      currentMillis = millis(); // Пока ходим по меню
    }
    // если меню
  }
  if (enc1.left())
  {
    if (showRadio)
    {
      nextStation(directionStations);
      notifyWebClients();
    }
    if (!showRadio)
    {
      nextStation(directionStations);
      stationDisplay(NEWStation);
      currentMillis = millis(); // Пока ходим по меню
    }
  }
  if (enc1.click())
  { // Меню станций
    showRadio = !showRadio;
    f_startProgress = true; // for starting
    if (!showRadio)
    {
      currentMillis = millis(); // начало отсчета времени простоя
      tft.fillRect(0, 0, 320, 220, TFT_BLACK);
      txtSprite.deleteSprite();
      txtTrek.deleteSprite();
      vuSprite.deleteSprite();
      stationDisplay(NEWStation);
    }
    if (showRadio)
    {
      first = true;
      txtSprite.createSprite(278, txtSpriteHight);
      txtTrek.createSprite(278, txtTrekHight);
      vuSprite.createSprite(60, 140);
      tft.fillRect(0, 0, 320, 220, TFT_BLACK);
      printStation(NEWStation);
      getClock = true; // получить время при переходе от меню станций
      lineondisp();
      printCodecAndBitrate();
      notifyWebClients();
      tft.setFreeFont(&CourierCyr10pt8b);
      tft.setTextSize(1);
      tft.setTextColor(TFT_CYAN, TFT_BLACK);
      tft.drawString(WiFi.localIP().toString(), 160, y_wifi);
    }
  }
  if (enc1.rightH())
  {
    audiovol = EEPROM.read(6);
    audiovol++;
    audio.setVolume(audiovol);
    EEPROM.write(6, audiovol);
    EEPROM.commit();
    audioVolume();
  }

  if (enc1.leftH())
  {
    audiovol = EEPROM.read(6);
    audiovol--;
    audio.setVolume(audiovol);
    EEPROM.write(6, audiovol);
    EEPROM.commit();
    audioVolume();
  }
  if (enc1.step(2))
  {
    WiFi.disconnect(false, true);
    wifiManager.resetSettings();
    Serial.println("Reseting creditals password.");
    delay(1000);
    ESP.restart();
  }
}

//----------------------------------
// ******* Menu stations ***********
// Вывод плейлиста на экран с центрированием текущей станции
//----------------------------------
void stationDisplay(int currentStation)
{
  const int MENU_SIZE = 8;     // Количество отображаемых станций
  const int HIGHLIGHT_POS = 4; // Позиция выделения (центр меню)
  const int LINE_HEIGHT = 25;  // Высота одной строки
  // Очищаем массив для отображения
  displayStations->clear();
  // Настраиваем шрифт и цвета
  tft.setTextSize(1);
  tft.setFreeFont(&CourierCyr12pt8b);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  // Заполняем массив для отображения
  for (int i = 0; i < MENU_SIZE; i++)
  {
    // Вычисляем индекс станции в общем списке с учетом циклического перехода
    int stationIndex = (currentStation - HIGHLIGHT_POS + i + numbStations) % (numbStations);
    displayStations[i] = nameStations[stationIndex];
  }
  // Отрисовываем все станции
  for (int i = 0; i < MENU_SIZE; i++)
  {
    int yPos = i * LINE_HEIGHT;
    // Очищаем область перед выводом
    tft.fillRect(65, yPos, 242, LINE_HEIGHT, TFT_BLACK);

    // Выводим текст станции
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(utf8rus(displayStations[i]), 65, yPos);
  }
  // Выделяем текущую станцию
  int highlightY = HIGHLIGHT_POS * LINE_HEIGHT;
  tft.fillRect(65, highlightY, 242, LINE_HEIGHT, ST_BG);
  tft.setTextColor(TFT_BLACK, ST_BG);
  tft.drawString(utf8rus(displayStations[HIGHLIGHT_POS]), 65, highlightY);
}
//----------------------------------
// Дополнить строку пробелами
String make_str(String str)
{
  for (int i = 0; i < (18 - str.length()); i++)
    str += char(32);
  return str;
}
//----------------------------
// Вывод наименования станции
//----------------------------
void printStation(uint8_t indexOfStation)
{
  uint8_t localIndex;
  String StName;
  localIndex = StationList[indexOfStation].indexOf('\t');
  StName = StationList[indexOfStation].substring(0, localIndex + 1);
  tft.setTextColor(TFT_BLACK, ST_BG);
  tft.setTextSize(1);
  tft.setFreeFont(BAHAMAS);
  tft.fillRect(0, 0, 318, 43, ST_BG);
  tft.fillRect(0, 44, 318, 43, TFT_BLACK); // очистка бегущей строки
  tft.drawString(utf8rus(StName), x_stName, y_stName);
  show_title = false;
} // end PrintStation
//----------------------------
// CodecName Bitrate
//----------------------------
void printCodecAndBitrate()
{
  int bit = audio.getBitRate(); // bitrate.toInt();
  tft.setFreeFont(CODE);
  tft.setTextSize(1);
  tft.setTextColor(ST_BG, TFT_BLACK);
  tft.fillRect(284, 45, 32, 33, ST_BG);
  tft.fillRect(286, 46, 28, 16, TFT_BLACK);
  if (bit < 128000)
  {
    tft.drawString(String(bit).substring(0, 2) + " ", 286, 46);
  }
  else
  {
    tft.drawString(String(bit).substring(0, 3), 286, 46);
  }
  tft.setTextColor(TFT_BLACK, ST_BG);
  tft.drawString(String(audio.getCodecname()).substring(0, 3) + " ", 286, 62);
  showBitrate = false;
  EEPROM.write(2, NEWStation);
  EEPROM.commit();
}

//-- new nextStation
void nextStation(bool stepStation)
{
  (stepStation) ? (NEWStation = (NEWStation - 1 + numbStations) % numbStations) : (NEWStation = (NEWStation + 1) % numbStations);
  EEPROM.write(2, NEWStation);
  EEPROM.commit();
}

//----------------------
//   Menu Stations
// Complect menu stations
//------------------------
void menuStation()
{
  int i = 0;
  int ind = 0;
  for (int i = 0; i < numbStations; i++)
  { // list stations
    ind = StationList[i].indexOf('\t');
    if (ind == -1)
    {
      Serial.printf("WARNING: No tab in StationList[%d]: %s\n", i, StationList[i].c_str());
      nameStations[i] = make_str(utf8rus(StationList[i])); // Берем всю строку
    }
    else
    {
      nameStations[i] = make_str(utf8rus(StationList[i].substring(0, ind))); // Получили именования
    }
  }
}
// SPIFFS
void initSpiffs()
{
  for (int i = 0; i < MAX_STATIONS; i++)
  {
    StationList[i] = "";
    nameStations[i] = "";
  }

  if (!SPIFFS.begin(true))
  {
    Serial.println("SPIFFS Mount Failed");
    numbStations = 0; // На всякий случай
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  myFile = SPIFFS.open("/playlist.txt", FILE_READ);
  if (!myFile)
  {
    Serial.println("------File does not exist!------");
    Serial.println("Failed to open stations file");
    numbStations = 0;
    return;
  }

  size_t fileSize = myFile.size();
  size_t freeSpace = SPIFFS.totalBytes() - SPIFFS.usedBytes();

  Serial.printf("File size: %d bytes\n", fileSize);
  Serial.printf("Lines in file: ");

  int i = 0;
  while (myFile.available() && i < MAX_STATIONS)
  {
    StationList[i] = myFile.readStringUntil('\n'); // Станции в массиве пронумерованы от 0
    if (i >= MAX_STATIONS)
      break; // Дополнительная защита
    i++;
  }
  myFile.close();
  numbStations = i; // Количесто реальных станций
  Serial.printf("Read %d stations, numbStations = %d\n", i, numbStations);
  Serial.printf("SPIFFS total: %d bytes\n", SPIFFS.totalBytes());
  Serial.printf("SPIFFS used: %d bytes\n", SPIFFS.usedBytes());
  Serial.printf("SPIFFS free: %d bytes\n", freeSpace);
  menuStation();
  listStaton();
  if (initStart)
  {
    yForInit += 20;
    tft.setCursor(40, yForInit);
    tft.print("Init SPIFFS has passed.");
    initStart = false;
  }
}
// EEPROM
void readEEprom()
{
  if (!EEPROM.begin(50))
  {
    Serial.println("failed to initialise EEPROM");
    delay(1000);
  }
  Serial.println(" bytes read from Flash . Values are:");

  (EEPROM.read(2) > 200) ? (NEWStation = 0) : (NEWStation = EEPROM.read(2));

  if (EEPROM.read(6) > 21)
  {
    sliderValue = 15;
    EEPROM.write(6, 15);
    EEPROM.commit();
  }
  else
  {
    sliderValue = EEPROM.read(6);
    audio.setVolume(sliderValue.toInt());
  }
  yForInit += 20;
  tft.setCursor(40, yForInit);
  tft.print("Init EEPROM has passed");
}
//****************************
//    WiFi
//****************************
void initWiFi()
{
  // Список известных WiFi сетей (SSID и пароль)
  const char *knownNetworks[][2] = {
      {"RT-GPON-D5D9", "tB5DVdR9"},
      {"WiFi-Repeater", "tB5DVdR9"}};

  const int networkCount = sizeof(knownNetworks) / sizeof(knownNetworks[0]);

  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);

  Serial.println("Scanning for available networks...");
  yForInit += 20;
  tft.setCursor(40, yForInit);
  tft.println("Scanning WiFi...");

  // Сканируем доступные сети
  int n = WiFi.scanNetworks();
  if (n == 0)
  {
    tft.setCursor(40, 140);
    Serial.println("No networks found");
    tft.println("No networks found");
    startWiFiManager();
    return;
  }
  // Ищем известные сети среди доступных
  struct NetworkInfo
  {
    String ssid;
    String password;
    int32_t rssi;
    int channel;
  };
  std::vector<NetworkInfo> availableNetworks;

  for (int i = 0; i < n; ++i)
  {
    String scannedSSID = WiFi.SSID(i);
    int32_t rssi = WiFi.RSSI(i);
    int channel = WiFi.channel(i);

    // Проверяем, есть ли эта сеть в списке известных
    for (int j = 0; j < networkCount; j++)
    {
      if (scannedSSID.equals(knownNetworks[j][0]))
      {
        NetworkInfo net;
        net.ssid = scannedSSID;
        net.password = knownNetworks[j][1];
        net.rssi = rssi;
        net.channel = channel;
        availableNetworks.push_back(net);
        break;
      }
    }
  }
  // Сортируем сети по силе сигнала (от сильного к слабому)
  std::sort(availableNetworks.begin(), availableNetworks.end(),
            [](const NetworkInfo &a, const NetworkInfo &b)
            {
              return a.rssi > b.rssi;
            });

  // Пытаемся подключиться к сетям по порядку (от сильного сигнала к слабому)
  for (const auto &net : availableNetworks)
  {
    Serial.print("Trying to connect to: ");
    Serial.print(net.ssid);
    Serial.print(" (RSSI: ");
    Serial.print(net.rssi);
    Serial.println(" dBm)");

    WiFi.begin(net.ssid.c_str(), net.password.c_str(), net.channel);

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000)
    {
      delay(500);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("\nConnected successfully!");
      printConnectionInfo();
      return;
    }

    Serial.println(" Failed");
    tft.println("Connection failed");
  }

  // Если ни к одной известной сети не удалось подключиться
  Serial.println("Could not connect to any known network");
  startWiFiManager();
}

void startWiFiManager()
{
  tft.fillScreen(TFT_BLACK);
  yForInit += 20;
  tft.setCursor(40, yForInit);
  Serial.println("Starting WiFiManager");
  tft.println("Starting WiFiManager");
  tft.println("SSID: ESP32-Clock");
  tft.println("IP: 192.168.4.1");

  wifiManager.setConfigPortalTimeout(180);
  if (!wifiManager.autoConnect("ESP32-Clock"))
  {
    Serial.println("Failed to connect and config portal timeout");
    tft.println("Failed to connect");
    delay(500);
    ESP.restart();
  }
}
void printConnectionInfo()
{
  Serial.println("\nWiFi connected successfully!");
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("RSSI: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");

  // Вывод информации о системе
  String sysInfo;
  sysInfo += F("\nChip Model: ");
  sysInfo += ESP.getChipModel();
  sysInfo += F("\nRevision: ");
  sysInfo += ESP.getChipRevision();
  sysInfo += F("\nCores: ");
  sysInfo += ESP.getChipCores();
  sysInfo += F("\nPSRAM: ");
  sysInfo += ESP.getPsramSize();
  sysInfo += F(" bytes");
  sysInfo += F("\nFlash Size: ");
  sysInfo += ESP.getFlashChipSize();
  sysInfo += F(" bytes");
  sysInfo += F("\nFree Heap: ");
  sysInfo += ESP.getFreeHeap();
  sysInfo += F(" bytes");
  sysInfo += F("\nHeapSize: ");
  sysInfo += ESP.getHeapSize();
  sysInfo += F(" bytes");
  sysInfo += F("\nFree PSRAM: ");
  sysInfo += ESP.getFreePsram();
  sysInfo += F(" bytes");

  Serial.println(sysInfo);

  // Вывод на TFT дисплей
  yForInit += 20;
  tft.setCursor(40, yForInit);
  tft.print("WiFi connected!");
  yForInit += 20;
  tft.setCursor(40, yForInit);
  tft.print("SSID: ");
  tft.println(WiFi.SSID());
  yForInit += 20;
  tft.setCursor(40, yForInit);
  tft.print("IP: ");
  tft.println(WiFi.localIP());
  yForInit += 20;
  tft.setCursor(40, yForInit);
  tft.print("Signal: ");
  tft.print(WiFi.RSSI());
  tft.println(" dBm");
}
// end initwifi

// уровень вафай
void wifiLevel()
{
  uint16_t x_wifi = 90, y_wifi = ypos;
  int8_t rssi = WiFi.RSSI();
  // Определяем количество полосок по уровню сигнала
  int bars;
  if (rssi >= -55)
    bars = 7;
  else if (rssi >= -65)
    bars = 6;
  else if (rssi >= -70)
    bars = 5;
  else if (rssi >= -75)
    bars = 4;
  else if (rssi >= -80)
    bars = 3;
  else if (rssi >= -85)
    bars = 2;
  else
    bars = 1;

  // Очищаем область перед отрисовкой
  tft.fillRect(x_wifi + 8, y_wifi - 12, 35, 15, TFT_BLACK);

  // Рисуем полоски сигнала
  for (int i = 0; i < 7; i++)
  {
    int height = 3 + i * 2;
    int y_pos = y_wifi - i * 2;
    uint16_t color = (i < bars) ? TFT_CYAN : 0x39C4;

    tft.fillRect(x_wifi + 8 + i * 5, y_pos + 3, 3, height, color);
  }
}

// Движение по меню через сайт
void onMenuPrev()
{
  if (showRadio)
  {
    nextStation(!directionStations);
    notifyWebClients();
    // printStation(NEWStation);
  }
  if (!showRadio)
  {
    nextStation(!directionStations);
    stationDisplay(NEWStation);
    currentMillis = millis(); // Пока ходим по меню
  }
}
// Движение по меню через сайт
void onMenuNext()
{
  if (showRadio)
  {
    nextStation(directionStations);
    notifyWebClients();
    // printStation(NEWStation);
  }
  if (!showRadio)
  {
    nextStation(directionStations);
    // menuStation();
    stationDisplay(NEWStation);
    currentMillis = millis(); // Пока ходим по меню
  }
}
// Показать меню радиостанций
void onMenu()
{
  showRadio = !showRadio;
  f_startProgress = true; // for starting
  if (!showRadio)
  {
    currentMillis = millis(); // начало отсчета времени простоя
    tft.fillRect(0, 0, 320, 220, TFT_BLACK);
    vuSprite.deleteSprite();
    txtSprite.deleteSprite();
    txtTrek.deleteSprite();
    stationDisplay(NEWStation);
  }
  if (showRadio)
  {
    first = true;
    txtSprite.createSprite(278, txtSpriteHight);
    txtTrek.createSprite(278, txtTrekHight);
    vuSprite.createSprite(60, 140);
    tft.fillRect(0, 0, 320, 220, TFT_BLACK);
    // printStation(NEWStation);
    // getClock = true; // получить время при переходе
    lineondisp();
    printCodecAndBitrate();
  }
}
/*
String readFile(fs::FS &fs, const char *path)
{
  Serial.printf("Reading file: %s\r\n", path);
  myFile = fs.open(path);
  if (!myFile || myFile.isDirectory())
  {
    Serial.println("- failed to open file for reading");
    return String();
  }
  String fileContent;
  while (mbedtls_entropy_f_source_ptravailable())
  {
    fileContent = myFile.readStringUntil('\n');
    break;
  }
  return fileContent;
}*/
// end readFile

String utf8rus(String source)
{
  int i, k;
  String target;
  unsigned char n;
  char m[2] = {'0', '\0'};
  k = source.length();
  i = 0;
  while (i < k)
  {
    delay(1);
    n = source[i];
    i++;
    if (n >= 127)
    {
      switch (n)
      {
      case 208:
      {
        n = source[i];
        i++;
        if (n == 129)
        {
          n = 192; // перекодируем букву Ё
          break;
        }
        break;
      }
      case 209:
      {
        n = source[i];
        i++;
        if (n == 145)
        {
          n = 193; // перекодируем букву ё
          break;
        }
        break;
      }
      }
    }
    m[0] = n;
    target = target + String(m);
  }
  return target;
}

void audio_showstreamtitle(const char *info)
{
  title_flag = true;
  show_title = true;
  MessageToScroll_1 = info;
  // width_txt = tft.textWidth(MessageToScroll_1);
  MessageToScroll_1 = F(" ");
  MessageToScroll_1 += trim(info);
  MessageToScroll_1 += F(" ");
  MessageToScroll_1 = utf8rus(MessageToScroll_1);
}
void audio_codec(const char *info)
{
  Serial.println(info);
}

void audio_bitrate(const char *info)
{
  bitrate = info;
}

// ---------new ListRadio ----------
void listStaton()
{
  String partlistStation;
  uint8_t i = 0;
  while (i <= numbStations) // ← важно: <, а не <=
  {
    int ind_to_scace = StationList[i].indexOf('\t');
    if (ind_to_scace == -1)
    {
      // Защита от некорректных строк
      i++;
      continue;
    }

    String nameStat = StationList[i].substring(0, ind_to_scace);
    String urlStat = StationList[i].substring(ind_to_scace + 1);

    String rowClass = "";
    if (i == NEWStation)
    {
      rowClass = "current-station";
    }
    partlistStation += "<tr class=\"station-row " + rowClass + "\" data-station-id=\"" + String(i) + "\""
                                                                                                     ">"
                                                                                                     "<td>" +
                       String(i) + "</td>"
                                   "<td>" +
                       nameStat + "</td>"
                                  "<td>" +
                       urlStat + "</td>"
                                 "</tr>";
    i++;
  }

  listRadio = "<table id=\"myTable\" class=\"table table-success table-striped\">"
              "<thead><tr><th>№</th><th>Station name</th><th>Station url</th></tr></thead>"
              "<tbody>" +
              partlistStation + "</tbody></table>";
}

// ---------end ---------------
String processor(const String &var)
{
  if (var == "SLIDERVALUE")
  {
    return String(EEPROM.read(6));
  }

  return String();
}

//------------------
void notFound(AsyncWebServerRequest *request)
{
  if (request->url().startsWith("/"))
  {
    request->send(SPIFFS, request->url(), String(), true);
  }
  else
  {
    request->send(404);
  }
}

void deleteFile(fs::FS &fs, const String &path)
{
  Serial.printf("Deleting file: %s\r\n", path);
  if (fs.remove(path))
  {
    Serial.println("- file deleted");
  }
  else
  {
    Serial.println("- delete failed");
  }
}
void lineondisp()
{
  // temperature
  // tft.drawRect(0, 0, 70, 55, TFT_CYAN);
  tft.setFreeFont();
  tft.setTextSize(2);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);

  // station and streametitle
  tft.drawRect(0, 0, 320, 45, TFT_CYAN);
  // VuMeter
  // tft.drawRect(0, 88, 70, 131, TFT_CYAN);
  // clock секунды
  // tft.drawRect(260, 87, 60, 43, 0x9772);
  tft.drawLine(260, 130, 320, 130, 0x9772);
  // tft.drawLine(284, 95, 284, 160, 0x9772);
  //  wifi level codec bitrate
  tft.drawRect(70, 175, 250, 42, TFT_CYAN);
  tft.setTextColor(TFT_CYAN);
  // tft.drawString("WiFi", x_wifi - 5, y_wifi);
  audioVolume();
}

//**********************************
//  Уровень громкости
//**********************************
int x_FP = 75, y_FP = ypos + 13; // position in line
void audioVolume()
{
  int16_t volumeLevel = audio.getVolume() * 10;
  tft.fillRect(x_FP, y_FP, 210, 5, TFT_BLACK);
  tft.drawRect(x_FP, y_FP, 210, 6, TFT_BLUE);
  tft.fillRect(x_FP, y_FP, volumeLevel, 6, color_volume);
  tft.setTextSize(1);
  tft.setFreeFont(RU8);
  tft.fillRect(290, y_FP - 5, 20, 17, TFT_BLACK);
  tft.drawString(String(volumeLevel / 10), 290, y_FP - 5);
}

static void rebootEspWithReason(String reason)
{
  Serial.println(reason);
  delay(1000);
  ESP.restart();
}

void performUpdate(Stream &updateSource, size_t updateSize)
{
  String result = "";
  if (Update.begin(updateSize))
  {
    size_t written = Update.writeStream(updateSource);
    if (written == updateSize)
    {
      Serial.println("Written : " + String(written) + " successfully");
    }
    else
    {
      Serial.println("Written only : " + String(written) + "/" + String(updateSize) + ". Retry?");
    }
    result += "Written : " + String(written) + "/" + String(updateSize) + " [" + String((written / updateSize) * 100) + "%] \n";
    if (Update.end())
    {
      Serial.println("OTA done!");
      result += "OTA Done: ";
      if (Update.isFinished())
      {
        Serial.println("Update successfully completed. Rebooting...");
        result += "Success!\n";
      }
      else
      {
        Serial.println("Update not finished? Something went wrong!");
        result += "Failed!\n";
      }
    }
    else
    {
      Serial.println("Error Occurred. Error #: " + String(Update.getError()));
      result += "Error #: " + String(Update.getError());
    }
  }
  else
  {
    Serial.println("Not enough space to begin OTA");
    result += "Not enough space for OTA";
  }
  // http send 'result'
}

String trim(const String &str)
{
  const String WHITESPACE = " \n\r\t\f\v";
  int start = 0;
  int end = str.length() - 1;
  // Пропускаем ведущие пробелы
  while (start <= end && WHITESPACE.indexOf(str.charAt(start)) >= 0)
  {
    start++;
  }
  // Пропускаем завершающие пробелы
  while (end >= start && WHITESPACE.indexOf(str.charAt(end)) >= 0)
  {
    end--;
  }
  // Возвращаем подстроку
  if (start > end)
    return "";
  return str.substring(start, end + 1);
}