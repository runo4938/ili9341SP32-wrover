#pragma once

#include <Arduino.h>
#include <vector>
#include <string>

class StationsManager {
public:
    StationsManager();
    void loadStations();        // Загрузить станции из SPIFFS/SD/PROGMEM
    int getStationCount();      // Количество станций
    String getStationName(int index); // Получить имя станции по индексу
    String getStationUrl(int index);  // Получить URL станции по индексу

private:
    std::vector<String> stationNames;
    std::vector<String> stationUrls;
};