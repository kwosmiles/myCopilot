#include <SimpleIni.h>
#include <utils.hpp>
#include <stdlib.h>
#include <shellapi.h>
#include "resource.hpp"
#include <tchar.h>
#include <windows.h>
#include <fmt/core.h>
#include <fmt/xchar.h>
#include "config.hpp"


extern HWND hMainWin;
extern NOTIFYICONDATA nid;
extern Window_Info winInfo;

int LoadWindowInfo(std::wstring iniFile, Window_Info& info) {
    info.Width = 500;
	CSimpleIniA ini;
	SI_Error rc = ini.LoadFile(iniFile.c_str());
	if (rc < 0) { return -1; };
    info.WinRect.left = _ttoi(ini.GetValue("Window", "Left"));
    info.WinRect.top = _ttoi(ini.GetValue("Window", "Top"));
    info.WinRect.right = _ttoi(ini.GetValue("Window", "Right"));
    info.WinRect.bottom = _ttoi(ini.GetValue("Window", "Bottom"));
    info.state = static_cast<WIN_STATE>(_ttoi(ini.GetValue("Window", "State")));
    info.Width = _ttoi(ini.GetValue("Window", "Width"));
    return 1;
}

int SaveWindowInfo(std::wstring iniFile, const Window_Info& info) {
    CSimpleIniA ini;
    TCHAR buffer[256];
    _itot(info.WinRect.left,buffer,10);
    ini.SetValue("Window", "Left", buffer);
    _itot(info.WinRect.top,buffer,10);
    ini.SetValue("Window", "Top", buffer);
    _itot(info.WinRect.right,buffer,10);
    ini.SetValue("Window", "Right", buffer);
    _itot(info.WinRect.bottom,buffer,10);
    ini.SetValue("Window", "Bottom", buffer);

    _itot((int)info.state,buffer,10);
    ini.SetValue("Window", "State", buffer);
    _itot((int)info.Width, buffer, 10);
    ini.SetValue("Window", "Width", buffer);

    SI_Error rc = ini.SaveFile(iniFile.c_str());
	if (rc < 0) { /* handle error */ };

    return true;
}

std::wstring readAppJsResource() {
    HMODULE hModule = GetModuleHandle(NULL);
    HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(IDI_SCRIPT), RT_RCDATA);
    HGLOBAL hLoadedResource = LoadResource(hModule, hResource);
    DWORD resourceSize = SizeofResource(hModule, hResource);
    const char* pResourceData = static_cast<const char*>(LockResource(hLoadedResource));
    auto fileContent = std::vector<char>(pResourceData, pResourceData + resourceSize);
    return std::wstring(pResourceData, pResourceData + resourceSize);
}

void AddTrayIcon(HWND hWnd,HINSTANCE hInstance) {
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = ID_TRAY_EXIT;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LOGO));;
    strcpy_s(nid.szTip, "Copilot");

    Shell_NotifyIcon(NIM_ADD, &nid);
}

void ShowTrayMenu(HWND hWnd) {
    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_KEEPLEFT, L"靠左固定");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_KEEPRIGHT, L"靠右固定");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_RESUME, L"悬浮窗口");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_HIDE, L"隐藏窗口");
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"退出程序");

    SetForegroundWindow(hWnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
    DestroyMenu(hMenu);
}

// 判断窗口是否在最前面
bool IsWindowInForeground(HWND hWnd) {
    HWND hForegroundWnd = GetForegroundWindow();
    return (hWnd == hForegroundWnd);
}

void getWindowRect() {
    LONG_PTR style = GetWindowLongPtr(hMainWin, GWL_STYLE);
    if((style & WS_THICKFRAME) && (style & WS_CAPTION)){
        GetWindowRect(hMainWin, &winInfo.WinRect);
    }
}

void setWindowRect(RECT rect) {
    if (winInfo.state == WIN_STATE::SHOW)
        winInfo.WinRect = rect;
}

