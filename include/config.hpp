#pragma once

#include <string>

// Copilot官网：copilot.microsoft.com

// 配置文件
namespace Config
{
    std::wstring title = L"Copilot";                  //窗口标题
    std::wstring index = L"https://copilot.microsoft.com/";       //打开窗口后加载的主页
    std::wstring userDataFolder = L"Copilot_userData";//用户数据文件夹名，生成在应用程序同目录下
    unsigned int dockedWindowWidth = 600;             //窗口靠边停时，窗口的宽度
    BOOL devToolsEnabled = FALSE;                   //启用浏览器开发者调试模式(F12)
    BOOL contextMenusEnabled = TRUE;                //启用右键默认菜单
    BOOL zoomControlEnabled  = TRUE;                //启用缩放控制(Ctrl+鼠标滚轮)

    //fsModifiers和vk是设置隐藏、显示窗口的快捷键的变量
    //fsModifiers控制的是修饰符，例如win、ctrl、alt、shift...设置多个按键请用|分开
    //vk控制的是虚拟键值，例如a、b、c、1、2、3、鼠标按下...
    //fsModifiers可用的值在：
    //https://learn.microsoft.com/zh-cn/windows/win32/api/winuser/nf-winuser-registerhotkey
    //vk可用的值在：
    //https://learn.microsoft.com/zh-cn/windows/win32/inputdev/virtual-key-codes
    //示例：当fsModifiers=MOD_ALT，vk=0x43时设置的快捷键是Alt+c
    UINT fsModifiers         = MOD_ALT;             //快捷键的修饰符
    UINT vk                  = 0x43;                //快捷键的虚拟键值代码

};