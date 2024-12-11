#include <iostream>
#include <comdef.h>
#include <Wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")

using namespace std;

int main() {
    HRESULT hr;

    hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        cerr << "Failed to initialize COM library. Error code: " << hr << endl;
        return 1;
    }

    hr = CoInitializeSecurity(
        nullptr,
        -1,
        nullptr,
        nullptr,
        RPC_C_AUTHN_LEVEL_PKT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        nullptr,
        EOAC_NONE,
        nullptr
    );
    if (FAILED(hr)) {
        cerr << "Failed to initialize security. Error code: " << hr << endl;
        CoUninitialize();
        return 1;
    }

    IWbemLocator* pLocator = nullptr;
    hr = CoCreateInstance(
        CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator,
        (LPVOID*)&pLocator
    );
    if (FAILED(hr)) {
        cerr << "Failed to create IWbemLocator object. Error code: " << hr << endl;
        CoUninitialize();
        return 1;
    }

    IWbemServices* pService = nullptr;
    hr = pLocator->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"),
        nullptr,
        nullptr,
        0,
        0,
        0,
        0,
        &pService
    );
    if (FAILED(hr)) {
        cerr << "Could not connect to WMI namespace. Error code: " << hr << endl;
        pLocator->Release();
        CoUninitialize();
        return 1;
    }

    hr = CoSetProxyBlanket(
        pService,
        RPC_C_AUTHN_WINNT,
        RPC_C_AUTHZ_NONE,
        nullptr,
        RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        nullptr,
        EOAC_NONE
    );
    if (FAILED(hr)) {
        cerr << "Could not set proxy blanket. Error code: " << hr << endl;
        pService->Release();
        pLocator->Release();
        CoUninitialize();
        return 1;
    }

    IEnumWbemClassObject* pEnumerator = nullptr;
    hr = pService->ExecQuery(
        bstr_t("WQL"),
        bstr_t("SELECT * FROM Win32_ComputerSystem"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        nullptr,
        &pEnumerator
    );
    if (FAILED(hr)) {
        cerr << "Query for system data failed. Error code: " << hr << endl;
        pService->Release();
        pLocator->Release();
        CoUninitialize();
        return 1;
    }

    IWbemClassObject* pClassObject = nullptr;
    ULONG uCount = 0;
    while (pEnumerator) {
        hr = pEnumerator->Next(WBEM_INFINITE, 1, &pClassObject, &uCount);
        if (0 == uCount) break;

        VARIANT varProperty;

        hr = pClassObject->Get(L"UserName", 0, &varProperty, 0, 0);
        wprintf(L"Username: %s\n", varProperty.bstrVal);
        VariantClear(&varProperty);

        hr = pClassObject->Get(L"Name", 0, &varProperty, 0, 0);
        wprintf(L"Computer name: %s\n", varProperty.bstrVal);
        VariantClear(&varProperty);

        hr = pClassObject->Get(L"Manufacturer", 0, &varProperty, 0, 0);
        wprintf(L"Manufacturer: %s\n", varProperty.bstrVal);
        VariantClear(&varProperty);

        hr = pClassObject->Get(L"Model", 0, &varProperty, 0, 0);
        wprintf(L"Model: %s\n", varProperty.bstrVal);
        VariantClear(&varProperty);

        pClassObject->Release();
    }

    pEnumerator->Release();
    pService->Release();
    pLocator->Release();
    CoUninitialize();

    return 0;
}
