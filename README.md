# 🧛 Vampire Survivors - Elite Internal Trainer v4.0

![GitHub stars](https://img.shields.io/github/stars/Xenopuss/vampire-survivors-trainer?style=social)
![Language](https://img.shields.io/badge/Language-C%2B%2B-blue)
![Platform](https://img.shields.io/badge/Platform-Windows%20x64-red)

A professional, DirectX 11 based internal mod menu for **Vampire Survivors**. This trainer uses an advanced 14-byte absolute jump engine to capture game pointers and provides a sleek Dear ImGui-based overlay.

---

## 🚀 Features

- **DirectX 11 Overlay:** Transparent mod menu that stays active inside the game window.
- **Smart Pointer Capture:** Automatically captures `pPlayer` and `pGoldBase` via assembly hooks.
- **God Mode:** Infinite health via memory patching.
- **Gold & EXP Hack:** Add 1,000,000 gold or 10,000 EXP with a single click.
- **Input Hooking:** Mouse and keyboard interactions are synchronized with the ImGui menu.

## 🛠 Tech Stack

- **Language:** C++20
- **GUI Engine:** [Dear ImGui](https://github.com/ocornut/imgui)
- **Hooking Library:** [MinHook](https://github.com/TsudaKageyu/minhook)
- **Graphics API:** DirectX 11 (DX11)

## 📦 Compilation Instructions

Ensure you have the **Visual Studio Build Tools** installed.

```powershell
# Run this in a Developer Command Prompt (x64)
cl /LD /EHsc /Fe:VampireSurvivorsTrainer.dll main.cpp imgui\imgui.cpp imgui\imgui_draw.cpp imgui\imgui_tables.cpp imgui\imgui_widgets.cpp imgui\backends\imgui_impl_win32.cpp imgui\backends\imgui_impl_dx11.cpp minhook\src\buffer.c minhook\src\hook.c minhook\src	rampoline.c minhook\src\hde\hde64.c minhook\src\hde\hde32.c User32.lib Kernel32.lib D3d11.lib Dxgi.lib /Iimgui /Iminhook\include

# Compile Injector
cl /EHsc /Fe:VampireSurvivorsTrainer.exe injector.cpp User32.lib Kernel32.lib
```

## 📖 Usage

1. Launch **Vampire Survivors**.
2. Run `VampireSurvivorsTrainer.exe` as Administrator.
3. The "Elite Trainer" menu will appear on the top-left corner.
4. Play the game! Pointers will be captured automatically.

---

## 👤 Author

- **Xenopuss**

---

*Disclaimer: This project is for educational purposes only. Modifying game memory may violate Terms of Service.*
