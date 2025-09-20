#include <TFT_eSPI.h>
#include <display.h>
#include <ESPmDNS.h>
#include <WiFiManager.h>
#include <GyverNTP.h>
#include <settings.h>
#include <routes.h>
#include <UnixTime.h>
#include <ImgWea60.h>

#include "../lib/fonts.h"
#include "../lib/CourierCyr10.h"
#include "../lib/Bahamas18.h"
#include "../lib/CourierCyr12.h" //для меню станций
#include "../lib/Free_Fonts.h"
#include "../lib/DS_DIGI28pt7b.h"
#define RU12 &FreeSansBold10pt8b
#define RU10 &FreeSans18pt7b
#define RU8 &FreeSans9pt7b
#define BAHAMAS &Bahamas18pt8b
#define SAN &FreeSans18pt7b
#define LED_BRIGHTNESS 70 // яркость дисплея при старте
#define LED_BUILT 22      // управление яркостью дисплея
#define DIG20 &DIG_Bold_20

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite txtSprite = TFT_eSprite(&tft); // Create Sprite
TFT_eSprite vuSprite = TFT_eSprite(&tft);  // Create Sprite

UnixTime stamp(3); // указать GMT (3 для Москвы)

// Loop
uint16_t ind;
int audiovol = 15;
String newSt;
const String space = " ";
//---------
// WiFiManager wifiManager;
WiFiManager wifiManager;
/* this info will be read by the python script */
#define FORMAT_SPIFFS_IF_FAILED true

TaskHandle_t myTaskHandle = NULL;

uint16_t PL_0 = tft.color565(115, 115, 115);
uint16_t PL_1 = tft.color565(89, 89, 89);
uint16_t PL_2 = tft.color565(56, 56, 56);
uint16_t PL_3 = tft.color565(35, 35, 35);
uint16_t ST_BG = tft.color565(231, 211, 90);
uint16_t VU_MIN = tft.color565(135, 125, 123);
uint16_t VU_MAX = tft.color565(231, 211, 90);
uint16_t color_volume = tft.color565(165, 165, 132);
uint16_t color_clock = tft.color565(231, 211, 90);

int ypos = 190; // position title
int xpos = 0;

String bitrate;

// Radio
uint8_t NEWStation = 0;
uint8_t OLDStation = 1;
int numbStations = 35; // количество радиостанций

String displayStations[8]; // Массив для станций на дисплее
String StationList[35];    // Всего станций
String nameStations[35];   // Наименования ст
// bool calendar = false;
bool getClock = true; // Получать время только при запуске
bool first = true;    // Вывести дату и день недели
// bool rnd = true;      // для случ числа

String listRadio; // радиостанции на странице
unsigned long lastTime = 0;
unsigned long lastTime_ssid = 0;
unsigned long timerDelay_ssid = 4000;
uint32_t vumetersDelay = 250;
int16_t spriteX = 320; // Начинаем справа
State currentState = MOVING_TO_LEFT_EDGE;
unsigned long stateStartTime = 0;
bool textUpdated = false;
unsigned long currentMillis;   // To return from the menu after the time has expired
unsigned long intervalForMenu; // Для возврата из меню по истечении времении
bool f_startProgress = true;
bool showRadio = true; // show radio or menu of station,
bool stations;         // Станция вверх или вниз (true or false)
void nextStation(bool stepStation);

EncButton enc1(CLK, DT, SW);
File file;
bool volUpdate = true;
String sliderValue;
const char *PARAM_INPUT = "value";
bool opened = false;
const char *PARAM = "file";
size_t content_len;

int y1_prev = 210;
int y1_lev = 210;
int y2_prev = 210;
int y2_lev = 210;

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

Audio audio;
GyverNTP ntp(3);
AsyncWebServer server(80);

const char *host = "esp32";
//---------------------------------
TaskHandle_t Task1;

void Task1code(void *pvParameters);
void printStation(uint8_t indexOfStation);
void printCodecAndBitrate();

void initSpiffs();
void readEEprom();
void initWiFi();
void wifiLevel();
void myEncoder();
void menuStation();
void clock_on_core0();
// void drawlineClock();
void soundShow();
void lineondisp();
void audioVolume();
void filePosition();
static void rebootEspWithReason(String reason);
void performUpdate(Stream &updateSource, size_t updateSize);
void updateFromFS(fs::FS &fs);

