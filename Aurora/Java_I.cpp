#include "Java_I.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>
#include <sys/stat.h>
#include <cstdlib>
#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#include <Urlmon.h>
#pragma comment(lib, "Urlmon.lib")
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#endif

// Функция для проверки существования файла
bool fileExists(const std::string& filePath) {
    std::ifstream file(filePath);
    return file.good();
}

// Функция для проверки существования директории
bool directoryExists(const std::string& directoryPath) {
#ifdef _WIN32
    DWORD fileAttributes = GetFileAttributesA(directoryPath.c_str());
    return (fileAttributes != INVALID_FILE_ATTRIBUTES && (fileAttributes & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat info;
    if (stat(directoryPath.c_str(), &info) != 0)
        return false;
    return (info.st_mode & S_IFDIR) != 0;
#endif
}

// Функция для создания директории, если она не существует
bool createDirectoryIfNotExists(const std::string& directoryPath) {
    if (directoryExists(directoryPath)) {
        std::cout << "Directory already exists: " << directoryPath << "\n";
        return true;
    }

#ifdef _WIN32
    int result = _mkdir(directoryPath.c_str());
#else
    mode_t mode = 0755;
    int result = mkdir(directoryPath.c_str(), mode);
#endif

    if (result != 0) {
        std::cout << "Failed to create directory: " << directoryPath << "\n";
        return false;
    }

    return true;
}

#ifdef _WIN32
// Функция для загрузки файла по URL
bool downloadFileIfNotExists(const std::string& url, const std::string& filePath) {
    if (fileExists(filePath)) {
        std::cout << "File already exists: " << filePath << "\n";
        return true;
    }

    HRESULT hr = URLDownloadToFileA(NULL, url.c_str(), filePath.c_str(), 0, NULL);
    if (SUCCEEDED(hr)) {
        std::cout << "File downloaded successfully.\n";
        return true;
    }
    else {
        std::cout << "Failed to download file from URL: " << url << "\n";
        return false;
    }
}
#else
// Заглушка для функции downloadFileIfNotExists в Linux
bool downloadFileIfNotExists(const std::string& url, const std::string& filePath) {
    std::cout << "Download file function not implemented for Linux.\n";
    return false;
}
#endif

// Функция для распаковки ZIP-архива без использования дополнительных библиотек
bool extractZipArchive(const std::string& archivePath, const std::string& destinationPath) {
    // Создание директории назначения, если она не существует
    if (!createDirectoryIfNotExists(destinationPath)) {
        std::cout << "Failed to create destination directory: " << destinationPath << "\n";
        return false;
    }

#ifdef _WIN32
    std::string command = "powershell -command \"[System.IO.Compression.ZipFile]::ExtractToDirectory('" + archivePath + "', '" + destinationPath + "')\"";
    int result = system(command.c_str());
#else
    std::string command = "unzip -o " + archivePath + " -d " + destinationPath;
    int result = std::system(command.c_str());
#endif

    if (result != 0) {
        std::cout << "Failed to extract ZIP archive.\n";
        return false;
    }

    return true;
}

// Функция для запуска JAR-файла с помощью Java
bool runJarWithJava(const std::wstring& javaExecutablePath, const std::wstring& jarPath) {
    std::wcout << L"Java executable path: " << javaExecutablePath << L"\n";
    std::wcout << L"Java JAR path: " << jarPath << L"\n";

    std::wstring command = javaExecutablePath + L" -jar \"" + jarPath + L"\"";

    std::wcout << L"Command: " << command << L"\n";

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    if (!CreateProcessW(NULL, const_cast<LPWSTR>(command.c_str()), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        std::wcout << L"Failed to execute JAR file.\n";
        return false;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return (exitCode == 0);
}

int Java_I::Java_I_main() {
    // Пути и ссылки
    std::string javaDownloadUrl = "https://download.oracle.com/java/20/latest/jdk-20_windows-x64_bin.zip";
    std::string javaArchivePath = "D:\\Aurora\\java_archive.zip";
    std::string javaExtractPath = "D:\\Aurora\\java_extracted";
    std::string javaJarUrl = "https://getfile.dokpub.com/yandex/get/https://disk.yandex.ru/d/kZCuek4AfD0-ZQ";
    std::string javaJarPath = "D:\\Aurora\\jar_some.jar";
    std::wstring javaExecutablePath = L"D:\\Aurora\\java_extracted\\jdk-20.0.1\\bin\\java.exe";

    // Создание директории, если она не существует
    std::cout << "Creating directories...\n";
    if (!createDirectoryIfNotExists("D:\\Aurora")) {
        std::cout << "Failed to create directories.\n";
        return 0;
    }

    // Загрузка Java архива (ZIP), только если он не существует
    std::cout << "Checking if Java archive exists...\n";
    if (!fileExists(javaArchivePath)) {
        std::cout << "Java archive does not exist. Downloading...\n";
        if (!downloadFileIfNotExists(javaDownloadUrl, javaArchivePath)) {
            std::cout << "Failed to download Java archive.\n";
            return 0;
        }
    }
    else {
        std::cout << "Java archive already exists.\n";
    }

    // Распаковка Java архива (ZIP), только если он не был распакован ранее
    std::cout << "Checking if Java archive is extracted...\n";
    if (!directoryExists(javaExtractPath)) {
        std::cout << "Java archive is not extracted. Extracting...\n";
        if (!extractZipArchive(javaArchivePath, javaExtractPath)) {
            std::cout << "Failed to extract Java archive.\n";
            return 0;
        }
    }
    else {
        std::cout << "Java archive is already extracted.\n";
    }

    // Загрузка JAR-файла, только если он не существует
    std::cout << "Checking if JAR file exists...\n";
    if (!fileExists(javaJarPath)) {
        std::cout << "JAR file does not exist. Downloading...\n";
        if (!downloadFileIfNotExists(javaJarUrl, javaJarPath)) {
            std::cout << "Failed to download JAR file.\n";
            return 0;
        }
    }
    else {
        std::cout << "JAR file already exists.\n";
    }

    // Запуск JAR-файла с помощью Java
    std::wcout << L"Running JAR file...\n";
    if (!runJarWithJava(javaExecutablePath, std::wstring(javaJarPath.begin(), javaJarPath.end()))) {
        std::wcout << L"Failed to execute JAR file.\n";
        return 0;
    }

    return 0;
}
