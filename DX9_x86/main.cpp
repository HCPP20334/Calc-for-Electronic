// Dear ImGui: standalone example application for DirectX 9

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include <d3d9.h>
#include <string>
#include <Psapi.h>
#include <stdint.h>
#include <thread>
#include <intrin.h>
#include <tchar.h>
#pragma once
// Data
static LPDIRECT3D9              g_pD3D = nullptr;
static LPDIRECT3DDEVICE9        g_pd3dDevice = nullptr;
static bool                     g_DeviceLost = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Main code
int main(int, char**)
{
    // Create application window
    WNDCLASSEXW wc = { sizeof(wc), CS_OWNDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"Electrik Calc", nullptr };
    wc.hIcon = LoadIcon(wc.hInstance, MAKEINTRESOURCE(101));
    ::RegisterClassExW(&wc);

    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Electrik Calc", WS_OVERLAPPEDWINDOW, 100, 100, 500, 600, nullptr, nullptr, wc.hInstance, nullptr);
    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
  ImFont* font = io.Fonts->AddFontFromFileTTF("WhiteRabbit2.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    //IM_ASSERT(font != nullptr);

    // Our state
    static bool fFrameWA = false;
    static bool fFrameOM = false;
    static bool fFrameDIA = false;
    static bool fTable = false;
    static bool fNautomat = false;
    static bool fC_phase = false;
    static bool fFramePH = false;//(fPV * fPV / fPR);
    static bool fCAboutW;
    int fN = 1;
    float rs = 0;
    static bool v_conState = false;
    static bool fFrameAbout = false;
    static bool fLimitCPUUsage = true;
    //
    static bool fNin_0 = false;
    static bool fNin_1 = false;
    static bool fNin_2 = false;
    static bool fNin_3 = false;
    static bool fNin_4 = false;
    //
    static bool bWindow0 = true;
    static bool bWindow1 = false;
    //
    int32_t fNinOffset = 13;//
    int32_t fStyleCount = 0;
    static bool f4800Offset = false;
    static bool f2800Offset = false;
    //
    int fAbuffer = 0;
    int fVbuffer = 0;
    std::string fCOffset = "";
    uint32_t fp = 0;
    uint32_t fOval = 0;
    //
    bool bDarkStyle = true;
    //
    bool fFr_main = true;
    std::string str_theme = "Dark";
    HWND fH_con = GetConsoleWindow();
    ShowWindow(fH_con, 0);
    ImDrawListSplitter CurrentMemoryBuffer;
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    int CPUInfo[4] = { -1 };
    unsigned   nExIds, i = 0;
    char CPUBrandString[0x40];
    // Get the information associated with each extended ID.
    __cpuid(CPUInfo, 0x80000000);
    nExIds = CPUInfo[0];
    for (i = 0x80000000; i <= nExIds; ++i)
    {
        __cpuid(CPUInfo, i);
        // Interpret CPU brand string
        if (i == 0x80000002)
            memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
        else if (i == 0x80000003)
            memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
        else if (i == 0x80000004)
            memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
    }
    // std::cout << "" << "GL_EXTENSIONS: " << fD_extention << std::endl;
     //ReadConfig(&bDarkStyle, "dark=false");
    // Main loop
    bool done = false;
    bool v_titleS = true;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle lost D3D9 device
        if (g_DeviceLost)
        {
            HRESULT hr = g_pd3dDevice->TestCooperativeLevel();
            if (hr == D3DERR_DEVICELOST)
            {
                ::Sleep(10);
                continue;
            }
            if (hr == D3DERR_DEVICENOTRESET)
                ResetDevice();
            g_DeviceLost = false;
        }

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            g_d3dpp.BackBufferWidth = g_ResizeWidth;
            g_d3dpp.BackBufferHeight = g_ResizeHeight;
            g_ResizeWidth = g_ResizeHeight = 0;
            ResetDevice();
        }

        // Start the Dear ImGui frame/
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
       // std::cout << "GPU: Unit OK" << std::endl;
        ImGui::Begin("Калькулятор электроника v 1.0.1(beta)DX9_x86", &fFr_main);
        //std::cout << "GPU: Render Frame:"<<io.Framerate << std::endl;
        ImGuiStyle& style = ImGui::GetStyle();
        if (bDarkStyle) {
            style.WindowRounding = 5.3f;
            style.FrameRounding = 2.3f;
            style.ScrollbarRounding = 0;
            style.Colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 0.90f);
            style.Colors[ImGuiCol_WindowBg] = ImVec4(0.01f, 0.01f, 0.02f, 1.0f);
            style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
            style.Colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.10f, 0.85f);
            style.Colors[ImGuiCol_Border] = ImVec4(0.70f, 0.70f, 0.70f, 1.0f);
            style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            style.Colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
            style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.90f, 0.80f, 0.80f, 0.40f);
            style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.90f, 0.65f, 0.65f, 0.45f);
            style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.08f, 0.08f, 0.80f);
            style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.01f, 0.01f, 0.02f, 0.80f);
            style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.20f, 0.25f, 0.30f, 0.60f);
            style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.55f, 0.53f, 0.55f, 0.51f);
            style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.56f, 1.00f);
            style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.91f);
            style.Colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.90f, 0.90f, 0.83f);
            style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.70f, 0.70f, 0.70f, 0.62f);
            style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.30f, 0.30f, 0.30f, 0.84f);
            style.Colors[ImGuiCol_Button] = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);
            style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.50f, 0.69f, 0.99f, 0.68f);
            style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.80f, 0.50f, 0.50f, 1.00f);
            style.Colors[ImGuiCol_Header] = ImVec4(0.30f, 0.69f, 1.00f, 0.53f);
            style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.44f, 0.61f, 0.86f, 1.00f);
            style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.38f, 0.62f, 0.83f, 1.00f);
            style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.85f);
            style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.60f);
            style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.90f);
            style.Colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
            style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
            style.FrameBorderSize = 1;
        }
        if (!bDarkStyle) {
            style.WindowRounding = 5.3f;
            style.FrameRounding = 2.3f;
            style.ScrollbarRounding = 0;
            style.Colors[ImGuiCol_Text] = ImVec4(0.20f, 1.0f, 0.40f, 1.0f);
            style.Colors[ImGuiCol_WindowBg] = ImVec4(1.01f, 1.01f, 1.02f, 1.0f);
            style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
            style.Colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.10f, 0.85f);
            style.Colors[ImGuiCol_Border] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
            style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            style.Colors[ImGuiCol_FrameBg] = ImVec4(0.90f, 0.90f, 0.90f, 0.90f);
            style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.90f, 0.80f, 0.80f, 0.40f);
            style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.90f, 0.65f, 0.65f, 0.45f);
            style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.40f, 0.40f, 0.40f, 0.80f);
            style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.01f, 0.01f, 0.02f, 0.80f);
            style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.20f, 0.25f, 0.30f, 0.60f);
            style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.55f, 0.53f, 0.55f, 0.51f);
            style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.56f, 1.00f);
            style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.91f);
            style.Colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.90f, 0.90f, 0.83f);
            style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.70f, 0.70f, 0.70f, 0.62f);
            style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.30f, 0.30f, 0.30f, 0.84f);
            style.Colors[ImGuiCol_Button] = ImVec4(1.01f, 1.01f, 1.02f, 1.0f);
            style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.50f, 0.69f, 0.99f, 0.68f);
            style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.80f, 0.50f, 0.50f, 1.00f);
            style.Colors[ImGuiCol_Header] = ImVec4(0.30f, 0.69f, 1.00f, 0.53f);
            style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.44f, 0.61f, 0.86f, 1.00f);
            style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.38f, 0.62f, 0.83f, 1.00f);
            style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.85f);
            style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.60f);
            style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.90f);
            style.Colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
            style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
            style.FrameBorderSize = 1;

        }
        ImGui::SetWindowSize(ImVec2(500.0f, 600.0f));
        ImGui::SetWindowPos(ImVec2(2.0f, 2.0f));
        if (GetKeyState('T') > 0) {
            bDarkStyle = true;
            str_theme = "Dark";
        }
        if (GetKeyState('T') < 0) {
            bDarkStyle = false;
            str_theme = "Light";
            //ImGui::SaveIniSettingsToDisk("imgui.ini");
           // ConfigSave(bDarkStyle, "dark=true");
        }
        if (GetKeyState('C') > 0) {
            ImGui::TextColored(ImVec4(0.20f, 1.0f, 0.40f, 1.0f), ((std::string)CPUBrandString +": fps_count:" + std::to_string(io.Framerate)).c_str());
        }
        ImGui::TextColored(ImVec4(0.20f, 1.0f, 0.40f, 1.0f), "[C] - units info , [T] - Theme");
        if (GetAsyncKeyState(VK_ESCAPE)) {
            bWindow1 = true;
        }
        // ImGui::End();
        // ImGui::Checkbox("Show Console", &v_conState);
        if (fLimitCPUUsage) {
            Sleep(13);
        }

        // ImGui::BeginMenuBar()
        // ImGui::EndMenuBar();
         //ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
         //ImGui::Text((str_theme).c_str());]
        CurrentMemoryBuffer.ClearFreeMemory();
        if (bWindow0) {
            ImGui::Begin("window_0", &bWindow0, ImGuiWindowFlags_NoTitleBar);
            if (ImGui::Button("X", ImVec2(32.0f, 32.0f))) { bWindow0 = false; }
            ImGui::Text("Калькулятор элктроника v 1.0");
            ImGui::Text("-------------------------------");
            ImGui::Text("Спасибо что используете мое приложение!!");
            ImGui::Text("[ESC] - Выйти из Приложения");
            ImGui::Text("[T] - Сменить Тему (Темная, Светлая)");
            if (ImGui::Button("OK")) {
                bWindow0 = false;
            }
            ImGui::End();
        }
        if (bWindow1) {//
            ImGui::Begin("window_0", &bWindow1, ImGuiWindowFlags_NoTitleBar);
            if (ImGui::Button("X", ImVec2(32.0f, 32.0f))) { bWindow1 = false; }
            ImGui::Text("Калькулятор элктроника v 1.0");
            ImGui::Text("-------------------------------");
            ImGui::Text("           Вы точно хотите выйти?");
            ImGui::Text("\t\t\t");
            ImGui::SameLine();
            if (ImGui::Button("ДА", ImVec2(32.f, 32.0f)))
            {
                //
                exit(0);
            }
            ImGui::SameLine();
            if (ImGui::Button("Отмена", ImVec2(64.0f, 32.0f)))
            {
                //
                bWindow1 = false;
            }
            ImGui::End();
        }
        if (ImGui::Button("Ватты в Амперы", ImVec2(150.f, 50.0f)))
        {
            //
            fFrameWA = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Расчет ОМА", ImVec2(100.f, 50.0f)))
        {
            //
            fFrameOM = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Расчет диаметра провода", ImVec2(200.f, 50.0f)))
        {
            //
            fFrameDIA = true;
        }
        if (ImGui::Button("P Тена", ImVec2(150.f, 50.0f)))
        {
            //
            fFramePH = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Расчет Номинала Автомата", ImVec2(300.f, 50.0f)))
        {
            fNautomat = true;
        }
        if (ImGui::Button("Расчет Фазосдвигающего\nКонденсатора", ImVec2(300.f, 50.0f)))
        {
            fC_phase = true;
        }
        if (fC_phase) {
            fFrameOM = false;
            fFrameDIA = false;
            fFrameWA = false;
            fFramePH = false;
            fTable = false;
            fNautomat = false;
            if (f4800Offset) {
                fp = 0; fOval = 4800;
            }
            if (f2800Offset) {
                fp = 1; fOval = 2800;
            }
            //
            std::string fStrOffsetValue[] = { "Треугольник","Звезда" };
            //
            ImGui::Begin("Расчет Фазосдвигающего Конденсатора", &fC_phase);
            ImGui::TextColored(ImVec4(0.20f, 1.0f, 0.40f, 1.0f), (std::to_string(io.Framerate)).c_str());
            CurrentMemoryBuffer.ClearFreeMemory();
            if (ImGui::Button("X")) { fC_phase = false; }
            ImGui::SetWindowSize(ImVec2(500.0f, 600.0f));
            ImGui::SetWindowPos(ImVec2(2.0f, 2.0f));
            if (ImGui::Button("Треугольник", ImVec2(200.f, 50.0f))) {
                f2800Offset = false;
                f4800Offset = true;
                ///fCOffset = std::to_string(fOval * fAbuffer / fVbuffer) + "mKf";
            }
            ImGui::SameLine();
            if (ImGui::Button("Звезда", ImVec2(200.f, 50.0f))) {
                f2800Offset = true;
                f4800Offset = false;
                //fCOffset = std::to_string(fOval * fAbuffer / fVbuffer) + "mKf";
            }
            ImGui::Text(("Тип:" + fStrOffsetValue[fp]).c_str());
            ImGui::Text(("C:" + fCOffset).c_str());
            ImGui::InputInt("A мотора:", &fAbuffer, 1, 100, 1);
            ImGui::InputInt("V мотора:", &fVbuffer, 1, 100, 1);
            if (fVbuffer < 1) {
                ImGui::Text("Ошибка: V не может быть равна 0!!");
            }
            if (fVbuffer > 0) {
                if (ImGui::Button("Расчитать", ImVec2(300.f, 50.0f))) {
                    fCOffset = std::to_string(fOval * fAbuffer / fVbuffer) + "мкф";
                }
            }
            //   fC_value.innerHTML = parseFloat(fMtype * fDI / fDV) + " ���";
            ImGui::End();
        }
        std::string fStrNominal = "\n1.13 — ток кратковременного отключения\n цепи\n1.45 — ток отключения\n, при котором машина не должна выключаться";
        std::string fStrNominal1 = "\n2.55 - ток, при котором\n машина должна выключиться в течение 1 минуты.\n5 - ток аварийной перегрузки,\n при которой машина должна выключиться в течение 10 секунд \n10 - ток аварийной перегрузки, при которой\n машина должна выключиться выключать";
        int32_t fNin[] = { 13,45,155,400,900 };
        if (fNautomat) {
            fFrameOM = false;
            fFrameDIA = false;
            fFrameWA = false;
            fFramePH = false;
            fTable = false;
            //
            //
            ImGui::Begin("Расчет номинала автомата (Идея DIMA XP)", &fNautomat);
            ImGui::TextColored(ImVec4(0.20f, 1.0f, 0.40f, 1.0f), (": fps_count:" + std::to_string(io.Framerate)).c_str());
            if (fNin_0) { fNinOffset = fNin[0]; }
            if (fNin_1) { fNinOffset = fNin[1]; }
            if (fNin_2) { fNinOffset = fNin[2]; }
            if (fNin_3) { fNinOffset = fNin[3]; }
            if (fNin_4) { fNinOffset = fNin[4]; }
            ImGui::SetWindowSize(ImVec2(500.0f, 600.0f));
            ImGui::SetWindowPos(ImVec2(2.0f, 2.0f));
            if (ImGui::Button("X")) { fNautomat = false; }
            // parseInt(fNdata) + (parseInt(fNdata) * (fProcentArray[fCPoint] / 100));
            ImGui::Text(("A:" + std::to_string(rs)).c_str());
            if (ImGui::Button("1.13", ImVec2(64.0f, 64.0f))) { fNinOffset = fNin[0]; }
            ImGui::SameLine();
            if (ImGui::Button("1.45", ImVec2(64.0f, 64.0f))) { fNinOffset = fNin[1]; }
            ImGui::SameLine();
            if (ImGui::Button("2.55", ImVec2(64.0f, 64.0f))) { fNinOffset = fNin[2]; }
            ImGui::SameLine();
            if (ImGui::Button("5", ImVec2(64.0f, 64.0f))) { fNinOffset = fNin[3]; }
            ImGui::SameLine();
            if (ImGui::Button("10", ImVec2(64.0f, 64.0f))) { fNinOffset = fNin[4]; }
            ImGui::InputInt("Номинал Автомата А:", &fN, 1, 100, 0);
            ImGui::Text(("Выбрано:" + std::to_string(fNinOffset) + "Введено:" + std::to_string(fN)).c_str());
            ImGui::TextUnformatted((fStrNominal).c_str());
            ImGui::TextUnformatted((fStrNominal1).c_str());
            if (ImGui::Button("Расчитать", ImVec2(150.0f, 30.0f))) {
                rs = ((float)fNinOffset / 100) * fN + fN;
                fStrNominal = std::to_string(rs);
            }

            ImGui::SameLine();
            if (ImGui::Button("Показать таблицы сечении", ImVec2(150.0f, 30.0f))) {
                fFrameOM = false;
                fFrameDIA = false;
                fFrameWA = false;
                fFramePH = false;
                fNautomat = false;
                fTable = true;
            }
            ImGui::End();
        }
        //
        int32_t amperArrayCp[] = { 11,15,17,23,26,30,41,50,80,100,140,170 };
        int32_t amperArrayAl[] = { 0,0,0,0,21,24,32,39,60,75,105,130 };
        float WattArrayCp[] = { 2.4f,3.3f,3.7f,5.0f,5.7f,6.6f,9.0f,11.0f,17.0f,22.0f,30.0f,37.0f };
        float WattArrayAl[] = { 0.0f,0.0f,6.4f,8.7f,9.8f,11.0f,15.0f,19.0f,30.0f,38.0f,53.0f,64.0f };
        float SectionArray[] = { 0.5f,0.75f,1.0f,1.5f,2.0f,2.5f,4.0f,6.0f,10.0f,16.0f,25.0f,35.0f };
        ImGui::Text("Created by HCPP 2024");
        if (fTable) {
            if (ImGui::Button("X")) { fTable = false; }
            static ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
            const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
            if (ImGui::BeginTable("table2", 6, flags))
            {
                ImGui::TableSetupColumn("Медь", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("A", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("S", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableSetupColumn("Алюминий", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("A", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("V", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();
                for (int row = 0; row < 12; row++)
                {
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text((std::to_string(WattArrayCp[row]) + "kW").c_str());
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text((std::to_string(amperArrayCp[row]) + "A").c_str());
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text((std::to_string(SectionArray[row]) + "mm").c_str());
                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text((std::to_string(WattArrayAl[row]) + "kW").c_str());
                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text((std::to_string(amperArrayAl[row]) + "A").c_str());
                    // ImGui::Text(std::to_string(amperArrayAl[row]).c_str());

                }
                ImGui::EndTable();
            }
        }
        if (fFramePH) {
            fFrameOM = false;
            fFrameDIA = false;
            fFrameWA = false;
            static float fWatt = 0;
            static float fR = 0;
            static float fVolt = 0;
            static float fValue = 0;
            std::string data_f;
            std::string lvl_0;
            data_f = lvl_0 + std::to_string(fValue);
            ImGui::Begin("Расчет мощности тена", &fFramePH);
            ImGui::TextColored(ImVec4(0.20f, 1.0f, 0.40f, 1.0f), (": fps_count:" + std::to_string(io.Framerate)).c_str());
            ImGui::SetWindowSize(ImVec2(500.0f, 600.0f));
            ImGui::SetWindowPos(ImVec2(2.0f, 2.0f));
            if (ImGui::Button("X")) { fFramePH = false; }
            ImGui::Text((data_f).c_str());
            ImGui::InputFloat("V:", &fVolt, 1, 1000, 0);
            ImGui::InputFloat("R:", &fR, 1, 1000, 0);
            //(fPV * fPV / fPR);
            if (ImGui::Button("Расчитать", ImVec2(130.0f, 50.0f)))
            {
                lvl_0 = "W:";
                fValue = (fVolt * fVolt) / fR;

            }
            ImGui::End();
        }
        if (fFrameWA)
        {
            fFrameOM = false;
            fFrameDIA = false;
            static float fWatt = 0;
            static bool v_state = false;
            static float fVoltageW = 0;
            static float fAmp = 0;
            static float fValue = 0;
            std::string data_f;
            std::string lvl_0;
            data_f = lvl_0 + std::to_string(fValue);
            ImGui::Begin("Ватты в Ампера и наоборот", &fFrameWA);
            ImGui::TextColored(ImVec4(0.20f, 1.0f, 0.40f, 1.0f), (": fps_count:" + std::to_string(io.Framerate)).c_str());
            ImGui::SetWindowSize(ImVec2(500.0f, 600.0f));
            ImGui::SetWindowPos(ImVec2(2.0f, 2.0f));
            if (ImGui::Button("X")) { fFrameWA = false; }
            //ImGui::Text("Watt");
            ImGui::Checkbox("Наоборот", &v_state);
            ImGui::TextColored(ImVec4(150.f, 150.0f, 255.0f, 1.0f), (data_f).c_str());
            ImGui::InputFloat("Вольтаж", &fVoltageW, 1, 1000, 0);
            if (v_state)
            {
                ImGui::InputFloat("Ватты", &fWatt, 1, 1000, 0);
                if (fWatt > 0 && fVoltageW > 0)
                {
                    if (ImGui::Button("Расчитать", ImVec2(130.0f, 50.0f)))
                    {
                        lvl_0 = "Watt";
                        fValue = fWatt / fVoltageW;

                    }
                }
            }
            if (!v_state)
            {
                ImGui::InputFloat("Амперы", &fAmp, 1, 1000, 0);
                if (ImGui::Button("Расчитать", ImVec2(130.0f, 50.0f)))
                {
                    lvl_0 = "Амперы";
                    fValue = fVoltageW * fAmp;
                }
            }
            ImGui::End();
        }
        if (fFrameOM)
        {
            fFrameWA = false;
            fFrameDIA = false;
            static float fResistor = 0;
            static float fVoltage = 0;
            static float fAmper = 0;
            ImGui::Begin("Расчет Ампер по ОМУ", &fFrameOM);
            ImGui::TextColored(ImVec4(0.20f, 1.0f, 0.40f, 1.0f), (": fps_count:" + std::to_string(io.Framerate)).c_str());
            ImGui::SetWindowSize(ImVec2(500.0f, 600.0f));
            ImGui::SetWindowPos(ImVec2(2.0f, 2.0f));
            if (ImGui::Button("X")) { fFrameOM = false; }
            ImGui::Text(("Амперы:" + std::to_string(fAmper)).c_str());
            ImGui::InputFloat("Вольтаж", &fVoltage, 1, 1000, 0);
            ImGui::InputFloat("Сопротивление", &fResistor, 1, 1000, 0);
            if (ImGui::Button("Расчитать", ImVec2(150.0f, 60.f)))
            {
                fAmper = fVoltage / fResistor;
            }
            ImGui::End();
        }
        if (fFrameDIA)
        {
            fFrameOM = false;
            fFrameWA = false;
            float fPi = 3.14f;
            float fdata_0 = 0.0f;
            static float fR_mm2 = 0.0f;
            ImGui::Begin("Расчет сечения из диаметра", &fFrameDIA);
            ImGui::TextColored(ImVec4(0.20f, 1.0f, 0.40f, 1.0f), (": fps_count:" + std::to_string(io.Framerate)).c_str());
            ImGui::SetWindowSize(ImVec2(500.0f, 600.0f));
            ImGui::SetWindowPos(ImVec2(2.0f, 2.0f));
            if (ImGui::Button("X")) { fFrameDIA = false; }
            ImGui::Text(("Сечение:" + std::to_string(fR_mm2)).c_str());
            static float fDia_value = 0.0f;
            ImGui::InputFloat("Диаметр", &fDia_value, 0.0f, 100000.0f, 0);
            if (ImGui::Button("Расчитать", ImVec2(150.0f, 60.f)))
            {
                // fDA = (fD0 / 2) * (fD0 / 2) * fPi;
                // fdata_0 = (fDA * fDA) * fPi;
                fR_mm2 = ((fDia_value / 2) * (fDia_value / 2) * fPi);
            }
            ImGui::End();
        }
            ImGui::End();
        // Rendering
        ImGui::EndFrame();
        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(clear_color.x*clear_color.w*255.0f), (int)(clear_color.y*clear_color.w*255.0f), (int)(clear_color.z*clear_color.w*255.0f), (int)(clear_color.w*255.0f));
        g_pd3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
        if (g_pd3dDevice->BeginScene() >= 0)
        {
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_pd3dDevice->EndScene();
        }
        HRESULT result = g_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);
        if (result == D3DERR_DEVICELOST)
            g_DeviceLost = true;
    }

    // Cleanup
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == nullptr)
        return false;

    // Create the D3DDevice
    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN; // Need to use an explicit format with alpha if needing per-pixel alpha composition.
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;           // Present with vsync
    //g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
    if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
        return false;

    return true;
}

void CleanupDeviceD3D()
{
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
    if (g_pD3D) { g_pD3D->Release(); g_pD3D = nullptr; }
}

void ResetDevice()
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
    if (hr == D3DERR_INVALIDCALL)
        IM_ASSERT(0);
    ImGui_ImplDX9_CreateDeviceObjects();
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
