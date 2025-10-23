#include "file_operations.h"

static File globalFile;  // статическая переменная в cpp файле

bool openGlobalFile(const char* filename) {
    globalFile = SD.open(filename, FILE_WRITE);
    return globalFile;
}

void writeToGlobalFile(const char* data) {
    if (globalFile) {
        globalFile.println(data);
    }
}

void closeGlobalFile() {
    if (globalFile) {
        globalFile.close();
    }
}

bool isGlobalFileOpen() {
    return globalFile;
}