#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include <SD.h>

bool openGlobalFile(const char* filename);
void writeToGlobalFile(const char* data);
void closeGlobalFile();
bool isGlobalFileOpen();

#endif