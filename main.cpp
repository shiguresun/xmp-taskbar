/*
 * xmp-taskbar Win10 fixed version
 * Based on FIX94 original code
 */

#include <windows.h>
#include <shobjidl.h>
#include <cstdio>
#include "xmpdsp.h"

static XMPFUNC_MISC *xmpfmisc;
static XMPFUNC_STATUS *xmpfstatus;
static ITaskbarList3* m_pTaskBarlist = NULL;
static UINT_PTR timer_ptr = NULL;
static CRITICAL_SECTION section;
static HWND xmpwin;
static bool comInit = false;

static void WINAPI DSP_About(HWND win);
static void *WINAPI DSP_New(void);
static void WINAPI DSP_Free(void *inst);
static const char *WINAPI DSP_GetDescription(void *inst);
static DWORD WINAPI DSP_GetConfig(void *inst, void *config);
static BOOL WINAPI DSP_SetConfig(void *inst, void *config, DWORD size);

static XMPDSP dsp = {
    XMPDSP_FLAG_NODSP,
    "Taskbar Progress",
    DSP_About,
    DSP_New,
    DSP_Free,
    DSP_GetDescription,
    NULL,
    DSP_GetConfig,
    DSP_SetConfig,
};

static void WINAPI DSP_About(HWND win)
{
sprintf(mBoxChar, "Taskbar Progress v0.3.1 for XMPlay\n"
		"Copyright (C) 2026 Grin\n"
		"Built: %s %s", __DATE__, __TIME__)"\n"
        "original code by FIX94";
}

static const char *WINAPI DSP_GetDescription(void *inst)
{
    return dsp.name;
}

VOID CALLBACK updateTaskbar(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    if (!m_pTaskBarlist)
        return;

    if (TryEnterCriticalSection(&section))
    {
        if (!IsWindow(xmpwin))
        {
            LeaveCriticalSection(&section);
            return;
        }

        int cPosMs = SendMessage(xmpwin, WM_USER, 0, IPC_GETOUTPUTTIME);
        int lTotal = SendMessage(xmpwin, WM_USER, 1, IPC_GETOUTPUTTIME) * 1000;
        int playStatus = SendMessage(xmpwin, WM_USER, 0, IPC_ISPLAYING);

        if (playStatus == 0 || lTotal <= 0 || cPosMs < 0)
        {
            m_pTaskBarlist->SetProgressState(hwnd, TBPF_NOPROGRESS);
        }
        else
        {
            // Win10 では NORMAL を使う
            if (playStatus == 3)
                m_pTaskBarlist->SetProgressState(hwnd, TBPF_PAUSED);
            else
                m_pTaskBarlist->SetProgressState(hwnd, TBPF_NORMAL);

            m_pTaskBarlist->SetProgressValue(hwnd, min(cPosMs, lTotal), lTotal);
        }

        LeaveCriticalSection(&section);
    }
}

static void *WINAPI DSP_New()
{
    // COM 初期化（Win10 では必須）
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hr))
        comInit = true;

    xmpwin = xmpfmisc->GetWindow();
    InitializeCriticalSection(&section);

    CoCreateInstance(
        CLSID_TaskbarList, NULL, CLSCTX_ALL,
        IID_ITaskbarList3, (void**)&m_pTaskBarlist);

    if (m_pTaskBarlist)
    {
        EnterCriticalSection(&section);
        m_pTaskBarlist->SetProgressState(xmpwin, TBPF_NOPROGRESS);
        LeaveCriticalSection(&section);

        timer_ptr = SetTimer(xmpwin, NULL, 20, updateTaskbar);
    }

    return (void*)1;
}

static void WINAPI DSP_Free(void *inst)
{
    if (timer_ptr)
    {
        KillTimer(xmpwin, timer_ptr);
        timer_ptr = NULL;
    }

    if (m_pTaskBarlist)
    {
        EnterCriticalSection(&section);
        m_pTaskBarlist->SetProgressState(xmpwin, TBPF_NOPROGRESS);
        m_pTaskBarlist->Release();
        m_pTaskBarlist = NULL;
        LeaveCriticalSection(&section);
    }

    DeleteCriticalSection(&section);

    if (comInit)
    {
        CoUninitialize();
        comInit = false;
    }
}

static DWORD WINAPI DSP_GetConfig(void *inst, void *config)
{
    return 0;
}

static BOOL WINAPI DSP_SetConfig(void *inst, void *config, DWORD size)
{
    return TRUE;
}

XMPDSP *WINAPI XMPDSP_GetInterface2(DWORD face, InterfaceProc faceproc)
{
    if (face != XMPDSP_FACE) return NULL;
    xmpfmisc = (XMPFUNC_MISC*)faceproc(XMPFUNC_MISC_FACE);
    xmpfstatus = (XMPFUNC_STATUS*)faceproc(XMPFUNC_STATUS_FACE);
    return &dsp;
}

BOOL WINAPI DllMain(HINSTANCE hDLL, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_ATTACH)
        DisableThreadLibraryCalls(hDLL);
    return 1;
}
