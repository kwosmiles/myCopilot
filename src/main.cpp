#include <windows.h>
#include <string>
#include <wrl.h>
#include <wil/com.h>
#include <variant>
#include <map>
#include <tchar.h>

#include "WebView2.h"
#include <fmt/core.h>
#include <fmt/xchar.h>


#include "resource.hpp"
#include "webview.hpp"
#include "utils.hpp"
#include "config.hpp"

LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
HINSTANCE           hInst;
HWND                hMainWin;
NOTIFYICONDATA      nid;
Window_Info         winInfo;


int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
    HANDLE hMutex = CreateMutex(NULL, TRUE, _T("UniqueAppMutex"));

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND hWnd = FindWindow(_T("Copilot_UniqueWindowClass"),NULL);
        if (hWnd) {
            if (IsIconic(hWnd)) {
                ShowWindow(hWnd, SW_RESTORE); // 显示窗口
            }
            else {
                if (!IsWindowVisible(hWnd)) {
                    ShowWindow(hWnd, SW_SHOW);
                }
                else {
                    SetForegroundWindow(hWnd);
                }
            }
        }
        CloseHandle(hMutex);
        return 0;
    }
    hInst = hInstance;
    
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	
    WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInst;
	wcex.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_LOGO));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = CreateSolidBrush(RGB(250,240,231));;
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = _T("Copilot_UniqueWindowClass");
	wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
	succeeded(
        RegisterClassEx(&wcex),
        ERROR,
        L"注册窗口类失败"
    );
    
	hMainWin = CreateWindowW(
		L"Copilot_UniqueWindowClass",
		Config::title.c_str(),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		NULL,
		NULL,
		hInst,
		NULL
	);

    succeeded(
        hMainWin!=nullptr,
        ERROR,
        L"创建窗口失败"
    );
    std::wstring iniFile = Config::userDataFolder + L"/copilot.ini";
    if(LoadWindowInfo(iniFile, winInfo)<0){
        getWindowRect();
        winInfo.state = WIN_STATE::SHOW;
    }

    AddTrayIcon(hMainWin,hInst);

    ShowWindow(hMainWin,nCmdShow);
	UpdateWindow(hMainWin);

    succeeded(
        RegisterHotKey(hMainWin, HOT_KEY1, Config::fsModifiers, Config::vk),
        WARNING,
        fmt::format(L"设置全局快捷键{}+{}失败",Config::fsModifiers,Config::vk)
    );

    reApplyWindowSettings(true);

    //创建webview2环境
	CreateCoreWebView2EnvironmentWithOptions(nullptr,Config::userDataFolder.c_str() , nullptr,pCreateEnvCallback.Get());

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
    //退出事件循环后为结束应用做收尾工作
    UnregisterHotKey(hMainWin, HOT_KEY1);
    SaveWindowInfo(iniFile, winInfo);
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
	return (int)msg.wParam;
}

//事件循环回调函数
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message){
    case WM_ACTIVATE:
        if (wParam != WA_INACTIVE) {
            if (webviewController != nullptr) {
                webviewController->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
            }
        }
        break;
    case WM_HOTKEY:
        if (wParam == 5 || wParam == 6) {
            reApplyWindowSettings();
        }
        break;
    case WM_CLOSE:
        ShowWindow(hMainWin, SW_HIDE);
        return 0;
	case WM_DESTROY:
        getWindowRect();
        Shell_NotifyIcon(NIM_DELETE, &nid);
		PostQuitMessage(0);
		break;
    case WM_MOVE:
    case WM_SIZE:
        RECT rect;
        GetWindowRect(hWnd, &rect);
        setWindowRect(rect);
        if(webviewController != nullptr){
            RECT bounds;
            GetClientRect(hMainWin, &bounds);
            webviewController->put_Bounds(bounds);
        }
        break;
    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP) {
            ShowTrayMenu(hWnd);
        } 
        break;
    case WM_COMMAND:
        WIN_STATE preState;
        switch (LOWORD(wParam)) {
        case ID_TRAY_EXIT:
            DestroyWindow(hWnd);
            break;
        case ID_TRAY_KEEPLEFT:
            preState = winInfo.state;
            winInfo.state=WIN_STATE::LEFT;
            reApplyWindowSettings(preState != winInfo.state);
            break;
        case ID_TRAY_KEEPRIGHT:
            preState = winInfo.state;
            winInfo.state=WIN_STATE::RIGHT;
            reApplyWindowSettings(preState != winInfo.state);
            break;
        case ID_TRAY_RESUME:
            preState = winInfo.state;
            winInfo.state=WIN_STATE::SHOW;
            reApplyWindowSettings(preState != winInfo.state);
            break;
        case ID_TRAY_HIDE:
            preState = winInfo.state;
            winInfo.state = WIN_STATE::SHOW;
            reApplyWindowSettings(preState != winInfo.state);
            break;
        }
        break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}

	return 0;
}