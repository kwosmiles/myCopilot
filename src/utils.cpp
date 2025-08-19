#include <SimpleIni.h>
#include <utils.hpp>
#include <stdlib.h>
#include <shellapi.h>
#include "resource.hpp"
#include <tchar.h>
#include <windows.h>
#include <fstream>
#include <sstream>
#include "config.hpp"
#include <iostream>

#include <map>
#include <string>
#include <vector>
extern HWND hMainWin;
extern NOTIFYICONDATA nid;
extern WINDOWSATUSINFO winInfo;
extern AssistantConfig assistantConfig;

std::wstring StringToWString(const std::string &str)
{
    // 计算所需缓冲区大小
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    // 执行转换
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// std::wstring StringToWString(const std::string& str) {
//     iconv_t cd = iconv_open("WCHAR_T", "UTF-8");
//     if (cd == (iconv_t)-1) throw std::runtime_error("iconv_open failed");
//
//     size_t inBytesLeft = str.size();
//     size_t outBytesLeft = str.size() * sizeof(wchar_t); // 估算最大输出空间
//     std::vector<char> outBuffer(outBytesLeft + sizeof(wchar_t));
//
//     char* inPtr = const_cast<char*>(str.c_str());
//     char* outPtr = &outBuffer[0];
//
//     if (iconv(cd, &inPtr, &inBytesLeft, &outPtr, &outBytesLeft) == (size_t)-1) {
//         iconv_close(cd);
//         throw std::runtime_error("iconv failed");
//     }
//
//     iconv_close(cd);
//
//     return std::wstring(reinterpret_cast<wchar_t*>(&outBuffer[0]), (outBuffer.size() - outBytesLeft) / sizeof(wchar_t));
// }

// std::wstring StringToWString(const std::string& str) {
//     std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
//     return myconv.from_bytes(str);
//     //std::wstring wstr(str.begin(), str.end());
//     //return wstr;
// }

std::string WStringToString(const std::wstring &wstr)
{
    std::string str(wstr.begin(), wstr.end());
    return str;
}

// 获取当前目录路径
std::string GetCurrentDirectoryPath()
{
    char buffer[MAX_PATH];
    DWORD size = GetModuleFileNameA(NULL, buffer, MAX_PATH);
    if (size == 0)
    {
        return "";
    }

    std::string path(buffer);
    size_t pos = path.find_last_of("\\/");
    return (std::string::npos == pos) ? "" : path.substr(0, pos);
}

int saveAssistantConfig(AssistantConfig *config, int length)
{
    std::string currentPath = GetCurrentDirectoryPath();
    std::string iniFilePath = currentPath + "\\web.ini";
    CSimpleIniA ini;
    TCHAR buffer[256];
    std::string section = "Web";
    for (int i = 0; i < length; i++)
    {
        std::string key = config[i].name; // WStringToString().c_str();
        std::string value = config[i].index;
        ini.SetValue(section.c_str(), key.c_str(), value.c_str());
    }
    SI_Error rc = ini.SaveFile(iniFilePath.c_str());
    if (rc < 0)
    { /* handle error */
    };
    return 0;
}

int loadAssistantConfig(AssistantConfig* config, int length)
{
    std::string currentPath = GetCurrentDirectoryPath();
    std::string iniFilePath = currentPath + "\\web.ini";

    // 尝试加载 ini 文件以检查是否存在
    CSimpleIniA ini(true, false, false);
    SI_Error rc = ini.LoadFile(iniFilePath.c_str());
    if (rc < 0)
    {
        config[0].id = 200;
        config[0].name = "1-通义";
        config[0].index = "https://tongyi.aliyun.com/";
        saveAssistantConfig(config, 1);
        return 0;
    }

    // 直接读取 ini 文件，按行解析以保留原始顺序
    std::ifstream file(iniFilePath);
    if (!file.is_open())
    {
        std::cerr << "Failed to open " << iniFilePath << std::endl;
        return -1;
    }

    const char* section = "Web";
    int baseId = 200;
    int currentIndex = 0;
    std::string line;
    bool inSection = false;

    while (std::getline(file, line) && currentIndex < length)
    {
        // 去除首尾空白
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        // 跳过空行
        if (line.empty())
            continue;

        // 检查是否进入 [Web] 节
        if (line == "[Web]")
        {
            inSection = true;
            continue;
        }

        // 仅处理 [Web] 节中的键值对
        if (inSection)
        {
            // 检查是否是键值对（包含 '='）
            size_t pos = line.find('=');
            if (pos != std::string::npos)
            {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);

                // 去除键和值的首尾空白
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);

                if (!key.empty() && !value.empty())
                {
                    config[currentIndex].id = baseId + currentIndex;
                    config[currentIndex].name = key;
                    config[currentIndex].index = value;
                    currentIndex++;
                }
            }
        }
    }

    file.close();
    std::cout << "Loaded " << currentIndex << " key-value pairs from [" << section << "]" << std::endl;
    return 0;
}

