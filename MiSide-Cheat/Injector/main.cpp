#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;

DWORD GetProcessIdByName(const std::wstring& processName) {
    DWORD pid = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);
        if (Process32FirstW(snapshot, &pe32)) {
            do {
                if (processName == pe32.szExeFile) {
                    pid = pe32.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snapshot, &pe32));
        }
        CloseHandle(snapshot);
    }
    return pid;
}

bool InjectDLL(DWORD pid, const std::wstring& dllPath) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) {
        std::cerr << "Failed to open process. Error: " << GetLastError() << std::endl;
        return false;
    }

    size_t size = (dllPath.length() + 1) * sizeof(wchar_t);
    void* pRemotePath = VirtualAllocEx(hProcess, nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!pRemotePath) {
        std::cerr << "Failed to allocate memory in remote process. Error: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        return false;
    }

    if (!WriteProcessMemory(hProcess, pRemotePath, dllPath.c_str(), size, nullptr)) {
        std::cerr << "Failed to write memory to remote process. Error: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pRemotePath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    if (!hKernel32) {
        std::cerr << "Failed to get kernel32 handle." << std::endl;
        VirtualFreeEx(hProcess, pRemotePath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    LPTHREAD_START_ROUTINE pLoadLibrary = (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32, "LoadLibraryW");
    if (!pLoadLibrary) {
        std::cerr << "Failed to get LoadLibraryW address." << std::endl;
        VirtualFreeEx(hProcess, pRemotePath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, pLoadLibrary, pRemotePath, 0, nullptr);
    if (!hThread) {
        std::cerr << "Failed to create remote thread. Error: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pRemotePath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    WaitForSingleObject(hThread, INFINITE);
    
    DWORD exitCode = 0;
    GetExitCodeThread(hThread, &exitCode);
    
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, pRemotePath, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    
    return exitCode != 0;
}

int main() {
    std::wstring targetProcessName = L"MiSide Zero.exe";
    std::wstring dllName = L"MiSide-Cheat.dll";

    std::cout << "Waiting for " << std::string(targetProcessName.begin(), targetProcessName.end()) << "..." << std::endl;

    DWORD pid = 0;
    while ((pid = GetProcessIdByName(targetProcessName)) == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cout << "Found: " << std::string(targetProcessName.begin(), targetProcessName.end()) << std::endl;
    std::cout << "Injecting in: " << std::string(targetProcessName.begin(), targetProcessName.end()) << std::endl;

    // Find DLL next to executable
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    fs::path currentPath = fs::path(exePath).parent_path();
    fs::path dllPath = currentPath / dllName;

    if (!fs::exists(dllPath)) {
        std::cerr << "DLL not found at: " << std::string(dllPath.string().begin(), dllPath.string().end()) << std::endl;
        std::cout << "Press Enter to exit...";
        std::cin.get();
        return 1;
    }

    if (InjectDLL(pid, dllPath.wstring())) {
        std::cout << "Successfully injected!" << std::endl;
    } else {
        std::cerr << "Injection failed." << std::endl;
        std::cout << "Press Enter to exit...";
        std::cin.get();
        return 1;
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));
    return 0;
}
