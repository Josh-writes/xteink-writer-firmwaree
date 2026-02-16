#include "file_manager.h"
#include "text_editor.h"
#include <Arduino.h>
#include <SDCardManager.h>
#include <Preferences.h>
#include <cstring>

// --- File list ---
static FileInfo fileList[MAX_FILES];
static int fileCount = 0;

// --- Book list ---
static FileInfo bookList[MAX_BOOKS];
static int bookCount = 0;
static Preferences bookPrefs;

// Shared state
extern UIState currentState;
extern bool screenDirty;

// Convert filename to a readable display title.
// "my_note_2.txt" -> "My Note 2"
static void filenameToTitle(const char* filename, char* out, int maxLen) {
  int j = 0;
  bool capitalizeNext = true;
  for (int i = 0; filename[i] != '\0' && filename[i] != '.' && j < maxLen - 1; i++) {
    char c = filename[i];
    if (c == '_') {
      if (j > 0) out[j++] = ' ';
      capitalizeNext = true;
    } else {
      if (capitalizeNext && c >= 'a' && c <= 'z') c -= 32;
      capitalizeNext = false;
      out[j++] = c;
    }
  }
  out[j] = '\0';
  if (j == 0) strncpy(out, "Untitled", maxLen - 1);
}

// Convert a title to a valid FAT filename (lowercase, spaces->underscores,
// non-alphanumeric stripped, ".txt" appended).
static void titleToFilename(const char* title, char* out, int maxLen) {
  int maxBase = maxLen - 5; // room for ".txt" + null
  int j = 0;
  for (int i = 0; title[i] != '\0' && j < maxBase; i++) {
    char c = title[i];
    if (c >= 'A' && c <= 'Z') c += 32;
    if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
      out[j++] = c;
    } else if (c == ' ' || c == '_' || c == '-') {
      if (j > 0 && out[j - 1] != '_') out[j++] = '_';
    }
  }
  while (j > 0 && out[j - 1] == '_') j--;
  if (j == 0) { strncpy(out, "note", maxLen - 1); j = 4; }
  strcpy(out + j, ".txt");
}

// Derive a unique /notes/ filename from a title, handling collisions with _2, _3 suffix.
void deriveUniqueFilename(const char* title, char* out, int maxLen) {
  titleToFilename(title, out, maxLen);

  char path[320];
  snprintf(path, sizeof(path), "/notes/%s", out);
  if (!SdMan.exists(path)) return;

  // Collision — strip .txt, try _2, _3 ...
  char base[MAX_FILENAME_LEN];
  strncpy(base, out, maxLen - 1);
  base[strlen(base) - 4] = '\0';

  int suffix = 2;
  while (SdMan.exists(path) && suffix <= 99) {
    snprintf(out, maxLen, "%s_%d.txt", base, suffix++);
    snprintf(path, sizeof(path), "/notes/%s", out);
  }
}

void fileManagerSetup() {
  if (!SdMan.begin()) {
    DBG_PRINTLN("SD Card mount failed!");
    return;
  }

  if (!SdMan.exists("/notes")) {
    SdMan.mkdir("/notes");
  }
  if (!SdMan.exists("/books")) {
    SdMan.mkdir("/books");
  }

  DBG_PRINTLN("SD Card initialized");
  refreshFileList();
}

void refreshFileList() {
  fileCount = 0;

  auto root = SdMan.open("/notes");
  if (!root || !root.isDirectory()) {
    if (root) root.close();
    return;
  }

  root.rewindDirectory();
  char name[256];

  for (auto file = root.openNextFile(); file; file = root.openNextFile()) {
    file.getName(name, sizeof(name));
    if (name[0] == '.' || fileCount >= MAX_FILES) {
      file.close();
      if (fileCount >= MAX_FILES) break;
      continue;
    }

    int nameLen = strlen(name);
    if (nameLen > 4 && strcmp(name + nameLen - 4, ".txt") == 0) {
      strncpy(fileList[fileCount].filename, name, MAX_FILENAME_LEN - 1);
      fileList[fileCount].filename[MAX_FILENAME_LEN - 1] = '\0';

      filenameToTitle(name, fileList[fileCount].title, MAX_TITLE_LEN);
      fileList[fileCount].modTime = 0;
      fileCount++;
    }
    file.close();
  }
  root.close();
  SdMan.sleep();

  DBG_PRINTF("File listing: %d files found\n", fileCount);
}

