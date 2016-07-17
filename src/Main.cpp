#include <windows.h>
#include "Vk.hpp"
#include "Application.hpp"

//void assert(bool flag, char *msg = "") {
//    if (!flag) {
//        OutputDebugStringA("ASSERT: ");
//        OutputDebugStringA(msg);
//        OutputDebugStringA("\n");
//        int *base = 0;
//        *base = 1;
//    }
//}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    Ls::Application::hInstance = hInstance;
    Ls::Application::width = 800;
    Ls::Application::height = 600;

    Ls::Application::CreateMainWindow();
    vk::LoadVulkanLibrary();
    vk::LoadExportedEntryPoints();
    vk::LoadGlobalLevelEntryPoints();
    Ls::Application::CreateInstance();
    vk::LoadInstanceLevelEntryPoints(Ls::Application::instance, Ls::Application::extensions);
    Ls::Application::CreateDevice();
    vk::LoadDeviceLevelEntryPoints(Ls::Application::device, Ls::Application::extensions);
    
    while (Ls::Application::Update()) {};
    return Ls::Application::msg.wParam;

    Ls::Application::FreeDevice();
    Ls::Application::FreeInstance();
    vk::UnloadVulkanLibrary();
}

