#pragma once

#include <Windows.h>
#include <string>

class DX12Renderer;

class Win32Application
{
public:
    static int Run(DX12Renderer* pRenderer, HINSTANCE hInstance, int nCmdShow);
    static HWND GetHwnd() { return m_hwnd; }

protected:
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    static HWND m_hwnd;
};