String trim(const String &str);
String make_str(String str);
String utf8rus(String source);
String readFile(fs::FS &fs, const char *path);

void listDir(fs::FS &fs, const char *dirname, uint8_t levels);

void printProgress(size_t prg, size_t sz);
void listStaton();

String processor_playlst(const String &var);
String processor(const String &var);
void newrelease();
void startWiFiManager();
void printConnectionInfo();
//--- START ---
void setup()
{
  pinMode(LED_BUILT, OUTPUT);
  analogWrite(LED_BUILT, LED_BRIGHTNESS); // первоначальная яркость дисплея

  Serial.begin(115200);
  tft.begin();
  tft.setRotation(3);
  // tft.loadFont(DS_DIGI28pt7b);

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(40, 60);
  tft.println("Starting Radio...");

  readEEprom();
  initSpiffs();
  initWiFi();
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(40, 90);
  tft.println("Connected to SSID: ");
  tft.setCursor(40, 120);
  tft.println(WiFi.SSID());
  delay(1000);

  // newVer();
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(EEPROM.read(6));
  // audio.setVolume(audiovol);

  tft.fillScreen(TFT_BLACK);
  // firs raw
  if (!MDNS.begin(host))
  {
    Serial.println("Error setting up MDNS responder!");
    while (1)
    {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");

  listDir(SPIFFS, "/", 0);

  setupRoutes(server);
  ntp.begin();
  server.onNotFound(notFound);
  server.begin();
  Update.onProgress(printProgress);

  // The first connection
  ind = StationList[NEWStation].indexOf(space);
  newSt = StationList[NEWStation].substring(ind + 1, StationList[NEWStation].length());
  const char *sl = newSt.c_str();
  audio.setVolume(EEPROM.read(6));
  audio.connecttohost(sl); // переключаем станцию
  Serial.println(sl);
  OLDStation = NEWStation;  //
  printStation(NEWStation); // display the name of the station on the screen
  printCodecAndBitrate();
  lineondisp();

  // For CORE0
  xTaskCreatePinnedToCore(
      Task1code,     /* Функция задачи. */
      "Task1",       /* Ее имя. */
      4000,          /* Размер стека функции */
      NULL,          /* Параметры */
      1,             /* Приоритет */
      &myTaskHandle, /* Дескриптор задачи для отслеживания */
      0);            /* Указываем пин для данного ядра */
  delay(1);

  tft.setSwapBytes(true);
  txtSprite.createSprite(250, 25); // Ширина и высота спрайта
  txtSprite.setTextSize(1);
  txtSprite.setTextColor(TFT_WHITE, TFT_BLACK);
  txtSprite.fillSprite(TFT_BLACK);
  txtSprite.setFreeFont(&CourierCyr10pt8b);
  txtSprite.setTextDatum(TL_DATUM); // Привязка к верхнему левому углу
  vuSprite.createSprite(60, 150);   // Ширина 60, высота 150
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
    vTaskDelay(2);
  }
}
//*******************************
// START loop
//*******************************
unsigned long timer_prev = 0;
int timer_interval = 3000;
bool allow = true;
int timer_interval_W = 4000;
bool allow_W = true;
bool direct, direct1;
int x_sprite = 65;
uint8_t ssid_show = 1;
void loop()
{
  if (title_flag)
  {
    title_flag = false;
    tft.fillRect(70, 47, 250, 25, TFT_BLACK);
    String str = MessageToScroll_1;
    char delimiter = '-';
    int pos = str.indexOf(delimiter); // Находим позицию символа
    if (pos != -1)
    {                                 // Если символ найден
      before = str.substring(0, pos); // До символа: "Hello"
      after = str.substring(pos + 1); // После символа: "World!"
      after = utf8rus(after) + char(0x20);
      tft.setTextSize(1);
      tft.setTextColor(0x9772);
      tft.setFreeFont(&CourierCyr10pt8b);
      // tft.fillRect(0, 47, 319, 20, TFT_BLACK);
      tft.drawString(utf8rus(before), 70, 47);
      txtSprite.drawString(after, 320, 0);
    }
  }

  if (enc1.tick())
    myEncoder();
  // для возврата из меню
  intervalForMenu = millis() - currentMillis;
  if (intervalForMenu > 10000 && showRadio == false) // если время истекло
  {
    // stations = false;
    tft.fillRect(0, 0, 320, ypos + 8, TFT_BLACK);
    NEWStation = OLDStation;
    printStation(NEWStation);
    wifiLevel();
    getClock = true; // получить время при переходе от меню станций
    showRadio = true;
    lineondisp();
    printCodecAndBitrate();
    first = true;
  }
  if (showRadio)
  {
    clock_on_core0();
    //-------------
    // Scrolling
    //-------------
    if (!show_title) // если не получены титры
    {
      txtSprite.fillScreen(TFT_BLACK);
      after = "";
    }
    if (millis() - lastUpdate > frameInterval)
    {
      lastUpdate = millis();
      unsigned long now = millis();
      switch (currentState)
      {
      case MOVING_TO_LEFT_EDGE:
        spriteX = spriteX - speed; // Движение влево
        if (spriteX <= 0)
        { // Достигли левого края
          spriteX = 0;
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
        spriteX -= speed; // Продолжаем движение влево
        // Serial.println("2 -- "+spriteX);
        int16_t width = 320 - tft.textWidth(after);
        if (spriteX <= -tft.textWidth(after) - width)
        {                                     // Полностью ушел за левый край
          spriteX = 320;                      // Появляемся с правого края
          currentState = MOVING_TO_LEFT_EDGE; // Начинаем цикл заново
        }
        break;
      }
      txtSprite.drawString(after, spriteX, 0);
      txtSprite.pushSprite(70, 64);
    }
    //----------------------------------------
    if (first && CurrentDate != "Not sync" && CurrentDate != "20.02.1611")
    { // выввод даты после меню станций
      tft.setTextSize(1);
      tft.setTextColor(0x9772);
      tft.setFreeFont(&CourierCyr12pt8b);
      tft.setCursor(285, 152);
      tft.print(utf8rus(days[CurrentWeek]));

      tft.setTextSize(1);
      tft.setFreeFont(RU8);
      tft.setTextColor(color_clock);
      tft.setCursor(x_data, y_data);
      tft.print(CurrentDate);
      printStation(NEWStation);
      wifiLevel();
      printCodecAndBitrate();
      first = false;
    }
    if (NEWStation != OLDStation)
    {
      // StationList[NEWStation].replace("_", space);
      ind = StationList[NEWStation].indexOf(space);
      newSt = StationList[NEWStation].substring(ind + 1, StationList[NEWStation].length());
      const char *sl = newSt.c_str();
      audio.pauseResume();
      printStation(NEWStation);
      delay(100);
      audio.setVolume(EEPROM.read(6));
      audio.connecttohost(sl); // новая станция
      OLDStation = NEWStation;
    }

    if (vumetersDelay < millis())
    {
      soundShow();
      vumetersDelay = millis() + 25;
    } //-----end vumeter

    unsigned long timer_curr = millis();
    if (timer_curr - timer_prev >= timer_interval) // 2 sec
    {
      allow = !allow;
      timer_prev = timer_curr;
      direct = false; // random(0, 2);
      wifiLevel();
      if (volUpdate)
      {
        audioVolume();
        volUpdate = false;
      }
    }

    if ((millis() - lastTime_ssid) > timerDelay_ssid)
    {
      printCodecAndBitrate();
      switch (ssid_show)
      {
      case 1:
        tft.setFreeFont(&CourierCyr10pt8b);
        tft.setTextSize(1);
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.fillRect(x_wifi_ssid, y_wifi_ssid + 5, 183, 30, TFT_BLACK);
        tft.drawString(WiFi.SSID(), x_wifi_ssid, y_wifi_ssid);
        lastTime_ssid = millis();
        ssid_show = 2;
        break;
      case 2:
        tft.setFreeFont(&CourierCyr10pt8b);
        tft.setTextSize(1);
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.fillRect(x_WiFi_localIP, y_WiFi_localIP + 5, 183, 20, TFT_BLACK);
        tft.drawString(WiFi.localIP().toString(), x_WiFi_localIP, y_WiFi_localIP);
        lastTime_ssid = millis();
        ssid_show = 1;
        break;
      }
    }
  }
} // end LOOP

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
// Показать VUmeter
void soundShow()
{
  int x_show = 0;
  int width = 25;         // ширина
  int space = 3;          // расстояние между каналами
  int total_height = 150; // Высота VU-метра
  int y_offset = 70;      // сдиг сверху

  // Получаем текущие уровни (замените на ваши реальные значения)
  uint16_t vulevel = audio.getVUlevel();
  uint8_t y1_lev = (vulevel >> 8) & 0xFF; // Левый канал
  uint8_t y2_lev = vulevel & 0xFF;        // Правый канал

  int segment_height = 10;
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
  tft.setTextColor(color_clock, TFT_BLACK);
  tft.setFreeFont(DIG20); //
  tft.setTextSize(3);     // 3
  ntp.tick();
  delay(2);
  hh = ntp.hour();
  mm = ntp.minute();
  ss = ntp.second();
  CurrentDate = ntp.dateString();
  CurrentWeek = ntp.dayWeek();
  if (targetTime_clock < millis())
  {
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
      stations = false;
      nextStation(stations);
      printStation(NEWStation);
    }
    if (!showRadio)
    {
      stations = false;
      nextStation(stations);
      // menuStation();
      stationDisplay(NEWStation);
      currentMillis = millis(); // Пока ходим по меню
    }
    // если меню
  }
  if (enc1.left())
  {
    if (showRadio)
    {
      stations = true;
      nextStation(stations);
      printStation(NEWStation);
    }
    if (!showRadio)
    {
      stations = true;
      nextStation(stations);
      // menuStation();
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
      stationDisplay(NEWStation);
    }
    if (showRadio)
    {
      first = true;
      tft.fillRect(0, 0, 320, ypos + 8, TFT_BLACK);
      printStation(NEWStation);
      getClock = true; // получить время при переходе от меню станций
      lineondisp();
      printCodecAndBitrate();
    }
  }
  if (enc1.rightH())
  {
    audiovol++;
    audio.setVolume(audiovol);
    EEPROM.write(6, 15);
    EEPROM.commit();
    filePosition();
  }

  if (enc1.leftH())
  {
    audiovol--;
    audio.setVolume(audiovol);
    EEPROM.write(6, 15);
    EEPROM.commit();
    filePosition();
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
//----------------------
//   Menu Stations
// Complect menu stations
//------------------------
void menuStation()
{
  int i = 0;
  int ind = 0;
  while (i <= numbStations)
  { // list stations
    delay(1);
    ind = StationList[i].indexOf(space);
    nameStations[i] = make_str(utf8rus(StationList[i].substring(0, ind))); // Получили наименования станций
    i++;
  }
}
//----------------------------------
// ******* Menu stations ***********
//----------------------------------
uint16_t TFT_DARKBROWN = tft.color565(96, 96, 96);
uint16_t TFT_DARKGREY1 = tft.color565(128, 128, 128);
uint16_t TFT_Y1 = tft.color565(255, 204, 153);

void stationDisplay(int st)
{
  uint8_t i;
  i = 0;
  while (i < 7)
  {
    displayStations[i] = "";
    i++;
  }
  tft.setTextSize(1);
  tft.setFreeFont(&CourierCyr12pt8b);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  // счетчик для меню
  int stanonMenu = 4; // Положение текущей станции в меню
  int k;              //
  int p;              // счечик по листу станций
  k = st - 4;

  if (k < 0 && k != -4)
  {
    p = numbStations + k + 1;
    i = 0;
    while (p <= numbStations)
    {
      displayStations[i] = nameStations[p];
      i++;
      p++;
    }
    p = 0;
    while (i <= 7)
    {
      displayStations[i] = nameStations[p];
      i++;
      p++;
    }
  }

  if (k == -4)
  {
    i = 0;
    p = numbStations - 2;
    while (i <= 2)
    {
      displayStations[i] = nameStations[p];
      i++;
      p++;
    }
    p = 0;
    while (i <= 7)
    {
      displayStations[i] = nameStations[p];
      i++;
      p++;
    }
  }
  p = k;
  if (k >= 0)
  {
    i = 0;
    while (i <= 7)
    {
      displayStations[i] = nameStations[p];
      p++;
      i++;
      if (p == numbStations + 1)
        p = 0;
    }
  }
  // выводим на дисплей
  i = 0;
  k = 0;
  // 1
  tft.setTextColor(PL_0, TFT_BLACK);
  tft.drawString(utf8rus(displayStations[i]), 65, k);
  i++;
  k = k + 25;
  // 2
  tft.setTextColor(PL_0, TFT_BLACK);
  tft.drawString(utf8rus(displayStations[i]), 65, k);
  i++;
  k = k + 25;
  // 2,3,4
  tft.setTextColor(PL_0, TFT_BLACK);
  while (i <= 4)
  {
    tft.fillRect(65, k, 242, 25, TFT_BLACK);
    tft.drawString(utf8rus(displayStations[i]), 65, k);
    i++;
    k = k + 25;
  }
  // 5
  tft.setTextColor(PL_0, TFT_BLACK);
  tft.drawString(utf8rus(displayStations[i]), 65, k);
  i++;
  k = k + 25;
  // 6
  tft.setTextColor(PL_0, TFT_BLACK);
  tft.drawString(utf8rus(displayStations[i]), 65, k);
  i++;
  k = k + 25;
  // 7
  tft.setTextColor(PL_0, TFT_BLACK);
  tft.drawString(utf8rus(displayStations[i]), 65, k);
  i++;
  k = k + 25;

  tft.setFreeFont(&CourierCyr12pt8b);
  tft.fillRect(65, (stanonMenu * 25), 242, 25, ST_BG);
  tft.setTextColor(TFT_BLACK, ST_BG);
  tft.drawString(utf8rus(displayStations[stanonMenu]), 65, (stanonMenu * 25));
}

// Разделитель минут и секунд
// void drawlineClock()
// { //             x    y    x    y
//   tft.drawRect(260, 50, 60, 37, 0x9772);
//   tft.drawRect(260, 87, 60, 33, 0x9772);
// }

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
  // String space = " ";
  localIndex = StationList[indexOfStation].indexOf(space);
  StName = StationList[indexOfStation].substring(0, localIndex + 1);
  tft.setTextColor(TFT_BLACK, ST_BG);
  tft.setTextSize(2);
  tft.setFreeFont(RU12);
  tft.fillRect(0, 0, 319, 43, ST_BG);
  tft.fillRect(0, 44, 319, 43, TFT_BLACK); // очистка бегущей строки
  Serial.println(StName);
  tft.drawString(utf8rus(StName), x_stName, y_stName);
  show_title = false;
} // end PrintStation
//----------------------------
// CodecName Bitrate
//----------------------------
void printCodecAndBitrate()
{
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setFreeFont(&CourierCyr10pt8b);
  tft.setTextSize(1);

  tft.drawString(String(audio.getCodecname()).substring(0, 3), x_codec, y_codec);

  int bit = bitrate.toInt();
  if (bit < 128000)
  {
    tft.drawString(String(bit).substring(0, 2) + "k ", x_bitrate, x_bitrate);
  }
  else
  {
    tft.drawString(bitrate.substring(0, 3) + "k", x_bitrate, y_bitrate);
  }
  EEPROM.write(2, NEWStation);
  EEPROM.commit();
}

// Next station
void nextStation(bool stepStation)
{
  if (stepStation)
  {
    if (NEWStation != 0)
    {
      NEWStation--;
    }
    else
    {
      NEWStation = numbStations;
    }
  }
  else
  {
    if (NEWStation != numbStations)
    {
      NEWStation++;
    }
    else
    {
      NEWStation = 0;
    }
  }
  EEPROM.write(2, NEWStation);
  EEPROM.commit();
}

// SPIFFS
void initSpiffs()
{
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  File myFile;
  File root = SPIFFS.open("/");

  File file = root.openNextFile();
  while (file)
  {
    Serial.print("FILE: ");
    Serial.println(file.name());
    file = root.openNextFile();
  }
  myFile = SPIFFS.open("/playlist.txt", FILE_READ);
  if (!myFile)
  {
    Serial.println("------File does not exist!------");
  }
  int i = 0;
  while (myFile.available())
  {
    StationList[i] = myFile.readStringUntil('\n');
    Serial.println(StationList[i]);
    i++;
  }
  myFile.close();

  numbStations = i - 1; // start numb station
  menuStation();
  listStaton();
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

  if (EEPROM.read(2) > 200)
  {
    NEWStation = 0;
  }
  else
  {
    NEWStation = EEPROM.read(2);
  }
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
}
//****************************
//    WiFi
//****************************
void initWiFi()
{
  // Список известных WiFi сетей (SSID и пароль)
  const char *knownNetworks[][2] = {
      {"WiFi-Repeater", "tB5DVdR9"},
      {"RT-GPON-D5D9", "tB5DVdR9"},
      {"Repeater-2", "19621962"}};

  const int networkCount = sizeof(knownNetworks) / sizeof(knownNetworks[0]);

  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);

  Serial.println("Scanning for available networks...");
  tft.setCursor(40, 120);
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

    tft.setCursor(40, 140);
    tft.print("Connecting to: ");
    tft.setCursor(40, 160);
    tft.println(net.ssid);

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
  sysInfo += F("\nFree PSRAM: ");
  sysInfo += ESP.getFreePsram();
  sysInfo += F(" bytes");

  Serial.println(sysInfo);

  // Вывод на TFT дисплей
  tft.println("WiFi connected!");
  tft.print("SSID: ");
  tft.println(WiFi.SSID());
  tft.print("IP: ");
  tft.println(WiFi.localIP());
  tft.print("Signal: ");
  tft.print(WiFi.RSSI());
  tft.println(" dBm");
}
// end initwifi

// уровень вафай
void wifiLevel()
{
  uint16_t x_wifi = 140, y_wifi = ypos, y_lev_wifi = 3;
  if (WiFi.RSSI() >= -60)
  {
    tft.fillRect(x_wifi + 8, y_wifi, 3, y_lev_wifi + 3, TFT_CYAN);
    tft.fillRect(x_wifi + 13, y_wifi - 2, 3, y_lev_wifi + 5, TFT_CYAN);
    tft.fillRect(x_wifi + 18, y_wifi - 2 * 2, 3, y_lev_wifi + 7, TFT_CYAN);
    tft.fillRect(x_wifi + 23, y_wifi - 2 * 3, 3, y_lev_wifi + 9, TFT_CYAN);
    tft.fillRect(x_wifi + 28, y_wifi - 2 * 4, 3, y_lev_wifi + 11, TFT_CYAN);
    tft.fillRect(x_wifi + 33, y_wifi - 2 * 5, 3, y_lev_wifi + 13, TFT_CYAN);
    tft.fillRect(x_wifi + 38, y_wifi - 2 * 6, 3, y_lev_wifi + 15, TFT_CYAN);
  }
  if (WiFi.RSSI() < -60 && WiFi.RSSI() >= -70)
  {
    tft.fillRect(x_wifi + 8, y_wifi, 3, y_lev_wifi + 3, TFT_CYAN);
    tft.fillRect(x_wifi + 13, y_wifi - 2, 3, y_lev_wifi + 5, TFT_CYAN);
    tft.fillRect(x_wifi + 18, y_wifi - 2 * 2, 3, y_lev_wifi + 7, TFT_CYAN);
    tft.fillRect(x_wifi + 23, y_wifi - 2 * 3, 3, y_lev_wifi + 9, TFT_CYAN);
    tft.fillRect(x_wifi + 28, y_wifi - 2 * 4, 3, y_lev_wifi + 11, TFT_CYAN);
    tft.fillRect(x_wifi + 33, y_wifi - 2 * 5, 3, y_lev_wifi + 13, TFT_CYAN);
    tft.fillRect(x_wifi + 38, y_wifi - 2 * 6, 3, y_lev_wifi + 15, 0x39C4);
  }
  if (WiFi.RSSI() < -70 && WiFi.RSSI() > -80)
  {
    tft.fillRect(x_wifi + 8, y_wifi, 3, y_lev_wifi + 3, TFT_CYAN);
    tft.fillRect(x_wifi + 13, y_wifi - 2, 3, y_lev_wifi + 5, TFT_CYAN);
    tft.fillRect(x_wifi + 18, y_wifi - 2 * 2, 3, y_lev_wifi + 7, TFT_CYAN);
    tft.fillRect(x_wifi + 23, y_wifi - 2 * 3, 3, y_lev_wifi + 9, TFT_CYAN);
    tft.fillRect(x_wifi + 28, y_wifi - 2 * 4, 3, y_lev_wifi + 11, TFT_CYAN);
    tft.fillRect(x_wifi + 33, y_wifi - 2 * 5, 3, y_lev_wifi + 13, 0x39C4);
    tft.fillRect(x_wifi + 38, y_wifi - 2 * 6, 3, y_lev_wifi + 15, 0x39C4);
  }

  if (WiFi.RSSI() < -80 && WiFi.RSSI() > -90)
  {
    tft.fillRect(x_wifi + 8, y_wifi, 3, y_lev_wifi + 3, TFT_CYAN);
    tft.fillRect(x_wifi + 13, y_wifi - 2, 3, y_lev_wifi + 5, TFT_CYAN);
    tft.fillRect(x_wifi + 18, y_wifi - 2 * 2, 3, y_lev_wifi + 7, TFT_CYAN);
    tft.fillRect(x_wifi + 23, y_wifi - 2 * 3, 3, y_lev_wifi + 9, TFT_CYAN);
    tft.fillRect(x_wifi + 28, y_wifi - 2 * 4, 3, y_lev_wifi + 11, 0x39C4);
    tft.fillRect(x_wifi + 33, y_wifi - 2 * 5, 3, y_lev_wifi + 13, 0x39C4);
    tft.fillRect(x_wifi + 38, y_wifi - 2 * 6, 3, y_lev_wifi + 15, 0x39C4);
  }
  if (WiFi.RSSI() < -90)
  {
    tft.fillRect(x_wifi + 8, y_wifi, 3, y_lev_wifi + 3, TFT_CYAN);
    tft.fillRect(x_wifi + 13, y_wifi - 2, 3, y_lev_wifi + 5, 0x39C4);
    tft.fillRect(x_wifi + 18, y_wifi - 2 * 2, 3, y_lev_wifi + 7, 0x39C4);
    tft.fillRect(x_wifi + 23, y_wifi - 2 * 3, 3, y_lev_wifi + 9, 0x39C4);
    tft.fillRect(x_wifi + 28, y_wifi - 2 * 4, 3, y_lev_wifi + 11, 0x39C4);
    tft.fillRect(x_wifi + 33, y_wifi - 2 * 5, 3, y_lev_wifi + 13, 0x39C4);
    tft.fillRect(x_wifi + 38, y_wifi - 2 * 6, 3, y_lev_wifi + 15, 0x39C4);
  }
}

// Движение по меню через сайт
void onMenuOn()
{
  if (showRadio)
  {
    stations = true;
    nextStation(stations);
    // printStation(NEWStation);
  }
  if (!showRadio)
  {
    stations = true;
    nextStation(stations);
    // menuStation();
    stationDisplay(NEWStation);
    currentMillis = millis(); // Пока ходим по меню
  }
}
// Движение по меню через сайт
void onMenuOff()
{
  if (showRadio)
  {
    stations = false;
    nextStation(stations);
    // printStation(NEWStation);
  }
  if (!showRadio)
  {
    stations = false;
    nextStation(stations);
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
    stationDisplay(NEWStation);
  }
  if (showRadio)
  {
    first = true;
    tft.fillRect(0, 0, 320, ypos + 8, TFT_BLACK);
    // printStation(NEWStation);
    getClock = true; // получить время при переходе от меню станций
    lineondisp();
    // printCodecAndBitrate();
  }
}

String readFile(fs::FS &fs, const char *path)
{
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path);
  if (!file || file.isDirectory())
  {
    Serial.println("- failed to open file for reading");
    return String();
  }
  String fileContent;
  while (file.available())
  {
    fileContent = file.readStringUntil('\n');
    break;
  }
  return fileContent;
}

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
  Serial.printf(info, "----");
  title_flag = true;
  show_title = true;
  MessageToScroll_1 = info;
  width_txt = tft.textWidth(MessageToScroll_1);
  MessageToScroll_1 = F(" ");
  MessageToScroll_1 += trim(info);
  MessageToScroll_1 += F(" ");
  MessageToScroll_1 = utf8rus(MessageToScroll_1);
}
// optional
/* void audio_info(const char *info)
{
  Serial.print("info        ");
  Serial.println(info);
}
void audio_id3data(const char *info)
{ // id3 metadata
  Serial.print("id3data     ");
  Serial.println(info);
}
void audio_eof_audio(const char *info)
{ // end of file
  Serial.print("eof_audio     ");
  Serial.println(info);
}
*/
void audio_bitrate(const char *info)
{
  bitrate = info;
}
/*
void audio_commercial(const char *info)
{ // duration in sec
  Serial.print("commercial  ");
  Serial.println(info);
}
void audio_icyurl(const char *info)
{ // homepage
  Serial.print("icyurl      ");
  Serial.println(info);

}
void audio_lasthost(const char *info)
{ // stream URL played
  Serial.print("lasthost    ");
  Serial.println(info);
}
void audio_eof_speech(const char *info)
{
  Serial.print("eof_speech  ");
  Serial.println(info);
}
*/

