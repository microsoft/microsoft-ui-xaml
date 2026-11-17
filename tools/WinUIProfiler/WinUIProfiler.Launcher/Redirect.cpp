#include "StartupSession.h"
#include <format>

namespace
{
    // LoadPrivateLibrary replaces LoadLibraryExW and receives only its arguments, so the state it reads is
    // process-wide. Initialize sets it on process attach, before attaching the hook.
    HMODULE (WINAPI* OriginalLoadLibraryExW)(LPCWSTR, HANDLE, DWORD) = LoadLibraryExW;
    decltype(&CreateProcessW) OriginalCreateProcessW = nullptr;
    decltype(&CreateProcessA) OriginalCreateProcessA = nullptr;
    decltype(&CreateProcessAsUserW) OriginalCreateProcessAsUserW = nullptr;
    decltype(&CreateProcessAsUserA) OriginalCreateProcessAsUserA = nullptr;
    profiler::StartupSession Session;
    profiler::StartupProcess* CurrentProcess = nullptr;
    profiler::Handle Changed;
    profiler::Handle Controller;
    thread_local bool CreatingProcess = false;
    HANDLE LogFile = INVALID_HANDLE_VALUE;
    SRWLOCK LogLock = SRWLOCK_INIT;
    std::wstring OriginalMux;
    std::wstring OriginalControls;
    std::wstring PrivateMux;
    std::wstring PrivateControls;
    bool RedirectControls = false;

    void FailStartup(DWORD error)
    {
        if (!error) error = ERROR_DLL_INIT_FAILED;
        if (CurrentProcess) InterlockedCompareExchange(&CurrentProcess->error, static_cast<LONG>(error), 0);
        if (Session.data) InterlockedCompareExchange(&Session.data->error, static_cast<LONG>(error), 0);
        if (Changed) SetEvent(Changed.get());
    }

    bool Log(const char* event, std::wstring_view requested = {}, std::wstring_view selected = {},
        DWORD error = ERROR_SUCCESS)
    {
        bool written = false;
        try
        {
            const auto line = std::format(
                R"({{"event":"{}","pid":{},"requested":{},"selected":{},"error":{}}})" "\n",
                event, GetCurrentProcessId(), profiler::Json(requested), profiler::Json(selected), error);
            AcquireSRWLockExclusive(&LogLock);
            written = LogFile != INVALID_HANDLE_VALUE && profiler::WriteAll(LogFile, line);
            ReleaseSRWLockExclusive(&LogLock);
        }
        catch (const std::bad_alloc&)
        {
            OutputDebugStringW(L"WinUI profiler: native diagnostic allocation failed.\n");
        }
        catch (const std::system_error& failure)
        {
            OutputDebugStringA(failure.what());
        }
        if (!written)
        {
            OutputDebugStringW(L"WinUI profiler: failed to write native startup diagnostics.\n");
        }
        return written;
    }

