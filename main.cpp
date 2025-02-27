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
#include <conio.h>
#include <fstream>
#include <string>
#include <Psapi.h>
#include <urlmon.h>
#include <clocale>
#include <stdint.h>
#include <thread>
#include <intrin.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "dSys.h"
//#include "Colors.h"
///
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
int64_t* fA_stack = 0;
static bool fCAboutW;
int fN = 1;
float rs = 0;

//
bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height, unsigned char* imgBuffer) {
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
    *imgBuffer = *image_data;
    if (image_data == NULL)
        return false;

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

    return true;
}//
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
        ImGui::Text("Error: Set0 to set0!=");
    }
    config << nameParam << std::endl;

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
//const ImWchar* buildGlyphRanges() {
//    ImFontAtlas* fonts = ImGui::GetIO().Fonts;
//    ImFontGlyphRangesBuilder glyphRangesBuilder;
//    static ImVector<ImWchar> glyphRanges;
//    glyphRangesBuilder.AddRanges(fonts->GetGlyphRangesDefault());
//    glyphRangesBuilder.AddRanges(getLangGlyphRanges());
//    if (Data.Font.GlyphRange & GlyphRange_Chinese) glyphRangesBuilder.AddRanges(fonts->GetGlyphRangesChineseFull());
//    if (Data.Font.GlyphRange & GlyphRange_Japanese) glyphRangesBuilder.AddRanges(fonts->GetGlyphRangesJapanese());
//    if (Data.Font.GlyphRange & GlyphRange_Cyrillic) glyphRangesBuilder.AddRanges(fonts->GetGlyphRangesCyrillic());
//    if (Data.Font.GlyphRange & GlyphRange_Korean) glyphRangesBuilder.AddRanges(fonts->GetGlyphRangesKorean());
//    if (Data.Font.GlyphRange & GlyphRange_Thai) glyphRangesBuilder.AddRanges(fonts->GetGlyphRangesThai());
//    if (Data.Font.GlyphRange & GlyphRange_Vietnamese) glyphRangesBuilder.AddRanges(fonts->GetGlyphRangesVietnamese());
//    glyphRangesBuilder.BuildRanges(&glyphRanges);
//    return &glyphRanges[0];
//}
/// <summary>
/// /
/// </summary>
/// <param name="argc"></param>
/// <param name="argv"></param>
/// <returns></returns>
int main(int argc, char** argv)
{
    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = { sizeof(wc), CS_OWNDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"Electrik Calc", nullptr };
    wc.hIcon = LoadIcon(wc.hInstance, MAKEINTRESOURCE(101));
    ::RegisterClassExW(&wc);

    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Калькулятор электроника v 1.0.2(beta)OpenGL3.3", WS_OVERLAPPEDWINDOW, 100, 100, 500, 600, nullptr, nullptr, wc.hInstance, nullptr);
    // Initialize OpenGL
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
    //window_flags |= ImGuiWindowFlags_NoTitleBar;
    //\\io.Fonts->AddFontDefault();
    ImFontAtlas* fonts = ImGui::GetIO().Fonts;
    fonts->AddFontFromFileTTF("WhiteRabbit2.ttf", 20.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    ImFont* font1 = io.Fonts->AddFontFromFileTTF("WhiteRabbit2.ttf", 40.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    ImFont* font2 = io.Fonts->AddFontFromFileTTF("WhiteRabbit2.ttf", 25.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
    ImFont* font3 = io.Fonts->AddFontFromFileTTF("WhiteRabbit2.ttf", 20.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
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
    //
    float fU2 = 0.0f;
    float fU1 = 0.0f;
    float fCoutData = 0.0f;
    int fIdata = 0;
    //
    int fAbuffer = 0;
    int fVbuffer = 0;
    std::string fCOffset = "";
    uint64_t fp = 0;
    uint64_t fOval = 0;
    //

    bool bDarkStyle = true;
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
    std::cout << "GPU: Unit" << std::endl;
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
    GLuint GLArrayBuffer;
    int Ix = 128; int Iy = 128;
    unsigned char imgBuffer;
    bool load;
    try {

        load = LoadTextureFromFile("logo.png", &GLArrayBuffer, &Ix, &Iy, &imgBuffer);
    }
    catch (std::exception_ptr& e) {
        ShowWindow(GetConsoleWindow(), 3);
        std::cout << "cmd /c echo Electrik Calc.exe - [GL] Fatal Error!!logo.png not Loaded!!stack:" << std::endl;

    }



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
        std::cout << "GPU: Unit OK" << std::endl;
        ImGui::Begin("Калькулятор электроника v 1.0.2(beta)OpenGL3.3", &fFr_main, ImGuiWindowFlags_NoTitleBar);
        //std::cout << "GPU: Render Frame:"<<io.Framerate << std::endl;
        ImGuiStyle& style = ImGui::GetStyle();
        if (bDarkStyle) {
            style.WindowRounding = 5.3f;
            style.FrameRounding = 0.0f;
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
            style.FrameRounding = 0.0f;
            style.ScrollbarRounding = 0;
            style.Colors[ImGuiCol_Text] = ImVec4(0.1f, 0.09f, 0.09f, 1.0f);
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
        if (ELC_LaunchBool) {
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
                ImGui::TextColored(ImVec4(0.20f, 1.0f, 0.40f, 1.0f), ((std::string)CPUBrandString + "\n" + fD_gpuModel + ": fps_count:" + std::to_string(io.Framerate) + "\nGL_VER:" + fD_gpuGLVer).c_str());
            }
            if (!fCAboutW) {//

                //ImGui::Image(fGLStackImage)
                ImGui::SetCursorPos(ImVec2(180, 60));
                ImGui::Image(GLArrayBuffer, ImVec2(128, 128));
                ImGui::PushFont(font1);
                ImGui::SetCursorPos(ImVec2(30, 230));
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.33f, 1.0f), "\t   КАЛЬКУЛЯТОР\n\t   ЭЛЕКТРОНИКА");
                ImGui::SetCursorPos(ImVec2(30, 330));
                ImGui::PopFont();
                if (ImGui::Button("Начать", ImVec2(150, 45))) {
                    ELC_LaunchBool = false;
                    fELCButtonsState = true;
                }
                ImGui::SetCursorPos(ImVec2(30, 380));
                if (ImGui::Button("О Программе", ImVec2(150, 45))) {
                    fCAboutW = true;
                }
            }
            if (fCAboutW) {

                ImGui::Text("Горячие клавиши");
                ImGui::Text("[ESC] - Выйти из Приложения");
                ImGui::Text("[T] - Сменить Тему (Темная, Светлая)");
                ImGui::TextColored(ImVec4(0.20f, 1.0f, 0.40f, 1.0f), "[C] - Информация о ПК [T] - Смена темы");
                ImGui::Text("Информация о ПК");
                ImGui::TextColored(ImVec4(0.20f, 1.0f, 0.40f, 1.0f), ((std::string)CPUBrandString + "\n" + fD_gpuModel + ": fps_count:" + std::to_string(io.Framerate) + "\nGL_VER:" + fD_gpuGLVer).c_str());
                ImGui::TextColored(ImVec4(0.20f, 0.40f, 1.0f, 1.0f), ("[--RAM Memory--]\nmem_load: " + std::to_string(fMemStatus(1)) + ":% | " + (std::to_string(fMemStatus(7) - fMemStatus(4))) + ":GB\nmem_free: " + std::to_string(fMemStatus(4)) + " :GB\nmem_total: " + std::to_string(fMemStatus(7)) + " :GB").c_str());
                ImGui::TextColored(ImVec4(0.20f, 0.40f, 1.0f, 1.0f), "\nmain_stack:%p", &main_stack);
                ImGui::Text("Создано  HCPP STUDIO\nБилд 1.0.2\nOpenGL3.3_x64 Создано 12.02.24 8:45");
                ImGui::PushFont(font2);
                ImGui::Text("Огромное спасибо этим людям");
                ImGui::PopFont();
                ImGui::PushFont(font3);
                ImGui::TextColored(ImVec4(0.20f, 0.40f, 1.0f, 1.0f), "Juriuscorp\nDima Xp\nDima Popov");
                ImGui::PopFont();
                if (ImGui::Button("Выход", ImVec2(150, 45))) {
                    fCAboutW = false;
                    //ELC_LaunchBool = false;
                }
            }
            if (bWindow1) {//
                ImGui::Begin("window_0", &bWindow1, ImGuiWindowFlags_NoTitleBar);
                if (ImGui::Button("X", ImVec2(32.0f, 32.0f))) { bWindow1 = false; }
                ImGui::Text("Калькулятор элктроника v 1.0.2");
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
           
        }
        if (fELCButtonsState)
        {
            if (ImGui::Button("Ватты\n в Амперы", ImVec2(150.f, 50.0f)))
            {
                //
                fFrameWA = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Расчет\n ОМА", ImVec2(100.f, 50.0f)))
            {
                //
                fFrameOM = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Расчет\nдиаметра провода", ImVec2(200.f, 50.0f)))
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
            if (ImGui::Button("Расчет\nНоминала Автомата", ImVec2(300.f, 50.0f)))
            {
                fNautomat = true;
            }
            if (ImGui::Button("Расчет Фазосдвигающего\nКонденсатора", ImVec2(300.f, 50.0f)))
            {
                fC_phase = true;
            }
            if (ImGui::Button("Расчет баластного\nКонденсатора", ImVec2(300.f, 50.0f)))
            {
                fC_balast = true;
            }
            if (ImGui::Button("На главную", ImVec2(300.f, 50.0f))) {
                ELC_LaunchBool = true;
                fELCButtonsState = false;
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
                //Uобщая= √((U1 * U1)-(U2 * U2))
               // C = 3200 * I / Uобщая;
                int64_t fUoutData = sqrt((fU1 * fU1) - (fU2 * fU2));
                ImGui::Text("Расчет Баластного Конденсатора", &fC_balast);
                CurrentMemoryBuffer.ClearFreeMemory();
                if (ImGui::Button("X")) {
                    fC_balast = false; fELCButtonsState = true;
                }
                ImGui::SetWindowSize(ImVec2(500.0f, 600.0f));
                ImGui::SetWindowPos(ImVec2(2.0f, 2.0f));
                ImGui::Text(("C:" + std::to_string(fCoutData) + " :мкф").c_str());
                ImGui::InputFloat("U_нагрузки:", &fU2, 0.0f, 10000);
                ImGui::InputFloat("U_Входное :", &fU1, 0.0f, 10000);
                ImGui::InputInt(" I:", &fIdata, 0, 10000);
                if (fU1 < 1 && fU2 < 1) {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.033f, 1.0f), "Ошибка: Деление на ноль нельзя!!\nВведите все данные ");
                }
                if (fU1 < 1 && fU2 > 0) {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.033f, 1.0f), "Ошибка: Деление на ноль нельзя!!\nВведите  U_Входное :");
                }
                if (fU1 > 0 && fU2 < 1) {
                    ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.033f, 1.0f), "Ошибка: Деление на ноль нельзя!!\nВведите U_нагрузки:");
                }
                if (fU1 > 0 && fU2 > 0) {
                    if (ImGui::Button("Рассчитать ")) {
                        fCoutData = (3200 * fIdata / fUoutData);
                    }
                }
                //ImGui::End();
            }
            if (fC_phase) {
                fFrameOM = false;
                fFrameDIA = false;
                fFrameWA = false;
                fFramePH = false;
                fTable = false;
                fNautomat = false;
                fC_balast = false; fELCButtonsState = false;
                if (f4800Offset) {
                    fp = 0; fOval = 4800;
                }
                if (f2800Offset) {
                    fp = 1; fOval = 2800;
                }
                //
                std::string fStrOffsetValue[] = { "Треугольник","Звезда" };
                //
                ImGui::Text("Расчет Фазосдвигающего Конденсатора");
                ImGui::TextColored(ImVec4(0.20f, 1.0f, 0.40f, 1.0f), (std::to_string(io.Framerate)).c_str());
                CurrentMemoryBuffer.ClearFreeMemory();
                if (ImGui::Button("X")) {
                    fC_phase = false; fELCButtonsState = true;
                }
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
                    if (ImGui::Button("Рассчитать ", ImVec2(300.f, 50.0f))) {
                        fCOffset = std::to_string(fOval * fAbuffer / fVbuffer) + "мкф";
                    }
                }
                //   fC_value.innerHTML = parseFloat(fMtype * fDI / fDV) + " ���";
            }
            std::string fStrNominal = "\n1.13 — ток кратковременного отключения\n цепи\n1.45 — ток отключения\n, при котором машина не должна выключаться";
            std::string fStrNominal1 = "\n2.55 - ток, при котором\n машина должна выключиться в течение 1 минуты.\n5 - ток аварийной перегрузки,\n при которой машина должна выключиться в течение 10 секунд \n10 - ток аварийной перегрузки, при которой\n машина должна выключиться выключать";
            int64_t fNin[] = { 13,45,155,400,900 };
            if (fNautomat) {
                fFrameOM = false;
                fFrameDIA = false;
                fFrameWA = false;
                fFramePH = false;
                fTable = false; fELCButtonsState = false;
                //
                //
                ImGui::Text("Расчет номинала автомата (Идея DIMA XP)", &fNautomat);
                ImGui::TextColored(ImVec4(0.20f, 1.0f, 0.40f, 1.0f), (": fps_count:" + std::to_string(io.Framerate)).c_str());
                static  const char* button_idStr[] = { "1.13","1.45","2.55","5","10" };
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
                ImGui::SetWindowSize(ImVec2(500.0f, 600.0f));
                ImGui::SetWindowPos(ImVec2(2.0f, 2.0f));
                if (ImGui::Button("X")) {
                    fNautomat = false; fELCButtonsState = true;
                }
                // parseInt(fNdata) + (parseInt(fNdata) * (fProcentArray[fCPoint] / 100));
                ImGui::Text(("A:" + std::to_string(rs)).c_str());
                if (ImGui::Button((b0_str).c_str(), ImVec2(64.0f, 64.0f))) {
                    /// MessageBeep(0x80000000);
                    fNinOffset = fNin[0]; b0_str = "[1.13]"; b1_str = button_idStr[1]; b2_str = button_idStr[2]; b3_str = button_idStr[3]; b4_str = button_idStr[4];
                }
                ImGui::SameLine();
                if (ImGui::Button((b1_str).c_str(), ImVec2(64.0f, 64.0f))) {
                    fNinOffset = fNin[1]; b1_str = "[1.45]"; b0_str = button_idStr[0]; b2_str = button_idStr[2]; b3_str = button_idStr[3]; b4_str = button_idStr[4];
                }
                ImGui::SameLine();
                if (ImGui::Button((b2_str).c_str(), ImVec2(64.0f, 64.0f))) {
                    fNinOffset = fNin[2]; b2_str = "[2.55]"; b1_str = button_idStr[1]; b0_str = button_idStr[0]; b3_str = button_idStr[3]; b4_str = button_idStr[4];
                }
                ImGui::SameLine();
                if (ImGui::Button((b3_str).c_str(), ImVec2(64.0f, 64.0f))) {
                    fNinOffset = fNin[3]; b3_str = "[ 5 ]"; b1_str = button_idStr[1]; b2_str = button_idStr[2]; b0_str = button_idStr[0]; b4_str = button_idStr[4];
                }
                ImGui::SameLine();
                if (ImGui::Button((b4_str).c_str(), ImVec2(64.0f, 64.0f))) {
                    fNinOffset = fNin[4]; b4_str = "[ 10 ]"; b1_str = button_idStr[1]; b2_str = button_idStr[2]; b3_str = button_idStr[3]; b0_str = button_idStr[0];
                }
                ImGui::Text("Номинал Автомата ");
                ImGui::InputInt("А:", &fN, 1, 100, 0);
                ImGui::Text(("Выбрано:" + std::to_string(fNinOffset) + "Введено:" + std::to_string(fN)).c_str());
                ImGui::TextUnformatted((fStrNominal).c_str());
                ImGui::TextUnformatted((fStrNominal1).c_str());
                if (ImGui::Button("Рассчитать ", ImVec2(150.0f, 50.0f))) {
                    rs = ((float)fNinOffset / 100) * fN + fN;
                    fStrNominal = std::to_string(rs);
                }

                ImGui::SameLine();
                if (ImGui::Button("Показать\nтаблицы сечении", ImVec2(180.0f, 50.0f))) {
                    fFrameOM = false;
                    fFrameDIA = false;
                    fFrameWA = false;
                    fFramePH = false;
                    fNautomat = false;
                    fTable = true; fELCButtonsState = false;
                }
            }
            //
            int64_t amperArrayCp[] = { 11,15,17,23,26,30,41,50,80,100,140,170 };
            int64_t amperArrayAl[] = { 0,0,0,0,21,24,32,39,60,75,105,130 };
            float WattArrayCp[] = { 2.4f,3.3f,3.7f,5.0f,5.7f,6.6f,9.0f,11.0f,17.0f,22.0f,30.0f,37.0f };
            float WattArrayAl[] = { 0.0f,0.0f,6.4f,8.7f,9.8f,11.0f,15.0f,19.0f,30.0f,38.0f,53.0f,64.0f };
            float SectionArray[] = { 0.5f,0.75f,1.0f,1.5f,2.0f,2.5f,4.0f,6.0f,10.0f,16.0f,25.0f,35.0f };
            //

            // 
          //  ImGui::Text("Created by HCPP 2024");
            if (fTable) {
                if (ImGui::Button("X")) {
                    fTable = false; fELCButtonsState = true;
                }
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
                fFrameWA = false; fELCButtonsState = false;
                static float fWatt = 0;
                static float fR = 0;
                static float fVolt = 0;
                static float fValue = 0;
                std::string data_f;
                std::string lvl_0;
                data_f = lvl_0 + std::to_string(fValue);
                ImGui::Text("Расчет мощности тена");
                ImGui::TextColored(ImVec4(0.20f, 1.0f, 0.40f, 1.0f), (": fps_count:" + std::to_string(io.Framerate)).c_str());
                ImGui::SetWindowSize(ImVec2(500.0f, 600.0f));
                ImGui::SetWindowPos(ImVec2(2.0f, 2.0f));
                if (ImGui::Button("X")) {
                    fFramePH = false; fELCButtonsState = true;
                }
                ImGui::Text((data_f).c_str());
                ImGui::InputFloat("V:", &fVolt, 1, 1000, 0);
                ImGui::InputFloat("R:", &fR, 1, 1000, 0);
                //(fPV * fPV / fPR);
                if (ImGui::Button("Рассчитать ", ImVec2(130.0f, 50.0f)))
                {
                    lvl_0 = "W:";
                    fValue = (fVolt * fVolt) / fR;

                }
            }
            if (fFrameWA)
            {
                fFrameOM = false;
                fFrameDIA = false; fELCButtonsState = false;
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
                ImGui::Text("Ватты в Ампера и наоборот");
                ImGui::TextColored(ImVec4(0.20f, 1.0f, 0.40f, 1.0f), (": fps_count:" + std::to_string(io.Framerate)).c_str());
                ImGui::SetWindowSize(ImVec2(500.0f, 600.0f));
                ImGui::SetWindowPos(ImVec2(2.0f, 2.0f));
                if (ImGui::Button("X")) {
                    fFrameWA = false; fELCButtonsState = true;
                }
                ImGui::Text("Ватты в Амперы");
                ImGui::TextColored(ImVec4(150.f, 150.0f, 255.0f, 1.0f), (data_f).c_str());
                ImGui::InputFloat("Вольтаж", &fVoltageW, 1, 1000, 0);
                ImGui::InputFloat("Ватты", &fWatt, 1, 1000, 0);
                if (fWatt > 0 && fVoltageW > 0)
                {
                    if (ImGui::Button("Рассчитать ", ImVec2(130.0f, 50.0f)))
                    {
                        lvl_0 = "Watt";
                        fValue = fWatt / fVoltageW;

                    }
                }
                ImGui::Text("Амперы в Ватты");
                ImGui::TextColored(ImVec4(150.f, 150.0f, 255.0f, 1.0f), (std::to_string(fValue2)).c_str());
                ImGui::InputFloat("U", &fVoltageW2, 1, 1000, 0);
                ImGui::InputFloat("I", &fAmp, 1, 1000, 0);
                if (ImGui::Button("Рассчитать .", ImVec2(130.0f, 50.0f)))
                {
                    lvl_0 = "Амперы";
                    fValue2 = fVoltageW2 * fAmp;
                }
            }
            if (fFrameOM)
            {
                fFrameWA = false;
                fFrameDIA = false; fELCButtonsState = false;
                static float fResistor = 0;
                static float fVoltage = 0;
                static float fAmper = 0;
                ImGui::Text("Расчет Ампер по ОМУ");
                ImGui::TextColored(ImVec4(0.20f, 1.0f, 0.40f, 1.0f), (": fps_count:" + std::to_string(io.Framerate)).c_str());
                ImGui::SetWindowSize(ImVec2(500.0f, 600.0f));
                ImGui::SetWindowPos(ImVec2(2.0f, 2.0f));
                if (ImGui::Button("X")) {
                    fFrameOM = false; fELCButtonsState = true;
                }
                ImGui::Text(("Амперы:" + std::to_string(fAmper)).c_str());
                ImGui::InputFloat("Вольтаж", &fVoltage, 1, 1000, 0);
                ImGui::InputFloat("Сопротивление", &fResistor, 1, 1000, 0);
                if (ImGui::Button("Рассчитать ", ImVec2(150.0f, 60.f)))
                {
                    fAmper = fVoltage / fResistor;
                }
            }
            if (fFrameDIA)
            {
                fFrameOM = false;
                fFrameWA = false; fELCButtonsState = false;
                float fPi = 3.14f;
                float fdata_0 = 0.0f;
                static float fR_mm2 = 0.0f;
                ImGui::Text("Расчет сечения из диаметра");
                ImGui::TextColored(ImVec4(0.20f, 1.0f, 0.40f, 1.0f), (": fps_count:" + std::to_string(io.Framerate)).c_str());
                ImGui::SetWindowSize(ImVec2(500.0f, 600.0f));
                ImGui::SetWindowPos(ImVec2(2.0f, 2.0f));
                if (ImGui::Button("X")) {
                    fFrameDIA = false; fELCButtonsState = true;
                }
                ImGui::Text(("Сечение:" + std::to_string(fR_mm2)).c_str());
                static float fDia_value = 0.0f;
                ImGui::InputFloat("Диаметр", &fDia_value, 0.0f, 100000.0f, 0);
                if (ImGui::Button("Рассчитать ", ImVec2(150.0f, 60.f)))
                {
                    // fDA = (fD0 / 2) * (fD0 / 2) * fPi;
                    // fdata_0 = (fDA * fDA) * fPi;
                    fR_mm2 = ((fDia_value / 2) * (fDia_value / 2) * fPi);
                }
            }
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