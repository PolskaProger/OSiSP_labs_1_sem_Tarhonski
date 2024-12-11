#include <windows.h>
#include <iostream>
#include <string>

#define SHARED_MEM_SIZE 1024
#define SHARED_MEM_NAME L"Local\\MySharedMem"
#define MUTEX_NAME L"Local\\MyMutex"
#define MAX_MSG_COUNT 10
#define SINGLE_MSG_SIZE 100

using namespace std;

HANDLE hSharedMem;
LPVOID pSharedMem;
HANDLE hMutex;
char* msgBuffer;

void initializeSharedMemory() {
    setlocale(LC_ALL, "ru");
    hMutex = CreateMutex(NULL, FALSE, MUTEX_NAME);
    if (hMutex == NULL) {
        cerr << "Ошибка при создании мьютекса: " << GetLastError() << endl;
        exit(EXIT_FAILURE);
    }

    hSharedMem = CreateFileMapping(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        0,
        SHARED_MEM_SIZE,
        SHARED_MEM_NAME);

    if (hSharedMem == NULL) {
        cerr << "Не удалось создать объект сопоставления файла: " << GetLastError() << endl;
        exit(EXIT_FAILURE);
    }

    pSharedMem = MapViewOfFile(hSharedMem, FILE_MAP_ALL_ACCESS, 0, 0, SHARED_MEM_SIZE);
    if (pSharedMem == NULL) {
        cerr << "Не удалось сопоставить представление файла: " << GetLastError() << endl;
        CloseHandle(hSharedMem);
        exit(EXIT_FAILURE);
    }

    msgBuffer = static_cast<char*>(pSharedMem);
}

void sendMessage(const string& msg) {
    setlocale(LC_ALL, "ru");
    WaitForSingleObject(hMutex, INFINITE);

    int msgCount = 0;
    for (int i = 0; i < MAX_MSG_COUNT; ++i) {
        if (msgBuffer[i * SINGLE_MSG_SIZE] == '\0') {
            strncpy_s(msgBuffer + (i * SINGLE_MSG_SIZE), SINGLE_MSG_SIZE, msg.c_str(), msg.size());
            Sleep(3000);
            cout << "Сообщение отправлено: " << msg << endl;
            break;
        }
        msgCount++;
    }

    if (msgCount == MAX_MSG_COUNT) {
        cout << "Буфер переполнен!" << endl;
    }

    ReleaseMutex(hMutex);
}

void receiveMessages() {
    setlocale(LC_ALL, "ru");
    WaitForSingleObject(hMutex, INFINITE);

    cout << "Сообщения в разделяемой памяти:" << endl;
    for (int i = 0; i < MAX_MSG_COUNT; ++i) {
        if (msgBuffer[i * SINGLE_MSG_SIZE] != '\0') {
            cout << "Сообщение " << (i + 1) << ": " << msgBuffer + (i * SINGLE_MSG_SIZE) << endl;
        }
        else {
            break;
        }
    }

    ReleaseMutex(hMutex);
}

void clearMessageBuffer() {
    setlocale(LC_ALL, "ru");
    WaitForSingleObject(hMutex, INFINITE);
    memset(msgBuffer, 0, SHARED_MEM_SIZE);
    cout << "Буфер сообщений очищен." << endl;
    ReleaseMutex(hMutex);
}

void messageSender() {
    setlocale(LC_ALL, "ru");
    initializeSharedMemory();
    string msg;

    while (true) {
        cout << "Введите сообщение (введите 'exit' для выхода): ";
        getline(cin, msg);
        if (msg == "exit") {
            break;
        }
        sendMessage(msg);
    }
}

void messageReceiver() {
    setlocale(LC_ALL, "ru");
    initializeSharedMemory();
    receiveMessages();
    clearMessageBuffer();
}

int main() {
    setlocale(LC_ALL, "ru");
    int option;

    while (true) {
        cout << "Выберите режим:\n1. Отправитель\n2. Получатель\n";
        cin >> option;
        cin.ignore();

        if (option == 1) {
            messageSender();
        }
        else if (option == 2) {
            messageReceiver();
        }
        else {
            break;
        }
    }

    UnmapViewOfFile(pSharedMem);
    CloseHandle(hSharedMem);
    CloseHandle(hMutex);

    return 0;
}