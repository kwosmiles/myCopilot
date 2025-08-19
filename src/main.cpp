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

#include <locale>
#include <codecvt>
#include "resource.hpp"
#include "webview.hpp"
#include "utils.hpp"
#include "config.hpp"

LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
HINSTANCE           hInst;
HWND                hMainWin;
WINDOWSATUSINFO     winInfo;
HMENU               hMenu, hSubMenu;

AssistantConfig assistantConfig[255];
void ShowCustomInputDialog();
std::string WStringToString(const std::wstring& wstr);
std::wstring StringToWString(const std::string& str);
// 检查是否已经设置为自启动
bool IsStartupSet(const TCHAR* appName) {
    HKEY hKey;
    LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, KEY_READ, &hKey);

    if (result == ERROR_SUCCESS) {
        TCHAR value[MAX_PATH];
        DWORD size = sizeof(value);

        result = RegQueryValueEx(hKey, appName, nullptr, nullptr, (LPBYTE)value, &size);

        RegCloseKey(hKey);

        if (result == ERROR_SUCCESS) {
            // 应用程序已经设置为开机自启动
            return true;
        }
    }

    // 应用程序未设置为开机自启动
    return false;
}

// 设置菜单项的选中状态
void SetAutoStartMenuItem(HMENU hMenu, UINT uID, bool isChecked) {
    UINT uCheck = isChecked ? MF_CHECKED : MF_UNCHECKED;
    CheckMenuItem(hMenu, uID, MF_BYCOMMAND | uCheck);
}

/*AppendMenuW(hSubMenu, MF_STRING, ID_TRAY_COPILOT, L"Copilot");
AppendMenuW(hSubMenu, MF_STRING, ID_TRAY_CHATGPT, L"ChatGPT");
AppendMenuW(hSubMenu, MF_STRING, ID_TRAY_GEMINI, L"Gemini");
AppendMenuW(hSubMenu, MF_STRING, ID_TRAY_POE, L"POE");
AppendMenuW(hSubMenu, MF_STRING, ID_TRAY_DOUBAO, L"豆包");
AppendMenuW(hSubMenu, MF_STRING, ID_TRAY_TONGYI, L"通义千问");
AppendMenuW(hSubMenu, MF_STRING, ID_TRAY_KIMI, L"KIMI");
AppendMenuW(hSubMenu, MF_STRING, ID_TRAY_XINGHUO, L"星火");*/

void SetAssistChecked(HMENU hMenu, int state, bool setting) {
     SetAutoStartMenuItem(hMenu, state, setting);
}

// 设置开机自启动
void SetStartup() {
    HKEY hKey;
    LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, KEY_SET_VALUE, &hKey);

    if (result == ERROR_SUCCESS) {
        const TCHAR* appName = TEXT("AiAssist");
        char buffer[MAX_PATH];
        DWORD size = GetModuleFileNameA(NULL, buffer, MAX_PATH);
        if (size == 0) {
            return;
        }
        const TCHAR* appPath = TEXT(buffer);

        if (strlen(appPath) != 0) {
            RegSetValueEx(hKey, appName, 0, REG_SZ, (BYTE*)appPath, (lstrlen(appPath) + 1) * sizeof(TCHAR));
            RegCloseKey(hKey);
        } 
    }
}
// 取消开机自启动
void RemoveStartup(const TCHAR* appName) {
    HKEY hKey;
    LONG result = RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, KEY_SET_VALUE, &hKey);

    if (result == ERROR_SUCCESS) {
        result = RegDeleteValue(hKey, appName);

        RegCloseKey(hKey);
    }
}

