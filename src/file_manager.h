#pragma once

#include "config.h"

void fileManagerSetup();
void refreshFileList();
int getFileCount();
FileInfo* getFileList();

void loadFile(const char* filename);
void saveCurrentFile();
void createNewFile();
void deriveUniqueFilename(const char* title, char* out, int maxLen);
void updateFileTitle(const char* filename, const char* newTitle);
void deleteFile(const char* filename);
