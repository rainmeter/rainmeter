#pragma once

#include <windows.h>
#include <stdio.h>

// 定义传递给回调函数的结构体
typedef struct {
    HMONITOR hTargetMonitor;
    HRGN hRemainingRgn;
    RECT rcWork;
} COVER_CHECK_DATA;

// 枚举窗口的回调函数
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    COVER_CHECK_DATA* pData = (COVER_CHECK_DATA*)lParam;

    // 1. 基本过滤：只处理可见窗口，忽略最小化的窗口
    if (!IsWindowVisible(hwnd) || IsIconic(hwnd)) {
        return TRUE;
    }

    // 2. 忽略特殊窗口：比如任务栏本身、桌面窗口
    char className[256];
    GetClassNameA(hwnd, className, sizeof(className));
    if (strcmp(className, "Shell_TrayWnd") == 0 ||
        strcmp(className, "Progman") == 0 ||
        strcmp(className, "WorkerW") == 0 ||
        strcmp(className, "Windows.UI.Core.CoreWindow") == 0) {
        return TRUE;
    }

    // 3. 获取窗口矩形
    RECT rcWin;
    if (GetWindowRect(hwnd, &rcWin)) {
        // 4. 判断窗口是否在目标显示器上（或者是否有交集）
        HMONITOR hMon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONULL);
        if (hMon == pData->hTargetMonitor) {
            HRGN hWinRgn = CreateRectRgnIndirect(&rcWin);

            int result = CombineRgn(pData->hRemainingRgn, pData->hRemainingRgn, hWinRgn, RGN_DIFF);
            DeleteObject(hWinRgn);
            return result != NULLREGION;
        }
    }

    return TRUE;
}

// 主函数：判断指定坐标所在的显示器桌面上是否被填满
BOOL IsDesktopCoveredOnMonitor(POINT pt) {
    // 1. 获取显示器信息
    HMONITOR hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO mi = { sizeof(mi) };
    if (!GetMonitorInfo(hMonitor, &mi)) return FALSE;

    // 2. 初始化数据
    COVER_CHECK_DATA data;
    data.hTargetMonitor = hMonitor;
    data.rcWork = mi.rcWork;
    // 创建初始区域为整个工作区
    data.hRemainingRgn = CreateRectRgnIndirect(&mi.rcWork);

    // 3. 枚举窗口进行切割
    EnumWindows(EnumWindowsProc, (LPARAM)&data);

    // 4. 检查最终区域是否为空
    RECT rcTest;
    int regionType = GetRgnBox(data.hRemainingRgn, &rcTest);
    BOOL isCovered = (regionType == NULLREGION);

    // 清理
    DeleteObject(data.hRemainingRgn);

    return isCovered;
}

/*
int main() {
    // 检查主显示器（0,0）
    POINT pt = { 0, 0 };
    if (IsDesktopCoveredOnMonitor(pt)) {
        printf("显示器桌面已被完全覆盖，看不见壁纸了。\n");
    } else {
        printf("显示器桌面尚未被完全覆盖，仍能看见部分壁纸。\n");
    }
    return 0;
}
*/