std::wstring StringToWString1(const std::string& str) {
    // Calculate the size needed for the wide string
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    if (size_needed == 0) return L""; // Handle error or empty string case

    std::wstring wstrTo(size_needed, 0);
    // Convert from UTF-8 to wide string
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
    HANDLE hMutex = CreateMutex(NULL, TRUE, _T("UniqueAiAssistMutex"));

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND hWnd = FindWindow(_T("AiAssist_UniqueWindowClass"),NULL);
        if (hWnd) {
            if (IsIconic(hWnd)) {
                ShowWindow(hWnd, SW_RESTORE); // 显示窗口
            } else {
                if (!IsWindowVisible(hWnd)) {
                    ShowWindow(hWnd, SW_SHOW);
                    SetForegroundWindow(hWnd);
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
	wcex.lpszClassName = _T("AiAssist_UniqueWindowClass");
	wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
	succeeded(
        RegisterClassEx(&wcex),
        ERROR,
        L"注册窗口类失败"
    );
    
	hMainWin = CreateWindowW(
		L"AiAssist_UniqueWindowClass",
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

    NOTIFYICONDATA nid;
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hMainWin;
    nid.uID = ID_TRAY_EXIT;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LOGO));;
    strcpy_s(nid.szTip, "AiAssist");
    Shell_NotifyIcon(NIM_ADD, &nid);

    hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_INDEX, L"更换助手");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_KEEPLEFT, L"靠左固定");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_KEEPRIGHT, L"靠右固定");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_RESUME, L"悬浮窗口");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_HIDE, L"隐藏窗口");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_AUTOSTART, L"自动启动");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"退出程序");
    // 创建二级菜单
    hSubMenu = CreatePopupMenu();
    loadAssistantConfig(assistantConfig, 255);
    for (int i = 0; i < 255; i++) {
        if (assistantConfig[i].name.empty()) {
            break;
        }
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        std::wstring wide_str = converter.from_bytes(assistantConfig[i].name);
        AppendMenuW(hSubMenu, MF_STRING, assistantConfig[i].id, wide_str.c_str());
    }   
    ////读取配置文件的设置
    //AppendMenuW(hSubMenu, MF_STRING, ID_TRAY_COPILOT, L"Copilot");
    //AppendMenuW(hSubMenu, MF_STRING, ID_TRAY_CHATGPT, L"ChatGPT");
    //AppendMenuW(hSubMenu, MF_STRING, ID_TRAY_GEMINI, L"Gemini");
    //AppendMenuW(hSubMenu, MF_STRING, ID_TRAY_GROK, L"Grok");
    //AppendMenuW(hSubMenu, MF_STRING, ID_TRAY_POE, L"POE");
    //AppendMenuW(hSubMenu, MF_STRING, ID_TRAY_DOUBAO, L"豆包");
    //AppendMenuW(hSubMenu, MF_STRING, ID_TRAY_TONGYI, L"通义千问");
    //AppendMenuW(hSubMenu, MF_STRING, ID_TRAY_KIMI, L"KIMI");
    //AppendMenuW(hSubMenu, MF_STRING, ID_TRAY_XINGHUO, L"星火");
    // 将二级菜单添加到“更换助手”菜单项下
    ModifyMenuW(hMenu, ID_TRAY_INDEX, MF_BYCOMMAND | MF_POPUP, (UINT_PTR)hSubMenu, L"更换助手");
    
    //设置快捷键
    succeeded(
        RegisterHotKey(hMainWin, HOTKEY_ID1, Config::fsModifiers, Config::vk),
        WARNING,
        L"设置全局快捷键失败"
    );
    succeeded(
        RegisterHotKey(hMainWin, HOTKEY_IDNUM6, Config::fsModifiers, Config::vk1),
        WARNING,
        L"设置全局快捷键失败"
    );
    succeeded(
        RegisterHotKey(hMainWin, HOTKEY_IDNUM7, Config::fsModifiers, Config::vk2),
        WARNING,
        L"设置全局快捷键失败"
    );
    succeeded(
        RegisterHotKey(hMainWin, HOTKEY_IDNUM8, Config::fsModifiers, Config::vk3),
        WARNING,
        L"设置全局快捷键失败"
    );
    succeeded(
        RegisterHotKey(hMainWin, HOTKEY_IDNUM9, Config::fsModifiers, Config::vk4),
        WARNING,
        L"设置全局快捷键失败"
    );
    succeeded(
        RegisterHotKey(hMainWin, HOTKEY_IDNUM10, Config::fsModifiers, Config::vk5),
        WARNING,
        L"设置全局快捷键失败"
    );

    if(loadWindowInfo(winInfo)<0){
        loadRectTo(winInfo);
        winInfo.state = WIN_STATE::FLAOT;
    }

    SetAutoStartMenuItem(hMenu, ID_TRAY_AUTOSTART, IsStartupSet("AiAssist"));
    SetAssistChecked(hMenu, winInfo.IndexState, true);
    if (winInfo.AutoStart) {
        if (!IsStartupSet("AiAssist")) {
            SetStartup();
        }
    }
    copilotShow(winInfo.state);
    //创建webview2环境
	CreateCoreWebView2EnvironmentWithOptions(nullptr,Config::userDataFolder.c_str() , nullptr,pCreateEnvCallback.Get());

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

    
    //退出事件循环后为结束应用做收尾工作
    UnregisterHotKey(hMainWin, HOTKEY_ID1);
    saveWindowInfo(winInfo);
    Shell_NotifyIcon(NIM_DELETE, &nid);
    DestroyMenu(hMenu);
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
	return (int)msg.wParam;
}

