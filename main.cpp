#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_win32.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <GL/GL.h>
#include <tchar.h>
#include <string>
//#include "JoyStick.h"
#include <iostream>
#include <fstream>
#include <string>
#include <Psapi.h>
#include <clocale>
#include <stdint.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "dSys.h"
#include "imgui_stdlib.h"
#include "imgui_internal.h"
#include <vector>
#include <codecvt>
//#include "Colors.h"
///
ImVec4 RGBA(float r, float g, float b, float a);
#pragma execution_character_set("utf-8")
// Data stored per platform window
struct WGL_WindowData { HDC hDC; };

// Data
static HGLRC            g_hRC;
static WGL_WindowData   g_MainWindow;
static int              g_Width;
static int              g_Height;

// Forward declarations of helper functions
bool CreateDeviceWGL(HWND hWnd, WGL_WindowData* data);
void CleanupDeviceWGL(HWND hWnd, WGL_WindowData* data);
static bool fFrameWA = false;
static bool fFrameOM = false;
static bool fFrameDIA = false;
static bool fTable = false;
static bool fNautomat = false;
static bool fC_phase = false;
static bool fFramePH = false;//(fPV * fPV / fPR);
static bool fC_balast = false;
static bool fC_Idevice = false;
static bool fFrameVoltageDrop = false;
static bool fFramePowerCos = false;
static bool bCalcSAW = false;
int64_t* fA_stack = 0;
static bool fCAboutW;
int fN = 1;
float rs = 0;
namespace ImGui {
    bool CustomToggle(bool th ,const char* label, bool* v) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        ImGui::PushID(label);ImGui::TextColored((*v ? (th ? ImVec4(1.0f, 1.0f, 1.0f, 1.00f) : ImVec4(0.0f, 0.0f, 0.0f, 1.00f)) : (th ? ImVec4(0.65f, 0.65f, 0.65f, 1.00f): ImVec4(0.30f, 0.30f, 0.30f, 1.00f))), label);
        ImGui::SameLine();
        float height = ImGui::GetFrameHeight();
        float width = height * 1.55f;
        ImGuiStyle& style = ImGui::GetStyle();

        ImVec2 p = ImGui::GetCursorScreenPos();

        // Colors
        ImU32 color_bg_on = ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImU32 color_bg_off = ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImU32 color_knob = *v ? ImGui::GetColorU32(ImVec4(0.34f, 0.34f, 0.44f, 1.00f)) :
            ImGui::GetColorU32(ImVec4(0.34f, 0.34f, 0.44f, 1.00f));
        ImU32 glow_color = ImGui::GetColorU32(ImVec4(0.34f, 0.34f, 0.44f, 0.00f)); // Цвет свечения
        ImU32 border_color = ImGui::GetColorU32(ImGuiCol_Border);

        ImGui::InvisibleButton(label, ImVec2(width, height));
        bool clicked = ImGui::IsItemClicked();
        if (clicked) *v = !(*v);

        // Background + Border
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), *v ? color_bg_on : color_bg_off, height);

        if (style.FrameBorderSize > 0.0f) {
            draw_list->AddRect(
                p,
                ImVec2(p.x + width, p.y + height),
                border_color,
                height * 0.5f,
                0,
                style.FrameBorderSize
            );
        }

        // Knob parameters
        float base_radius = *v ? 10 : 8;
        ImVec2 knob_pos = *v ?
            ImVec2(p.x + width - height * 0.5f, p.y + height * 0.5f) :
            ImVec2(p.x + height * 0.5f, p.y + height * 0.5f);

        // Glow effect (only when active)
        if (*v) {
            // Рисуем несколько слоев для эффекта свечения
            const int glow_layers = 3;
            for (int i = 0; i < glow_layers; i++) {
                float alpha = 0.3f * (1.0f - (float)i / glow_layers);
                float radius = base_radius + 2.0f * (i + 1);
                draw_list->AddCircle(
                    knob_pos,
                    radius,
                    ImGui::GetColorU32(ImVec4(0.44f, 0.46f, 0.48f, alpha)),
                    0, // сегменты (0 = автоматически)
                    2.0f // толщина линии
                );
            }
        }

        // Main knob
        draw_list->AddCircleFilled(knob_pos, base_radius, color_knob);

        ImGui::PopID();
        return clicked;
    }
    bool SliderInt64(const char* label, int64_t* v, int64_t v_min, int64_t v_max, const char* format, ImGuiSliderFlags flags)
    {
        return SliderScalar(label, ImGuiDataType_S64, v, &v_min, &v_max, format, flags);
    }
    bool InputInt64(const char* label, int64_t* v, int64_t step, int64_t step_fast, ImGuiInputTextFlags flags)
    {
        // Hexadecimal input provided as a convenience but the flag name is awkward. Typically you'd use InputText() to parse your own data, if you want to handle prefixes.
        const char* format = (flags & ImGuiInputTextFlags_CharsHexadecimal) ? "%08X" : "%i";
        return InputScalar(label, ImGuiDataType_S64, (void*)v, (void*)(step > 0 ? &step : NULL), (void*)(step_fast > 0 ? &step_fast : NULL), format, flags);
    }
}
//
bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height, unsigned char* imgBuffer) {
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
    if (!image_data)
    {
        return false;
    }
    else {
        *imgBuffer = *image_data;
        GLuint image_texture;
        glGenTextures(1, &image_texture);
        glBindTexture(GL_TEXTURE_2D, image_texture);

        // Setup filtering parameters for display
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
        stbi_image_free(image_data);

        *out_texture = image_texture;
        *out_width = image_width;
        *out_height = image_height;

        glBindTexture(GL_TEXTURE_2D, 0);
    }
   
}//
std::string WideStringToUTF8(const std::wstring& wstr) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.to_bytes(wstr);
}
void ResetDeviceWGL();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
int64_t ConfigSave(bool booleanSetItem, std::string nameParam) {
    int64_t countCall = 0;
    std::string lastBuffer = nameParam;
    std::ofstream config("config.txt");
    config.is_open();
    countCall++;
    if (countCall >= countCall)
    {
        nameParam += +"\n" + nameParam;
    }
    if (lastBuffer == nameParam) {
        nameParam = "";
        ImGui::SetCursorPosX(5);ImGui::Text("Error: Set0 to set0!=");
    }
    config << nameParam << std::endl;

}
uint64_t maxU64(uint64_t a, uint64_t b) {
    if (b > a) {
        return b;
    }
    else {
        return a;
    }
}
uint64_t fDataMemUsage() // Work Function !!! Check Sym RAM to Current Program //
{
    PROCESS_MEMORY_COUNTERS pmc;
    pmc.cb = sizeof(pmc);
    GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
    return pmc.WorkingSetSize;
}
int64_t getValidatorAPIKey(int64_t v) {
   int64_t vout =  v >> 64;
    return vout;
}
int64_t ReadConfig(bool* booleanSetItem, std::string nameParam) {

    std::ifstream CurrentConfig("config.txt");
    std::string bufferOffset;
    CurrentConfig.is_open();
    while (std::getline(CurrentConfig, bufferOffset)) {
    }
    if (bufferOffset == nameParam) {
        *booleanSetItem = true;
    }
    else {
        *booleanSetItem = false;
    }
    return *booleanSetItem;
}
std::string BUILD_VER = "1.1.3";
std::string BACKEND_API = "OpenGL3.3";
///
ImVec2 HwndWSizeA(HWND HwNd)
{
    RECT fSizeWindow;
    GetWindowRect(HwNd, &fSizeWindow);
    int64_t fWctx = fSizeWindow.right - fSizeWindow.left;
    int64_t fWcty = fSizeWindow.bottom - fSizeWindow.top;
    return ImVec2(
        static_cast<float>(fWctx), static_cast<float>(fWcty)
    );
}
static void DarkStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    style.Colors[ImGuiCol_WindowBg] = RGBA(7, 7, 9, 255);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.10f, 1.0f);
    style.Colors[ImGuiCol_Border] = RGBA(30, 30, 40, 255);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 0.00f, 0.50f, 0.00f);
    style.Colors[ImGuiCol_FrameBg] = RGBA(10, 10, 40, 10);
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
    style.Colors[ImGuiCol_Button] =  RGBA(30, 30, 40, 255);
    style.Colors[ImGuiCol_ButtonHovered] = RGBA(50, 50, 80, 255);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(1.0f, 0.0f, .50f, 1.0f);
    style.Colors[ImGuiCol_Header] = RGBA(30, 30, 40, 255);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.44f, 0.61f, 0.86f, 1.00f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.38f, 0.62f, 0.83f, 1.00f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.85f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.00f, 1.00f, 1.00f, 0.60f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.90f);
    style.Colors[ImGuiCol_PlotLines] = RGBA(30, 30, 40, 255);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.07f, 0.07f, 0.08f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(1.0f, 0.0f, 0.50f, 1.00f);//1234f5
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0, 0, 0, 1);
    style.Colors[ImGuiCol_Separator] = RGBA(30, 30, 40, 255);
    style.FrameBorderSize = 1;
}
ImVec4 RGBA(float r, float g, float b, float a) {
    int64_t i64MAXColorValue = 255;
    ImVec4 outV2 = {
        r / static_cast<float>(i64MAXColorValue),
        g / static_cast<float>(i64MAXColorValue),
        b / static_cast<float>(i64MAXColorValue),
        a / static_cast<float>(i64MAXColorValue)
    };
    return outV2;
}
void SetCenterToX(ImVec2 size) {
    float wsize = ImGui::GetWindowWidth();
    float wsizeItem = size.x;
    float fOutPos = (wsize - wsizeItem) / 2;
    ImGui::SetCursorPosX(fOutPos);
}
//
std::string title, text;
bool isOK;

