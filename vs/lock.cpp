#include <windows.h>
#include <string>

// --- KLAVYE ENGELLEME MODÜLÜ ---
HHOOK hHook = NULL;
std::string inputBuffer = "";
const std::string SECRET_PASS = "0123";

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;
        char key = (char)p->vkCode;
        if (key >= '0' && key <= '9') {
            inputBuffer += key;
            if (inputBuffer.length() > SECRET_PASS.length()) inputBuffer.erase(0, 1);
            if (inputBuffer == SECRET_PASS) PostQuitMessage(0); 
        }
        bool alt = (p->flags & LLKHF_ALTDOWN) || (GetAsyncKeyState(VK_MENU) & 0x8000);
        bool ctrl = GetAsyncKeyState(VK_CONTROL) & 0x8000;
        if (p->vkCode == VK_LWIN || p->vkCode == VK_RWIN || p->vkCode == VK_ESCAPE || 
            p->vkCode == VK_TAB || (alt && p->vkCode == VK_F4) || (ctrl && p->vkCode == VK_ESCAPE)) {
            return 1;
        }
    }
    return CallNextHookEx(hHook, nCode, wParam, lParam);
}

// --- PENCERE YÖNETİMİ ---
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
        break;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rect;
        GetClientRect(hwnd, &rect);

        // Arka plan: TAM KIRMIZI
        HBRUSH brush = CreateSolidBrush(RGB(220, 0, 0));
        FillRect(hdc, &rect, brush);
        DeleteObject(brush);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255)); // Yazılar: BEYAZ

        // 1. DEV KİLİT SEMBOLÜ (🔒)
        // Not: Geniş karakter desteği için DrawTextW kullanıyoruz
        HFONT hFontLock = CreateFontW(200, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI Symbol");
        SelectObject(hdc, hFontLock);
        RECT lockRect = rect;
        lockRect.bottom -= 200; // Merkezin biraz yukarısı
        DrawTextW(hdc, L"\xD83D\xDD12", -1, &lockRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE); 
        DeleteObject(hFontLock);

        // 2. ANA UYARI METNİ
        HFONT hFontMain = CreateFontA(60, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Arial");
        SelectObject(hdc, hFontMain);
        RECT mainRect = rect;
        mainRect.top += 250;
        DrawTextA(hdc, "System locked", -1, &mainRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        DeleteObject(hFontMain);

        // 3. ALT ÖZEL METİN ALANI
        HFONT hFontSub = CreateFontA(24, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Consolas");
        SelectObject(hdc, hFontSub);
        RECT subRect = rect;
        subRect.top = rect.bottom - 150;
        DrawTextA(hdc, "The by Mr.bay", -1, &subRect, DT_CENTER | DT_TOP | DT_SINGLELINE);
        DeleteObject(hFontSub);

        EndPaint(hwnd, &ps);
        break;
    }

    case WM_DESTROY:
        if (hHook) UnhookWindowsHookEx(hHook);
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    const char CLASS_NAME[] = "RedBlocker";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_NO);

    RegisterClass(&wc);

    HWND hwnd = CreateWindowExA(WS_EX_TOPMOST, CLASS_NAME, "Locked", WS_POPUP | WS_VISIBLE,
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), NULL, NULL, hInstance, NULL);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
