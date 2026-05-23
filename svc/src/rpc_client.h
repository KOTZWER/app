#pragma once
#define UNICODE
#define _UNICODE

#include <windows.h>
#include <rpc.h>
#include "TrayService_h.h"

#pragma comment(lib, "rpcrt4.lib")

#define ALPC_ENDPOINT  L"TrayServiceEndpoint"
#define SVC_NAME       L"TrayService"

// ---------------------------------------------------------------------------
// RPC memory (required by MIDL runtime on the client side too)
// ---------------------------------------------------------------------------
inline void* __RPC_USER MIDL_user_allocate(size_t size) { return malloc(size); }
inline void  __RPC_USER MIDL_user_free(void* p)          { free(p); }

// ---------------------------------------------------------------------------
// RpcClient – thin RAII wrapper
// ---------------------------------------------------------------------------
class RpcClient
{
public:
    RpcClient() : m_hBinding(nullptr) {}

    ~RpcClient() { Disconnect(); }

    bool Connect()
    {
        RPC_WSTR strBinding = nullptr;
        RPC_STATUS s = RpcStringBindingCompose(
            nullptr,
            reinterpret_cast<RPC_WSTR>(const_cast<wchar_t*>(L"ncalrpc")),
            nullptr,
            reinterpret_cast<RPC_WSTR>(const_cast<wchar_t*>(ALPC_ENDPOINT)),
            nullptr,
            &strBinding);
        if (s != RPC_S_OK) return false;

        s = RpcBindingFromStringBinding(strBinding, &m_hBinding);
        RpcStringFree(&strBinding);
        return (s == RPC_S_OK);
    }

    void Disconnect()
    {
        if (m_hBinding)
        {
            RpcBindingFree(&m_hBinding);
            m_hBinding = nullptr;
        }
    }

    // Ask the service to stop (and thus kill all TrayApp instances)
    bool CallStopService()
    {
        if (!m_hBinding) return false;
        RpcTryExcept
        {
            ::StopService(m_hBinding);
        }
        RpcExcept(1)
        {
            return false;
        }
        RpcEndExcept
        return true;
    }

private:
    handle_t m_hBinding;
};

// ---------------------------------------------------------------------------
// Convenience free function – used from tray-app exit handlers
// ---------------------------------------------------------------------------
inline void RequestServiceStop()
{
    RpcClient client;
    if (client.Connect())
        client.CallStopService();
}

// ---------------------------------------------------------------------------
// Service state helpers
// ---------------------------------------------------------------------------
inline SERVICE_STATUS_PROCESS QueryServiceState()
{
    SERVICE_STATUS_PROCESS ssp = {};
    SC_HANDLE hSCM = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!hSCM) return ssp;

    SC_HANDLE hSvc = OpenService(hSCM, SVC_NAME, SERVICE_QUERY_STATUS);
    if (hSvc)
    {
        DWORD needed = 0;
        QueryServiceStatusEx(hSvc, SC_STATUS_PROCESS_INFO,
                             reinterpret_cast<LPBYTE>(&ssp), sizeof(ssp), &needed);
        CloseServiceHandle(hSvc);
    }
    CloseServiceHandle(hSCM);
    return ssp;
}

// Start the service and wait until it reaches SERVICE_RUNNING (or timeout)
inline bool StartAndWaitService(DWORD timeoutMs = 30000)
{
    SC_HANDLE hSCM = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!hSCM) return false;

    SC_HANDLE hSvc = OpenService(hSCM, SVC_NAME,
                                 SERVICE_START | SERVICE_QUERY_STATUS);
    bool ok = false;
    if (hSvc)
    {
        StartService(hSvc, 0, nullptr); // ignore error if already starting

        DWORD elapsed = 0;
        const DWORD interval = 500;
        SERVICE_STATUS_PROCESS ssp = {};
        DWORD needed = 0;
        while (elapsed < timeoutMs)
        {
            QueryServiceStatusEx(hSvc, SC_STATUS_PROCESS_INFO,
                                 reinterpret_cast<LPBYTE>(&ssp), sizeof(ssp), &needed);
            if (ssp.dwCurrentState == SERVICE_RUNNING) { ok = true; break; }
            Sleep(interval);
            elapsed += interval;
        }
        CloseServiceHandle(hSvc);
    }
    CloseServiceHandle(hSCM);
    return ok;
}
