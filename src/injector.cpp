#include <windows.h>
#include <iostream>
#include <tlhelp32.h>
#include <string>

// Function to find process ID by name
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
    const char* dllName = "VampireSurvivorsTrainer.dll";
    const char* procName = "VampireSurvivors.exe";
    char fullPath[MAX_PATH];

    // Get absolute path of the DLL
    if (!GetFullPathNameA(dllName, MAX_PATH, fullPath, nullptr)) {
        std::cerr << "[-] Error: Could not locate DLL path!" << std::endl;
        return 1;
    }

    std::cout << "[*] Waiting for " << procName << "..." << std::endl;
    DWORD procId = 0;
    while (!procId) {
        procId = GetProcId(procName);
        Sleep(100);
    }

    std::cout << "[+] Process found! ID: " << procId << std::endl;

    // Open target process
    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procId);
    if (!hProc) {
        std::cerr << "[-] Error: Could not open process!" << std::endl;
        return 1;
    }

    // Allocate memory in target process for DLL path
    void* loc = VirtualAllocEx(hProc, nullptr, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!loc) {
        std::cerr << "[-] Error: Memory allocation failed!" << std::endl;
        return 1;
    }

    // Write DLL path to target process memory
    WriteProcessMemory(hProc, loc, fullPath, strlen(fullPath) + 1, nullptr);

    // Create remote thread to load the DLL
    HANDLE hThread = CreateRemoteThread(hProc, nullptr, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, loc, 0, nullptr);
    if (!hThread) {
        std::cerr << "[-] Error: Remote thread creation failed!" << std::endl;
        return 1;
    }

    std::cout << "[+] DLL injected successfully!" << std::endl;

    CloseHandle(hThread);
    CloseHandle(hProc);

    return 0;
}
