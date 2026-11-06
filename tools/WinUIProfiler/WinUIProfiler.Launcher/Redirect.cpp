#include "NativeSupport.h"
#include <detours.h>

namespace
{
    HMODULE (WINAPI* OriginalLoadLibraryExW)(LPCWSTR, HANDLE, DWORD) = LoadLibraryExW;
    HANDLE LogFile = INVALID_HANDLE_VALUE;
    SRWLOCK LogLock = SRWLOCK_INIT;
    std::wstring OriginalMux;
    std::wstring OriginalControls;
    std::wstring PrivateMux;
    std::wstring PrivateControls;
    bool RedirectControls = false;

    bool Log(const char* event, std::wstring_view requested = {}, std::wstring_view selected = {},
        DWORD error = ERROR_SUCCESS)
    {
        try
        {
            const auto line = std::string("{\"event\":\"") + event + "\",\"pid\":" +
                std::to_string(GetCurrentProcessId()) + ",\"requested\":" + profiler::Json(requested) +
                ",\"selected\":" + profiler::Json(selected) + ",\"error\":" + std::to_string(error) + "}\n";
            AcquireSRWLockExclusive(&LogLock);
            const bool written = LogFile != INVALID_HANDLE_VALUE && profiler::WriteAll(LogFile, line);
            ReleaseSRWLockExclusive(&LogLock);
            if (written) return true;
        }
        catch (const std::bad_alloc&) { OutputDebugStringW(L"WinUI profiler: native diagnostic allocation failed.\n"); }
        catch (const std::system_error& failure) { OutputDebugStringA(failure.what()); }
        OutputDebugStringW(L"WinUI profiler: failed to write native startup diagnostics.\n");
        return false;
    }

    const wchar_t* RedirectedPath(LPCWSTR requested)
    {
        if (!requested) return nullptr;
        const std::wstring_view path(requested);
        if (profiler::EqualPath(path, OriginalMux) || profiler::EqualPath(path, L"Microsoft.UI.Xaml.dll"))
            return PrivateMux.c_str();
        if (RedirectControls &&
            (profiler::EqualPath(path, OriginalControls) || profiler::EqualPath(path, L"Microsoft.UI.Xaml.Controls.dll")))
            return PrivateControls.c_str();
        return nullptr;
    }

    HMODULE WINAPI LoadPrivateLibrary(LPCWSTR name, HANDLE file, DWORD flags)
    {
        const DWORD incomingError = GetLastError();
        const wchar_t* replacement = RedirectedPath(name);
        SetLastError(incomingError);
        HMODULE module = OriginalLoadLibraryExW(replacement ? replacement : name, file, flags);
        const DWORD error = GetLastError();
        if (replacement)
        {
            std::wstring observed;
            if (module && !(flags & (LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE |
                LOAD_LIBRARY_AS_IMAGE_RESOURCE)))
                observed = profiler::ModulePath(module);
            Log(module ? "redirected" : "load_failed", name, observed.empty() ? replacement : observed,
                module ? ERROR_SUCCESS : error);
        }
        SetLastError(error);
        return module;
    }

    bool Initialize(HINSTANCE instance)
    {
        const auto logPath = profiler::Environment(L"WINUI_PROFILER_LOG");
        LogFile = CreateFileW(logPath.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (LogFile == INVALID_HANDLE_VALUE) return false;

        const auto expectedExe = profiler::Environment(L"WINUI_PROFILER_EXE");
        const auto source = profiler::Environment(L"WINUI_PROFILER_SOURCE");
        const auto target = profiler::Environment(L"WINUI_PROFILER_PRIVATE");
        const auto ready = profiler::Environment(L"WINUI_PROFILER_READY");
        if (expectedExe.empty() || source.empty() || target.empty() || ready.empty() ||
            !profiler::EqualPath(expectedExe, profiler::ModulePath(nullptr)))
        {
            Log("invalid_configuration", expectedExe, profiler::ModulePath(nullptr), ERROR_INVALID_PARAMETER);
            return false;
        }
        if (GetModuleHandleW(L"Microsoft.UI.Xaml.dll") || GetModuleHandleW(L"Microsoft.UI.Xaml.Controls.dll"))
        {
            Log("runtime_already_loaded", {}, {}, ERROR_ALREADY_INITIALIZED);
            return false;
        }

        OriginalMux = source + L"\\Microsoft.UI.Xaml.dll";
        OriginalControls = source + L"\\Microsoft.UI.Xaml.Controls.dll";
        PrivateMux = target + L"\\Microsoft.UI.Xaml.dll";
        PrivateControls = target + L"\\Microsoft.UI.Xaml.Controls.dll";
        RedirectControls = profiler::Environment(L"WINUI_PROFILER_CONTROLS") == L"1";
        if (GetFileAttributesW(PrivateMux.c_str()) == INVALID_FILE_ATTRIBUTES ||
            (RedirectControls && GetFileAttributesW(PrivateControls.c_str()) == INVALID_FILE_ATTRIBUTES))
        {
            Log("missing_private_runtime", {}, target, ERROR_FILE_NOT_FOUND);
            return false;
        }

        DetourRestoreAfterWith();
        HMODULE kernelBase = GetModuleHandleW(L"KernelBase.dll");
        OriginalLoadLibraryExW = kernelBase ? reinterpret_cast<decltype(OriginalLoadLibraryExW)>(
            GetProcAddress(kernelBase, "LoadLibraryExW")) : nullptr;
        if (!OriginalLoadLibraryExW)
        {
            Log("loader_entry_missing", {}, {}, ERROR_PROC_NOT_FOUND);
            return false;
        }
        LONG error = DetourTransactionBegin();
        if (error == NO_ERROR)
        {
            error = DetourUpdateThread(GetCurrentThread());
            if (error == NO_ERROR)
                error = DetourAttach(reinterpret_cast<PVOID*>(&OriginalLoadLibraryExW), LoadPrivateLibrary);
            if (error == NO_ERROR) error = DetourTransactionCommit();
            else DetourTransactionAbort();
        }
        if (error != NO_ERROR)
        {
            Log("hook_failed", {}, {}, static_cast<DWORD>(error));
            return false;
        }
        HMODULE pinned = nullptr;
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(instance), &pinned))
        {
            Log("pin_failed", {}, {}, GetLastError());
            return false;
        }
        if (!Log("hook_ready", source, target)) return false;
        profiler::Handle signal(OpenEventW(EVENT_MODIFY_STATE, FALSE, ready.c_str()));
        if (!signal || !SetEvent(signal.get()))
        {
            Log("ready_signal_failed", {}, {}, GetLastError());
            return false;
        }
        return true;
    }
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
    if (DetourIsHelperProcess()) return TRUE;
    if (reason == DLL_PROCESS_ATTACH)
    {
        // The statically linked CRT requires thread attach/detach notifications.
        return Initialize(instance) ? TRUE : FALSE;
    }
    if (reason == DLL_PROCESS_DETACH && LogFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(LogFile);
        LogFile = INVALID_HANDLE_VALUE;
    }
    return TRUE;
}
