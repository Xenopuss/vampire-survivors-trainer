#include <windows.h>
#include <d3d11.h>
#include <iostream>
#include <vector>

// ImGui Headers
#include "../vendor/imgui/imgui.h"
#include "../vendor/imgui/backends/imgui_impl_win32.h"
#include "../vendor/imgui/backends/imgui_impl_dx11.h"

// MinHook Headers
#include "../vendor/minhook/include/MinHook.h"

#pragma comment(lib, "d3d11.lib")

// --- KÜRESEL POINTERLAR ---
uintptr_t pPlayer = 0;
uintptr_t pGoldBase = 0;

// Hile Durumları
bool bGodMode = false;
BYTE godModeOriginal[] = { 0xF3, 0x0F, 0x11, 0x86, 0x28, 0x02, 0x00, 0x00 };
uintptr_t godModeAddress = 0;

// Bellek Araçları
void Patch(BYTE* dst, BYTE* src, unsigned int size) {
    DWORD oldprotect;
    VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &oldprotect);
    memcpy(dst, src, size);
    VirtualProtect(dst, size, oldprotect, &oldprotect);
}

void Nop(BYTE* dst, unsigned int size) {
    DWORD oldprotect;
    VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &oldprotect);
    memset(dst, 0x90, size);
    VirtualProtect(dst, size, oldprotect, &oldprotect);
}

std::vector<int> PatternToBytes(const char* pattern) {
    std::vector<int> bytes;
    char* start = const_cast<char*>(pattern);
    char* end = start + strlen(pattern);
    for (char* current = start; current < end; ++current) {
        if (*current == '?') {
            ++current;
            if (*current == '?') ++current;
            bytes.push_back(-1); 
        } else {
            bytes.push_back(strtol(current, &current, 16));
        }
    }
    return bytes;
}

uintptr_t PatternScan(const char* moduleName, const char* pattern) {
    HMODULE hModule = GetModuleHandleA(moduleName);
    if (!hModule) return 0;
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hModule;
    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((BYTE*)hModule + dosHeader->e_lfanew);
    DWORD sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;
    std::vector<int> patternBytes = PatternToBytes(pattern);
    BYTE* scanBytes = (BYTE*)hModule;
    size_t s = patternBytes.size();
    int* d = patternBytes.data();
    for (size_t i = 0; i < sizeOfImage - s; ++i) {
        bool found = true;
        for (size_t j = 0; j < s; ++j) {
            if (scanBytes[i + j] != d[j] && d[j] != -1) {
                found = false;
                break;
            }
        }
        if (found) {
            return (uintptr_t)&scanBytes[i];
        }
    }
    return 0;
}

void* CreateHook(BYTE* targetAddress, BYTE* payload, size_t payloadSize, size_t overwrittenLength) {
    void* shellcode = VirtualAlloc(nullptr, payloadSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!shellcode) return nullptr;
    memcpy(shellcode, payload, payloadSize);
    uintptr_t returnAddress = (uintptr_t)targetAddress + overwrittenLength;
    memcpy((BYTE*)shellcode + payloadSize - 8, &returnAddress, 8);
    DWORD oldProtect;
    VirtualProtect(targetAddress, overwrittenLength, PAGE_EXECUTE_READWRITE, &oldProtect);
    memset(targetAddress, 0x90, overwrittenLength);
    targetAddress[0] = 0xFF; targetAddress[1] = 0x25;
    targetAddress[2] = 0x00; targetAddress[3] = 0x00;
    targetAddress[4] = 0x00; targetAddress[5] = 0x00;
    memcpy(&targetAddress[6], &shellcode, 8);
    VirtualProtect(targetAddress, overwrittenLength, oldProtect, &oldProtect);
    return shellcode;
}

// --- IMGUI VE DIRECTX 11 MOTORU ---
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

typedef HRESULT(__stdcall* Present_t)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
Present_t oPresent = nullptr;

HWND window = nullptr;
WNDPROC oWndProc;
ID3D11Device* pDevice = nullptr;
ID3D11DeviceContext* pContext = nullptr;
ID3D11RenderTargetView* mainRenderTargetView = nullptr;
bool init = false;

// Fare ve Klavye Girdilerini Yakalama
LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (init) {
        ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
        // İmleç menü üzerindeyken oyunun tıklamaları algılamasını engellemek için:
        if (ImGui::GetIO().WantCaptureMouse) {
            return TRUE;
        }
    }
    return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}