int loadWindowInfo(WINDOWSATUSINFO &info)
{
    CSimpleIniA ini;
    info.AutoStart = false;
    info.Width = 500;
    info.Index = L"https://tongyi.aliyun.com/";
    info.IndexState = 200;
    info.WinRect.left = 100;
    info.WinRect.top = 100;
    info.WinRect.right = 1100;
    info.WinRect.bottom = 900;
    std::string currentPath = GetCurrentDirectoryPath();
    std::string iniFilePath = currentPath + "\\aiassist.ini";
    SI_Error rc = ini.LoadFile(iniFilePath.c_str());
    if (rc < 0)
    {
        saveWindowInfo(info);
        return -1;
    };

    const char *pVal = ini.GetValue("Window", "Index", "https://tongyi.aliyun.com/");
    info.IndexState = _ttoi(ini.GetValue("Window", "IndexState"));
    info.Index = StringToWString(pVal);
    info.WinRect.left = _ttoi(ini.GetValue("Window", "Left"));
    info.WinRect.top = _ttoi(ini.GetValue("Window", "Top"));
    info.WinRect.right = _ttoi(ini.GetValue("Window", "Right"));
    info.WinRect.bottom = _ttoi(ini.GetValue("Window", "Bottom"));
    info.state = static_cast<WIN_STATE>(_ttoi(ini.GetValue("Window", "State")));
    info.Width = _ttoi(ini.GetValue("Window", "Width"));
    const char *autoStartStr = ini.GetValue("Window", "AutoStart", "0");
    info.AutoStart = (std::atoi(autoStartStr) == 1);
    return 1;
}

std::wstring GetIndex()
{
    return winInfo.Index;
}

int saveWindowInfo(const WINDOWSATUSINFO &info)
{
    std::string currentPath = GetCurrentDirectoryPath();
    std::string iniFilePath = currentPath + "\\aiassist.ini";
    CSimpleIniA ini;
    TCHAR buffer[256];
    std::string value = WStringToString(info.Index);
    ini.SetValue("Window", "Index", value.c_str());
    _itot((int)info.IndexState, buffer, 10);
    ini.SetValue("Window", "IndexState", buffer);
    _itot(info.WinRect.left, buffer, 10);
    ini.SetValue("Window", "Left", buffer);
    _itot(info.WinRect.top, buffer, 10);
    ini.SetValue("Window", "Top", buffer);
    _itot(info.WinRect.right, buffer, 10);
    ini.SetValue("Window", "Right", buffer);
    _itot(info.WinRect.bottom, buffer, 10);
    ini.SetValue("Window", "Bottom", buffer);

    _itot((int)info.state, buffer, 10);
    ini.SetValue("Window", "State", buffer);
    _itot((int)info.Width, buffer, 10);
    ini.SetValue("Window", "Width", buffer);
    ini.SetValue("Window", "AutoStart", info.AutoStart ? "1" : "0");
    SI_Error rc = ini.SaveFile(iniFilePath.c_str());
    if (rc < 0)
    { /* handle error */
    };

    return true;
}

std::wstring readAppJsResource()
{
    HMODULE hModule = GetModuleHandle(NULL);
    HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(IDI_SCRIPT), RT_RCDATA);
    HGLOBAL hLoadedResource = LoadResource(hModule, hResource);
    DWORD resourceSize = SizeofResource(hModule, hResource);
    const char *pResourceData = static_cast<const char *>(LockResource(hLoadedResource));
    auto fileContent = std::vector<char>(pResourceData, pResourceData + resourceSize);
    return std::wstring(pResourceData, pResourceData + resourceSize);
}

