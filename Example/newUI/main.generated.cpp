#include <windows.h>
#include <commctrl.h>
#include <string>
#include <sstream>
#include <thread>
#include <vector>
#include <stdio.h>
#include <bits/stdc++.h>

using namespace std;

static HWND g_limdCMainWindow = NULL;
static constexpr UINT LIMDC_SET_PROGRESS = WM_APP + 1;
static constexpr UINT LIMDC_APPEND_CMD_TEXT = WM_APP + 2;
struct LimdCProgressUpdate { HWND control; int value; };
struct LimdCCmdAppend { HWND control; std::string text; };

static HWND g_button0 = NULL;
static HFONT g_button0Font = NULL;
static COLORREF g_button0Bg = RGB(15, 23, 42);
static COLORREF g_button0TextColor = RGB(224, 242, 254);
static const char* g_button0Text = "List Item A";
static const int g_button0Id = 1000;
static HWND g_button1 = NULL;
static HFONT g_button1Font = NULL;
static COLORREF g_button1Bg = RGB(30, 41, 59);
static COLORREF g_button1TextColor = RGB(187, 247, 208);
static const char* g_button1Text = "List Item B";
static const int g_button1Id = 1001;
static HWND g_button2 = NULL;
static HFONT g_button2Font = NULL;
static COLORREF g_button2Bg = RGB(245, 158, 11);
static COLORREF g_button2TextColor = RGB(255, 255, 255);
static const char* g_button2Text = "List Item C";
static const int g_button2Id = 1002;
static HWND g_list0 = NULL;
static HWND g_list0Title = NULL;
static HFONT g_list0TitleFont = NULL;
static const COLORREF g_list0FrameColor = RGB(203, 213, 225);
static const COLORREF g_list0BgColor = RGB(248, 250, 252);
static const COLORREF g_list0TitleColor = RGB(30, 58, 138);
static HBRUSH g_list0BgBrush = NULL;
static HWND g_text0 = NULL;
static HFONT g_text0Font = NULL;
static const COLORREF g_text0Color = RGB(255, 255, 255);
static const COLORREF g_text0BgColor = RGB(17, 24, 39);
static HBRUSH g_text0BgBrush = NULL;
static HWND g_cmd0 = NULL;
static HFONT g_cmd0Font = NULL;
static const COLORREF g_cmd0Color = RGB(255, 255, 255);
static const COLORREF g_cmd0BgColor = RGB(0, 0, 0);
static HBRUSH g_cmd0BgBrush = NULL;
static HWND g_checkbox0 = NULL;
static HFONT g_checkbox0Font = NULL;
static HWND g_select0 = NULL;
static HFONT g_select0Font = NULL;
static HWND g_progress0 = NULL;
static void LimdCAppendText(HWND control, const char* text) {
    int length = GetWindowTextLengthA(control);
    SendMessageA(control, EM_SETSEL, length, length);
    SendMessageA(control, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(text));
}

static void concprogram();

