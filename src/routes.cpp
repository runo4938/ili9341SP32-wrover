
#include "settings.h"
#include "routes.h"
#define FIRMWARE_VERSION "1.0.0"

String filelist = "";
#define VOLUME_EEPROM_ADDR 6
uint8_t currentVolumePercent = 10; // 0–100%

// Преобразует 0–100% в 21–0 (инверсия!)
uint8_t percentToVolume(uint8_t percent)
{
    // Ограничиваем вход
    if (percent > 21)
        percent = 21;
    return percent;
}

uint8_t volumeToPercent(uint8_t vol)
{
    if (vol > 21)
        vol = 21;
    return vol;
}

void setVolumePercent(uint8_t percent)
{
    currentVolumePercent = percent;
    uint8_t vol = percentToVolume(percent);
    audio.setVolume(vol); // ← основной вызов!
    volUpdate = true;
    // Сохраняем в EEPROM
    EEPROM.write(VOLUME_EEPROM_ADDR, percent);
    EEPROM.commit();
}
// Функция настройки всех роутов
void setupRoutes(AsyncWebServer &server)
{
    server.on("/stations", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                listStaton(); // генерирует listRadio с актуальным currentStationIndex
                request->send(200, "text/html", listRadio); });

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/index.html", String(), false, processor_playlst); });

              
    ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
               {
  if (type == WS_EVT_CONNECT) {
    // При подключении — отправляем текущую станцию
    String json = "{\"currentStationIndex\":" + String(NEWStation) + "}";
    client->text(json);
  } });
    server.addHandler(&ws);

    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/style.css", "text/css"); });

    server.on("/setting", HTTP_GET, [](AsyncWebServerRequest *requiest)
              { requiest->send(SPIFFS, "/settings.html", String(), false, processor); });

    server.on("/slider", HTTP_GET, [](AsyncWebServerRequest *request)
              {
    String inputMessage;
    // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      sliderValue = inputMessage;
      audio.setVolume(sliderValue.toInt());
      EEPROM.write(6, sliderValue.toInt());
      EEPROM.commit();
      volUpdate=true;
    }
    else {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "OK"); });

    server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/update.html", String(), false); });

    server.on("/newrelease", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(204);
               newrelease(); });

    server.on("/filesystem", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/fs.html", String(), false, processor_update); });

    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request)
              {
    request->send(200, "text/plain", "Device will reboot in 2 seconds");
    delay(2000);
    ESP.restart(); });

    server.on(
        "/doUpdate", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
        {
            handleDoUpdate(request, filename, index, data, len, final);
        });

    server.on(
        "/doUpload", HTTP_POST, [](AsyncWebServerRequest *request)
        { opened = false; },
        [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
        {
            handleDoUpload(request, filename, index, data, len, final);
        });

    server.on("/delete", HTTP_GET, [](AsyncWebServerRequest *request)
              {
    String inputMessage;
    String inputParam;
    if (request->hasParam(PARAM)) {
      inputMessage = request->getParam(PARAM)->value();
      inputParam = PARAM;
      deleteFile(SPIFFS, inputMessage);
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    request->send(200, "text/plain", "OK"); });

    //------------------------------------
    // Если переключили станцию вперед
    ////------------------------------------
    server.on("/Next", HTTP_GET, [](AsyncWebServerRequest *request)
              {  
                request->send(200, "text/plain", "OK - Next"); 
                onMenuNext(); });

    //    Если переключили станцию назад
    //----------------------------------
    server.on("/Prev", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                request->send(200,"text/plain", "OK");
                onMenuPrev(); });

    server.on("/Menu", HTTP_GET, [](AsyncWebServerRequest *request)
              { 
                request->send(20,"text/plain", "OK");
                onMenu(); });

    // Добавьте остальные роуты здесь...
    server.on("/volume", HTTP_GET, [](AsyncWebServerRequest *request)
              {
    currentVolumePercent = EEPROM.read(VOLUME_EEPROM_ADDR);
    String json = "{\"percent\":" + String(currentVolumePercent) + "}";
    request->send(200, "application/json", json); });

    server.on("/set_volume", HTTP_GET, [](AsyncWebServerRequest *request)
              {
    if (request->hasParam("percent")) {
        String pStr = request->getParam("percent")->value();
        uint16_t percent = pStr.toInt();
        if (percent <= 21) {
            setVolumePercent(percent);
            request->send(200, "text/plain", "OK");
            return;
        }
    }
    request->send(400, "text/plain", "Invalid percent (0-21)"); });

    server.on("/play_station", HTTP_GET, [](AsyncWebServerRequest *request)
              {
    if (request->hasParam("station")) {
        String idStr = request->getParam("station")->value();
        int stationIndex = idStr.toInt();

        if (stationIndex >= 0 && stationIndex <= numbStations) {
            NEWStation = stationIndex;
            notifyWebClients();
            request->send(200, "text/plain", "OK");
            return;
        }
    }
    request->send(400, "text/plain", "Invalid station"); });

    server.onNotFound([](AsyncWebServerRequest *request)
                      { request->send(404, "text/plain", "Not Found"); });
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
    // filelist = "";
    int i = 0;
    String partlist;
    // Serial.printf("Listing directory: %s\r\n", dirname);
    File root = fs.open(dirname);
    if (!root)
    {
        Serial.println("- failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            if (levels)
            {
                listDir(fs, file.name(), levels - 1);
            }
        }
        else
        {
            i++;
            String st_after_symb = String(file.name()).substring(String(file.name()).indexOf("/") + 1);

            partlist += String("<tr><td>") + String(i) + String("</td><td>") + String("<a href='") + String(file.name()) + String("'>") + st_after_symb + String("</td><td>") + String(file.size() / 1024) + String("</td><td>") + String("<input type='button' class='btndel' onclick=\"deletef('") + String(file.name()) + String("')\" value='X'>") + String("</td></tr>");
            filelist = String("<table><tbody><tr><th>#</th><th>File name</th><th>Size(KB)</th><th></th></tr>") + partlist + String(" </tbody></table>");
        }
        file = root.openNextFile();
    }
    filelist = String("<table><tbody><tr><th>#</th><th>File name</th><th>Size(KB)</th><th></th></tr>") + partlist + String(" </tbody></table>");
}