void loadRectTo(WINDOWSATUSINFO &info)
{
    if (!IsIconic(hMainWin) && IsWindowVisible(hMainWin) && winInfo.state == WIN_STATE::FLAOT)
    {
        GetWindowRect(hMainWin, &info.WinRect);
    }
}

void setTimeout(std::function<void()> func, int delay)
{
    auto threadFunc = [](void *param) -> DWORD
    {
        auto [func, delay] = *static_cast<std::pair<std::function<void()>, int> *>(param);
        Sleep(delay);
        func();
        delete static_cast<std::pair<std::function<void()>, int> *>(param);
        return 0;
    };
    auto *param = new std::pair<std::function<void()>, int>(func, delay);
    CreateThread(NULL, 0, threadFunc, param, 0, NULL);
}
void succeeded(BOOL result, int type, std::wstring msg)
{
    if (!result)
    {
        std::wstring errorMessage = L"错误码: " + std::to_wstring(GetLastError());
        MessageBoxW(NULL, msg.c_str(), errorMessage.c_str(), MB_OK | (type ? MB_ICONWARNING : MB_ICONERROR));
        if (type == ERROR)
        {
            PostQuitMessage(0);
        }
    }
}

void copilotShow(WIN_STATE state)
{
    APPBARDATA abd;
    abd.cbSize = sizeof(APPBARDATA);
    abd.uCallbackMessage = WM_ABDMSG;
    abd.hWnd = hMainWin;
    SHAppBarMessage(ABM_REMOVE, &abd);
    switch (state)
    {
    case WIN_STATE::HIDE:
        loadRectTo(winInfo);
        ShowWindow(hMainWin, SW_HIDE);
        break;
    case WIN_STATE::FLAOT:
    {
        int x = winInfo.WinRect.left;
        int y = winInfo.WinRect.top;
        int w = winInfo.WinRect.right - winInfo.WinRect.left;
        int h = winInfo.WinRect.bottom - winInfo.WinRect.top;
        SetWindowLongPtr(hMainWin, GWL_STYLE, WS_OVERLAPPEDWINDOW);
        SetWindowLongPtr(hMainWin, GWL_EXSTYLE, WS_EX_OVERLAPPEDWINDOW);
        SetWindowPos(hMainWin, HWND_NOTOPMOST, x, y, w, h, SWP_FRAMECHANGED);
        ShowWindow(hMainWin, SW_SHOW);
        SetForegroundWindow(hMainWin);
        break;
    }

    default:
    {
        loadRectTo(winInfo);
        RECT workAreaRc;
        SystemParametersInfo(SPI_GETWORKAREA, 0, &workAreaRc, 0);
        if (state == WIN_STATE::LEFT)
        {
            workAreaRc.right = workAreaRc.left + winInfo.Width;
            // abd.uEdge = ABE_LEFT;
        }
        else if (state == WIN_STATE::RIGHT)
        {
            workAreaRc.left = workAreaRc.right - winInfo.Width;
            // abd.uEdge = ABE_RIGHT;
        }
        abd.rc = workAreaRc;
        // SHAppBarMessage(ABM_NEW, &abd);
        SHAppBarMessage(ABM_QUERYPOS, &abd);
        SHAppBarMessage(ABM_SETPOS, &abd);
        SetWindowLongPtr(hMainWin, GWL_STYLE, WS_POPUP);
        SetWindowLongPtr(hMainWin, GWL_EXSTYLE, WS_EX_TOOLWINDOW);
        int x = abd.rc.left;
        int y = abd.rc.top;
        int w = abd.rc.right - abd.rc.left;
        int h = abd.rc.bottom - abd.rc.top;
        SetWindowPos(hMainWin, HWND_TOP, x, y, w, h, SWP_FRAMECHANGED);
        if (state == WIN_STATE::LEFT)
        {
            AnimateWindow(hMainWin, 100, AW_HOR_POSITIVE);
        }
        else if (state == WIN_STATE::RIGHT)
        {
            AnimateWindow(hMainWin, 100, AW_HOR_NEGATIVE);
        }
        SetForegroundWindow(hMainWin);
        break;
    }
    }
}