void listStaton()
{
  String partlistStation;
  uint8_t i = 0;
  while (i <= numbStations)
  {
    int ind_to_scace = StationList[i].indexOf(space);
    String nameStat = StationList[i].substring(0, ind_to_scace);

    String newStat = StationList[i].substring(ind_to_scace + 1, StationList[i].length());

    partlistStation += String("<tr><td>") + String(i) + String("</td>") + String("<td>") + nameStat + String("</td>") + String("<td>") + newStat + String("</td></tr>");
    listRadio = String("<table class=\"table table-success table-striped\"> <thead><tr><th>№</th><th>Station name</th><th>Station url</th></tr></thead><tbody>") + partlistStation + String("</tbody></table>");
    i++;
  }
}

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
  // wifi level codec bitrate
  tft.drawRect(70, 175, 250, 42, TFT_CYAN);
  tft.setTextColor(TFT_CYAN);
  tft.drawString("WiFi", x_wifi, y_wifi);
  audioVolume();
}

int volumeLevel;
int x_FP = 75, y_FP = ypos + 13; // position in line
void audioVolume()
{
  volumeLevel = audio.getVolume() * 10;
  tft.fillRect(x_FP, y_FP, 242, 5, TFT_BLACK);
  tft.drawRect(x_FP, y_FP, 242, 6, color_volume);
  tft.fillRect(x_FP, y_FP, volumeLevel, 6, color_volume);
}
//**********************************
// File position Уровень громкости
//**********************************
int x1_FP = 75, y1_FP = ypos + 12; // position in line
void filePosition()
{
  Serial.print("Volume = ");
  Serial.println(audiovol);
  audiovol = audio.getVolume();
  tft.fillRect(x1_FP, y1_FP - 2, 248, 8, TFT_BLACK);
  tft.drawRect(x1_FP, y1_FP, 248, 6, TFT_GREEN);
  tft.fillRect(x1_FP, y1_FP, audiovol * 10, 6, TFT_GREEN);
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

// void updateFromFS(fs::FS &fs)
// {
//   File updateBin = fs.open("/firmware.bin");
//   if (updateBin)
//   {
//     if (updateBin.isDirectory())
//     {
//       Serial.println("Error, update.bin is not a file");
//       updateBin.close();
//       return;
//     }

//     size_t updateSize = updateBin.size();

//     if (updateSize > 0)
//     {
//       Serial.println("Trying to start update");
//       performUpdate(updateBin, updateSize);
//     }
//     else
//     {
//       Serial.println("Error, file is empty");
//     }
//     updateBin.close();
//     // when finished remove the binary from spiffs to indicate end of the process
//     Serial.println("Removing update file");
//     fs.remove("/firmware.bin");
//     rebootEspWithReason("Rebooting to complete OTA update");
//   }
//   else
//   {
//     Serial.println("Could not load update.bin from spiffs root");
//   }
// }