void newrelease()
{
    EEPROM.write(3, 1); // UPDATE
    EEPROM.commit();
    ESP.restart();
}

void handleDoUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
    if (!index)
    {
        content_len = request->contentLength();
        Serial.printf("UploadStart: %s\n", filename.c_str());
    }
    if (opened == false)
    {
        opened = true;
        file = SPIFFS.open(String("/") + filename, FILE_WRITE);
        if (!file)
        {
            Serial.println("- failed to open file for writing");
            return;
        }
    }
    if (file.write(data, len) != len)
    {
        Serial.println("- failed to write");
        return;
    }
    if (final)
    {
        AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Ok");
        response->addHeader("Refresh", "20");
        response->addHeader("Location", "/filesystem");
        request->send(response);
        file.close();
        opened = false;
        initSpiffs();
        Serial.println("---------------");
        Serial.println("Upload complete");
    }
}

void printProgress(size_t prg, size_t sz)
{
    Serial.printf("Progress: %d%%\n", (prg * 100) / content_len);
}

String processor_playlst(const String &var)
{
    if (var == "nameST")
    {
        return listRadio;
    }

    return String();
}

String processor_update(const String &var)
{
    Serial.println(var);
    if (var == "list")
    {
        return filelist;
    }
    return String();
}

void handleDoUpdate(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
{
  if (!index)
  {
    content_len = request->contentLength();
    int cmd = (filename.indexOf("spiffs") > -1) ? U_PART : U_FLASH;
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd))
    {
      Update.printError(Serial);
    }
  }

  if (Update.write(data, len) != len)
  {
    Update.printError(Serial);
    Serial.printf("Progress: %d%%\n", (Update.progress() * 100) / Update.size());
  }

  if (final)
  {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Ok");
    response->addHeader("Refresh", "30");
    response->addHeader("Location", "/");
    request->send(response);
    if (!Update.end(true))
    {
      Update.printError(Serial);
    }
    else
    {
      Serial.println("Update complete");
      Serial.flush();
      ESP.restart();
    }
  }
}