void showEdgeWindow(HWND hWnd , RECT rect , bool withFrame,bool left) {
    LONG_PTR style = GetWindowLongPtr(hWnd, GWL_STYLE);
    if (withFrame) {
        style |= WS_THICKFRAME | WS_CAPTION;
    }
    else {
        style &= ~WS_THICKFRAME & ~WS_CAPTION;
    }
    SetWindowLongPtr(hWnd, GWL_STYLE, style);
    APPBARDATA abd;
    abd.cbSize = sizeof(APPBARDATA);
    abd.hWnd = hWnd;
    abd.uCallbackMessage = WM_ABDMSG;
    SHAppBarMessage(ABM_NEW, &abd);
    if (left) {
        abd.uEdge = ABE_LEFT;
    }
    else {
        abd.uEdge = ABE_RIGHT;
    }
    abd.rc = rect;
    SHAppBarMessage(ABM_QUERYPOS, &abd); 
    SHAppBarMessage(ABM_SETPOS, &abd);

    SetWindowPos(hWnd, HWND_TOP, rect.left, rect.top,rect.right - rect.left, rect.bottom - rect.top, SWP_SHOWWINDOW);
}

void hideWindow(HWND hWnd) {
    APPBARDATA abd;
    abd.cbSize = sizeof(APPBARDATA);
    abd.hWnd = hWnd;
    SHAppBarMessage(ABM_REMOVE, &abd);
    SetWindowPos(hWnd, HWND_TOP, winInfo.WinRect.left, winInfo.WinRect.top,
        winInfo.WinRect.right - winInfo.WinRect.left, winInfo.WinRect.bottom - winInfo.WinRect.top, SWP_HIDEWINDOW);
}

void reApplyWindowSettings(bool forceShowWnd){
    RECT screenRc;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &screenRc, 0);
    LONG_PTR style = GetWindowLongPtr(hMainWin, GWL_STYLE);
    switch (winInfo.state){
    case WIN_STATE::LEFT:
        if (!IsWindowVisible(hMainWin) || forceShowWnd) {
            screenRc.right = screenRc.left + winInfo.Width;
            showEdgeWindow(hMainWin, screenRc, false, true); 
        }
        else {
            hideWindow(hMainWin);
        }
        break;
    case WIN_STATE::RIGHT:
        if (!IsWindowVisible(hMainWin) || forceShowWnd) {
            screenRc.left = screenRc.right - winInfo.Width;
            showEdgeWindow(hMainWin, screenRc, false, false);
        }
        else {
            hideWindow(hMainWin);
        }
        break;
    case WIN_STATE::SHOW:
        style |= WS_THICKFRAME | WS_CAPTION;
        SetWindowLongPtr(hMainWin, GWL_STYLE, style);
        if (!IsWindowVisible(hMainWin) || forceShowWnd) {
            APPBARDATA abd;
            abd.cbSize = sizeof(APPBARDATA);
            abd.hWnd = hMainWin;
            SHAppBarMessage(ABM_REMOVE, &abd);
            SetWindowPos(hMainWin, HWND_TOP, winInfo.WinRect.left, winInfo.WinRect.top,
                winInfo.WinRect.right - winInfo.WinRect.left, winInfo.WinRect.bottom - winInfo.WinRect.top,
                SWP_SHOWWINDOW); 
            SetForegroundWindow(hMainWin);
        }
        else {
            if (IsWindowInForeground(hMainWin)) {
                SetWindowPos(hMainWin, HWND_TOP, winInfo.WinRect.left, winInfo.WinRect.top,
                    winInfo.WinRect.right - winInfo.WinRect.left, winInfo.WinRect.bottom - winInfo.WinRect.top, SWP_HIDEWINDOW);
            }
            else {
                SetForegroundWindow(hMainWin);
            }
        }
        break;
    case WIN_STATE::HIDE:
        style |= WS_THICKFRAME | WS_CAPTION;
        SetWindowLongPtr(hMainWin, GWL_STYLE, style);
        if (IsWindowVisible(hMainWin)) {
            SetWindowPos(hMainWin, HWND_TOP, winInfo.WinRect.left, winInfo.WinRect.top,
                winInfo.WinRect.right - winInfo.WinRect.left, winInfo.WinRect.bottom - winInfo.WinRect.top, SWP_HIDEWINDOW);
        }
        break;
    default:
        break;
    }
}

void succeeded(BOOL result,int type,std::wstring msg){
    if(!result){
        MessageBoxW(NULL,msg.c_str(),fmt::format(L"错误码:{}",GetLastError()).c_str(),MB_OK|(type?MB_ICONWARNING:MB_ICONERROR));
        if(type==ERROR){
            PostQuitMessage(0);
        }
    }
}