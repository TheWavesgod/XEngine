#include "Win32Application.h"
#include "DX12Renderer.h"

HWND Win32Application::m_hwnd = nullptr;

int Win32Application::Run(DX12Renderer *pRenderer, HINSTANCE hInstance, int nCmdShow)
{
    // Parse the command line parameters
    int argc;
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    LocalFree(argv);

    // Initialize the window class.
    WNDCLASSEXW windowClass = {0};
    windowClass.cbSize = sizeof(WNDCLASSEXW);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.lpszClassName = L"DX12RayTracingClass";
    RegisterClassExW(&windowClass);

    RECT windowRect = {0, 0, 1280, 720};
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    // Create the window and store a handle to it.
    m_hwnd = CreateWindowW(windowClass.lpszClassName, L"DX12 Ray Tracing", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                           CW_USEDEFAULT, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
                           nullptr, // We have no parent window.
                           nullptr, // We aren't using menus.
                           hInstance, pRenderer);

    // Initialize the sample. OnInit is defined in the DX12Renderer class.
    pRenderer->OnInit();

    ShowWindow(m_hwnd, nCmdShow);

    // Main sample loop.
    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        // Process any messages in the queue.
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            pRenderer->OnUpdate();
            pRenderer->OnRender();
        }
    }

    pRenderer->OnDestroy();

    // Return this part of the WM_QUIT message to Windows.
    return static_cast<char>(msg.wParam);
}

LRESULT CALLBACK Win32Application::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    DX12Renderer *pRenderer = reinterpret_cast<DX12Renderer *>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));

    switch (message)
    {
    case WM_CREATE:
    {
        // Save the DX12Renderer* passed in to CreateWindow.
        LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
    }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    // Handle any messages the switch statement didn't.
    return DefWindowProcW(hWnd, message, wParam, lParam);
}