// Oyunun Görüntüsüne Sızdığımız Ana Döngü
HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
    if (!init) {
        if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice))) {
            pDevice->GetImmediateContext(&pContext);
            DXGI_SWAP_CHAIN_DESC sd;
            pSwapChain->GetDesc(&sd);
            window = sd.OutputWindow;

            ID3D11Texture2D* pBackBuffer = nullptr;
            pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
            if (pBackBuffer) {
                pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &mainRenderTargetView);
                pBackBuffer->Release();
            }

            // Fare girdileri için WndProc kancası
            oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);

            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

            // ELITE Tema Tasarımı
            ImGui::StyleColorsDark();
            ImGuiStyle& style = ImGui::GetStyle();
            style.WindowRounding = 8.0f;
            style.Colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.95f);
            style.Colors[ImGuiCol_Border] = ImVec4(0.8f, 0.0f, 0.0f, 0.8f);
            style.Colors[ImGuiCol_Button] = ImVec4(0.4f, 0.0f, 0.0f, 1.0f);
            style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.6f, 0.1f, 0.1f, 1.0f);
            style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
            style.Colors[ImGuiCol_TitleBg] = ImVec4(0.4f, 0.0f, 0.0f, 1.0f);
            style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.6f, 0.0f, 0.0f, 1.0f);

            ImGui_ImplWin32_Init(window);
            ImGui_ImplDX11_Init(pDevice, pContext);
            init = true;
        } else {
            return oPresent(pSwapChain, SyncInterval, Flags);
        }
    }

    // Her Kare (Frame) Başlangıcı
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Menü Çizimi
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(350, 320), ImGuiCond_FirstUseEver);
    
    ImGui::Begin("Vampire Survivors Elite Trainer V4.0", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
    
    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "ZORG-OMEGA Overlay Engine");
    ImGui::Separator();
    ImGui::Spacing();
    
    if (pPlayer) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[+] Player Data: Captured");
    } else {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "[-] Player Data: Waiting (Move in-game)");
    }

    if (pGoldBase) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[+] Gold Data: Captured");
    } else {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "[-] Gold Data: Waiting (Open shop)");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // God Mode Checkbox
    if (ImGui::Checkbox("Enable God Mode", &bGodMode)) {
        if (godModeAddress) {
            if (bGodMode) Nop((BYTE*)godModeAddress, 8);
            else Patch((BYTE*)godModeAddress, godModeOriginal, 8);
        }
    }

    ImGui::Spacing();
    
    // Altın Butonu
    if (ImGui::Button("Add 1,000,000 Gold", ImVec2(-1, 35))) {
        if (pGoldBase) {
            float currentGold = *(float*)(pGoldBase + 0x84);
            *(float*)(pGoldBase + 0x84) = currentGold + 1000000.0f;
        }
    }

    // EXP Butonu
    if (ImGui::Button("Add 10,000 EXP", ImVec2(-1, 35))) {
        if (pPlayer) {
            float currentExp = *(float*)(pPlayer + 0x23C);
            *(float*)(pPlayer + 0x23C) = currentExp + 10000.0f;
        }
    }

    // Can Fulleme Butonu
    if (ImGui::Button("Restore Full Health", ImVec2(-1, 35))) {
        if (pPlayer) {
            *(float*)(pPlayer + 0x228) = 9999.0f;
        }
    }

    ImGui::End();

    ImGui::Render();

    pContext->OMSetRenderTargets(1, &mainRenderTargetView, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    return oPresent(pSwapChain, SyncInterval, Flags);
}

// Ana Başlatma Zinciri
DWORD WINAPI MainThread(LPVOID lpReserved) {
    // 1. MinHook'u Başlat
    MH_Initialize();

    // 2. İmza Taramaları ve Kancalar (Hack Motoru)
    godModeAddress = PatternScan("GameAssembly.dll", "F3 0F 11 86 28 02 00 00");
    uintptr_t playerReadAddress = PatternScan("GameAssembly.dll", "F3 0F 10 B3 28 02 00 00 48 8B 82");
    uintptr_t goldReadAddress = PatternScan("GameAssembly.dll", "F3 0F 10 80 84 00 00 00 48 89 5C 24 50");

    if (playerReadAddress) {
        BYTE playerPayload[] = {
            0x50, 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x89, 0x18, 0x58,
            0xF3, 0x0F, 0x10, 0xB3, 0x28, 0x02, 0x00, 0x00, 0x48, 0x8B, 0x82, 0x58, 0x05, 0x00, 0x00,
            0xFF, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };
        uintptr_t pPlayerAddr = (uintptr_t)&pPlayer;
        memcpy(&playerPayload[3], &pPlayerAddr, 8);
        CreateHook((BYTE*)playerReadAddress, playerPayload, sizeof(playerPayload), 15);
    }

    if (goldReadAddress) {
        BYTE goldPayload[] = {
            0x53, 0x48, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x89, 0x03, 0x5B,
            0xF3, 0x0F, 0x10, 0x80, 0x84, 0x00, 0x00, 0x00, 0x48, 0x89, 0x5C, 0x24, 0x50, 0x0F, 0x29, 0x74, 0x24, 0x30,
            0xFF, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };
        uintptr_t pGoldAddr = (uintptr_t)&pGoldBase;
        memcpy(&goldPayload[3], &pGoldAddr, 8);
        CreateHook((BYTE*)goldReadAddress, goldPayload, sizeof(goldPayload), 18);
    }

    // 3. DirectX 11 Present Fonksiyonunu Bul ve Kancala
    HWND hWnd = GetForegroundWindow();
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    IDXGISwapChain* pDummySwapChain = nullptr;
    ID3D11Device* pDummyDevice = nullptr;
    ID3D11DeviceContext* pDummyContext = nullptr;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, &featureLevel, 1, D3D11_SDK_VERSION, &sd, &pDummySwapChain, &pDummyDevice, nullptr, &pDummyContext);
    
    if (SUCCEEDED(hr)) {
        void** pVTable = *reinterpret_cast<void***>(pDummySwapChain);
        void* pPresent = pVTable[8]; // Present is at index 8 in IDXGISwapChain

        MH_CreateHook(pPresent, hkPresent, reinterpret_cast<void**>(&oPresent));
        MH_EnableHook(pPresent);

        pDummyDevice->Release();
        pDummyContext->Release();
        pDummySwapChain->Release();
    }

    return 0;
}

// DLL Giriş Noktası
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CloseHandle(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, nullptr));
    } else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
        if (oWndProc) {
            SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)oWndProc);
        }
    }
    return TRUE;
}
