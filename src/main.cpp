#include "DX12Renderer.h"
#include "Win32Application.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow)
{
    DX12Renderer renderer(1280, 720, L"DX12 Ray Tracing");
    return Win32Application::Run(&renderer, hInstance, nCmdShow);
}
