#pragma once

#include <string>
#include <vector>
#include <functional>
#include "config.hpp"
//自定义事件id，事件循环message参数
#define WM_ABDMSG   (WM_USER + 1)
#define WM_TRAYICON (WM_USER + 2)

//托盘右键菜单事件id，事件循环wParam参数

#define ID_TRAY_INDEX 1100
#define ID_TRAY_EXIT 1101
#define ID_TRAY_KEEPLEFT 1102
#define ID_TRAY_KEEPRIGHT 1103
#define ID_TRAY_RESUME 1104
#define ID_TRAY_HIDE 1105
#define ID_TRAY_AUTOSTART 1106
#define ID_TRAY_BASE 200
#define ID_TRAY_COPILOT 201
#define ID_TRAY_CHATGPT 202
#define ID_TRAY_DOUBAO 203
#define ID_TRAY_TONGYI 204
#define ID_TRAY_GEMINI 205
#define ID_TRAY_KIMI 206
#define ID_TRAY_XINGHUO 207
#define ID_TRAY_POE 208
#define ID_TRAY_GROK 209


//用于作为报错处理函数的处理方式值
//当使用WARNING时仅警告
//当使用ERROR时报错并退出应用程序
#define WARNING 1
#define ERROR 0

//快捷键id
#define HOTKEY_ID1 5
#define HOTKEY_IDNUM6 6
#define HOTKEY_IDNUM7 7
#define HOTKEY_IDNUM8 8
#define HOTKEY_IDNUM9 9
#define HOTKEY_IDNUM10 10

//管理窗口状态的四个枚举值
enum class WIN_STATE{
    LEFT = 1,
    RIGHT,
    FLAOT,
    HIDE
};

//保存窗口的位置大小和状态信息
class WINDOWSATUSINFO{
public:
    std::wstring Index;
    LONG IndexState;
    RECT WinRect;
    WIN_STATE state;
    LONG Width;
    BOOLEAN AutoStart;
};

//从资源文件中读取js脚本文件
//用于注入到webview中
std::wstring readAppJsResource();

std::wstring GetIndex();
//从文件中获取Window_Info数据
//失败时返回值<0;
int saveAssistantConfig(AssistantConfig* config, int length);
int loadAssistantConfig(AssistantConfig* config, int length);
int loadWindowInfo(WINDOWSATUSINFO& info);

//将Window_Info数据储存到文件中
//失败时返回值<0;
int saveWindowInfo(const WINDOWSATUSINFO& info);

//从当前窗口获取窗口大小和位置保存到info.WinRect中
void loadRectTo(WINDOWSATUSINFO &info);

//启动一个新线程来延时处理函数，不阻塞当前线程的运行
void setTimeout(std::function<void()> func, int delay);

//判断函数执行结果是否成功，失败弹出相关信息
//当type为WARNING时，仅警告
//当type为ERROR时，报错并退出应用程序
void succeeded(BOOL result,int type,std::wstring msg);

//控制窗口显示的函数
void copilotShow(WIN_STATE state);