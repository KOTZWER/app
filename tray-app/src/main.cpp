#define UNICODE
#define _UNICODE

#include <windows.h>
#include <shellapi.h>
#include <strsafe.h>
#include <tlhelp32.h>
#include <rpc.h>
#include <string>
#include "resource.h"

// Pull in the RPC client helper (shared header from svc/)
// Resolved via include path in CMakeLists
#include "rpc_client.h"

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
#define WM_TRAYICON         (WM_USER + 1)
#define IDM_OPEN            1001
#define IDM_EXIT            1002
#define IDM_FILE_EXIT       1003
#define TRAY_UID            1
#define MUTEX_NAME          L"Local\\TrayAppSingleInstance_User"
#define SVC_PROCESS_NAME    L"TrayService.exe"

static UINT g_uTaskbarCreated = 0;

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
static HINSTANCE g_hInst  = nullptr;
static HWND      g_hWnd   = nullptr;
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
void ExitViaService();

// ---------------------------------------------------------------------------
// Check if our parent process is TrayService.exe
// ---------------------------------------------------------------------------
static bool ParentIsService()
{
    DWORD parentPid = 0;
    DWORD myPid     = GetCurrentProcessId();

    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32 pe = {};
    pe.dwSize = sizeof(pe);
    if (Process32First(hSnap, &pe))
    {
        do {
            if (pe.th32ProcessID == myPid)
            {
                parentPid = pe.th32ParentProcessID;
                break;
            }
        } while (Process32Next(hSnap, &pe));
    }

    bool found = false;
    if (parentPid && Process32First(hSnap, &pe))
    {
        do {
            if (pe.th32ProcessID == parentPid)
            {
                // Compare process name (case-insensitive)
                std::wstring name(pe.szExeFile);
                for (auto& c : name) c = towlower(c);
                std::wstring svcName(SVC_PROCESS_NAME);
                for (auto& c : svcName) c = towlower(c);
                found = (name == svcName);
                break;
            }
        } while (Process32Next(hSnap, &pe));
    }

    CloseHandle(hSnap);
    return found;
}

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

    SetForegroundWindow(hWnd);
    POINT pt;
    GetCursorPos(&pt);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_RIGHTALIGN,
                   pt.x, pt.y, 0, hWnd, nullptr);
    PostMessage(hWnd, WM_NULL, 0, 0);
    DestroyMenu(hMenu);
}

// ---------------------------------------------------------------------------
// Show / Hide
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

// ---------------------------------------------------------------------------
// Exit: stop the service via RPC, then quit this process
// ---------------------------------------------------------------------------
void ExitViaService()
{
    RemoveTrayIcon();
    RequestServiceStop();   // RPC call → service stops all TrayApp instances
    PostQuitMessage(0);
}

// ---------------------------------------------------------------------------
// Window procedure
// ---------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == g_uTaskbarCreated && g_uTaskbarCreated != 0)
    {
        g_iconAdded = false;
        AddTrayIcon(hWnd);
        return 0;
    }

    switch (uMsg)
    {
    case WM_TRAYICON:
        switch (LOWORD(lParam))
        {
        case WM_LBUTTONUP: ShowMainWindow();      break;
        case WM_RBUTTONUP: ShowContextMenu(hWnd); break;
        }
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDM_OPEN:       ShowMainWindow();  break;
        case IDM_EXIT:
        case IDM_FILE_EXIT:  ExitViaService();  break;
        }
        return 0;

    case WM_CLOSE:
        HideMainWindow();
        return 0;

    case WM_DESTROY:
        RemoveTrayIcon();
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// ---------------------------------------------------------------------------
// Menu
// ---------------------------------------------------------------------------
HMENU CreateMainMenu()
{
    HMENU hMenuBar  = CreateMenu();
    HMENU hFileMenu = CreatePopupMenu();
    AppendMenu(hFileMenu, MF_STRING, IDM_FILE_EXIT, L"Выход");
    AppendMenu(hMenuBar,  MF_POPUP, (UINT_PTR)hFileMenu, L"Файл");
    return hMenuBar;
}

// ---------------------------------------------------------------------------
// WinMain
// ---------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int)
{
    g_hInst = hInstance;

    // -----------------------------------------------------------------------
    // Requirement 2.1/10: single instance guard
    // -----------------------------------------------------------------------
    HANDLE hMutex = CreateMutex(nullptr, TRUE, MUTEX_NAME);
    if (!hMutex || GetLastError() == ERROR_ALREADY_EXISTS)
    {
        if (hMutex) CloseHandle(hMutex);
        return 0;
    }

    // -----------------------------------------------------------------------
    // Requirement 2.2/1: check service state; start if stopped, then exit
    // -----------------------------------------------------------------------
    {
        SERVICE_STATUS_PROCESS ssp = QueryServiceState();
        if (ssp.dwCurrentState == SERVICE_STOPPED ||
            ssp.dwCurrentState == SERVICE_STOP_PENDING ||
            ssp.dwCurrentState == 0 /* not found */)
        {
            // Start the service and wait; this instance's job is done
            StartAndWaitService();
            CloseHandle(hMutex);
            return 0;
        }
    }

    // -----------------------------------------------------------------------
    // Requirement 2.2/2: parent must be TrayService.exe
    // -----------------------------------------------------------------------
    if (!ParentIsService())
    {
        CloseHandle(hMutex);
        return 0;
    }

    // -----------------------------------------------------------------------
    // Register TaskbarCreated message
    // -----------------------------------------------------------------------
    g_uTaskbarCreated = RegisterWindowMessage(L"TaskbarCreated");

    // -----------------------------------------------------------------------
    // Register window class
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
    RegisterClassEx(&wc);

    // -----------------------------------------------------------------------
    // Create main window
    // -----------------------------------------------------------------------
    g_hWnd = CreateWindowEx(
        0, L"TrayAppMainWnd", L"TrayApp",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 350,
        nullptr, CreateMainMenu(), hInstance, nullptr);

    if (!g_hWnd) { CloseHandle(hMutex); return 1; }

    // -----------------------------------------------------------------------
    // Requirement 2.1/7: --hidden suppresses initial window
    // -----------------------------------------------------------------------
    bool startHidden = (lpCmdLine && strstr(lpCmdLine, "--hidden") != nullptr);
    if (!startHidden)
        ShowMainWindow();

    AddTrayIcon(g_hWnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CloseHandle(hMutex);
    return static_cast<int>(msg.wParam);
}
