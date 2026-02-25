# 🧛 Vampire Survivors - Elite Internal Trainer v4.0

![Language](https://img.shields.io/badge/Language-C%2B%2B-blue)
![Platform](https://img.shields.io/badge/Platform-Windows%20x64-red)

A professional, DirectX 11 based internal mod menu for **Vampire Survivors**.

---

## 🚀 Features
- **DirectX 11 Overlay:** Transparent mod menu that stays active inside the game window.
- **Smart Pointer Capture:** Automatically captures `pPlayer` and `pGoldBase` via assembly hooks.
- **God Mode:** Infinite health via memory patching.
- **Gold & EXP Hack:** Add 1,000,000 gold or 10,000 EXP with a single click.

## 🛠 Project Structure
- `src/` - Source code for the trainer and injector.
- `vendor/` - Third-party libraries (ImGui, MinHook).
- `bin/` - Output directory for compiled binaries.

## 📦 Compilation
Ensure you have the **Visual Studio Build Tools** installed and run this from the root directory:

```powershell
# Compile Trainer (DLL) & Injector (EXE)
cmd /c 'call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" && cl /LD /EHsc /Fe:bin\VampireSurvivorsTrainer.dll src\main.cpp vendor\imgui\imgui.cpp vendor\imgui\imgui_draw.cpp vendor\imgui\imgui_tables.cpp vendor\imgui\imgui_widgets.cpp vendor\imgui\backends\imgui_impl_win32.cpp vendor\imgui\backends\imgui_impl_dx11.cpp vendor\minhook\src\buffer.c vendor\minhook\src\hook.c vendor\minhook\src\trampoline.c vendor\minhook\src\hde\hde64.c vendor\minhook\src\hde\hde32.c User32.lib Kernel32.lib D3d11.lib Dxgi.lib /Ivendor\imgui /Ivendor\minhook\include && cl /EHsc /Fe:bin\VampireSurvivorsTrainer.exe src\injector.cpp User32.lib Kernel32.lib'
```

## 👤 Author
- **Xenopuss**
