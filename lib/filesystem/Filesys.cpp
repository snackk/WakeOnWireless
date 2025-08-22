#include <Filesys.h>

FilesysClass Filesys;

void FilesysClass::initFS() {
    if (LittleFS.begin()) {
        Serial.println("[LITTLEFS] - Setup done");
    } else {
        Serial.println("[LITTLEFS] - error has occurred while mounting.");
    }
}

String FilesysClass::readFirstLine(const char* path) {
    Serial.printf("[LITTLEFS] - Reading file #%s\n", path);

    File file = LittleFS.open(path, "r");
    if(!file || file.isDirectory()) {
        Serial.println("[LITTLEFS] - failed to open file in RO mode.");
        return "";
    }

    String fileContent;
    while(file.available()) {
        fileContent = file.readStringUntil('\n');// Read first line
        break;
    }

    file.close();
    return fileContent;
}

String FilesysClass::readEntireFile(const char* path) {
    Serial.printf("[LITTLEFS] - Reading file #%s\n", path);

    File file = LittleFS.open(path, "r");
    if(!file || file.isDirectory()) {
        Serial.println("[LITTLEFS] - failed to open file in RO mode.");
        return "";
    }

    String fileContent = file.readString();  // Read entire file
    file.close();

    return fileContent;
}

void FilesysClass::writeFile(const char* path, const char* data) {
    Serial.printf("[LITTLEFS] - Writing file #%s\r\n", path);

    File file = LittleFS.open(path, "w");
    if(!file) {
        Serial.println("[LITTLEFS] - failed to open file in RW mode.");
        return;
    }

    if(!file.print(data)) {
        Serial.println("[LITTLEFS] - failed to write file.");
    }

    file.close();
}