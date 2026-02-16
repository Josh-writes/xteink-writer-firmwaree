#pragma once

#include "config.h"

void fileManagerSetup();
void refreshFileList();
int getFileCount();
FileInfo* getFileList();

void loadFile(const char* filename);
void saveCurrentFile(bool refreshList = true);
void createNewFile();
void deriveUniqueFilename(const char* title, char* out, int maxLen);
void updateFileTitle(const char* filename, const char* newTitle);
void deleteFile(const char* filename);

// Book reader
void refreshBookList();
int getBookCount();
FileInfo* getBookList();
void loadBook(const char* filename);
void saveBookPosition();
int loadBookPosition(const char* filename);
