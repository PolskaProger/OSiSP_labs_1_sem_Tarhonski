#include <windows.h>
#include <tchar.h>
#include <iostream>
using namespace std;

STARTUPINFO si;
PROCESS_INFORMATION pi;

void RunChildProcess(bool saveHandle)
{
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    wstring CommandLine(L"Child.exe");
    LPWSTR lpwCmdLine = &CommandLine[0];

    // Start the child process.
    if (!CreateProcess(NULL,    // No module name (use command line)
        lpwCmdLine,      // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory
        &si,            // Pointer to STARTUPINFO structure
        &pi)           // Pointer to PROCESS_INFORMATION structure
        ) {
        std::cout << "������ �������� ��������\n";
        return;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    if (saveHandle) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

void CheckZombie()
{
    auto result = WaitForSingleObject(pi.hProcess, 0);

    if (result == WAIT_OBJECT_0) {
        cout << "������� ����������\n";
    }
    else {
        cout << "����� �������\n";
    }
}

int main() {
    setlocale(LC_ALL, "ru");
    cout << "�������� �������\n";
    int savehandle;
    while (true) {
        cout << "��������� ���������� ��������? 1 - ��, 0 - ���, 2 - �����\n";

        cin >> savehandle;
        if (savehandle == 2) {
            return 0;
        }

        RunChildProcess(savehandle);
        CheckZombie();
    }
}