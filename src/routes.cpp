
#include "settings.h"
#include "routes.h"
#define FIRMWARE_VERSION "1.0.0"

String filelist = "";

// Функция настройки всех роутов
void setupRoutes(AsyncWebServer &server)
{

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/index.html", String(), false, processor_playlst); });

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

    server.on("/filelist", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send_P(200, "text/plain", filelist.c_str()); });

    server.on("/testpage", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/testpage.html", String(), false); });

    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request)
              {
    request->send(200, "text/plain", "Device will reboot in 2 seconds");
    delay(2000);
    ESP.restart(); });

    // server.on(
    //     "/doUpdate", HTTP_POST,
    //     [](AsyncWebServerRequest *request) {},
    //     [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
    //     {
    //         handleDoUpdate(request, filename, index, data, len, final);
    //     });

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
    // GET input1 value on <ESP_IP>/update?state=<inputMessage>
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
    server.on("/off", HTTP_ANY, [](AsyncWebServerRequest *request)
              {
              request->send(204); // SPIFFS, "/index.html", String(), false, processor_playlst);
              onMenuOff(); });
    //    Если переключили станцию назад
    //----------------------------------
    server.on("/on", HTTP_ANY, [](AsyncWebServerRequest *request)
              {
              request->send(204);
              onMenuOn(); });
    server.on("/menu", HTTP_GET, [](AsyncWebServerRequest *request)
              {
             request->send(204);
             onMenu(); });

    // Добавьте остальные роуты здесь...

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
    // Serial.println(var);
    // Serial.println(listRadio);
    if (var == "nameST")
    {
        return listRadio;
    }
    if (var == "version")
    {
        return FIRMWARE_VERSION;
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
