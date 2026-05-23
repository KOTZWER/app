#define UNICODE
#define _UNICODE
#include <windows.h>
#include <shellapi.h>
#include <strsafe.h>
#include "resource.h"

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
#define WM_TRAYICON         (WM_USER + 1)
#define IDM_OPEN            1001
#define IDM_EXIT            1002
#define IDM_FILE_EXIT       1003
#define TRAY_UID            1
#define MUTEX_NAME          L"Local\\TrayAppSingleInstance_User"

// Shell_NotifyIcon re-registration message (sent when Explorer restarts)
static UINT g_uTaskbarCreated = 0;

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
static HINSTANCE g_hInst   = nullptr;
static HWND      g_hWnd    = nullptr;
static NOTIFYICONDATA g_nid = {};
static bool      g_iconAdded = false;

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void AddTrayIcon(HWND hWnd);
void RemoveTrayIcon();
void ShowContextMenu(HWND hWnd);
void ShowMainWindow();
void HideMainWindow();
void ExitApp();

// ---------------------------------------------------------------------------
// Tray icon helpers
// ---------------------------------------------------------------------------
void AddTrayIcon(HWND hWnd)
{
    g_nid.cbSize           = sizeof(NOTIFYICONDATA);
    g_nid.hWnd             = hWnd;
    g_nid.uID              = TRAY_UID;
    g_nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon            = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_APPICON));
    if (!g_nid.hIcon)
        g_nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    StringCchCopy(g_nid.szTip, ARRAYSIZE(g_nid.szTip), L"TrayApp");

    if (Shell_NotifyIcon(NIM_ADD, &g_nid))
        g_iconAdded = true;
}

void RemoveTrayIcon()
{
    if (g_iconAdded)
    {
        Shell_NotifyIcon(NIM_DELETE, &g_nid);
        g_iconAdded = false;
    }
}

// ---------------------------------------------------------------------------
// Context menu
// ---------------------------------------------------------------------------
void ShowContextMenu(HWND hWnd)
{
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;

    AppendMenu(hMenu, MF_STRING, IDM_OPEN, L"Открыть");
    AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenu, MF_STRING, IDM_EXIT, L"Выход");

    // Required so the menu closes when focus is lost
    SetForegroundWindow(hWnd);

    POINT pt;
    GetCursorPos(&pt);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_RIGHTALIGN,
                   pt.x, pt.y, 0, hWnd, nullptr);
    PostMessage(hWnd, WM_NULL, 0, 0);

    DestroyMenu(hMenu);
}

// ---------------------------------------------------------------------------
// Main window visibility
// ---------------------------------------------------------------------------
void ShowMainWindow()
{
    ShowWindow(g_hWnd, SW_SHOW);
    SetForegroundWindow(g_hWnd);
}

void HideMainWindow()
{
    ShowWindow(g_hWnd, SW_HIDE);
}

void ExitApp()
{
    RemoveTrayIcon();
    PostQuitMessage(0);
}

// ---------------------------------------------------------------------------
// Window procedure
// ---------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Handle the "TaskbarCreated" message (Explorer restarted → re-add icon)
    if (uMsg == g_uTaskbarCreated && g_uTaskbarCreated != 0)
    {
        g_iconAdded = false;
        AddTrayIcon(hWnd);
        return 0;
    }

    switch (uMsg)
    {
    // ------------------------------------------------------------------
    // Tray icon notifications
    // ------------------------------------------------------------------
    case WM_TRAYICON:
        switch (LOWORD(lParam))
        {
        case WM_LBUTTONUP:
            ShowMainWindow();
            break;
        case WM_RBUTTONUP:
            ShowContextMenu(hWnd);
            break;
        }
        return 0;

    // ------------------------------------------------------------------
    // Menu commands
    // ------------------------------------------------------------------
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDM_OPEN:
            ShowMainWindow();
            break;

        case IDM_EXIT:
        case IDM_FILE_EXIT:
            ExitApp();
            break;
        }
        return 0;

    // ------------------------------------------------------------------
    // Close button → hide, keep running in background
    // ------------------------------------------------------------------
    case WM_CLOSE:
        HideMainWindow();
        return 0;

    case WM_DESTROY:
        // Should not normally reach here, but guard anyway
        RemoveTrayIcon();
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// ---------------------------------------------------------------------------
// CreateMainMenu
// ---------------------------------------------------------------------------
HMENU CreateMainMenu()
{
    HMENU hMenuBar = CreateMenu();
    HMENU hFileMenu = CreatePopupMenu();

    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_EXIT, L"Выход");
    AppendMenu(hMenuBar,  MF_POPUP, (UINT_PTR)hFileMenu, L"Файл");

    return hMenuBar;
}

// ---------------------------------------------------------------------------
// WinMain
// ---------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/,
                   LPSTR lpCmdLine, int /*nCmdShow*/)
{
    g_hInst = hInstance;

    // -----------------------------------------------------------------------
    // 1. Single-instance guard using a named mutex (per user, "Local\\" scope)
    // -----------------------------------------------------------------------
    HANDLE hMutex = CreateMutex(nullptr, TRUE, MUTEX_NAME);
    if (!hMutex || GetLastError() == ERROR_ALREADY_EXISTS)
    {
        if (hMutex) CloseHandle(hMutex);
        return 0;   // Another instance is running → exit silently
    }

    // -----------------------------------------------------------------------
    // 2. Register "TaskbarCreated" message for taskbar re-creation detection
    // -----------------------------------------------------------------------
    g_uTaskbarCreated = RegisterWindowMessage(L"TaskbarCreated");

    // -----------------------------------------------------------------------
    // 3. Register window class
    // -----------------------------------------------------------------------
    WNDCLASSEX wc    = {};
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));
    if (!wc.hIcon)
        wc.hIcon     = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"TrayAppMainWnd";
    wc.hIconSm       = wc.hIcon;

    if (!RegisterClassEx(&wc))
    {
        CloseHandle(hMutex);
        return 1;
    }

    // -----------------------------------------------------------------------
    // 4. Create (hidden) main window
    // -----------------------------------------------------------------------
    g_hWnd = CreateWindowEx(
        0,
        L"TrayAppMainWnd",
        L"TrayApp",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 350,
        nullptr, CreateMainMenu(), hInstance, nullptr);

    if (!g_hWnd)
    {
        CloseHandle(hMutex);
        return 1;
    }

    // -----------------------------------------------------------------------
    // 5. Parse command line: "--hidden" suppresses initial window
    // -----------------------------------------------------------------------
    bool startHidden = (lpCmdLine && strstr(lpCmdLine, "--hidden") != nullptr);
    if (!startHidden)
        ShowMainWindow();

    // -----------------------------------------------------------------------
    // 6. Add tray icon
    // -----------------------------------------------------------------------
    AddTrayIcon(g_hWnd);

    // -----------------------------------------------------------------------
    // 7. Message loop
    // -----------------------------------------------------------------------
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CloseHandle(hMutex);
    return static_cast<int>(msg.wParam);
}
