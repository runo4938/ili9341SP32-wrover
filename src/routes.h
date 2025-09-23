#ifndef ROUTES_H
#define ROUTES_H
#include <settings.h>

// Объявляем обработчики
String processor(const String &var);
String processor_playlst(const String &var);

// Объявляем функцию для настройки роутов

extern size_t content_len;
extern bool opened;
extern const char *PARAM;
#endif