int getFileCount() { return fileCount; }
FileInfo* getFileList() { return fileList; }

void loadFile(const char* filename) {
  char path[320];
  snprintf(path, sizeof(path), "/notes/%s", filename);

  auto file = SdMan.open(path, O_RDONLY);
  if (!file) {
    DBG_PRINTF("Could not open: %s\n", path);
    return;
  }

  char* buf = editorGetBuffer();
  int readResult = file.read(buf, TEXT_BUFFER_SIZE - 1);
  size_t bytesRead = (readResult > 0) ? (size_t)readResult : 0;
  buf[bytesRead] = '\0';
  file.close();

  editorSetCurrentFile(filename);
  editorLoadBuffer(bytesRead);

  // Title comes from the filename, not the file content
  char title[MAX_TITLE_LEN];
  filenameToTitle(filename, title, MAX_TITLE_LEN);
  editorSetCurrentTitle(title);
  editorSetUnsavedChanges(false);

  currentState = UIState::TEXT_EDITOR;
  SdMan.sleep();
  DBG_PRINTF("Loaded: %s (%d bytes)\n", filename, (int)bytesRead);
}

void saveCurrentFile(bool refreshList) {
  const char* filename = editorGetCurrentFile();
  if (filename[0] == '\0') return;

  char path[320], tmpPath[336], bakPath[336];
  snprintf(path, sizeof(path), "/notes/%s", filename);
  snprintf(tmpPath, sizeof(tmpPath), "%s.tmp", path);
  snprintf(bakPath, sizeof(bakPath), "%s.bak", path);

  // Step 1: Write new content to .tmp
  auto file = SdMan.open(tmpPath, O_WRONLY | O_CREAT | O_TRUNC);
  if (!file) {
    DBG_PRINTF("saveCurrentFile: could not create tmp: %s\n", tmpPath);
    return;
  }

  size_t toWrite = editorGetLength();
  size_t written = file.write((const uint8_t*)editorGetBuffer(), toWrite);
  file.close();

  // Step 2: Verify bytes written match expected length
  if (written != toWrite) {
    DBG_PRINTF("saveCurrentFile: write mismatch (%d/%d) — aborting\n", (int)written, (int)toWrite);
    SdMan.remove(tmpPath);
    return;
  }

  // Step 3: Rotate original → .bak (original is now safe in .tmp, preserve previous .bak)
  if (SdMan.exists(path)) {
    SdMan.remove(bakPath);          // Remove old .bak (if any)
    SdMan.rename(path, bakPath);    // Original becomes new .bak
  }

  // Step 4: Promote .tmp → original
  SdMan.rename(tmpPath, path);

  editorSetUnsavedChanges(false);
  if (refreshList) refreshFileList();
  SdMan.sleep();
  DBG_PRINTF("Saved: %s\n", filename);
}

void createNewFile() {
  editorClear();
  editorSetCurrentFile("");       // filename derived from title when user confirms
  editorSetCurrentTitle("Untitled");
  editorSetUnsavedChanges(true);
}

// Rename a file on disk to match a new title, updating editor state if needed.
void updateFileTitle(const char* filename, const char* newTitle) {
  char newFilename[MAX_FILENAME_LEN];
  deriveUniqueFilename(newTitle, newFilename, MAX_FILENAME_LEN);

  if (strcmp(newFilename, filename) != 0) {
    char oldPath[320], newPath[320];
    snprintf(oldPath, sizeof(oldPath), "/notes/%s", filename);
    snprintf(newPath, sizeof(newPath), "/notes/%s", newFilename);
    SdMan.rename(oldPath, newPath);

    if (strcmp(editorGetCurrentFile(), filename) == 0) {
      editorSetCurrentFile(newFilename);
    }
  }

  refreshFileList();
  SdMan.sleep();
}

