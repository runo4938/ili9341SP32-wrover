#ifndef ROUTES_H
#define ROUTES_H
#include <settings.h>

// Объявляем обработчики
String processor(const String &var);
String processor_playlst(const String &var);
extern String listRadio; // радиостанции на странице
// Объявляем функцию для настройки роутов

extern size_t content_len;
extern bool opened;
extern const char *PARAM;
void handleDoUpdate(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final);
#endif