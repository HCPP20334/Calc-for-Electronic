
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
#include <string>
#include <stdint.h>


///

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
static bool fFrameWA= false;
static bool fFrameOM = false;
static bool fFrameDIA = false;
int64_t* fA_stack = 0;
static bool fCAboutW;


//
void ResetDeviceWGL();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

//
int main(int argc, char**)
{
    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = { sizeof(wc), CS_OWNDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"JEx64 - OpenGL3", WS_OVERLAPPEDWINDOW, 100, 100, 500, 500, nullptr, nullptr, wc.hInstance, nullptr);

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
    //io.Fonts->AddFontDefault();
    io.Fonts->AddFontFromFileTTF(".\\WhiteRabbit.ttf", 15.0f);
    // Main loop
    bool done = false;
    static bool v_conState = false;
    static bool fFrameAbout = false;
    HWND fH_con = GetConsoleWindow();
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
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin(" Calc for Electic v 0.4", &fFr_main);
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 5.3f;
        style.FrameRounding = 2.3f;
        style.ScrollbarRounding = 0;
        style.Colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 0.90f);
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.01f, 0.01f, 0.02f, 0.80f);
        style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
        style.Colors[ImGuiCol_PopupBg] = ImVec4(0.05f, 0.05f, 0.10f, 0.85f);
        style.Colors[ImGuiCol_Border] = ImVec4(0.70f, 0.70f, 0.70f, 0.65f);
        style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(0.00f, 0.00f, 0.01f, 1.00f);
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
        style.Colors[ImGuiCol_Button] = ImVec4(0.03f, 0.8f, 0.5f, 0.80f);
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
        ImGui::SetWindowSize(ImVec2(400.0f, 400.0f));
        ImGui::SetWindowPos(ImVec2(50.0f, 50.0f));
        ImGui::Text(" Wellcome To Calc !!\n\nBuild 0.4 C++20 OpenGL3\nCreated by HCPP 2024");
        ImGui::BeginMainMenuBar();
        if (ImGui::SmallButton("ABOUT")){fFrameAbout = true;}
        ImGui::EndMainMenuBar();
        ImGui::Checkbox("Show Console", &v_conState);
    
        if (fFrameAbout)
        {
            ImGui::Begin("About",&fFrameAbout);
            ImGui::SetWindowPos(ImVec2(50.0f, 50.0f));
            ImGui::TextColored(ImVec4(150.0f, 150.0f, 255.0f, 1.0f), "Creator: HCPP \n Electic Calc v0.3 Debug OpenGL\n\n Writtein C++20 SSE4.2 version\n\n\n Inspired by the idea : DIMA XP ");
            ImGui::End();
        }
        if (v_conState)
        {
            ShowWindow(fH_con, 0);
        }
        else
        {
            ShowWindow(fH_con, 2);
        }
        if (ImGui::Button("Watt to Amper", ImVec2(200.f, 50.0f)))
        {
            //
            fFrameWA = true;
        }
        if (ImGui::Button("Get Om", ImVec2(200.f, 50.0f)))
        {
            //
            fFrameOM = true;
        }
        if (ImGui::Button("calc diameter wire", ImVec2(200.f, 50.0f)))
        {
            //
            fFrameDIA = true;
        }
        //
        if (fFrameWA)
        {
            fFrameOM = false;
            fFrameDIA = false;
            static int fWatt = 0;
            static bool v_state = false;
            static int fVoltageW = 0;
            static int fAmp = 0;
            static int fValue = 0;
            std::string data_f;
            std::string lvl_0;
            data_f = lvl_0 + std::to_string(fValue);
            ImGui::Begin("Watt to Amper", &fFrameWA);
            if (ImGui::Button("X")) { fFrameWA = false; }
            ImGui::Text("Watt");
            ImGui::Checkbox("Reverd", &v_state);
            ImGui::TextColored(ImVec4(150.f, 150.0f, 255.0f,1.0f), (data_f).c_str());
            ImGui::InputInt("Voltage", &fVoltageW, 1, 1000, 0);
            if (v_state)
            {
                ImGui::InputInt("Watt", &fWatt, 1, 1000, 0);
                if (fWatt > 0 && fVoltageW > 0)
                {
                    if (ImGui::Button("Calculate", ImVec2(130.0f, 50.0f)))
                    {
                        lvl_0 = "Watt";
                        fValue = fWatt / fVoltageW;

                    }
                }
            }
            if (!v_state)
            {
                ImGui::InputInt("Amper", &fAmp, 1, 1000, 0);
                if (ImGui::Button("Calculate", ImVec2(130.0f, 50.0f)))
                {
                    lvl_0 = "Amper";
                    fValue = fVoltageW * fAmp;
                }
            }
            ImGui::End();
        }
        if (fFrameOM)
        {
            fFrameWA = false;
            fFrameDIA = false;
            static int fResistor = 0;
            static int fVoltage = 0;
            static int fAmper = 0;
            ImGui::Begin("Calc AMPER", &fFrameOM);
            if (ImGui::Button("X")) { fFrameOM = false; }
            ImGui::Text(("Amper:" + std::to_string(fAmper)).c_str());
            ImGui::InputInt("Voltage",&fVoltage, 1, 1000, 0);
            ImGui::InputInt("fResistor", &fResistor, 1, 1000, 0);
            if (ImGui::Button("Calculate", ImVec2(150.0f, 60.f)))
            {
                    fAmper =  fResistor / fVoltage;
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
            ImGui::Begin("Diametr Calc", &fFrameDIA);
            if (ImGui::Button("X")) { fFrameDIA = false; }
            ImGui::Text(("Section:" + std::to_string(fR_mm2)).c_str());
            static float fDia_value = 0.0f;
            ImGui::InputFloat("Diametr", &fDia_value, 0.0f, 100000.0f, 0);
            if (ImGui::Button("Calculate", ImVec2(150.0f, 60.f)))
            {
               // fDA = (fD0 / 2) * (fD0 / 2) * fPi;
               // fdata_0 = (fDA * fDA) * fPi;
                fR_mm2 = ((fDia_value / 2) * (fDia_value / 2) * fPi);
            }
            ImGui::End();
        }
        ImGui::End();
           // Rendering
        ImGui::Render();
        glViewport(0, 0, 500, 500);
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

// Helper functions
bool CreateDeviceWGL(HWND hWnd, WGL_WindowData* data)
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

void CleanupDeviceWGL(HWND hWnd, WGL_WindowData* data)
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