static void concprogram() {
    std::thread([](){
        std::vector<std::thread> workers;
        workers.emplace_back([](){
            auto* update = new LimdCProgressUpdate{g_progress0, 100};
            PostMessageA(g_limdCMainWindow, LIMDC_SET_PROGRESS, 0, reinterpret_cast<LPARAM>(update));
        });
        workers.emplace_back([](){
            auto* append = new LimdCCmdAppend{g_cmd0, "hallo"};
            PostMessageA(g_limdCMainWindow, LIMDC_APPEND_CMD_TEXT, 0, reinterpret_cast<LPARAM>(append));
        });
        for (std::thread& worker : workers) { worker.join(); }
    }).detach();
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case LIMDC_SET_PROGRESS: {
            auto* update = reinterpret_cast<LimdCProgressUpdate*>(lParam);
            if (update) {
                SendMessageA(update->control, PBM_SETPOS, update->value, 0);
                InvalidateRect(update->control, NULL, TRUE);
                delete update;
            }
            return 0;
        }
        case LIMDC_APPEND_CMD_TEXT: {
            auto* append = reinterpret_cast<LimdCCmdAppend*>(lParam);
            if (append) {
                LimdCAppendText(append->control, append->text.c_str());
                delete append;
            }
            return 0;
        }
        case WM_PAINT: {
            LRESULT result = DefWindowProcA(hwnd, message, wParam, lParam);
            HDC hdc = GetDC(hwnd);
            HBRUSH list0Brush = CreateSolidBrush(g_list0BgColor);
            RECT list0Rect = {40, 35, 372, 263};
            FillRect(hdc, &list0Rect, list0Brush);
            DeleteObject(list0Brush);
            HPEN list0Pen = CreatePen(PS_SOLID, 1, g_list0FrameColor);
            HPEN list0OldPen = reinterpret_cast<HPEN>(SelectObject(hdc, list0Pen));
            MoveToEx(hdc, 40, 35, NULL);
            LineTo(hdc, 372, 35);
            LineTo(hdc, 372, 263);
            LineTo(hdc, 40, 263);
            LineTo(hdc, 40, 35);
            SelectObject(hdc, list0OldPen);
            DeleteObject(list0Pen);
            ReleaseDC(hwnd, hdc);
            return result;
        }
        case WM_CTLCOLORSTATIC: {
            HDC controlDc = reinterpret_cast<HDC>(wParam);
            HWND control = reinterpret_cast<HWND>(lParam);
            if (control == g_list0 || control == g_list0Title) {
                SetBkColor(controlDc, g_list0BgColor);
                SetTextColor(controlDc, g_list0TitleColor);
                return reinterpret_cast<LRESULT>(g_list0BgBrush);
            }
            if (control == g_text0) {
                SetBkColor(controlDc, g_text0BgColor);
                SetTextColor(controlDc, g_text0Color);
                return reinterpret_cast<LRESULT>(g_text0BgBrush);
            }
            if (control == g_cmd0) {
                SetBkColor(controlDc, g_cmd0BgColor);
                SetTextColor(controlDc, g_cmd0Color);
                return reinterpret_cast<LRESULT>(g_cmd0BgBrush);
            }
            break;
        }
        case WM_CTLCOLOREDIT: {
            HDC controlDc = reinterpret_cast<HDC>(wParam);
            HWND control = reinterpret_cast<HWND>(lParam);
            if (control == g_text0) {
                SetBkColor(controlDc, g_text0BgColor);
                SetTextColor(controlDc, g_text0Color);
                return reinterpret_cast<LRESULT>(g_text0BgBrush);
            }
            if (control == g_cmd0) {
                SetBkColor(controlDc, g_cmd0BgColor);
                SetTextColor(controlDc, g_cmd0Color);
                return reinterpret_cast<LRESULT>(g_cmd0BgBrush);
            }
            break;
        }
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
            if (item && item->hwndItem == g_button1) {
                HRGN clipRegion = CreateRoundRectRgn(item->rcItem.left, item->rcItem.top, item->rcItem.right + 1, item->rcItem.bottom + 1, item->rcItem.bottom - item->rcItem.top, item->rcItem.bottom - item->rcItem.top);
                SelectClipRgn(item->hDC, clipRegion);
                HBRUSH brush = CreateSolidBrush(g_button1Bg);
                HBRUSH oldBrush = reinterpret_cast<HBRUSH>(SelectObject(item->hDC, brush));
                HPEN borderPen = CreatePen(PS_SOLID, 1, g_button1TextColor);
                HPEN oldPen = reinterpret_cast<HPEN>(SelectObject(item->hDC, borderPen));
                RoundRect(item->hDC, item->rcItem.left, item->rcItem.top, item->rcItem.right, item->rcItem.bottom, item->rcItem.bottom - item->rcItem.top, item->rcItem.bottom - item->rcItem.top);
                SelectObject(item->hDC, oldPen);
                SelectObject(item->hDC, oldBrush);
                DeleteObject(borderPen);
                DeleteObject(brush);
                SelectClipRgn(item->hDC, NULL);
                DeleteObject(clipRegion);
                SetBkMode(item->hDC, TRANSPARENT);
                SetTextColor(item->hDC, g_button1TextColor);
                HFONT oldFont = reinterpret_cast<HFONT>(SelectObject(item->hDC, g_button1Font));
                SIZE textSize = {};
                GetTextExtentPoint32A(item->hDC, g_button1Text, lstrlenA(g_button1Text), &textSize);
                const int buttonWidth = item->rcItem.right - item->rcItem.left;
                const int buttonHeight = item->rcItem.bottom - item->rcItem.top;
                const int textX = item->rcItem.left + (buttonWidth - textSize.cx) / 2;
                const int textY = item->rcItem.top + (buttonHeight - textSize.cy) / 2;
                TextOutA(item->hDC, textX, textY, g_button1Text, lstrlenA(g_button1Text));
                SelectObject(item->hDC, oldFont);
                if (item->itemState & ODS_FOCUS) { DrawFocusRect(item->hDC, &item->rcItem); }
                return TRUE;
            }
            if (item && item->hwndItem == g_button2) {
                HRGN clipRegion = CreateRoundRectRgn(item->rcItem.left, item->rcItem.top, item->rcItem.right + 1, item->rcItem.bottom + 1, item->rcItem.bottom - item->rcItem.top, item->rcItem.bottom - item->rcItem.top);
                SelectClipRgn(item->hDC, clipRegion);
                HBRUSH brush = CreateSolidBrush(g_button2Bg);
                HBRUSH oldBrush = reinterpret_cast<HBRUSH>(SelectObject(item->hDC, brush));
                HPEN borderPen = CreatePen(PS_SOLID, 1, g_button2TextColor);
                HPEN oldPen = reinterpret_cast<HPEN>(SelectObject(item->hDC, borderPen));
                RoundRect(item->hDC, item->rcItem.left, item->rcItem.top, item->rcItem.right, item->rcItem.bottom, item->rcItem.bottom - item->rcItem.top, item->rcItem.bottom - item->rcItem.top);
                SelectObject(item->hDC, oldPen);
                SelectObject(item->hDC, oldBrush);
                DeleteObject(borderPen);
                DeleteObject(brush);
                SelectClipRgn(item->hDC, NULL);
                DeleteObject(clipRegion);
                SetBkMode(item->hDC, TRANSPARENT);
                SetTextColor(item->hDC, g_button2TextColor);
                HFONT oldFont = reinterpret_cast<HFONT>(SelectObject(item->hDC, g_button2Font));
                SIZE textSize = {};
                GetTextExtentPoint32A(item->hDC, g_button2Text, lstrlenA(g_button2Text), &textSize);
                const int buttonWidth = item->rcItem.right - item->rcItem.left;
                const int buttonHeight = item->rcItem.bottom - item->rcItem.top;
                const int textX = item->rcItem.left + (buttonWidth - textSize.cx) / 2;
                const int textY = item->rcItem.top + (buttonHeight - textSize.cy) / 2;
                TextOutA(item->hDC, textX, textY, g_button2Text, lstrlenA(g_button2Text));
                SelectObject(item->hDC, oldFont);
                if (item->itemState & ODS_FOCUS) { DrawFocusRect(item->hDC, &item->rcItem); }
                return TRUE;
            }
            break;
        }
        case WM_COMMAND: {
            const int commandId = LOWORD(wParam);
            if (commandId == g_button0Id) {
                g_button0TextColor = RGB(255, 238, 88);
                InvalidateRect(g_button0, NULL, TRUE);
                return 0;
            }
            if (commandId == g_button1Id) {
                InvalidateRect(g_button1, NULL, TRUE);
                return 0;
            }
            if (commandId == g_button2Id) {
                InvalidateRect(g_button2, NULL, TRUE);
                return 0;
            }
            break;
        }
        case WM_SIZE:
            InvalidateRect(hwnd, NULL, TRUE);
            RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
            return 0;
        case WM_DESTROY:
            if (g_button0Font) { DeleteObject(g_button0Font); g_button0Font = NULL; }
            if (g_button1Font) { DeleteObject(g_button1Font); g_button1Font = NULL; }
            if (g_button2Font) { DeleteObject(g_button2Font); g_button2Font = NULL; }
            if (g_list0TitleFont) { DeleteObject(g_list0TitleFont); g_list0TitleFont = NULL; }
            if (g_list0BgBrush) { DeleteObject(g_list0BgBrush); g_list0BgBrush = NULL; }
            if (g_text0Font) { DeleteObject(g_text0Font); g_text0Font = NULL; }
            if (g_text0BgBrush) { DeleteObject(g_text0BgBrush); g_text0BgBrush = NULL; }
            if (g_cmd0Font) { DeleteObject(g_cmd0Font); g_cmd0Font = NULL; }
            if (g_cmd0BgBrush) { DeleteObject(g_cmd0BgBrush); g_cmd0BgBrush = NULL; }
            if (g_checkbox0Font) { DeleteObject(g_checkbox0Font); g_checkbox0Font = NULL; }
            if (g_select0Font) { DeleteObject(g_select0Font); g_select0Font = NULL; }
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcA(hwnd, message, wParam, lParam);
}