//
void ELCMessageBox(std::string title, std::string text, bool *bOK = false) {
    if (ImGui::BeginPopup("elc_messageBox", ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text(title.c_str());
        ImGui::Separator();
        ImGui::Text(text.c_str());
        if (ImGui::Button("OK", ImVec2(100, 30))) {
            *bOK = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Отмена", ImVec2(100, 30))) {
            *bOK = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
     
}
int main(int argc, char** argv)
{
    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = { sizeof(wc), CS_OWNDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"Electrik Calc", nullptr };
    wc.hIcon = LoadIcon(wc.hInstance, MAKEINTRESOURCE(101));
    ::RegisterClassExW(&wc);

    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Калькулятор электроника v 1.1.4 (OGL3)", WS_OVERLAPPEDWINDOW | WS_OVERLAPPEDWINDOW, 100, 100, 530, 700, nullptr, nullptr, wc.hInstance, nullptr);
    // Initialize OpenGL
    
    ::SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & ~WS_SIZEBOX);
    if (!CreateDeviceWGL(hwnd, &g_MainWindow))
    {
        CleanupDeviceWGL(hwnd, &g_MainWindow);
        ::DestroyWindow(hwnd);
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }
    wglMakeCurrent(g_MainWindow.hDC, g_hRC);

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;    // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_InitForOpenGL(hwnd);
    ImGui_ImplOpenGL3_Init();
    ImVec4 clear_color = ImVec4(0.00f, 0.00f, 0.00f, 0.83f);
    bool fFr_main = true;
    static bool bFontSelector;
    //window_flags |= ImGuiWindowFlags_NoTitleBar;
    //\\io.Fonts->AddFontDefault();
    ImFontAtlas* fonts = ImGui::GetIO().Fonts;
    fonts->AddFontFromFileTTF("SFMono-Bold.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    ImFont* font1 = io.Fonts->AddFontFromFileTTF("SFMono-Bold.ttf", 22.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    ImFont* font2 = io.Fonts->AddFontFromFileTTF("SFMono-Bold.ttf", 20.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    ImFont* font3 = io.Fonts->AddFontFromFileTTF("SFMono-Bold.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    ImFont* font4 = io.Fonts->AddFontFromFileTTF("SFMono-Bold.ttf", 14.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    //
    ImFontAtlas* fonts2 = ImGui::GetIO().Fonts;
    fonts->AddFontFromFileTTF("BOUNDED.ttf", 20.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    ImFont* font12 = io.Fonts->AddFontFromFileTTF("BOUNDED.ttf", 22.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    ImFont* font22 = io.Fonts->AddFontFromFileTTF("BOUNDED.ttf", 20.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    ImFont* font32 = io.Fonts->AddFontFromFileTTF("BOUNDED.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    ImFont* font42 = io.Fonts->AddFontFromFileTTF("BOUNDED.ttf", 14.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    ImFontAtlas* fontsBuffer = ImGui::GetIO().Fonts;
    ImFont* Font0Buffer = 0;
    ImFont* Font1Buffer = 0;
    ImFont* Font2Buffer = 0;
    ImFont* Font3Buffer = 0;
   
    //glyphRangesBuilder.AddRanges(getLangGlyphRanges());
//io.AddInputCharacter();
// Main loop
    bool done = false;
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
    int64_t fNinOffset = 13;//
    int64_t fStyleCount = 0;
    static bool f4800Offset = false;
    static bool f2800Offset = false;
    static bool fELCButtonsState = false;
    static bool ELC_LaunchBool = true;
    static bool g_bCalcWpola = false;
    //
    static int getAddrValue = 0;
    static int getAddrValue2 = 0;
    float fU2 = 0.0f;
    float fU1 = 0.0f;
    float fCoutData = 0.0f;
    int fIdata = 0;
    //
 
    //

    bool bDarkStyle = false ;
    bool bT_switch = true;
    bool bT_switch1 = false;
    //
    std::string str_theme = "Dark";
    HWND fH_con = GetConsoleWindow();
    ShowWindow(fH_con, 0);
    ImDrawListSplitter CurrentMemoryBuffer;
    static int64_t main_stack;
    struct GPU_DATA {
        std::string E_Brand = (const char*)glGetString(GL_VENDOR);
        std::string E_Model = (const char*)glGetString(GL_RENDERER);
        std::string E_GLVer = (const char*)glGetString(GL_VERSION);
        std::string E_extention = (const char*)glGetString(GL_EXTENSIONS);
    };
    GPU_DATA GPU;
    std::string fD_gpuBrand = GPU.E_Brand;
    std::string fD_gpuModel = GPU.E_Model;
    std::string fD_gpuGLVer = GPU.E_GLVer;
    std::string fD_extention = GPU.E_extention;
    //std::cout << "GPU: Unit" << std::endl;
    // SetColorAMD64(240);
    std::cout << "" << std::endl;
    std::cout << "" << "GL_VENDOR: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "" << "GL_RENDERER: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "" << "GL_VERSION: " << glGetString(GL_VERSION) << std::endl;
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
    GLuint GLArrayBuffer,GLArrayBuffer2;
    int Ix = 128; int Iy = 128; int Ix2 = 128; int Iy2 = 128;
    unsigned char imgBuffer, imgBuffer2;
    bool load,load2;
    if (!LoadTextureFromFile("HCPP_STUDIO_LOGO.png", &GLArrayBuffer, &Ix, &Iy, &imgBuffer)) {
        MessageBoxA(hwnd, std::string("::Electric Calc.exe::\nPlease support to tg: @hcppstudio\ncode: 0x00E4\n[opengl] Fatal: Error file HCPP_STUDIO_LOGO.png occurred and not found![file:" + std::string(__FILE__)+"->line:" + std::to_string(__LINE__)).c_str(), "ELC::Errors", 0);
    }


    if (!LoadTextureFromFile("logo.png", &GLArrayBuffer2, &Ix2, &Iy2, &imgBuffer2)) {
        MessageBoxA(hwnd, "[opengl] Fatal: Error file logo.png occurred and not found!", "ELC::Errors", 0);
    }

    
    static int64_t i64buf[] = {
        88,
        65,
        55,
        55
    };
    bool fC_Dvolt = false;
    static bool bFlag0 = false;
    static bool bFlag1 = false;
    std::string configLine;
    int64_t parseCountLines = 0;
    std::ifstream elcConfigLoad("config.txt");
    std::cout << "Read Config.." << std::endl;
    static bool fSwitchFont = false;
    if (elcConfigLoad.is_open()) {
        std::cout << "Config Opened!" << std::endl;
        while (getline(elcConfigLoad, configLine)) {
            parseCountLines++;
            if (configLine == "elcTheme=dark_theme") {
                bT_switch = true;

            }
            if (configLine == "elcFont=SFMono_Bold") {
                fSwitchFont = true;
            }
            if (configLine == "elc-20334")
            {
                std::cout << "Activated!!"<< std::endl;
                bFlag0 = true;
                bFlag1 = bFlag0;
            }
        }
        std::cout << "ConfigParams:" << parseCountLines << std::endl;
        std::cout << "Params:\n" << configLine << std::endl;
        std::cout << "Font::[State: " << fSwitchFont << ", Name:" << (fSwitchFont ? "SFMono-Bold" : "PixelizerBold ] ") << std::endl;
    }
    else {
        std::cout << "Error Read Config!! config.txt empty and not found!" << std::endl;
        bT_switch = true;
        bFontSelector = false;
    }
    //
    int64_t i64MaxFPS = 0;
    //
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
        bool v_titleS = true;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        i64MaxFPS = 1000;
       // std::cout << "GPU: Unit OK" << std::endl;
        ImGui::Begin("Калькулятор электроника v 1.1.3 OpenGL3.3", &fFr_main, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
        i64MaxFPS = i64MaxFPS;
        //std::cout << "GPU: Render Frame:"<<io.Framerate << std::endl;
        if (bFontSelector) {
            Font0Buffer = font1;
            Font1Buffer = font2;
            Font2Buffer = font3;
            Font3Buffer = font4;
        }
        else {
            Font0Buffer = font12;
            Font1Buffer = font22;
            Font2Buffer = font32;
            Font3Buffer = font42;
        }
        ImGuiStyle& style = ImGui::GetStyle();
        bFontSelector = (fSwitchFont ? true : false);
        //bDarkStyle = (GetKeyState('S') > 0) ? true : false;
        if (!bDarkStyle) {
            DarkStyle();
        }
        if (bDarkStyle) {
            ImGuiStyle& style = ImGui::GetStyle();
            ImVec4* colors = ImGui::GetStyle().Colors;

            // Основные цвета
            colors[ImGuiCol_Text] = ImVec4(0.0f, 0.0f, 0.0f, 1.00f); // Черный текст
            colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
            colors[ImGuiCol_WindowBg] = RGBA(183,183,210,255); // Светло-серый фон
            colors[ImGuiCol_ChildBg] = RGBA(183, 183, 210, 255);
            colors[ImGuiCol_PopupBg] = RGBA(183, 183, 210, 255);

            // Рамки и границы
            colors[ImGuiCol_Border] = RGBA(7, 7, 9, 255);
            colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

            // Элементы управления
            colors[ImGuiCol_FrameBg] = RGBA(183, 183, 210, 255); // Белый фон
                colors[ImGuiCol_FrameBgHovered] = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);
            colors[ImGuiCol_FrameBgActive] = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);

            // Заголовки окон
            colors[ImGuiCol_TitleBg] = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);
            colors[ImGuiCol_TitleBgActive] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
            colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);

            // Меню и скроллбары
            colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
            colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
            colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
            colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
            colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);

            // Чекбоксы и слайдеры
            colors[ImGuiCol_CheckMark] = ImVec4(0.16f, 0.62f, 0.87f, 1.00f); // Синий акцент
            colors[ImGuiCol_SliderGrab] = ImVec4(0.16f, 0.62f, 0.87f, 0.78f);
            colors[ImGuiCol_SliderGrabActive] = ImVec4(0.20f, 0.70f, 1.00f, 1.00f);

            // Кнопки
            colors[ImGuiCol_Button] = RGBA(174, 174, 194, 255); // Синий акцент
            colors[ImGuiCol_ButtonHovered] = RGBA(183, 183, 250, 255);
            colors[ImGuiCol_ButtonActive] = RGBA(183, 183, 250, 255);

            // Заголовки секций
            colors[ImGuiCol_Header] = ImVec4(0.16f, 0.62f, 0.87f, 0.31f);
            colors[ImGuiCol_HeaderHovered] = ImVec4(0.16f, 0.62f, 0.87f, 0.80f);
            colors[ImGuiCol_HeaderActive] = ImVec4(0.16f, 0.62f, 0.87f, 1.00f);

            // Разделители
            colors[ImGuiCol_Separator] = RGBA(7, 7, 9, 255);
            colors[ImGuiCol_SeparatorHovered] = ImVec4(0.34f, 0.06f, 0.98f, 1.00f);
            colors[ImGuiCol_SeparatorActive] = ImVec4(0.34f, 0.06f, 0.98f, 1.00f);

            // Вкладки
            colors[ImGuiCol_Tab] = ImVec4(0.80f, 0.80f, 0.80f, 0.86f);
            colors[ImGuiCol_TabHovered] = ImVec4(0.16f, 0.62f, 0.87f, 0.80f);
            colors[ImGuiCol_TabActive] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);

            // Графики
            colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
            colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
            colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);

            // Таблицы
            colors[ImGuiCol_TableHeaderBg] = ImVec4(0.78f, 0.87f, 0.98f, 1.00f);
            colors[ImGuiCol_TableBorderStrong] = ImVec4(0.57f, 0.57f, 0.57f, 1.00f);
            style.FrameBorderSize = 1.0f;
        }
        ImGui::SetWindowSize(ImVec2(HwndWSizeA(hwnd).x - 20, HwndWSizeA(hwnd).y - 43));
        ImGui::SetWindowPos(ImVec2(2.0f, 2.0f));
        if (GetAsyncKeyState(VK_ESCAPE)) {
            bWindow1 = true;
        }
        // ImGui::End();
        // ImGui::Checkbox("Show Console", &v_conState);
        static int64_t i64latencyOffset = 13;
        Sleep((fLimitCPUUsage ? i64latencyOffset : 0));

        // ImGui::BeginMenuBar()
        // ImGui::EndMenuBar();
         //ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
         //ImGui::SetCursorPosX(5);ImGui::Text((str_theme).c_str());]
        ImGui::PushFont(Font2Buffer);
        bDarkStyle = (!bT_switch ? true : false);
        static std::string sDbgMtitle, sDbgMtext;
        static bool bDbgMPresedOK;
        CurrentMemoryBuffer.ClearFreeMemory();
        if (GetAsyncKeyState('H')) {
            ImGui::OpenPopup("debug");
        }
        if (GetAsyncKeyState('P')) {
            ImGui::OpenPopup("per_mode");
        }
        
        
        static std::string sProduct = "";
        static int64_t i64Offset_d = 0;
        if (ImGui::BeginPopup("debug")) {

            float fPmemoryUsage = static_cast<float>(fMemStatus(1)) / 100.0f;
            float fPmemoryUsageGB = (static_cast<float>(fMemStatus(7)) - static_cast<float>(fMemStatus(4))) / static_cast<float>(fMemStatus(7));
            float fPmemoryFreeGB = static_cast<float>(fMemStatus(4)) / (static_cast<float>(fMemStatus(7)));
            ImGui::SetCursorPosX(15); ImGui::Text("Debug");
            ImGui::Separator();
            ImGui::SetCursorPosX(25); ImGui::TextColored(ImVec4(1.0f, 0.10f, 0.40f, 1.0f), ((std::string)CPUBrandString + "\n" + fD_gpuModel + ": fps_count:" + std::to_string(io.Framerate) + "\nGL_VER:" + fD_gpuGLVer).c_str());
            //ImGui::TextColored(ImVec4(0.20f, 0.40f, 1.0f, 1.0f), ("[:: MEMORY_OFFSETS ::]\nmem_load: " + std::to_string((int64_t)fMemStatus(1)) + "%% | " + (std::to_string((int64_t)fMemStatus(7) - (int64_t)fMemStatus(4))) + ":GB\nmem_free: " + std::to_string((int64_t)fMemStatus(4)) + " :GB\nmem_total: " + std::to_string((int64_t)fMemStatus(7)) + " :GB").c_str());
            ImGui::SetCursorPosX(25); ImGui::Text("MemLoad:"); ImGui::SameLine(); ImGui::ProgressBar(fPmemoryUsage, ImVec2(200, 20), ("(" + std::to_string(static_cast<int64_t>(fMemStatus(1))) + "%) " + std::to_string((int64_t)(fMemStatus(7) - fMemStatus(4))) + " / " + std::to_string((int64_t)(fMemStatus(7))) + "GB").c_str());
            ImGui::SetCursorPosX(25); ImGui::Text("MemFree:"); ImGui::SameLine(); ImGui::ProgressBar(fPmemoryFreeGB, ImVec2(200, 20), (std::to_string((int64_t)(fMemStatus(4))) + "/" + std::to_string((int64_t)(fMemStatus(7))) + "GB").c_str());
            ImGui::SetCursorPosX(25); ImGui::Text("Framerate"); ImGui::SameLine(); ImGui::ProgressBar(io.Framerate / (float)i64MaxFPS, ImVec2(200, 20), (std::to_string(io.Framerate) + "/" + std::to_string(i64MaxFPS)).c_str());
            ImGui::SetCursorPosX(25); ImGui::CustomToggle(true, "Vsync/60hz", &fLimitCPUUsage);
            ImGui::SetCursorPosX(25); ImGui::Text("*** Внимание!!\nИзменение LatCPU сможет привести в лагам ***");
            ImGui::SetCursorPosX(25); ImGui::Text("*** Критические параметры ***");
            ImGui::SetCursorPosX(25); ImGui::SliderInt64("LatCPU", &i64latencyOffset, 0, 100,"%i", 0);
            ImGui::SetCursorPosX(25); ImGui::Text("*****************************");
            ImGui::SetCursorPosX(25); ImGui::TextColored(ImVec4(0.20f, 0.40f, 1.0f, 1.0f), "STACK_MAIN:%p", &fFr_main);
            ImGui::SetCursorPosX(25); ImGui::TextColored(ImVec4(0.20f, 1.0f, 0.40f, 1.0f), "[:: PROCESS_MEMORY ::]");
            ImGui::SetCursorPosX(25); ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.33f, 1.0f), ("App: Electrik Calc.exe + dll's: mem_usage:" + std::to_string((int64_t)fDataMemUsage() / 1024 / 1024) + ":MB").c_str());
            ImGui::Separator();
            ImGui::SetCursorPosX(25); ImGui::TextColored((!isOK ? RGBA(255, 0, 120, 255) : RGBA(0, 255, 120, 255)), "bool:%s",(isOK ? "true" : "false"));
            ImGui::SetCursorPosX(25); if (ImGui::Button("OK", ImVec2(150, 30))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
           
        if (ELC_LaunchBool) {
            if (!fCAboutW) {//
                static bool iGLSwap;
                static int64_t i64SwapCount = 0;
                static int64_t i64SwapDelay = 0;
                i64SwapDelay++;
                if (i64SwapDelay > 15) {
                    i64SwapDelay = 0;
                    i64SwapCount++;
                }
                if (i64SwapCount > 1) {
                    i64SwapCount = 0;
                }
                iGLSwap = static_cast<bool>(i64SwapCount);
                //ImGui::Image(fGLStackImage)
                ImGui::SetCursorPos(ImVec2((ImGui::GetWindowWidth() - 198) / 2, 100));
                ImGui::Image(GLArrayBuffer,ImVec2(198,198));//GLArrayBuffer
                ImGui::PushFont(Font1Buffer);
                ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("КАЛЬКУЛЯТОР ЭЛЕКТРОНИКА").x) / 2);
                ImGui::Text("КАЛЬКУЛЯТОР ЭЛЕКТРОНИКА");
                ImGui::PopFont();
                if (bFlag1 && bFlag0) {
                ImGui::SetCursorPosX((ImGui::GetWindowWidth() - (ImGui::CalcTextSize("Начать").x + 20 + ImGui::CalcTextSize("О Программе").x + 20)) / 2);
                    if (ImGui::Button("Начать", ImVec2(ImGui::CalcTextSize("Начать").x + 20, 40))) {
                        ELC_LaunchBool = false;
                        fELCButtonsState = true;

                    }
                    ImGui::SameLine();
                    if (ImGui::Button("О Программе", ImVec2(ImGui::CalcTextSize("О Программе").x + 20, 40))) {
                        fCAboutW = true;
                    }
                }
                else {
                    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - (ImGui::CalcTextSize("О Программе").x + 20)) / 2);
                    if (ImGui::Button("О Программе", ImVec2(ImGui::CalcTextSize("О Программе").x + 20, 40))) {
                        fCAboutW = true;
                    }
                }
                ImGui::SetCursorPosX((ImGui::GetWindowWidth() - (ImGui::CalcTextSize("[H] - Debug").x + 20)) / 2);
                ImGui::Text("[H] - Debug");
            }
            if (fCAboutW) {

                CurrentMemoryBuffer.ClearFreeMemory();
                ImGui::BeginChild("Настроики", ImVec2(HwndWSizeA(hwnd).x - 45, HwndWSizeA(hwnd).y - 65) , ImGuiChildFlags_Border | ImGuiChildFlags_FrameStyle);
                if (ImGui::BeginPopup("d", ImGuiWindowFlags_NoTitleBar | ImGuiChildFlags_Border | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
                  
                    i64Offset_d = (getAddrValue >> 16);
                    ImGui::SetWindowPos(ImVec2(15, 180));
                    ImGui::Text("Введите Ключ активации");
                    ImGui::Separator();
                    ImGui::SetNextItemWidth(100);
                    ImGui::InputText("-", &sProduct);
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(150);
                    ImGui::InputInt("KeyI64", &getAddrValue);
                    ImGui::Text((sProduct + "-" + std::to_string(i64Offset_d)).c_str());
                    if (ImGui::Button("Активировать")) {
                        if (i64Offset_d == (getAddrValue >> 16)) {
                            bFlag0 = true;
                        }
                        if (sProduct == "elc") {
                            bFlag1 = true;
                        }
                   }
                    if (bFlag1 && bFlag0) {
                        //MessageBoxA(hwnd, "Success", "ELC", 0);
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
                if (ImGui::BeginPopup("about", ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::SetCursorPosX(15); ImGui::Text("О Программе");
                    ImGui::SetCursorPosX(15); ImGui::Separator();
                    ImGui::SetCursorPosX(15); ImGui::Text("Сборка 1.1.4.5");
                    ImGui::SetCursorPosX(15); ImGui::Text("(OGL3)_AMD64\nC++20 (MSVC)\nVisual Studio 2022 x64");
                    ImGui::SetCursorPosX(5);
                    ImGui::PushFont(Font2Buffer);
                    ImGui::SetCursorPosX(15); ImGui::Text("Огромное спасибо этим людям");
                    ImGui::PopFont();
                    ImGui::PushFont(Font2Buffer);
                    ImGui::SetCursorPosX(15); ImGui::TextColored(ImVec4(0.20f, 0.40f, 1.0f, 1.0f), "Juriuscorp\nDima Xp\nDima Popov");
                    ImGui::PopFont();
                    if (ImGui::Button("OK", ImVec2(100, 30))) {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
                ImGui::SetCursorPos(ImVec2(20,20));
                ImGui::SetCursorPosX(15); ImGui::Text("Настроики");
                ImGui::SetCursorPosX(15); ImGui::Separator();
                ImGui::SetCursorPosX(15);
                ImGui::PushFont(font42);
                 ImGui::TextColored(!bDarkStyle ? RGBA(133, 133, 133, 255) : RGBA(7, 7, 7, 255), "Можно сменить тему на светлую или темную");
                ImGui::PopFont();
                ImGui::SetCursorPosX(15);
                ImGui::CustomToggle(!bDarkStyle,"Сменить Тему ", &bT_switch); ImGui::SameLine(); ImGui::TextColored((bT_switch ? ImVec4(0.34f, 0.06f, 0.98f, 1.00f) : ImVec4(0.98f, 0.06f, 0.34f, 1.00f)), (bT_switch ? "Темная  " : "Светлая"));
                ImGui::SetCursorPosX(15);
                ImGui::PushFont(font42);
                 ImGui::TextColored(!bDarkStyle ? RGBA(133, 133, 133, 255) : RGBA(7, 7, 7, 255), "Меняет шрифт на другой");
                ImGui::PopFont();
                ImGui::SetCursorPosX(15);
                ImGui::CustomToggle(!bDarkStyle,"Сменить шрифт", &fSwitchFont); ImGui::SameLine(); ImGui::TextColored((fSwitchFont ? ImVec4(1.0f, 1.0f, 1.0f, 1.00f) : ImVec4(1.0f, 0.0f, 0.50f, 1.00f)), (fSwitchFont ? "SFMono-Bold      " : "BOUNDED"));
                
                if (ImGui::BeginPopup("per_mode", ImGuiWindowFlags_AlwaysAutoResize)) {
                    int64_t g_i64perwX = 30;
                    int64_t g_i64FrameRate = io.Framerate;
                    static std::string sPerStatus;
                    static bool bPerState = false;
                    ImGui::SetCursorPosX(g_i64perwX); ImGui::SetCursorPosY(30);
                    ImGui::Text("Идет настройка");
                    ImGui::SetCursorPosX(g_i64perwX);
                    ImGui::Text("Частота кадров: %.0f", io.Framerate);
                    ImGui::SetCursorPosX(g_i64perwX);
                    ImGui::Text("%s", sPerStatus);
                    if (bPerState) {
                        ImGui::SetCursorPosX(g_i64perwX);
                        if (ImGui::Button("OK")) {
                            ImGui::CloseCurrentPopup();
                        }
                    }
                    if (g_i64FrameRate >= 60) {
                        i64latencyOffset = i64latencyOffset;
                        sPerStatus = "Готово!";
                        bPerState = true;
                    }
                    else {
                        i64latencyOffset--;
                        if (!i64latencyOffset) {
                            fLimitCPUUsage = false;
                        }
                    }
                    ImGui::EndPopup();
                }
                ImGui::SetCursorPosX(15); ImGui::Text("Производительность");
                ImGui::SetCursorPosX(15); ImGui::Separator();
                ImGui::SetCursorPosX(15);
                ImGui::PushFont(font42);
                 ImGui::TextColored(!bDarkStyle ? RGBA(133, 133, 133, 255) : RGBA(7, 7, 7, 255), "Эта настройка автоматически ограничит\nчастоту кадров и задержку под ваше железо\nЗначительно снижает нагрузку на ЦП и ГПУ");
                ImGui::PopFont();
                ImGui::SetCursorPosX(15);
                if (ImGui::Button("Автонастройка", ImVec2(340, 30))) {
                    ImGui::OpenPopup("per_mode");
                }
                ImGui::SetCursorPosX(15); 
                ImGui::PushFont(font42);
                ImGui::TextColored(!bDarkStyle ? RGBA(133, 133, 133, 255) : RGBA(7, 7, 7, 255), "если выключить то из-за большого количества кадров\nбудет большое потребление ЦП\nОграничьте в драйверах на частоту вашего монитора");
                ImGui::PopFont();
                ImGui::SetCursorPosX(15);
                if (ImGui::CustomToggle(!bDarkStyle,"Ограничение кадров",&fLimitCPUUsage)) {
                   
                }
                ImGui::SetCursorPosX(15);
                ImGui::PushFont(font42);
                 ImGui::TextColored(!bDarkStyle ? RGBA(133, 133, 133, 255) : RGBA(7, 7, 7, 255), "Выведит информацию о сборке и авторе");
                ImGui::PopFont();
                ImGui::SetCursorPosX(15);
                if (ImGui::Button("О Программе", ImVec2(340, 30))) {
                    ImGui::OpenPopup("about");
                }
                //ImGui::OpenPopup("per_mode");
                ImGui::SetCursorPosX(15);
                ImGui::PushFont(font42);
                 ImGui::TextColored(!bDarkStyle ? RGBA(133, 133, 133, 255) : RGBA(7, 7, 7, 255), "Активация приложения.\nОно добавлено чтобы не копировали \nи не присваивали авторство)\nа так ключ бесплатный");
                ImGui::PopFont();
                ImGui::SetCursorPosX(15);
                if (ImGui::Button((bFlag1 && bFlag0) ? "Активировано" : "Aктивировать", ImVec2(ImGui::CalcTextSize("Активировать").x + 54, 40))) {
                    if (!bFlag1 && !bFlag0) { ImGui::OpenPopup("d"); }
                }
                ImGui::SameLine();
                if (ImGui::Button("Выход", ImVec2(150, 40))) {
                    fCAboutW = false;
                    std::ofstream elcConfig("config.txt");
                    elcConfig.is_open();
                    elcConfig << "elcTheme=" << (bT_switch ? "dark_theme" : "light_theme") << std::endl;
                    elcConfig << "elcFont=" << (fSwitchFont ? "SFMono_Bold" : "PixelizerBold") << std::endl;
                    elcConfig << ((bFlag0 && bFlag1) ? "elc-20334" : "") << std::endl;
                    elcConfig << sProduct << "-" << (getAddrValue >> 16) << std::endl;
                    elcConfig.close();
                    //ELC_LaunchBool = false;
                }
                ImGui::EndChild();
            }
            if (bWindow1) {//
                ImGui::Begin("window_0", &bWindow1, ImGuiWindowFlags_NoTitleBar);
                ImGui::SetCursorPosX(15);ImGui::Text("Калькулятор электроника v 1.1.3");
                ImGui::Separator();
                ImGui::SetCursorPosX(15);ImGui::Text("Вы точно хотите выйти?");
                ImGui::SetCursorPosX(15);
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
           
        }
        if (!fELCButtonsState) {
            style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
        }
        if (fELCButtonsState)
        {
            ImVec2 ButtonSize = { ImGui::GetWindowWidth() - 40,40.0f };
            CurrentMemoryBuffer.ClearFreeMemory();
            ImGui::PushFont(Font1Buffer);
            ImGui::SetCursorPosX(40);ImGui::Text("КАЛЬКУЛЯТОР ЭЛЕКТРОНИКА");
            ImGui::Separator();
            ImGui::PopFont();
          //  style.ButtonTextAlign = ImVec2(0.1f, 0.5f);
            SetCenterToX(ButtonSize);
            if (ImGui::Button("Ватты в Амперы", ButtonSize))
            {
                //
                fFrameWA = true;
            }
            SetCenterToX(ButtonSize);
            if (ImGui::Button("Расчет ампер по ОМУ", ButtonSize))
            {
                //
                fFrameOM = true;
            }
            SetCenterToX(ButtonSize);
            if (ImGui::Button("Расчет диаметра провода", ButtonSize))
            {
                //
                fFrameDIA = true;
            }
            SetCenterToX(ButtonSize);
            if (ImGui::Button("Расчет мощности  Тена", ButtonSize))
            {
                //
                fFramePH = true;
            }
            SetCenterToX(ButtonSize);
            if (ImGui::Button("Расчет Номинала Автомата", ButtonSize))
            {
                fNautomat = true;
            }
            SetCenterToX(ButtonSize);
            if (ImGui::Button("Расчет Фазосдвигающего Конденсатора", ButtonSize))
            {
                fC_phase = true;
            }
            SetCenterToX(ButtonSize);
            if (ImGui::Button("Расчет баластного Конденсатора", ButtonSize))
            {
                fC_balast = true;
            }
            SetCenterToX(ButtonSize);
            if (ImGui::Button("Расчет Тока КЗ Нагрузки", ButtonSize))
            {
                fC_Idevice = true;//fC_Dvolt
            }
            SetCenterToX(ButtonSize);
            if (ImGui::Button("Расчет Умножителя напряжения", ButtonSize))
            {
                fC_Dvolt = true;//fC_Dvolt
            }
            SetCenterToX(ButtonSize);
            if (ImGui::Button("Расчет стоимости в минуту \nиз потребления", ButtonSize))
            {
                ImGui::OpenPopup("sym2w");
            }
            SetCenterToX(ButtonSize);
            if (ImGui::Button("Расчет Теплового пола", ButtonSize))
            {
                g_bCalcWpola = true;//fC_Dvolt
            }
            SetCenterToX(ButtonSize);
            if (ImGui::Button("Расчет падания напряжения на кабеле", ButtonSize))
            {
                fFrameVoltageDrop = true;//fC_DvoltfFramePowerCos
            }
            SetCenterToX(ButtonSize);
            if (ImGui::Button("Мощность по току и cos фи", ButtonSize))
            {
                fFramePowerCos = true;//fC_DvoltfFramePowerCos
            }
            SetCenterToX(ButtonSize);
            if (ImGui::Button("На главную", ButtonSize)) {
                ELC_LaunchBool = true;
                fELCButtonsState = false;
            }
            
          
        }
        
        if (g_bCalcWpola) {
            fFrameOM = false;
            fFrameDIA = false;
            fFrameWA = false;
            fFramePH = false;
            fTable = false;
            fNautomat = false;
            fC_phase = false;
            fELCButtonsState = false;
            static int64_t g_i64S = 0;
            static int64_t g_i64uWatt = 0;
            static int64_t g_i64kcm = 0;
            static int64_t g_i64SectionWatt = 0, g_i64CableM  = 0;
            ImGui::Text("Расчет нагревателя теплого пола"); ImGui::SameLine();
            if (ImGui::Button("X", ImVec2(30, 30))) {
                g_bCalcWpola = false; fELCButtonsState = true;
            }
            ImGui::Separator();
            ImGui::PushFont(font42);
            ImGui::TextColored(!bDarkStyle ? RGBA(133, 133, 133, 255) : RGBA(7, 7, 7, 255), "Для площади %d нужно %dм",g_i64kcm, g_i64CableM);
            ImGui::TextColored(!bDarkStyle ? RGBA(133, 133, 133, 255) : RGBA(7, 7, 7, 255), "Для площади нужен сегмент мощностью %d Ватт", g_i64SectionWatt);
            ImGui::PopFont();
            ImGui::Text("Удельная мощность");
            ImGui::PushFont(font42);
            ImGui::TextColored(!bDarkStyle ? RGBA(133, 133, 133, 255) : RGBA(7, 7, 7, 255), "Удельная мощность это мощность на м2\nИзмеряется в ватт/m2");
            ImGui::PopFont();
            ImGui::InputInt64("w", &g_i64uWatt, 1, 100,0);
			ImGui::Text("Площадь пола или участка");
            ImGui::PushFont(font42);
            ImGui::TextColored(!bDarkStyle ? RGBA(133, 133, 133, 255) : RGBA(7, 7, 7, 255), "Площадь обогреваемого участка\nИзмеряется в m2");
            ImGui::PopFont();
            ImGui::InputInt64("m2", &g_i64S, 1, 100, 0);
            ImGui::Text("Шаг укладки");
            ImGui::PushFont(font42);
            ImGui::TextColored(!bDarkStyle ? RGBA(133, 133, 133, 255) : RGBA(7, 7, 7, 255), "Шаг укладки - это шаг между проводов(укладки)\nИзмеряется в см");
            ImGui::PopFont();
            ImGui::InputInt64("cm", &g_i64kcm, 1, 100, 0);
            ImVec2 bSizev2 = ImVec2(ImGui::GetWindowWidth() - 10, 45);
            ImGui::SetCursorPosX(10);
            if (ImGui::Button("Расчитать", ImVec2(200, 45))) {
                g_i64SectionWatt = (g_i64S * g_i64uWatt);
                g_i64CableM = g_i64S * g_i64kcm;
            }
        }
        if (fFramePowerCos) {
            fFrameOM = false;
            fFrameDIA = false;
            fFrameWA = false;
            fFramePH = false;
            fTable = false;
            fNautomat = false;
            fC_phase = false;
            fELCButtonsState = false;
            static float fCurrent = 0, fVoltage = 220, fCosPhi = 0.8f;
            static float fActive = 0, fReactive = 0, fApparent = 0;

            ImGui::Text("Мощность по току и cos фи");
            ImGui::SameLine();
            if (ImGui::Button("X", ImVec2(30, 30))) { fFramePowerCos = false; fELCButtonsState = true; }

            ImGui::InputFloat("Ток (А)", &fCurrent);
            ImGui::InputFloat("Напряжение (В)", &fVoltage);
            ImGui::InputFloat("cos фи", &fCosPhi, 0.01f, 1.0f);

            if (ImGui::Button("Рассчитать", ImVec2(150, 35))) {
                fApparent = fVoltage * fCurrent / 1000.0f;
                fActive = fApparent * fCosPhi;
                fReactive = fApparent * sqrtf(1 - fCosPhi * fCosPhi);
                //SaveResult("P=" + std::to_string(fActive) + "кВт");
            }

            ImGui::Text("Полная: %.3f кВА", fApparent);
            ImGui::Text("Активная: %.3f кВт", fActive);
            ImGui::Text("Реактивная: %.3f квар", fReactive);
        }
        if (fFrameVoltageDrop) {
            // Сброс других окон
            fFrameOM = fFrameDIA = fFrameWA = fFramePH = fTable = fNautomat = fC_phase = fC_Idevice = false;
            fELCButtonsState = false;

            // === ВХОДНЫЕ ДАННЫЕ ===
            static float fPower = 2000.0f;           // Вт
            static float fVoltage = 220.0f;          // В
            static float fCosPhi = 0.95f;            // cos φ
            static float fLength = 100.0f;           // м
            static float fArea = 2.5f;               // мм²
            static float fAmbientTemp = 25.0f;       // °C
            static int material = 0;                 // 0=медь, 1=алюминий
            static int phases = 0;                   // 0=1-фазная, 1=3-фазная

            // === РЕЗУЛЬТАТЫ ===
            static float fCurrent = 0;
            static float fResistivity20 = 0.0175f;   // при 20°C
            static float fAlpha = 0.0039f;
            static float fR20 = 0, fR_actual = 0;
            static float fVoltageDrop = 0, fPercentDrop = 0;
            static float fCableTemp = 0;
            static bool dangerDrop = false, dangerTemp = false;

            // === ЗАГОЛОВОК ===
            ImGui::PushFont(font3);
            ImGui::Text("Расчет падения напряжения");
            ImGui::SameLine();
            if (ImGui::Button("X", ImVec2(30, 30))) {
                fFrameVoltageDrop = false; fELCButtonsState = true;
            }
            ImGui::PopFont();

            // === ВЫБОР ===
            ImGui::Text("Сеть:");
            ImGui::RadioButton("1-фазная", &phases, 0); ImGui::SameLine();
            ImGui::RadioButton("3-фазная", &phases, 1);

            ImGui::Text("Материал:");
            ImGui::RadioButton("Медь", &material, 0); ImGui::SameLine();
            ImGui::RadioButton("Алюминий", &material, 1);
            if (material == 0) { fResistivity20 = 0.0175f; fAlpha = 0.0039f; }
            else { fResistivity20 = 0.0290f; fAlpha = 0.0040f; }

            // === ВВОД ===
            ImGui::InputFloat("Мощность (Вт)", &fPower, 100.0f, 10000.0f);
            ImGui::InputFloat("Напряжение (В)", &fVoltage, 1.0f, 400.0f);
            ImGui::InputFloat("cos φ", &fCosPhi, 0.01f, 1.0f, "%.3f");
            ImGui::InputFloat("Длина (м)", &fLength, 1.0f, 500.0f);
            ImGui::InputFloat("Сечение (мм²)", &fArea, 0.1f, 50.0f);
            ImGui::InputFloat("Температура воздуха (°C)", &fAmbientTemp, 1.0f, 50.0f);

            // === РАСЧЁТ ===
            if (ImGui::Button("Рассчитать", ImVec2(180, 40))) {
                // 1. Ток с учётом cos φ
                if (phases == 0) { // 1-фазная
                    fCurrent = fPower / (fVoltage * fCosPhi);
                }
                else { // 3-фазная
                    fCurrent = fPower / (fVoltage * fCosPhi * 1.732f);
                }

                // 2. Сопротивление при 20°C (туда-обратно!)
                fR20 = 2.0f * fResistivity20 * fLength / fArea;

                // 3. Примерная температура кабеля (по потерям)
                float P_loss = fCurrent * fCurrent * fR20;
                float K_heat = (fArea < 4.0f) ? 30.0f : 20.0f; // °C/Вт
                fCableTemp = fAmbientTemp + P_loss * K_heat / 1000.0f;

                // 4. Сопротивление при реальной температуре
                fR_actual = fR20 * (1.0f + fAlpha * (fCableTemp - 20.0f));

                // 5. Падение напряжения
                fVoltageDrop = fCurrent * fR_actual;
                fPercentDrop = (fVoltageDrop / fVoltage) * 100.0f;

                // 6. Проверки
                dangerDrop = (fPercentDrop > 3.0f);
                dangerTemp = (fCableTemp > 70.0f);

            }

            // === РЕЗУЛЬТАТЫ ===
            ImGui::Separator();

            ImGui::Text("Ток: %.2f А", fCurrent);

            ImGui::Text("R при 20°C: %.4f Ом", fR20);
            ImGui::Text("R при %.1f°C: %.4f Ом", fCableTemp, fR_actual);

            // Падение напряжения
            ImVec4 dropColor = dangerDrop ? ImVec4(1, 0.3, 0.3, 1) : ImVec4(0.3, 1, 0.3, 1);
            ImGui::TextColored(dropColor, "Падение: %.3f В (%.2f%%)", fVoltageDrop, fPercentDrop);

            // Температура
            ImVec4 tempColor = dangerTemp ? ImVec4(1, 0.3, 0.3, 1) : ImVec4(0.3, 1, 0.3, 1);
            ImGui::TextColored(tempColor, "Температура кабеля: %.1f°C", fCableTemp);

            // === ПРЕДУПРЕЖДЕНИЯ ===
            if (dangerDrop) {
                ImGui::TextColored(ImVec4(1, 0.7, 0.3, 1), "ВНИМАНИЕ: Падение > 3%%!");
                float minArea = 2 * fResistivity20 * fLength * fCurrent / (fVoltage * 0.03f);
                ImGui::Text("Нужно сечение ≥ %.2f мм²", minArea);
            }
            if (dangerTemp) {
                ImGui::TextColored(ImVec4(1, 0.3, 0.3, 1), "ОПАСНО: Перегрев кабеля!");
            }
        }
        if (fC_Idevice) {
            fFrameOM = false;
            fFrameDIA = false;
            fFrameWA = false;
            fFramePH = false;
            fTable = false;
            fNautomat = false;
            fC_phase = false;
            fELCButtonsState = false;
            //
            static float fUpowerOffset = 0;
            static float fUnonePowerOffset = 0;
            static float fRpower = 0.0f;
            static float fIkz = 0.0f;
            static float fU2U = 0;
            //
            ImGui::SetCursorPos(ImVec2(20, 20));
            ImGui::SetCursorPosX(5); ImGui::Text("Расчет Тока КЗ Нагрузки");
            CurrentMemoryBuffer.ClearFreeMemory();
            ImGui::SameLine();
            if (ImGui::Button("X",ImVec2(30,30))) {
                fC_Idevice = false; fELCButtonsState = true;
            }
            ImGui::BeginChild("##result", ImVec2(HwndWSizeA(hwnd).x - 50, 50), ImGuiChildFlags_Border | ImGuiChildFlags_FrameStyle);
            ImGui::Text("Iкз: %.3f A \n[Uнсн: %.3f,Uх:%.3f,Rн: %.3f]", fIkz,fUpowerOffset,fUnonePowerOffset,fRpower);
            ImGui::EndChild();
            ImGui::SetCursorPosX(5); ImGui::Text("U под нагрузкой");
            ImGui::InputFloat("Uн", &fUpowerOffset,1,1,0);
            ImGui::SetCursorPosX(5); ImGui::Text("U холостое");
            ImGui::InputFloat("Uх", &fUnonePowerOffset, 1, 1, 0);
            ImGui::SetCursorPosX(5); ImGui::Text("R нагрузки");
            ImGui::InputFloat("Rн", &fRpower, 1, 1, 0);
            if (ImGui::Button("Расчитать(I)",ImVec2(150,30))) {
                //fIkz = fUnonePowerOffset / (((fUnonePowerOffset - fUpowerOffset) / fUpowerOffset) * fRpower);
                //i = ux / (((ux - uh) / ux) * rh)
                fU2U = (fUnonePowerOffset - fUpowerOffset) / fUpowerOffset;
                fIkz = fUnonePowerOffset / (fU2U * fRpower);
            }

            }
            if (fC_balast) {
                fFrameOM = false;
                fFrameDIA = false;
                fFrameWA = false;
                fFramePH = false;
                fTable = false;
                fNautomat = false;
                fC_phase = false;
                fELCButtonsState = false;
                fC_Idevice = false;
                //Uобщая= √((U1 * U1)-(U2 * U2))
               // C = 3200 * I / Uобщая;
                
               static int64_t fUoutData = 0;
                ImGui::SetCursorPos(ImVec2(20,20));
                ImGui::SetCursorPosX(5);ImGui::Text("Расчет Баластного Конденсатора", &fC_balast);
                CurrentMemoryBuffer.ClearFreeMemory();
                ImGui::SameLine();
                if (ImGui::Button("X",ImVec2(30,30))) {
                    fC_balast = false; fELCButtonsState = true;
                }
                fUoutData = sqrt(pow(fU1, 2) - pow(fU2, 2));
                ImGui::SetWindowPos(ImVec2(2.0f, 2.0f));
                ImGui::SetCursorPosX(5);ImGui::Text("C: %.1f:мкф", fCoutData);
                ImGui::SetCursorPosX(5);ImGui::InputFloat("U_нагрузки:", &fU2, 0.0f, 10000);
                ImGui::SetCursorPosX(5);ImGui::InputFloat("U_Входное :", &fU1, 0.0f, 10000);
                ImGui::SetCursorPosX(5);ImGui::InputInt(" I:", &fIdata, 0, 10000);
                if (fUoutData > 1) {
                    ImGui::SetCursorPosX(5);
                    if (ImGui::Button("Рассчитать ", ImVec2(150, 30))) {
                        fCoutData = (3200 * fIdata / fUoutData);
                    }
                }
            }
            if (fC_Dvolt) {
                fFrameOM = false;
                fFrameDIA = false;
                fFrameWA = false;
                fFramePH = false;
                fTable = false;
                fNautomat = false;
                fC_phase = false;
                fELCButtonsState = false;
                fC_Idevice = false;
                //С (МкФ) = 2,85×N×Iн / (Кп×Uвых) = 2,85×N / (Кп×Rн),
                //N—кратность умножения напряжения;
                //Iн — ток нагрузки, мА;
                //Кп — допустимый коэффициент пульсаций выходного напряжения, %;
                //Uвыx—выходное напряжение, В.
                // = 2.85f * i64N_offset / (i64KPulseOffset * i64R_offset);
                std::string sInfoStrings =
                    "\nN    - кратность умножения напряжения"
                    "\nIн   - ток нагрузки, мА"
                    "\nRн   - Сопротивление нагрузки, Ом"
                    "\nКп   - допустимый коэффициент\n пульсаций выходного напряжения, %"
                    "\nUвыx - выходное напряжение, В.";
                std::string sScheme  = R"(
       +---+       +---+  
Вход ~-|D1 |--+----┤D2 ├---+  
       +---+  |    +---+   |  
              C1           C2  
              |            |  
              +-----┬------+  
                    |  
                   Rн (нагрузка)  
                    |  
                   GND  
)";
                static float fC_offset = 0;
                static int64_t i64Ip_offset = 0;
                static int64_t i64KPulseOffset = 0;
                static int64_t i64Uout_offset = 0;
                static int64_t i64N_offset = 0;
                static int64_t i64R_offset = 0;

                //
                
                //
                ImGui::SetCursorPosX(5); ImGui::Text("Расчет умножителя напряжения"); ImGui::SameLine();
                if (ImGui::Button("X", ImVec2(30, 30))) {
                    fC_Dvolt = false; fELCButtonsState = true;
                }
                ImGui::SetCursorPosX(5); ImGui::Separator();
                ImGui::SetCursorPosX(5); ImGui::Text("%s", (sInfoStrings).c_str());
                ImGui::SetCursorPosX(5); ImGui::Text("%s", sScheme.c_str());
                ImGui::Separator();
                ImGui::SetCursorPosX(5); ImGui::BeginChild("Out", ImVec2(ImGui::GetWindowWidth() - 100, 30), ImGuiChildFlags_Border | ImGuiChildFlags_FrameStyle);
                ImGui::TextColored(ImVec4(1, 0, 0.50f, 1), " [C1,C2]%.4f нф ", (fC_offset * 1000 ));
                ImGui::EndChild();
                ImGui::SetCursorPosX(5); ImGui::InputInt64("Iн (мА)", &i64Ip_offset,1,10,0);
                ImGui::SetCursorPosX(5); ImGui::InputInt64("Uвыx (В)", &i64Uout_offset, 1, 10, 0);
                ImGui::SetCursorPosX(5); ImGui::InputInt64("Кп (%)", &i64KPulseOffset, 1, 10, 0);
                ImGui::SetCursorPosX(5); ImGui::InputInt64("N", &i64N_offset, 1, 10, 0);
                if (ImGui::Button("Расчитать", ImVec2(100, 30))) {
                    fC_offset = (2.85f * i64N_offset * i64Ip_offset) / static_cast<float>(i64KPulseOffset * i64Uout_offset);
                }
            }
            if (fC_phase) {
                fFrameOM = false;
                fFrameDIA = false;
                fFrameWA = false;
                fFramePH = false;
                fTable = false;
                fNautomat = false;
                fC_balast = false; fELCButtonsState = false; fC_Idevice = false;
                static float fAbuffer = 0;
                static float fVbuffer = 0;
                static float fCOffset = 0.0f;
                static int64_t fp = 0;
                static float fOval = 0;
                if (f4800Offset) {
                    fp = 0; fOval = 4800;
                }
                if (f2800Offset) {
                    fp = 1; fOval = 2800;
                }
                //
                std::string fStrOffsetValue[] = { "Треугольник","Звезда" };
                //
                ImGui::SetCursorPosX(5);ImGui::Text("Расчет Фазосдвигающего\nКонденсатора");
                ImGui::SetCursorPosX(5);ImGui::TextColored(ImVec4(0.20f, 1.0f, 0.40f, 1.0f), (std::to_string(io.Framerate)).c_str());
                CurrentMemoryBuffer.ClearFreeMemory();
                if (ImGui::Button("X",ImVec2(30,30))) {
                    fC_phase = false; fELCButtonsState = true;
                }
                ImGui::SetWindowSize(HwndWSizeA(hwnd));
                ImGui::SetCursorPosX(5);
                if (ImGui::Button("Треугольник", ImVec2(130.f, 50.0f))) {
                    f2800Offset = false;
                    f4800Offset = true;
                    ///fCOffset = std::to_string(fOval * fAbuffer / fVbuffer) + "mKf";
                }
                ImGui::SameLine();
                if (ImGui::Button("Звезда", ImVec2(130.f, 50.0f))) {
                    f2800Offset = true;
                    f4800Offset = false;
                    //fCOffset = std::to_string(fOval * fAbuffer / fVbuffer) + "mKf";
                }
                ImGui::SetCursorPosX(5);ImGui::Text(("Тип:" + fStrOffsetValue[fp]).c_str());
                ImGui::SetCursorPosX(5);ImGui::Text("C:%.1f мкф",fCOffset);
                ImGui::SetCursorPosX(5);ImGui::InputFloat("A мотора:", &fAbuffer, 1, 100);
                ImGui::SetCursorPosX(5);ImGui::InputFloat("V мотора:", &fVbuffer, 1, 100);
                if (fVbuffer < 1) {
                    ImGui::SetCursorPosX(5);ImGui::Text("Ошибка: V не может быть равна 0!!");
                }
                if (fVbuffer > 0) {
                    ImGui::SetCursorPosX(5); if (ImGui::Button("Рассчитать ", ImVec2(100.f, 30.0f))) {
                        fCOffset = fOval * fAbuffer / fVbuffer;
                    }
                }
                //   fC_value.innerHTML = parseFloat(fMtype * fDI / fDV) + " ���";
            }
            std::string fStrNominal = " 1.13 — ток кратковременного откл цепи\n1.45 — ток откл, при котором машина не должна выкл";
            std::string fStrNominal1 = "2.55 - ток, при котором машина должна выкл в течение 1 минуты.\n5 - ток аварийной перегрузки,при которой машина должна выкл в течение 10 секунд \n10 - ток аварийной перегруз при которой машина должна выкл выключать";
            int64_t fNin[] = { 13,45,155,400,900 };
            if (fNautomat) {
                fFrameOM = false;
                fFrameDIA = false;
                fFrameWA = false;
                fFramePH = false; fC_Idevice = false;
                fTable = false; fELCButtonsState = false;
                //
                //
                ImGui::SetCursorPosX(5);ImGui::Text("Расчет номинала автомата (Идея DIMA XP)", &fNautomat);
                ImGui::SetCursorPosX(5);ImGui::TextColored(ImVec4(0.20f, 1.0f, 0.40f, 1.0f), "FPS:%.2f",io.Framerate);
                static  const char* button_idStr[] = { "1.13","1.45","2.55","5.00","10.0" };
                static  std::string b0_str = button_idStr[0];
                static  std::string b1_str = button_idStr[1];
                static  std::string b2_str = button_idStr[2];
                static  std::string b3_str = button_idStr[3];
                static  std::string b4_str = button_idStr[4];
                if (fNin_0) { fNinOffset = fNin[0]; }
                if (fNin_1) { fNinOffset = fNin[1]; }
                if (fNin_2) { fNinOffset = fNin[2]; }
                if (fNin_3) { fNinOffset = fNin[3]; }
                if (fNin_4) { fNinOffset = fNin[4]; }
                //
                
                ImGui::SetWindowPos(ImVec2(2.0f, 2.0f));
                if (ImGui::Button("X",ImVec2(30,30))) {
                    fNautomat = false; fELCButtonsState = true;
                }
                // parseInt(fNdata) + (parseInt(fNdata) * (fProcentArray[fCPoint] / 100));
                ImGui::SetCursorPosX(5);ImGui::Text("A:%.2f", rs);
                ImGui::PushFont(Font3Buffer);
                if (ImGui::Button((b0_str).c_str(), ImVec2(50, 50.0f))) {
                    /// MessageBeep(0x80000000);
                    fNinOffset = fNin[0]; b0_str = "[1.13]"; b1_str = button_idStr[1]; b2_str = button_idStr[2]; b3_str = button_idStr[3]; b4_str = button_idStr[4];
                }
                ImGui::SameLine();
                if (ImGui::Button((b1_str).c_str(), ImVec2(50, 50.0f))) {
                    fNinOffset = fNin[1]; b1_str = "[1.45]"; b0_str = button_idStr[0]; b2_str = button_idStr[2]; b3_str = button_idStr[3]; b4_str = button_idStr[4];
                }
                ImGui::SameLine();
                if (ImGui::Button((b2_str).c_str(), ImVec2(50, 50.0f))) {
                    fNinOffset = fNin[2]; b2_str = "[2.55]"; b1_str = button_idStr[1]; b0_str = button_idStr[0]; b3_str = button_idStr[3]; b4_str = button_idStr[4];
                }
                ImGui::SameLine();
                if (ImGui::Button((b3_str).c_str(), ImVec2(50, 50.0f))) {
                    fNinOffset = fNin[3]; b3_str = "[5.00]"; b1_str = button_idStr[1]; b2_str = button_idStr[2]; b0_str = button_idStr[0]; b4_str = button_idStr[4];
                }
                ImGui::SameLine();
                if (ImGui::Button((b4_str).c_str(), ImVec2(50, 50.0f))) {
                    fNinOffset = fNin[4]; b4_str = "[10.0]"; b1_str = button_idStr[1]; b2_str = button_idStr[2]; b3_str = button_idStr[3]; b0_str = button_idStr[0];
                }
                ImGui::PopFont();
                ImGui::SetCursorPosX(5);ImGui::Text("Номинал Автомата ");
                ImGui::SetCursorPosX(5);ImGui::InputInt("А:", &fN, 1, 100, 0);
                ImGui::PushFont(Font3Buffer);
                ImGui::SetCursorPosX(5);ImGui::Text("1.13 - ток кратковременного отключения цепи");
                ImGui::Separator();
                ImGui::SetCursorPosX(5);ImGui::Text("1.45 — ток отключения, при котором \nмашина не должна выключаться");
                ImGui::Separator();
                ImGui::SetCursorPosX(5);ImGui::Text("2.55 - ток, при котором машина\nдолжна выключиться в течение 1 минуты");
                ImGui::Separator();
                ImGui::SetCursorPosX(5);ImGui::Text("5.00 - ток аварийной перегрузки при которой машина\nдолжна выключиться в течение 10 секунд ");
                ImGui::Separator();
                ImGui::SetCursorPosX(5);ImGui::Text("10.0 - ток аварийной перегрузки\nпри которой машина должна выключиться выключать");
                ImGui::PopFont();
                if (ImGui::Button("Рассчитать ", ImVec2(150.0f, 30.0f))) {
                    rs = ((float)fNinOffset / 100) * fN + fN;
                    fStrNominal = std::to_string(rs);
                }

                ImGui::SameLine();
                if (ImGui::Button("Показать таблицу", ImVec2(180.0f, 30.0f))) {
                    fFrameOM = false;
                    fFrameDIA = false;
                    fFrameWA = false;
                    fFramePH = false;
                    fNautomat = false;
                    fTable = true; fELCButtonsState = false; fC_Idevice = false;
                }
            }
           
            // 
          //  ImGui::SetCursorPosX(5);ImGui::Text("Created by HCPP 2024");
            if (fTable) {
                if (ImGui::Button("X",ImVec2(30,30))) {
                    fTable = false; fELCButtonsState = true;
                }
                //
               
                //

                static ImGuiTableFlags flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
                const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
                if (ImGui::BeginTable("table2", 6, flags))
                {
                    ImGui::TableSetupColumn("CU(kW)", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoResize);
                    ImGui::TableSetupColumn("A(A)", ImGuiTableColumnFlags_WidthStretch   | ImGuiTableColumnFlags_NoResize);
                    ImGui::TableSetupColumn("S(mm)", ImGuiTableColumnFlags_WidthStretch  | ImGuiTableColumnFlags_NoResize);
                    ImGui::TableSetupColumn("AL(kW)", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoResize);
                    ImGui::TableSetupColumn("A(A)", ImGuiTableColumnFlags_WidthStretch   | ImGuiTableColumnFlags_NoResize);
                    ImGui::TableSetupColumn("U(v)", ImGuiTableColumnFlags_WidthStretch   | ImGuiTableColumnFlags_NoResize);
                    ImGui::TableHeadersRow();
                    for (int row = 0; row < 12; row++)
                    {
                        float amperArrayCp[] = { 11,15,17,23,26,30,41,50,80,100,140,170 };
                        float amperArrayAl[] = { 0,0,0,0,21,24,32,39,60,75,105,130 };
                        float WattArrayCp[] = { 2.4f,3.3f,3.7f,5.0f,5.7f,6.6f,9.0f,11.0f,17.0f,22.0f,30.0f,37.0f };
                        float WattArrayAl[] = { 0.0f,0.0f,6.4f,8.7f,9.8f,11.0f,15.0f,19.0f,30.0f,38.0f,53.0f,64.0f };
                        float SectionArray[] = { 0.5f,0.75f,1.0f,1.5f,2.0f,2.5f,4.0f,6.0f,10.0f,16.0f,25.0f,35.0f };
                        ImGui::TableNextRow();

                        ImGui::TableSetColumnIndex(0);
                        ImGui::Text("%.1f", WattArrayCp[row]);
                        ImGui::TableSetColumnIndex(1);
                        ImGui::Text("%.1f", amperArrayCp[row]);
                        ImGui::TableSetColumnIndex(2);
                        ImGui::Text("%.1f", SectionArray[row]);
                        ImGui::TableSetColumnIndex(3);
                        ImGui::Text("%.1f", WattArrayAl[row]);
                        ImGui::TableSetColumnIndex(4);
                        ImGui::Text("%.1f", amperArrayAl[row]);
                        ImGui::TableSetColumnIndex(5);
                        ImGui::Text("%.1f", 230.0f);
                    }
                    ImGui::EndTable();
                }
            }
            if (fFramePH) {
                fFrameOM = false;
                fFrameDIA = false;
                fFrameWA = false; fELCButtonsState = false; fC_Idevice = false;
                static float fWatt = 0;
                static float fR = 0;
                static float fVolt = 0;
                static float fValue = 0;
                std::string data_f;
                std::string lvl_0;
                data_f = lvl_0 + std::to_string(fValue);
                ImGui::SetCursorPosX(5);ImGui::Text("Расчет мощности тена");
                ImGui::SetCursorPosX(5);ImGui::TextColored(ImVec4(0.20f, 1.0f, 0.40f, 1.0f), (": fps_count:" + std::to_string(io.Framerate)).c_str());
                
                ImGui::SetWindowPos(ImVec2(2.0f, 2.0f));
                if (ImGui::Button("X",ImVec2(30,30))) {
                    fFramePH = false; fELCButtonsState = true;
                }
                ImGui::SetCursorPosX(5);ImGui::Text((data_f).c_str());
                ImGui::SetCursorPosX(5);ImGui::InputFloat("V:", &fVolt, 1, 1000, 0);
                ImGui::SetCursorPosX(5);ImGui::InputFloat("R:", &fR, 1, 1000, 0);
                //(fPV * fPV / fPR);
                if (ImGui::Button("Рассчитать ", ImVec2(130.0f, 30.0f)))
                {
                    lvl_0 = "W:";
                    fValue = (fVolt * fVolt) / fR;

                }
            }
            if (ImGui::BeginPopup("sym2w", ImGuiWindowFlags_AlwaysAutoResize)) {
                static int64_t i64Watt2HourValue = 0;
                static int64_t i64MinuteValue = 0;
                static float fPriceOneKw_HValue = 0.0f;
                static float fOutHour = 0.0f;
                static float fOutKW = 0.0f;
                static float fOutPowerValue = 0.0f;
                static float fOutPriceToCurrentMinute = 0.0f;
                ImGui::Text("Расчет стоимости из ватт");
                ImGui::Separator();
                ImGui::Text("Нагорит %.1fквт/ч\nстоимость %.1fр за %i минут", fOutPowerValue, fOutPriceToCurrentMinute, i64MinuteValue);
                ImGui::Text("Сколько потребляет вт в час");
                ImGui::InputInt64("вт*ч", &i64Watt2HourValue, 1, 100, 0);
                ImGui::Text("За сколько минут рассчитать");
                ImGui::InputInt64("минуты", &i64MinuteValue, 1, 100, 0);
                ImGui::Text("Сколько стоит квт * ч");
                ImGui::InputFloat("руб", &fPriceOneKw_HValue, 0.0f, 100.0f, "%.3f", 0);
                if (ImGui::Button("Расчитать стоимость")) {
                    fOutHour = static_cast<float>(i64MinuteValue)   / 60.0f;
                    fOutKW   = static_cast<float>(i64Watt2HourValue) / 1000.0f;
                    fOutPowerValue = fOutKW * fOutHour;
                    fOutPriceToCurrentMinute = fOutPowerValue * fPriceOneKw_HValue;
                }
                ImGui::SameLine();
                if (ImGui::Button("Выйти")) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            if (fFrameWA)
            {
               
                fFrameOM = false;
                fFrameDIA = false; fELCButtonsState = false; fC_Idevice = false;
                static float fWatt = 0;
                static bool v_state = false;
                static float fVoltageW = 0;
                static float fVoltageW2 = 0;
                static float fAmp = 0;
                static float fValue = 0;
                static float fValue2 = 0;
                std::string data_f;
                std::string lvl_0;
                data_f = lvl_0 + std::to_string(fValue);
                ImGui::SetCursorPosX(5); ImGui::Text("Ватты в Ампера и наоборот"); ImGui::SameLine(); if (ImGui::Button("X", ImVec2(30, 30))) {
                    fFrameWA = false; fELCButtonsState = true;
                }
                ImGui::SetCursorPosX(5);ImGui::TextColored(ImVec4(0.20f, 1.0f, 0.40f, 1.0f), (": fps_count:" + std::to_string(io.Framerate)).c_str());
                ImGui::BeginChild("a", ImVec2(HwndWSizeA(hwnd).x - 50, 180), ImGuiChildFlags_Border | ImGuiChildFlags_FrameStyle);
                ImGui::SetCursorPos(ImVec2(20, 20));
                ImGui::SetCursorPosX(5);
                ImGui::SetCursorPosX(5);ImGui::Text("Ватты в Амперы");
                ImGui::SetCursorPosX(5);
                ImGui::SetCursorPosX(5);ImGui::TextColored(ImVec4(150.f, 150.0f, 255.0f, 1.0f), (data_f).c_str());
                ImGui::SetCursorPosX(5);
                ImGui::SetCursorPosX(5);ImGui::InputFloat("Вольтаж", &fVoltageW, 1, 1000, 0);
                ImGui::SetCursorPosX(5);
                ImGui::SetCursorPosX(5);ImGui::InputFloat("Ватты", &fWatt, 1, 1000, 0);
             
                if (fWatt > 0 && fVoltageW > 0)
                {
                    ImGui::SetCursorPosX(5);
                    if (ImGui::Button("Рассчитать(A)", ImVec2(130.0f, 30.0f)))
                    {
                        lvl_0 = "Watt";
                        fValue = fWatt / fVoltageW;

                    }
                }
               
                ImGui::EndChild();
                ImGui::BeginChild("j", ImVec2(HwndWSizeA(hwnd).x - 50, 180), ImGuiChildFlags_Border | ImGuiChildFlags_FrameStyle);
                ImGui::SetCursorPos(ImVec2(20, 20));
                ImGui::SetCursorPosX(5);ImGui::Text("Амперы в Ватты");
                ImGui::SetCursorPosX(5);
                ImGui::SetCursorPosX(5);ImGui::TextColored(ImVec4(150.f, 150.0f, 255.0f, 1.0f), (std::to_string(fValue2)).c_str());
                ImGui::SetCursorPosX(5);
                ImGui::SetCursorPosX(5);ImGui::InputFloat("U", &fVoltageW2, 1, 1000, 0);
                ImGui::SetCursorPosX(5);
                ImGui::SetCursorPosX(5);ImGui::InputFloat("I", &fAmp, 1, 1000, 0);
                ImGui::SetCursorPosX(5);
                if (ImGui::Button("Рассчитать(W)", ImVec2(130.0f, 30.0f)))
                {
                    lvl_0 = "Амперы";
                    fValue2 = fVoltageW2 * fAmp;
                }
                ImGui::EndChild();
            }
            if (fFrameOM)
            {
                fFrameWA = false;
                fFrameDIA = false; fELCButtonsState = false; fC_Idevice = false;
                static float fResistor = 0;
                static float fVoltage = 0;
                static float fAmper = 0;
                ImGui::SetCursorPosX(5);ImGui::TextColored(ImVec4(0.20f, 1.0f, 0.40f, 1.0f), (": fps_count:" + std::to_string(io.Framerate)).c_str());
                ImGui::BeginChild("c", ImVec2(HwndWSizeA(hwnd).x - 50, 180), ImGuiChildFlags_Border | ImGuiChildFlags_FrameStyle);
                ImGui::SetCursorPos(ImVec2(20, 20));
                ImGui::SetCursorPosX(5);ImGui::Text("Расчет Ампер по ОМУ");
                ImGui::SameLine();
                if (ImGui::Button("X",ImVec2(30,30))) {
                    fFrameOM = false; fELCButtonsState = true;
                }
                ImGui::SetCursorPosX(5);
                ImGui::SetCursorPosX(5);ImGui::Text(("Амперы:" + std::to_string(fAmper)).c_str());
                ImGui::SetCursorPosX(5);
                ImGui::SetCursorPosX(5);ImGui::InputFloat("Вольтаж", &fVoltage, 1, 1000, 0);
                ImGui::SetCursorPosX(5);
                ImGui::SetCursorPosX(5);ImGui::InputFloat("Сопротивление", &fResistor, 1, 1000, 0);
                ImGui::SetCursorPosX(5);
                if (ImGui::Button("Рассчитать ", ImVec2(150.0f, 30.f)))
                {
                    fAmper = fVoltage / fResistor;
                }
                ImGui::EndChild();
            }
            if (fFrameDIA)
            {
                fFrameOM = false;
                fFrameWA = false; fELCButtonsState = false; fC_Idevice = false;
                float fPi = 3.14f;
                float fdata_0 = 0.0f;
                static float fR_mm2 = 0.0f;
                ImGui::SetCursorPosX(5);ImGui::Text("Расчет сечения из диаметра");
                ImGui::SetCursorPosX(5);ImGui::TextColored(ImVec4(0.20f, 1.0f, 0.40f, 1.0f), (": fps_count:" + std::to_string(io.Framerate)).c_str());
                
                ImGui::SetWindowPos(ImVec2(2.0f, 2.0f));
                if (ImGui::Button("X",ImVec2(30,30))) {
                    fFrameDIA = false; fELCButtonsState = true;
                }
                ImGui::SetCursorPosX(5);ImGui::Text(("Сечение:" + std::to_string(fR_mm2)).c_str());
                static float fDia_value = 0.0f;
                ImGui::SetCursorPosX(5);ImGui::InputFloat("Диаметр", &fDia_value, 0.0f, 100000.0f, 0);
                if (ImGui::Button("Рассчитать ", ImVec2(150.0f, 30.f)))
                {
                    // fDA = (fD0 / 2) * (fD0 / 2) * fPi;
                    // fdata_0 = (fDA * fDA) * fPi;
                    fR_mm2 = ((fDia_value / 2) * (fDia_value / 2) * fPi);
                }
            }
            ImGui::PopFont();
            ImGui::End();
            // Rendering
            ImGui::Render();
            glViewport(0, 0, 329, 240);
            glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            // Present
            ::SwapBuffers(g_MainWindow.hDC);
        }

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        CleanupDeviceWGL(hwnd, &g_MainWindow);
        wglDeleteContext(g_hRC);
        ::DestroyWindow(hwnd);
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

        return 0;
    }

    // Helper functionsd
    bool CreateDeviceWGL(HWND hWnd, WGL_WindowData * data)
    {
        HDC hDc = ::GetDC(hWnd);
        PIXELFORMATDESCRIPTOR pfd = { 0 };
        pfd.nSize = sizeof(pfd);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 32;

        const int pf = ::ChoosePixelFormat(hDc, &pfd);
        if (pf == 0)
            return false;
        if (::SetPixelFormat(hDc, pf, &pfd) == FALSE)
            return false;
        ::ReleaseDC(hWnd, hDc);

        data->hDC = ::GetDC(hWnd);
        if (!g_hRC)
            g_hRC = wglCreateContext(data->hDC);
        return true;
    }

    void CleanupDeviceWGL(HWND hWnd, WGL_WindowData * data)
    {
        wglMakeCurrent(nullptr, nullptr);
        ::ReleaseDC(hWnd, data->hDC);
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
            if (wParam != SIZE_MINIMIZED)
            {
                g_Width = LOWORD(lParam);
                g_Height = HIWORD(lParam);
            }
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