void deleteFile(const char* filename) {
  char path[320], bakPath[336];
  snprintf(path, sizeof(path), "/notes/%s", filename);
  snprintf(bakPath, sizeof(bakPath), "%s.bak", path);
  SdMan.remove(path);
  SdMan.remove(bakPath);
  refreshFileList();
  SdMan.sleep();
  DBG_PRINTF("Deleted: %s\n", filename);
}

// ===========================================================================
// Book reader functions
// ===========================================================================

void refreshBookList() {
  bookCount = 0;

  auto root = SdMan.open("/books");
  if (!root || !root.isDirectory()) {
    if (root) root.close();
    return;
  }

  root.rewindDirectory();
  char name[256];

  for (auto file = root.openNextFile(); file; file = root.openNextFile()) {
    file.getName(name, sizeof(name));
    if (name[0] == '.' || bookCount >= MAX_BOOKS) {
      file.close();
      if (bookCount >= MAX_BOOKS) break;
      continue;
    }

    int nameLen = strlen(name);
    if (nameLen > 4 && strcmp(name + nameLen - 4, ".txt") == 0) {
      strncpy(bookList[bookCount].filename, name, MAX_FILENAME_LEN - 1);
      bookList[bookCount].filename[MAX_FILENAME_LEN - 1] = '\0';

      filenameToTitle(name, bookList[bookCount].title, MAX_TITLE_LEN);
      bookList[bookCount].modTime = 0;
      bookCount++;
    }
    file.close();
  }
  root.close();
  SdMan.sleep();

  DBG_PRINTF("Book listing: %d books found\n", bookCount);
}

int getBookCount() { return bookCount; }
FileInfo* getBookList() { return bookList; }

// NVS key from filename: strip .txt, truncate to 15 chars
static void bookNvsKey(const char* filename, char* key, int maxLen) {
  int j = 0;
  for (int i = 0; filename[i] != '\0' && filename[i] != '.' && j < maxLen - 1; i++) {
    key[j++] = filename[i];
  }
  key[j] = '\0';
}

void saveBookPosition() {
  const char* filename = editorGetCurrentFile();
  if (filename[0] == '\0') return;

  char key[16];
  bookNvsKey(filename, key, sizeof(key));

  bookPrefs.begin("book_pos", false);
  bookPrefs.putInt(key, editorGetCursorPosition());
  bookPrefs.end();

  DBG_PRINTF("Saved book position: %s = %d\n", key, editorGetCursorPosition());
}

int loadBookPosition(const char* filename) {
  char key[16];
  bookNvsKey(filename, key, sizeof(key));

  bookPrefs.begin("book_pos", true);
  int pos = bookPrefs.getInt(key, 0);
  bookPrefs.end();

  return pos;
}

void loadBook(const char* filename) {
  char path[320];
  snprintf(path, sizeof(path), "/books/%s", filename);

  auto file = SdMan.open(path, O_RDONLY);
  if (!file) {
    DBG_PRINTF("Could not open book: %s\n", path);
    return;
  }

  char* buf = editorGetBuffer();
  int readResult = file.read(buf, TEXT_BUFFER_SIZE - 1);
  size_t bytesRead = (readResult > 0) ? (size_t)readResult : 0;
  buf[bytesRead] = '\0';
  file.close();

  editorSetCurrentFile(filename);
  int savedPos = loadBookPosition(filename);
  editorLoadBufferAtPosition(bytesRead, savedPos);

  char title[MAX_TITLE_LEN];
  filenameToTitle(filename, title, MAX_TITLE_LEN);
  editorSetCurrentTitle(title);
  editorSetUnsavedChanges(false);

  currentState = UIState::BOOK_READER;
  screenDirty = true;
  SdMan.sleep();
  DBG_PRINTF("Loaded book: %s (%d bytes, pos %d)\n", filename, (int)bytesRead, savedPos);
}