int main() {
    string txt = "Editable text";
    HINSTANCE hInstance = GetModuleHandleA(NULL);
    INITCOMMONCONTROLSEX icc = { sizeof(INITCOMMONCONTROLSEX), ICC_PROGRESS_CLASS };
    InitCommonControlsEx(&icc);
    auto LimdCToString = [](const auto& value) { std::ostringstream stream; stream << value; return stream.str(); };
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

    HWND mainwindow = CreateWindowExA(0, className, "LimdC UI Components Test", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 820, 560, NULL, NULL, hInstance, NULL);
    if (!mainwindow) {
        MessageBoxA(NULL, "CreateWindowExA failed", "LimdC", MB_ICONERROR);
        return 1;
    }

    g_list0BgBrush = CreateSolidBrush(g_list0BgColor);
    g_list0 = CreateWindowExA(0, "STATIC", "", WS_CHILD | WS_VISIBLE, 40, 35, 332, 228, mainwindow, NULL, hInstance, NULL);
    (void)g_list0;
    g_list0TitleFont = CreateFontA(-18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Microsoft YaHei UI");
    g_list0Title = CreateWindowExA(0, "STATIC", "List Card", WS_CHILD | WS_VISIBLE | SS_CENTER, 56, 51, 300, 34, mainwindow, NULL, hInstance, NULL);
    SendMessageA(g_list0Title, WM_SETFONT, reinterpret_cast<WPARAM>(g_list0TitleFont), TRUE);

    g_button0Font = CreateFontA(-21, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Microsoft YaHei UI");
    g_button0 = CreateWindowExA(0, "BUTTON", g_button0Text, WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 56, 85, 300, 54, mainwindow, reinterpret_cast<HMENU>(g_button0Id), hInstance, NULL);
    g_button1Font = CreateFontA(-21, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Microsoft YaHei UI");
    g_button1 = CreateWindowExA(0, "BUTTON", g_button1Text, WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 56, 139, 300, 54, mainwindow, reinterpret_cast<HMENU>(g_button1Id), hInstance, NULL);
    g_button2Font = CreateFontA(-21, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Microsoft YaHei UI");
    g_button2 = CreateWindowExA(0, "BUTTON", g_button2Text, WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 56, 193, 300, 54, mainwindow, reinterpret_cast<HMENU>(g_button2Id), hInstance, NULL);
    std::string g_text0Value = LimdCToString(txt);
    g_text0BgBrush = CreateSolidBrush(g_text0BgColor);
    g_text0Font = CreateFontA(-18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Microsoft YaHei UI");
    g_text0 = CreateWindowExA(0, "EDIT", g_text0Value.c_str(), WS_CHILD | WS_VISIBLE | ES_LEFT | ES_MULTILINE | WS_BORDER | WS_VSCROLL, 40, 285, 300, 46, mainwindow, NULL, hInstance, NULL);
    SendMessageA(g_text0, WM_SETFONT, reinterpret_cast<WPARAM>(g_text0Font), TRUE);
    g_cmd0Font = CreateFontA(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, "Consolas");
    g_cmd0BgBrush = CreateSolidBrush(g_cmd0BgColor);
    g_cmd0 = CreateWindowExA(0, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL, 380, 35, 360, 150, mainwindow, NULL, hInstance, NULL);
    SendMessageA(g_cmd0, WM_SETFONT, reinterpret_cast<WPARAM>(g_cmd0Font), TRUE);
    std::string g_cmd0Output;
    g_cmd0Output += "LimdC command output\n";
    g_cmd0Output += "checkbox/select/progress ready";
    SetWindowTextA(g_cmd0, g_cmd0Output.c_str());
    g_checkbox0Font = CreateFontA(-15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Microsoft YaHei UI");
    g_checkbox0 = CreateWindowExA(0, "BUTTON", "Enable feature", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 380, 210, 220, 28, mainwindow, NULL, hInstance, NULL);
    SendMessageA(g_checkbox0, WM_SETFONT, reinterpret_cast<WPARAM>(g_checkbox0Font), TRUE);
    SendMessageA(g_checkbox0, BM_SETCHECK, BST_CHECKED, 0);
    g_select0Font = CreateFontA(-15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Microsoft YaHei UI");
    g_select0 = CreateWindowExA(0, "COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 380, 255, 220, 120, mainwindow, NULL, hInstance, NULL);
    SendMessageA(g_select0, WM_SETFONT, reinterpret_cast<WPARAM>(g_select0Font), TRUE);
    SendMessageA(g_select0, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("Light"));
    SendMessageA(g_select0, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("Dark"));
    SendMessageA(g_select0, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("System"));
    SendMessageA(g_select0, CB_SETCURSEL, 1, 0);
    g_progress0 = CreateWindowExA(0, PROGRESS_CLASSA, "", WS_CHILD | WS_VISIBLE, 380, 315, 300, 24, mainwindow, NULL, hInstance, NULL);
    SendMessageA(g_progress0, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SendMessageA(g_progress0, PBM_SETPOS, 65, 0);

    g_limdCMainWindow = mainwindow;
    ShowWindow(mainwindow, SW_SHOWDEFAULT);
    UpdateWindow(mainwindow);
    RedrawWindow(mainwindow, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
    std::thread([](){
        Sleep(1000);
        concprogram();
    }).detach();

    MSG message = {};
    while (GetMessageA(&message, NULL, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }
    return static_cast<int>(message.wParam);
}