    const wchar_t* RedirectedPath(LPCWSTR requested)
    {
        if (!requested)
        {
            return nullptr;
        }
        const std::wstring_view path(requested);
        if (profiler::EqualPath(path, OriginalMux) || profiler::EqualPath(path, L"Microsoft.UI.Xaml.dll"))
        {
            return PrivateMux.c_str();
        }
        if (RedirectControls &&
            (profiler::EqualPath(path, OriginalControls) || profiler::EqualPath(path, L"Microsoft.UI.Xaml.Controls.dll")))
        {
            return PrivateControls.c_str();
        }
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
            {
                observed = profiler::ModulePath(module);
            }
            Log(module ? "redirected" : "load_failed", name, observed.empty() ? replacement : observed,
                module ? ERROR_SUCCESS : error);
            if (!module)
            {
                FailStartup(error);
            }
            else if (!observed.empty() && profiler::EqualPath(observed, PrivateMux))
            {
                InterlockedExchange(&CurrentProcess->runtimeLoaded, 1);
                SetEvent(Changed.get());
            }
        }
        SetLastError(error);
        return module;
    }

    bool ConfigureRedirection(const std::wstring& source, const std::wstring& target)
    {
        OriginalMux = source + L"\\Microsoft.UI.Xaml.dll";
        OriginalControls = source + L"\\Microsoft.UI.Xaml.Controls.dll";
        PrivateMux = target + L"\\Microsoft.UI.Xaml.dll";
        PrivateControls = target + L"\\Microsoft.UI.Xaml.Controls.dll";
        RedirectControls = Session.data->redirectControls != 0;
        if (GetFileAttributesW(PrivateMux.c_str()) == INVALID_FILE_ATTRIBUTES ||
            (RedirectControls && GetFileAttributesW(PrivateControls.c_str()) == INVALID_FILE_ATTRIBUTES))
        {
            Log("missing_private_runtime", {}, target, ERROR_FILE_NOT_FOUND);
            return false;
        }
        return true;
    }

    template<typename Create>
    BOOL CreateChild(DWORD flags, PROCESS_INFORMATION* processInfo, Create&& create)
    {
        if (CreatingProcess || !processInfo || !Session.data ||
            WaitForSingleObject(Controller.get(), 0) != WAIT_TIMEOUT)
        {
            return create(flags, processInfo);
        }
        LONG64 active = profiler::ReadState(Session.data->active);
        while (active > 0)
        {
            const LONG64 previous = InterlockedCompareExchange64(&Session.data->active,
                active + profiler::StartupGenerationIncrement + 1, active);
            if (previous == active) break;
            active = previous;
        }
        if (active <= 0) return create(flags, processInfo);
        struct CreationScope
        {
            CreationScope() { CreatingProcess = true; }
            ~CreationScope()
            {
                const DWORD error = GetLastError();
                CreatingProcess = false;
                InterlockedDecrement64(&Session.data->active);
                SetEvent(Changed.get());
                SetLastError(error);
            }
        } scope;
        if (!create(flags | CREATE_SUSPENDED, processInfo)) return FALSE;
        const DWORD incomingError = GetLastError();
        DWORD error = ERROR_SUCCESS;
        bool selected = false;
        profiler::StartupProcess* child = nullptr;
        try
        {
            selected = profiler::ReadState(Session.data->active) > 0 &&
                WaitForSingleObject(Controller.get(), 0) == WAIT_TIMEOUT &&
                profiler::MatchesStartupIdentity(processInfo->hProcess, *Session.data) &&
                profiler::SameProcessUser(processInfo->hProcess);
            if (selected)
            {
                child = Session.Register(processInfo->hProcess, Session.Index(*CurrentProcess));
                if (!child) error = GetLastError();
                if (!error && !Session.CopyPayload(processInfo->hProcess)) error = GetLastError();
                if (!error)
                {
                    const auto dll = profiler::ImportPath(Session.data->dll);
                    LPCSTR libraries[] = { dll.c_str() };
                    if (dll.empty() || !DetourUpdateProcessWithDll(processInfo->hProcess, libraries, 1))
                    {
                        error = GetLastError();
                        if (!error) error = ERROR_DLL_INIT_FAILED;
                    }
                }
                Log(error ? "child_injection_failed" : "child_injected",
                    std::to_wstring(processInfo->dwProcessId), Session.data->executable, error);
                SetEvent(Changed.get());
            }
        }
        catch (const std::bad_alloc&) { error = ERROR_NOT_ENOUGH_MEMORY; }
        catch (const std::system_error& failure) { error = static_cast<DWORD>(failure.code().value()); }
        if (error)
        {
            if (child) InterlockedCompareExchange(&child->error, static_cast<LONG>(error), 0);
            FailStartup(error);
            if (selected)
            {
                TerminateProcess(processInfo->hProcess, error);
                CloseHandle(processInfo->hThread);
                CloseHandle(processInfo->hProcess);
                *processInfo = {};
                SetLastError(error);
                return FALSE;
            }
        }
        if (!(flags & CREATE_SUSPENDED) && ResumeThread(processInfo->hThread) == static_cast<DWORD>(-1))
        {
            error = GetLastError();
            FailStartup(error);
            TerminateProcess(processInfo->hProcess, error);
            CloseHandle(processInfo->hThread);
            CloseHandle(processInfo->hProcess);
            *processInfo = {};
            SetLastError(error);
            return FALSE;
        }
        SetLastError(incomingError);
        return TRUE;
    }

    BOOL WINAPI CreateChildW(LPCWSTR application, LPWSTR command, LPSECURITY_ATTRIBUTES processSecurity,
        LPSECURITY_ATTRIBUTES threadSecurity, BOOL inherit, DWORD flags, LPVOID environment,
        LPCWSTR directory, LPSTARTUPINFOW startup, LPPROCESS_INFORMATION process)
    {
        return CreateChild(flags, process, [&](DWORD actualFlags, PROCESS_INFORMATION* result)
        {
            return OriginalCreateProcessW(application, command, processSecurity, threadSecurity,
                inherit, actualFlags, environment, directory, startup, result);
        });
    }

    BOOL WINAPI CreateChildA(LPCSTR application, LPSTR command, LPSECURITY_ATTRIBUTES processSecurity,
        LPSECURITY_ATTRIBUTES threadSecurity, BOOL inherit, DWORD flags, LPVOID environment,
        LPCSTR directory, LPSTARTUPINFOA startup, LPPROCESS_INFORMATION process)
    {
        return CreateChild(flags, process, [&](DWORD actualFlags, PROCESS_INFORMATION* result)
        {
            return OriginalCreateProcessA(application, command, processSecurity, threadSecurity,
                inherit, actualFlags, environment, directory, startup, result);
        });
    }

    BOOL WINAPI CreateChildAsUserW(HANDLE token, LPCWSTR application, LPWSTR command,
        LPSECURITY_ATTRIBUTES processSecurity, LPSECURITY_ATTRIBUTES threadSecurity, BOOL inherit,
        DWORD flags, LPVOID environment, LPCWSTR directory, LPSTARTUPINFOW startup,
        LPPROCESS_INFORMATION process)
    {
        return CreateChild(flags, process, [&](DWORD actualFlags, PROCESS_INFORMATION* result)
        {
            return OriginalCreateProcessAsUserW(token, application, command, processSecurity, threadSecurity,
                inherit, actualFlags, environment, directory, startup, result);
        });
    }

    BOOL WINAPI CreateChildAsUserA(HANDLE token, LPCSTR application, LPSTR command,
        LPSECURITY_ATTRIBUTES processSecurity, LPSECURITY_ATTRIBUTES threadSecurity, BOOL inherit,
        DWORD flags, LPVOID environment, LPCSTR directory, LPSTARTUPINFOA startup,
        LPPROCESS_INFORMATION process)
    {
        return CreateChild(flags, process, [&](DWORD actualFlags, PROCESS_INFORMATION* result)
        {
            return OriginalCreateProcessAsUserA(token, application, command, processSecurity, threadSecurity,
                inherit, actualFlags, environment, directory, startup, result);
        });
    }

    bool Initialize(HINSTANCE instance)
    {
        if (!Session.OpenPayload()) return false;
        FILETIME created{}, exited{}, kernel{}, user{};
        if (!GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user)) return false;
        CurrentProcess = Session.Find(GetCurrentProcessId(), created);
        if (!CurrentProcess)
        {
            SetLastError(ERROR_INVALID_DATA);
            return false;
        }
        DetourRestoreAfterWith();
        Controller.reset(OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, Session.data->launcherPid));
        if (!Controller || !GetProcessTimes(Controller.get(), &created, &exited, &kernel, &user) ||
            CompareFileTime(&created, &Session.data->launcherCreated) != 0 ||
            WaitForSingleObject(Controller.get(), 0) != WAIT_TIMEOUT)
        {
            InterlockedExchange64(&Session.data->active, 0);
            InterlockedExchange(&CurrentProcess->error, ERROR_CANCELLED);
            return true;
        }
        Changed.reset(OpenEventW(EVENT_MODIFY_STATE, FALSE, Session.data->event));
        if (!Changed) return false;
        const auto logPath = profiler::RedirectLog(*Session.data, *CurrentProcess);
        LogFile = CreateFileW(logPath.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (LogFile == INVALID_HANDLE_VALUE)
        {
            return false;
        }

        const std::wstring expectedExe(Session.data->executable);
        const std::wstring source(Session.data->source);
        const std::wstring target(Session.data->target);
        if (expectedExe.empty() || source.empty() || target.empty() ||
            !profiler::MatchesStartupIdentity(GetCurrentProcess(), *Session.data))
        {
            Log("invalid_configuration", expectedExe, profiler::ModulePath(nullptr), ERROR_INVALID_PARAMETER);
            SetLastError(ERROR_INVALID_PARAMETER);
            return false;
        }
        if (GetModuleHandleW(L"Microsoft.UI.Xaml.dll") || GetModuleHandleW(L"Microsoft.UI.Xaml.Controls.dll"))
        {
            Log("runtime_already_loaded", {}, {}, ERROR_ALREADY_INITIALIZED);
            SetLastError(ERROR_ALREADY_INITIALIZED);
            return false;
        }
        if (!ConfigureRedirection(source, target))
        {
            return false;
        }

        HMODULE kernelBase = GetModuleHandleW(L"KernelBase.dll");
        OriginalLoadLibraryExW = kernelBase ? reinterpret_cast<decltype(OriginalLoadLibraryExW)>(
            GetProcAddress(kernelBase, "LoadLibraryExW")) : nullptr;
        OriginalCreateProcessW = kernelBase ? reinterpret_cast<decltype(OriginalCreateProcessW)>(
            GetProcAddress(kernelBase, "CreateProcessW")) : nullptr;
        OriginalCreateProcessA = kernelBase ? reinterpret_cast<decltype(OriginalCreateProcessA)>(
            GetProcAddress(kernelBase, "CreateProcessA")) : nullptr;
        OriginalCreateProcessAsUserW = kernelBase ? reinterpret_cast<decltype(OriginalCreateProcessAsUserW)>(
            GetProcAddress(kernelBase, "CreateProcessAsUserW")) : nullptr;
        OriginalCreateProcessAsUserA = kernelBase ? reinterpret_cast<decltype(OriginalCreateProcessAsUserA)>(
            GetProcAddress(kernelBase, "CreateProcessAsUserA")) : nullptr;
        if (!OriginalLoadLibraryExW || !OriginalCreateProcessW || !OriginalCreateProcessA ||
            !OriginalCreateProcessAsUserW || !OriginalCreateProcessAsUserA)
        {
            Log("loader_entry_missing", {}, {}, ERROR_PROC_NOT_FOUND);
            SetLastError(ERROR_PROC_NOT_FOUND);
            return false;
        }
        LONG error = DetourTransactionBegin();
        if (error == NO_ERROR)
        {
            error = DetourUpdateThread(GetCurrentThread());
            if (error == NO_ERROR)
            {
                error = DetourAttach(reinterpret_cast<PVOID*>(&OriginalLoadLibraryExW), LoadPrivateLibrary);
            }
            if (error == NO_ERROR)
                error = DetourAttach(reinterpret_cast<PVOID*>(&OriginalCreateProcessW), CreateChildW);
            if (error == NO_ERROR)
                error = DetourAttach(reinterpret_cast<PVOID*>(&OriginalCreateProcessA), CreateChildA);
            if (error == NO_ERROR)
                error = DetourAttach(reinterpret_cast<PVOID*>(&OriginalCreateProcessAsUserW), CreateChildAsUserW);
            if (error == NO_ERROR)
                error = DetourAttach(reinterpret_cast<PVOID*>(&OriginalCreateProcessAsUserA), CreateChildAsUserA);
            if (error == NO_ERROR)
            {
                error = DetourTransactionCommit();
            }
            else
            {
                DetourTransactionAbort();
            }
        }
        if (error != NO_ERROR)
        {
            Log("hook_failed", {}, {}, static_cast<DWORD>(error));
            SetLastError(static_cast<DWORD>(error));
            return false;
        }
        HMODULE pinned = nullptr;
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(instance), &pinned))
        {
            Log("pin_failed", {}, {}, GetLastError());
            return false;
        }
        if (!Log("hook_ready", source, target))
        {
            return false;
        }
        InterlockedExchange(&CurrentProcess->helperReady, 1);
        if (!SetEvent(Changed.get()))
        {
            Log("ready_signal_failed", {}, {}, GetLastError());
            return false;
        }
        return true;
    }
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
    if (DetourIsHelperProcess())
    {
        return TRUE;
    }
    if (reason == DLL_PROCESS_ATTACH)
    {
        // The statically linked CRT requires thread attach/detach notifications.
        DWORD error = ERROR_DLL_INIT_FAILED;
        try
        {
            if (Initialize(instance)) return TRUE;
            error = GetLastError();
        }
        catch (const std::bad_alloc&) { error = ERROR_NOT_ENOUGH_MEMORY; }
        catch (const std::system_error& failure) { error = static_cast<DWORD>(failure.code().value()); }
        FailStartup(error);
        return FALSE;
    }
    if (reason == DLL_PROCESS_DETACH && LogFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(LogFile);
        LogFile = INVALID_HANDLE_VALUE;
    }
    return TRUE;
}
