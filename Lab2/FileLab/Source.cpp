#include <windows.h>
#include <iostream>
#include <chrono>

void InsertCharacter(void* buffer, DWORD fileSize, char insertChar, DWORD position, void*& newBuffer, DWORD& newBufferSize) {
    if (position > fileSize) {
        std::cerr << "Ошибка: позиция вставки выходит за пределы размера файла." << std::endl;
        return;
    }

    newBufferSize = fileSize + 1;
    newBuffer = malloc(newBufferSize);

    if (!newBuffer) {
        std::cerr << "Ошибка выделения памяти для нового буфера." << std::endl;
        return;
    }

    memcpy(newBuffer, buffer, position);
    static_cast<char*>(newBuffer)[position] = insertChar;
    memcpy(static_cast<char*>(newBuffer) + position + 1, static_cast<char*>(buffer) + position, fileSize - position);
}

void MapFileToMemory(const char* inputFilename, char insertChar, DWORD insertPosition) {
    HANDLE hFile = CreateFileA(inputFilename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Ошибка открытия файла для чтения." << std::endl;
        return;
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == INVALID_FILE_SIZE) {
        std::cerr << "Ошибка получения размера файла." << std::endl;
        CloseHandle(hFile);
        return;
    }

    HANDLE hMapping = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (hMapping == NULL) {
        std::cerr << "Ошибка создания отображения файла." << std::endl;
        CloseHandle(hFile);
        return;
    }
    std::cout << "MAP. Отображение файла создано." << std::endl;

    void* newBuffer = nullptr;
    DWORD newBufferSize = 0;

    auto start = std::chrono::high_resolution_clock::now();

    void* dataArray = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
    if (dataArray == NULL) {
        std::cerr << "Ошибка отображения файла в память." << std::endl;
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return;
    }
    std::cout << "MAP. Файл отображен в память." << std::endl;

    InsertCharacter(dataArray, fileSize, insertChar, insertPosition, newBuffer, newBufferSize);

    if (!newBuffer) {
        UnmapViewOfFile(dataArray);
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return;
    }

    HANDLE hOutputFile = CreateFileA("MapFinal.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hOutputFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Ошибка открытия выходного файла." << std::endl;
        UnmapViewOfFile(dataArray);
        CloseHandle(hMapping);
        CloseHandle(hFile);
        return;
    }

    DWORD bytesWritten;
    if (!WriteFile(hOutputFile, newBuffer, newBufferSize, &bytesWritten, NULL)) {
        std::cerr << "Ошибка записи в выходной файл." << std::endl;
    }
    else if (bytesWritten != newBufferSize) {
        std::cerr << "Количество записанных байт не соответствует размеру буфера!" << std::endl;
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "MAP. Время копирования данных: " << elapsed.count() << " секунд." << std::endl;

    UnmapViewOfFile(dataArray);
    CloseHandle(hOutputFile);
    CloseHandle(hMapping);
    CloseHandle(hFile);
}

void CopyFileData(const char* sourceFilename, const char* destFilename, char insertChar, DWORD insertPosition) {
    HANDLE hSourceFile = CreateFileA(sourceFilename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hSourceFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Ошибка открытия файла для чтения: " << GetLastError() << std::endl;
        return;
    }

    HANDLE hDestFile = CreateFileA(destFilename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hDestFile == INVALID_HANDLE_VALUE) {
        std::cerr << "Ошибка открытия файла для записи: " << GetLastError() << std::endl;
        CloseHandle(hSourceFile);
        return;
    }

    const DWORD bufferSize = 4096;
    char buffer[bufferSize];
    DWORD bytesRead, bytesWritten;
    DWORD totalBytesRead = 0;
    char* newBuffer = nullptr;
    DWORD newBufferSize = 0;

    auto start = std::chrono::high_resolution_clock::now();
    std::cout << "SYNC. Чтение и запись файла с добавлением символа." << std::endl;

    while (ReadFile(hSourceFile, buffer, bufferSize, &bytesRead, NULL) && bytesRead > 0) {
        if (totalBytesRead <= insertPosition && insertPosition < totalBytesRead + bytesRead) {
            DWORD localInsertPos = insertPosition - totalBytesRead;
            InsertCharacter(buffer, bytesRead, insertChar, localInsertPos, reinterpret_cast<void*&>(newBuffer), newBufferSize);
        }
        else {
            newBuffer = buffer;
            newBufferSize = bytesRead;
        }

        totalBytesRead += bytesRead;

        if (!WriteFile(hDestFile, newBuffer, newBufferSize, &bytesWritten, NULL) || bytesWritten != newBufferSize) {
            std::cerr << "Ошибка записи в файл: " << GetLastError() << std::endl;
            CloseHandle(hSourceFile);
            CloseHandle(hDestFile);
            return;
        }
        if (newBuffer != buffer) {
            delete[] newBuffer;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "SYNC. Время обработки: " << elapsed.count() << " секунд." << std::endl;

    CloseHandle(hSourceFile);
    CloseHandle(hDestFile);
}

int main() {
    setlocale(LC_ALL, "ru");
    const char* filename = "test.txt";
    char insertChar = 'Z';
    DWORD insertPosition = 6;

    CopyFileData(filename, "SyncFinal.txt", insertChar, insertPosition);
    MapFileToMemory(filename, insertChar, insertPosition);

    return 0;
}