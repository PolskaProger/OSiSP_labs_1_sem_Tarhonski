#include <iostream>
#include <windows.h>
using namespace std;

int main() {
	setlocale(LC_ALL, "ru");
	cout << "Запусик дочернего процесса" << endl;
	cout << "Дочерний процес запущен...." << endl;
	Sleep(10000);
	cout << "Дочерний процесс завершён" << endl;
	exit(0);
}