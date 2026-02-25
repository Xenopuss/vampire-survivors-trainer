#include <windows.h>
#include <iostream>
#include <tlhelp32.h>
#include <string>

DWORD GetProcId(const char* procName) {
    DWORD procId = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 procEntry;
        procEntry.dwSize = sizeof(procEntry);

        if (Process32First(hSnap, &procEntry)) {
            do {
                if (!_stricmp(procEntry.szExeFile, procName)) {
                    procId = procEntry.th32ProcessID;
                    break;
                }
            } while (Process32Next(hSnap, &procEntry));
        }
    }
    CloseHandle(hSnap);
    return procId;
}

int main() {
    const char* dllPath = "VampireSurvivorsTrainer.dll";
    const char* procName = "VampireSurvivors.exe";
    char fullPath[MAX_PATH];

    if (!GetFullPathNameA(dllPath, MAX_PATH, fullPath, nullptr)) {
        std::cerr << "[-] DLL yolu bulunamadi!" << std::endl;
        return 1;
    }

    std::cout << "[*] VampireSurvivors.exe bekleniyor..." << std::endl;
    DWORD procId = 0;
    while (!procId) {
        procId = GetProcId(procName);
        Sleep(100);
    }

    std::cout << "[+] Process bulundu! ID: " << procId << std::endl;

    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procId);
    if (!hProc) {
        std::cerr << "[-] Process acilamadi!" << std::endl;
        return 1;
    }

    void* loc = VirtualAllocEx(hProc, nullptr, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!loc) {
        std::cerr << "[-] Bellek ayrilamadi!" << std::endl;
        return 1;
    }

    WriteProcessMemory(hProc, loc, fullPath, strlen(fullPath) + 1, nullptr);

    HANDLE hThread = CreateRemoteThread(hProc, nullptr, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, loc, 0, nullptr);
    if (!hThread) {
        std::cerr << "[-] Uzak thread olusturulamadi!" << std::endl;
        return 1;
    }

    std::cout << "[+] DLL basariyla enjekte edildi!" << std::endl;

    CloseHandle(hThread);
    CloseHandle(hProc);

    return 0;
}
