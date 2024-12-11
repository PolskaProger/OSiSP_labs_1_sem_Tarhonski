#include <iostream>
#include <windows.h>
#include <thread>
#include <vector>
#include <chrono>
#include <mutex>
#include <atomic>

HANDLE semaphoreReaders; 
HANDLE mutexWriters; 
CRITICAL_SECTION readersCountLock;

int readersCount = 0;
int writersWaiting = 0;
int sharedData = 0;   

std::mutex statsMutex;
int successfulReads = 0;
int successfulWrites = 0;
int failedReads = 0;
int failedWrites = 0;
double totalReadTime = 0.0;
double totalWriteTime = 0.0;
double totalReadBlockTime = 0.0;
double totalWriteBlockTime = 0.0;

std::atomic<bool> isRunning(true);

void reader(int id, int duration) {
    while (isRunning) {
        auto blockStartTime = std::chrono::high_resolution_clock::now();

        EnterCriticalSection(&readersCountLock);
        if (readersCount == 0 && writersWaiting > 0) {
            LeaveCriticalSection(&readersCountLock);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));

            {
                std::lock_guard<std::mutex> lock(statsMutex);
                failedReads++;
                std::cout << "Читатель " << id << " не смог прочитать (ожидание писателей)." << std::endl;
            }
            continue;
        }

        readersCount++;
        LeaveCriticalSection(&readersCountLock);

        auto blockEndTime = std::chrono::high_resolution_clock::now();
        totalReadBlockTime += std::chrono::duration<double>(blockEndTime - blockStartTime).count();

        auto startTime = std::chrono::high_resolution_clock::now();
        std::this_thread::sleep_for(std::chrono::milliseconds(duration));
        std::cout << "Читатель " << id << " читает данные: " << sharedData << std::endl;
        auto endTime = std::chrono::high_resolution_clock::now();

        EnterCriticalSection(&readersCountLock);
        readersCount--;
        if (readersCount == 0) {
            ReleaseSemaphore(semaphoreReaders, 1, NULL);
        }
        LeaveCriticalSection(&readersCountLock);

        {
            std::lock_guard<std::mutex> lock(statsMutex);
            successfulReads++;
            totalReadTime += std::chrono::duration<double>(endTime - startTime).count();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void writer(int id, int duration) {
    while (isRunning) {
        auto blockStartTime = std::chrono::high_resolution_clock::now();
        ++writersWaiting;

        DWORD waitResult = WaitForSingleObject(mutexWriters, 10); // Тайм-аут 10 миллисекунд
        if (waitResult == WAIT_TIMEOUT) {
            std::lock_guard<std::mutex> lock(statsMutex);
            failedWrites++;

            --writersWaiting;
            std::cout << "Писатель " << id << " не смог дождаться доступа для записи." << std::endl;
            continue;
        }

        auto blockEndTime = std::chrono::high_resolution_clock::now();
        totalWriteBlockTime += std::chrono::duration<double>(blockEndTime - blockStartTime).count();

        sharedData++;
        std::cout << "Писатель " << id << " записывает данные: " << sharedData << std::endl;
        ReleaseMutex(mutexWriters);

        --writersWaiting;

        {
            std::lock_guard<std::mutex> lock(statsMutex);
            successfulWrites++;
            totalWriteTime += duration / 1000.0;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void cleanup() {
    CloseHandle(semaphoreReaders);
    CloseHandle(mutexWriters);
    DeleteCriticalSection(&readersCountLock);
}

int main() {
    setlocale(LC_ALL, "ru"); 

    int numReaders = 3;    
    int numWriters = 4;       
    int readDuration = 200;   
    int writeDuration = 1000; 

    semaphoreReaders = CreateSemaphore(NULL, numReaders, numReaders, NULL);
    mutexWriters = CreateMutex(NULL, FALSE, NULL);
    InitializeCriticalSection(&readersCountLock);

    std::vector<std::thread> threads;

    for (int i = 0; i < numReaders; ++i) {
        threads.emplace_back(reader, i + 1, readDuration);
    }

    for (int i = 0; i < numWriters; ++i) {
        threads.emplace_back(writer, i + 1, writeDuration);
    }

    std::this_thread::sleep_for(std::chrono::seconds(3));
    isRunning = false;

    for (auto& th : threads) {
        th.join();
    }

    std::cout << "\n=== Результаты симуляции ===\n";
    std::cout << "Успешные чтения: " << successfulReads << std::endl;
    std::cout << "Неудачные чтения: " << failedReads << std::endl;
    std::cout << "Успешные записи: " << successfulWrites << std::endl;
    std::cout << "Неудачные записи: " << failedWrites << std::endl;
    std::cout << "Среднее время чтения: " << (successfulReads ? totalReadTime / successfulReads : 0) << " сек." << std::endl;
    std::cout << "Среднее время записи: " << (successfulWrites ? totalWriteTime / successfulWrites : 0) << " сек." << std::endl;
    std::cout << "Общее время блокировки чтения: " << totalReadBlockTime << " сек." << std::endl;
    std::cout << "Общее время блокировки записи: " << totalWriteBlockTime << " сек." << std::endl;

    cleanup();
    return 0;
}