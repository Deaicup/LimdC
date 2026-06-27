#include <windows.h>
#include <stdio.h>
#include <bits/stdc++.h>

static HWND g_button0 = NULL;
static HFONT g_button0Font = NULL;
static COLORREF g_button0Bg = RGB(254, 16, 239);
static COLORREF g_button0TextColor = RGB(0, 0, 0);
static const char* g_button0Text = "A button";
static const int g_button0Id = 1000;

static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_DRAWITEM: {
            LPDRAWITEMSTRUCT item = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
            if (item && item->hwndItem == g_button0) {
                HRGN clipRegion = CreateRoundRectRgn(item->rcItem.left, item->rcItem.top, item->rcItem.right + 1, item->rcItem.bottom + 1, item->rcItem.bottom - item->rcItem.top, item->rcItem.bottom - item->rcItem.top);
                SelectClipRgn(item->hDC, clipRegion);
                HBRUSH brush = CreateSolidBrush(g_button0Bg);
                HBRUSH oldBrush = reinterpret_cast<HBRUSH>(SelectObject(item->hDC, brush));
                HPEN borderPen = CreatePen(PS_SOLID, 1, g_button0TextColor);
                HPEN oldPen = reinterpret_cast<HPEN>(SelectObject(item->hDC, borderPen));
                RoundRect(item->hDC, item->rcItem.left, item->rcItem.top, item->rcItem.right, item->rcItem.bottom, item->rcItem.bottom - item->rcItem.top, item->rcItem.bottom - item->rcItem.top);
                SelectObject(item->hDC, oldPen);
                SelectObject(item->hDC, oldBrush);
                DeleteObject(borderPen);
                DeleteObject(brush);
                SelectClipRgn(item->hDC, NULL);
                DeleteObject(clipRegion);
                SetBkMode(item->hDC, TRANSPARENT);
                SetTextColor(item->hDC, g_button0TextColor);
                HFONT oldFont = reinterpret_cast<HFONT>(SelectObject(item->hDC, g_button0Font));
                SIZE textSize = {};
                GetTextExtentPoint32A(item->hDC, g_button0Text, lstrlenA(g_button0Text), &textSize);
                const int buttonWidth = item->rcItem.right - item->rcItem.left;
                const int buttonHeight = item->rcItem.bottom - item->rcItem.top;
                const int textX = item->rcItem.left + (buttonWidth - textSize.cx) / 2;
                const int textY = item->rcItem.top + (buttonHeight - textSize.cy) / 2;
                TextOutA(item->hDC, textX, textY, g_button0Text, lstrlenA(g_button0Text));
                SelectObject(item->hDC, oldFont);
                if (item->itemState & ODS_FOCUS) { DrawFocusRect(item->hDC, &item->rcItem); }
                return TRUE;
            }
            break;
        }
        case WM_COMMAND: {
            const int commandId = LOWORD(wParam);
            if (commandId == g_button0Id) {
                g_button0TextColor = RGB(1, 254, 1);
                InvalidateRect(g_button0, NULL, TRUE);
                return 0;
            }
            break;
        }
        case WM_DESTROY:
            if (g_button0Font) { DeleteObject(g_button0Font); g_button0Font = NULL; }
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcA(hwnd, message, wParam, lParam);
}

int main() {
    HINSTANCE hInstance = GetModuleHandleA(NULL);
    const char* className = "LimdCWindowClass";
    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = className;
    if (!RegisterClassExA(&wc)) {
        MessageBoxA(NULL, "RegisterClassExA failed", "LimdC", MB_ICONERROR);
        return 1;
    }

    HWND mainwindow = CreateWindowExA(0, className, "title", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 300, 500, NULL, NULL, hInstance, NULL);
    if (!mainwindow) {
        MessageBoxA(NULL, "CreateWindowExA failed", "LimdC", MB_ICONERROR);
        return 1;
    }

    g_button0Font = CreateFontA(-30, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Microsoft YaHei UI");
    g_button0 = CreateWindowExA(0, "BUTTON", g_button0Text, WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 10, 5, 250, 100, mainwindow, reinterpret_cast<HMENU>(g_button0Id), hInstance, NULL);

    ShowWindow(mainwindow, SW_SHOWDEFAULT);
    UpdateWindow(mainwindow);

    MSG message = {};
    while (GetMessageA(&message, NULL, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }
    return static_cast<int>(message.wParam);
}