//事件循环回调函数
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
    case WM_ACTIVATE:
        if(wParam != WA_INACTIVE){
            if(webviewController != nullptr){
                webviewController->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
            }
        }
        break;
    case WM_HOTKEY:
        switch (wParam) {
            case HOTKEY_ID1:
                if (IsIconic(hMainWin)) {
                    ShowWindow(hMainWin, SW_RESTORE);
                    SetForegroundWindow(hMainWin);
                }
                else {
                    if (IsWindowVisible(hMainWin)) {
                        if (GetForegroundWindow() != hMainWin) {
                            SetForegroundWindow(hMainWin);
                        }
                        else {
                            copilotShow(WIN_STATE::HIDE);
                        }
                    }
                    else {
                        copilotShow(winInfo.state);
                    }
                }
                break;
            case HOTKEY_IDNUM6:
                if (assistantConfig[0].id == 0) {
                    break;
                }
                winInfo.Index = StringToWString(assistantConfig[0].index);
                SetAssistChecked(hMenu, winInfo.IndexState, false);
                winInfo.IndexState = 200;
                SetAssistChecked(hMenu, winInfo.IndexState, true);
                webview->Navigate(winInfo.Index.c_str());
                break;
            case HOTKEY_IDNUM7:
                if (assistantConfig[1].id == 0){
                    break;
                }
                winInfo.Index = StringToWString(assistantConfig[1].index);
                SetAssistChecked(hMenu, winInfo.IndexState, false);
                winInfo.IndexState = 201;
                SetAssistChecked(hMenu, winInfo.IndexState, true);
                webview->Navigate(winInfo.Index.c_str());
                break;
            case HOTKEY_IDNUM8:
                if (assistantConfig[2].id == 0) {
                    break;
                }
                winInfo.Index = StringToWString(assistantConfig[2].index);
                SetAssistChecked(hMenu, winInfo.IndexState, false);
                winInfo.IndexState = 202;
                SetAssistChecked(hMenu, winInfo.IndexState, true);
                webview->Navigate(winInfo.Index.c_str());
                break;
            case HOTKEY_IDNUM9:
                if (assistantConfig[3].id == 0) {
                    break;
                }
                winInfo.Index = StringToWString(assistantConfig[3].index);
                SetAssistChecked(hMenu, winInfo.IndexState, false);
                winInfo.IndexState = 203;
                SetAssistChecked(hMenu, winInfo.IndexState, true);
                webview->Navigate(winInfo.Index.c_str());
                break;
            case HOTKEY_IDNUM10:
                if (assistantConfig[4].id == 0) {
                    break;
                }
                winInfo.Index = StringToWString(assistantConfig[4].index);
                SetAssistChecked(hMenu, winInfo.IndexState, false);
                winInfo.IndexState = 204;
                SetAssistChecked(hMenu, winInfo.IndexState, true);
                webview->Navigate(winInfo.Index.c_str());
                break;
            default:
                break;
        };
        //if (wParam == HOTKEY_ID1) {
        //    //相当于单击托盘图标
        //    SendMessage(hWnd, WM_TRAYICON, 0, WM_LBUTTONUP);
        //}
        break;
    case WM_CLOSE:
        copilotShow(WIN_STATE::HIDE);
        return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
    case WM_SIZE:
        if(webviewController != nullptr){
            RECT bounds;
            GetClientRect(hMainWin, &bounds);
            webviewController->put_Bounds(bounds);
        }
        break;
    case WM_TRAYICON:
            if (lParam == WM_RBUTTONUP) {
                POINT pt;
                GetCursorPos(&pt);
                SetForegroundWindow(hMainWin);
                TrackPopupMenu(hMenu, TPM_RIGHTBUTTON|TPM_LEFTBUTTON|TPM_VERNEGANIMATION, pt.x, pt.y, 0, hMainWin, NULL);
            }
            switch(lParam) {
                case WM_LBUTTONUP:
                    if (IsIconic(hMainWin)) {
                        ShowWindow(hMainWin, SW_RESTORE);
                        SetForegroundWindow(hMainWin);
                    }
                    else {
                        if (IsWindowVisible(hMainWin)) {
                            if (GetForegroundWindow() != hMainWin) {
                                SetForegroundWindow(hMainWin);
                            }
                            else {
                                copilotShow(WIN_STATE::HIDE);
                            }
                        }
                        else {
                            copilotShow(winInfo.state);
                        }
                    }
                    break;
                case WM_RBUTTONUP:
                    break;
                case WM_LBUTTONDBLCLK:
                    break;
                case WM_MBUTTONUP:
                    break;
            }
            break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_TRAY_AUTOSTART:
            winInfo.AutoStart = !winInfo.AutoStart;
            if (winInfo.AutoStart) {
                if (!IsStartupSet("AiAssist")) {
                    SetStartup();
                }
            }
            else {
                if (IsStartupSet("AiAssist")) {
                    RemoveStartup("AiAssist");
                }
            }
            SetAutoStartMenuItem(hMenu, ID_TRAY_AUTOSTART, winInfo.AutoStart);
            break;
        case ID_TRAY_EXIT:
            loadRectTo(winInfo);
            DestroyWindow(hMainWin);
            break;
        case ID_TRAY_KEEPLEFT:
            if(winInfo.state!=WIN_STATE::LEFT){
                winInfo.state=WIN_STATE::LEFT;
                copilotShow(WIN_STATE::LEFT);
            }
            break;
        case ID_TRAY_KEEPRIGHT:
            if(winInfo.state!=WIN_STATE::RIGHT){
                winInfo.state=WIN_STATE::RIGHT;
                copilotShow(WIN_STATE::RIGHT);
            }
            break;
        case ID_TRAY_RESUME:
            winInfo.state=WIN_STATE::FLAOT;
            copilotShow(WIN_STATE::FLAOT);
            break;
        case ID_TRAY_HIDE:
            copilotShow(WIN_STATE::HIDE);
            break;
            default :
                if (LOWORD(wParam) >= 200 && LOWORD(wParam) < ID_TRAY_BASE + 255) {
                    int index = LOWORD(wParam) - ID_TRAY_BASE;
                    winInfo.Index = StringToWString(assistantConfig[index].index);
                    SetAssistChecked(hMenu, winInfo.IndexState, false);
                    winInfo.IndexState = LOWORD(wParam);
                    SetAssistChecked(hMenu, winInfo.IndexState, true);
                    webview->Navigate(winInfo.Index.c_str());
                }
        }
        break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}
    loadRectTo(winInfo);
    saveWindowInfo(winInfo);
	return 0;
}

