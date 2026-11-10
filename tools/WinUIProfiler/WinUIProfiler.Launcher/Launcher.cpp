#include "NativeSupport.h"
#include <detours.h>
#include <shellapi.h>
#include <shobjidl.h>
#include <appmodel.h>
#include <wrl/client.h>
#include <format>
#include <map>
#include <memory>

using Microsoft::WRL::ComPtr;

namespace
{
    using Arguments = std::map<std::wstring, std::wstring>;

    struct DebugContext
    {
        DWORD size;
        DWORD version;
        FILETIME notBefore;
        DWORD sessionId;
        volatile LONG claimedPid;
        volatile LONG error;
        wchar_t executable[profiler::MaxPathCharacters];
        wchar_t dll[profiler::MaxPathCharacters];
        wchar_t result[profiler::MaxPathCharacters];
        wchar_t package[profiler::MaxPathCharacters];
    };

    struct UnmapView
    {
        void operator()(DebugContext* view) const noexcept
        {
            UnmapViewOfFile(view);
        }
    };
    using MappingView = std::unique_ptr<DebugContext, UnmapView>;

    struct PackageLaunchState
    {
        bool initialized = false;
        bool enabled = false;
        std::wstring package;
        ComPtr<IPackageDebugSettings> debugging;
        ComPtr<IApplicationActivationManager> activation;
        profiler::Handle mapping;
        MappingView context;

        HRESULT Close()
        {
            const HRESULT result = enabled ? debugging->DisableDebugging(package.c_str()) : S_OK;
            enabled = false;
            activation.Reset();
            debugging.Reset();
            if (initialized)
            {
                CoUninitialize();
            }
            initialized = false;
            return result;
        }
        ~PackageLaunchState()
        {
            if (FAILED(Close()))
            {
                OutputDebugStringW(L"WinUI profiler: package-debug cleanup failed.\n");
            }
        }
    };

    std::wstring Value(const Arguments& args, const wchar_t* name)
    {
        const auto it = args.find(name);
        return it == args.end() ? std::wstring{} : it->second;
    }

    bool CreationTime(HANDLE process, FILETIME& created)
    {
        FILETIME exited{}, kernel{}, user{};
        return GetProcessTimes(process, &created, &exited, &kernel, &user) != FALSE;
    }

    // A process ID can be reused after the process exits; the ID and its creation time name one process.
    bool SaveResult(const std::wstring& output, DWORD pid, const FILETIME& created, DWORD error,
        std::wstring_view stage, std::wstring_view package = {}, std::wstring_view aumid = {})
    {
        const auto creation = (static_cast<ULONGLONG>(created.dwHighDateTime) << 32) | created.dwLowDateTime;
        const auto line = std::format(
            R"({{"schemaVersion":1,"pid":{},"creationTimeFileTime":{},"error":{},)"
            R"("stage":{},"packageFullName":{},"aumid":{}}})" "\n",
            pid, creation, error, profiler::Json(stage), profiler::Json(package), profiler::Json(aumid));
        const auto temporary = output + L".tmp";
        {
            profiler::Handle file(CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr,
                CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
            if (!file)
            {
                return false;
            }
            if (!profiler::WriteAll(file.get(), line))
            {
                return false;
            }
        }
        return MoveFileExW(temporary.c_str(), output.c_str(), MOVEFILE_WRITE_THROUGH) != FALSE;
    }

    std::vector<wchar_t> ProfileEnvironment(const Arguments& args, const std::wstring& source,
        const std::wstring& target, const std::wstring& signal)
    {
        const std::pair<const wchar_t*, std::wstring> values[] = {
            {L"WINUI_PROFILER_EXE", Value(args, L"--exe")},
            {L"WINUI_PROFILER_SOURCE", source},
            {L"WINUI_PROFILER_PRIVATE", target},
            {L"WINUI_PROFILER_LOG", Value(args, L"--log")},
            {L"WINUI_PROFILER_READY", signal},
            {L"WINUI_PROFILER_CONTROLS", Value(args, L"--controls")}
        };
        std::vector<wchar_t> block;
        for (const auto& [name, value] : values)
        {
            std::wstring entry = std::wstring(name) + L"=" + value;
            block.insert(block.end(), entry.begin(), entry.end());
            block.push_back(L'\0');
        }
        block.push_back(L'\0');
        return block;
    }

    DWORD ProcessIdArgument(const Arguments& args, const wchar_t* name)
    {
        const auto text = Value(args, name);
        if (text.empty())
        {
            return 0;
        }
        ULONGLONG value = 0;
        for (wchar_t ch : text)
        {
            if (ch < L'0' || ch > L'9')
            {
                return 0;
            }
            value = value * 10 + static_cast<unsigned>(ch - L'0');
            if (value > MAXDWORD)
            {
                return 0;
            }
        }
        return static_cast<DWORD>(value);
    }

    DWORD ResumeDebugThread(DWORD pid, DWORD tid)
    {
        if (!pid || !tid)
        {
            return ERROR_INVALID_PARAMETER;
        }
        profiler::Handle thread(OpenThread(THREAD_SUSPEND_RESUME | THREAD_QUERY_INFORMATION, FALSE, tid));
        if (!thread)
        {
            return GetLastError();
        }
        if (GetProcessIdOfThread(thread.get()) != pid)
        {
            return ERROR_INVALID_PARAMETER;
        }
        return ResumeThread(thread.get()) == static_cast<DWORD>(-1) ? GetLastError() : ERROR_SUCCESS;
    }

    DWORD DebuggerLaunch(const Arguments& args)
    {
        const DWORD pid = ProcessIdArgument(args, L"-p");
        const DWORD tid = ProcessIdArgument(args, L"-tid");
        const auto contextName = Value(args, L"--context");
        if (!pid || !tid || !contextName.starts_with(L"Local\\WinUIPerfTools.Redirect."))
        {
            return ERROR_INVALID_PARAMETER;
        }
        profiler::Handle mapping(OpenFileMappingW(FILE_MAP_READ | FILE_MAP_WRITE, FALSE,
            contextName.c_str()));
        if (!mapping)
        {
            const DWORD error = GetLastError();
            // The launch can be cancelled before this callback starts. Resume the callback's thread, without injection.
            const DWORD resumed = ResumeDebugThread(pid, tid);
            OutputDebugStringW(L"WinUI profiler: launch context expired; startup helper was not inserted.\n");
            return resumed ? resumed : error;
        }
        MappingView context{ static_cast<DebugContext*>(MapViewOfFile(mapping.get(),
            FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, sizeof(DebugContext))) };
        if (!context)
        {
            const DWORD error = GetLastError();
            const DWORD resumed = ResumeDebugThread(pid, tid);
            OutputDebugStringW(L"WinUI profiler: failed to map startup callback configuration.\n");
            return resumed ? resumed : error;
        }
        if (context->size != sizeof(DebugContext) || context->version != profiler::ProtocolVersion ||
            !wmemchr(context->executable, 0, profiler::MaxPathCharacters) ||
            !wmemchr(context->dll, 0, profiler::MaxPathCharacters) ||
            !wmemchr(context->result, 0, profiler::MaxPathCharacters) ||
            !wmemchr(context->package, 0, profiler::MaxPathCharacters))
        {
            const DWORD resumed = ResumeDebugThread(pid, tid);
            OutputDebugStringW(L"WinUI profiler: invalid startup callback configuration.\n");
            return resumed ? resumed : ERROR_INVALID_DATA;
        }

        profiler::Handle process(OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
            PROCESS_VM_READ | PROCESS_VM_WRITE | SYNCHRONIZE, FALSE, pid));
        const DWORD processError = process ? ERROR_SUCCESS : GetLastError();
        profiler::Handle thread(OpenThread(THREAD_SUSPEND_RESUME | THREAD_QUERY_INFORMATION, FALSE, tid));
        const DWORD threadError = thread ? ERROR_SUCCESS : GetLastError();
        DWORD error = ERROR_SUCCESS;
        std::wstring actual(profiler::MaxPathCharacters, L'\0');
        DWORD length = static_cast<DWORD>(actual.size());
        FILETIME created{}, exited{}, kernel{}, user{};
        DWORD session = 0;
        if (!process || !thread)
        {
            error = processError ? processError : threadError;
        }
        else if (GetProcessIdOfThread(thread.get()) != pid)
        {
            error = ERROR_INVALID_PARAMETER;
        }
        else if (!GetProcessTimes(process.get(), &created, &exited, &kernel, &user) ||
            !ProcessIdToSessionId(pid, &session))
        {
            error = GetLastError();
        }
        else if (CompareFileTime(&created, &context->notBefore) < 0 || session != context->sessionId)
        {
            error = ERROR_INVALID_DATA;
        }
        else if (!QueryFullProcessImageNameW(process.get(), 0, actual.data(), &length))
        {
            error = GetLastError();
        }
        else
        {
            actual.resize(length);
            if (!profiler::EqualPath(actual, context->executable))
            {
                error = ERROR_BAD_EXE_FORMAT;
            }
            else
            {
                UINT32 packageLength = 0;
                LONG status = GetPackageFullName(process.get(), &packageLength, nullptr);
                std::wstring package(packageLength, L'\0');
                if (status == ERROR_INSUFFICIENT_BUFFER)
                {
                    status = GetPackageFullName(process.get(), &packageLength, package.data());
                }
                if (status != ERROR_SUCCESS)
                {
                    error = static_cast<DWORD>(status);
                }
                else
                {
                    package.resize(packageLength - 1);
                    if (package != context->package)
                    {
                        error = ERROR_INVALID_DATA;
                    }
                }
            }
            if (!error && InterlockedCompareExchange(&context->claimedPid, static_cast<LONG>(pid), 0) != 0)
            {
                error = ERROR_ALREADY_EXISTS;
            }
            if (!error)
            {
                const auto dll = profiler::ImportPath(context->dll);
                LPCSTR libraries[] = { dll.c_str() };
                if (dll.empty() || !DetourUpdateProcessWithDll(process.get(), libraries, 1))
                {
                    error = GetLastError();
                }
            }
        }
        // Even a rejected callback must not strand a package-created process suspended.
        if (thread && GetProcessIdOfThread(thread.get()) == pid &&
            ResumeThread(thread.get()) == static_cast<DWORD>(-1) && !error)
        {
            error = GetLastError();
        }
        if (!SaveResult(context->result, pid, created, error, L"package_debugger") && !error)
        {
            error = GetLastError();
        }
        InterlockedCompareExchange(&context->error, static_cast<LONG>(error), ERROR_IO_PENDING);
        return error;
    }

    DWORD Launch(const Arguments& args)
    {
        const auto output = Value(args, L"--result");
        const auto exe = Value(args, L"--exe");
        // The helper matches "<source>\Microsoft.UI.Xaml.dll", so C:\App\ and C:/App must become C:\App.
        const auto source = profiler::DirectoryPath(Value(args, L"--source"));
        const auto target = profiler::DirectoryPath(Value(args, L"--private"));
        const auto log = Value(args, L"--log");
        const auto dll = profiler::ParentPath(profiler::ModulePath(nullptr)) + L"\\ProfilerRedirect.dll";
        DWORD error = ERROR_SUCCESS;
        DWORD pid = 0;
        std::wstring stage = L"configuration";
        if (output.empty() || exe.empty() || source.empty() || target.empty() || log.empty())
        {
            return ERROR_INVALID_PARAMETER;
        }
        FILETIME launcherCreated{};
        if (!CreationTime(GetCurrentProcess(), launcherCreated) ||
            !SaveResult(output + L".owner.json", GetCurrentProcessId(), launcherCreated, ERROR_SUCCESS, L"launcher"))
        {
            return GetLastError();
        }
        if (exe.size() >= profiler::MaxPathCharacters || source.size() >= profiler::MaxPathCharacters ||
            target.size() >= profiler::MaxPathCharacters || output.size() + 32 >= profiler::MaxPathCharacters ||
            Value(args, L"--package").size() >= profiler::MaxPathCharacters ||
            (Value(args, L"--controls") != L"0" && Value(args, L"--controls") != L"1") ||
            Value(args, L"--aumid").empty() != Value(args, L"--package").empty())
        {
            SaveResult(output, 0, {}, ERROR_INVALID_PARAMETER, L"configuration");
            return ERROR_INVALID_PARAMETER;
        }

        GUID id{};
        HRESULT hr = CoCreateGuid(&id);
        if (FAILED(hr))
        {
            return static_cast<DWORD>(hr);
        }
        wchar_t guid[40]{};
        StringFromGUID2(id, guid, ARRAYSIZE(guid));
        const std::wstring eventName = std::wstring(L"Local\\WinUIPerfTools.Redirect.") + guid;
        profiler::Handle ready(CreateEventW(nullptr, TRUE, FALSE, eventName.c_str()));
        if (!ready)
        {
            return GetLastError();
        }
        auto environment = ProfileEnvironment(args, source, target, eventName);
        PROCESS_INFORMATION processInfo{};
        const auto aumid = Value(args, L"--aumid");
        PackageLaunchState packageState;
        if (aumid.empty())
        {
            for (const wchar_t* entry = environment.data(); *entry; entry += wcslen(entry) + 1)
            {
                const std::wstring_view pair(entry);
                const auto equal = pair.find(L'=');
                if (!SetEnvironmentVariableW(std::wstring(pair.substr(0, equal)).c_str(),
                    std::wstring(pair.substr(equal + 1)).c_str()))
                {
                    error = GetLastError();
                }
            }
            STARTUPINFOW startup{};
            startup.cb = sizeof(startup);
            std::wstring command = profiler::Quote(exe);
            const auto extra = Value(args, L"--arguments");
            if (!extra.empty())
            {
                command += L" " + extra;
            }
            const auto ansiDll = profiler::ImportPath(dll);
            if (ansiDll.empty() && !error)
            {
                error = GetLastError();
            }
            stage = L"create_process";
            if (!error && !DetourCreateProcessWithDllExW(exe.c_str(), command.data(), nullptr, nullptr,
                FALSE, CREATE_SUSPENDED, nullptr, profiler::ParentPath(exe).c_str(), &startup,
                &processInfo, ansiDll.c_str(), nullptr))
            {
                error = GetLastError();
                if (!error)
                {
                    error = ERROR_DLL_INIT_FAILED;
                }
                if (processInfo.dwProcessId)
                {
                    stage = L"insert_helper";
                }
                // Detours already terminates the child and closes both handles when insertion fails.
                processInfo.hProcess = nullptr;
                processInfo.hThread = nullptr;
            }
            if (!error)
            {
                pid = processInfo.dwProcessId;
                if (ResumeThread(processInfo.hThread) == static_cast<DWORD>(-1))
                {
                    error = GetLastError();
                }
            }
        }
        else
        {
            stage = L"package_activation";
            hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
            if (FAILED(hr))
            {
                error = static_cast<DWORD>(hr);
            }
            else
            {
                packageState.initialized = true;
                auto& debugging = packageState.debugging;
                auto& activation = packageState.activation;
                auto& context = packageState.context;
                stage = L"package_debug_settings";
                hr = CoCreateInstance(CLSID_PackageDebugSettings, nullptr, CLSCTX_INPROC_SERVER,
                    IID_PPV_ARGS(&debugging));
                packageState.package = Value(args, L"--package");
                const auto& package = packageState.package;
                const auto debuggerResult = output + L".debugger.json";
                const auto contextName = eventName + L".Context";
                packageState.mapping.reset(CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
                    0, sizeof(DebugContext), contextName.c_str()));
                if (packageState.mapping)
                {
                    context.reset(static_cast<DebugContext*>(MapViewOfFile(packageState.mapping.get(),
                        FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, sizeof(DebugContext))));
                }
                if (!context)
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                }
                else
                {
                    context->size = sizeof(DebugContext);
                    context->version = profiler::ProtocolVersion;
                    context->error = ERROR_IO_PENDING;
                    GetSystemTimeAsFileTime(&context->notBefore);
                    if (!ProcessIdToSessionId(GetCurrentProcessId(), &context->sessionId))
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                    }
                    wcscpy_s(context->executable, exe.c_str());
                    wcscpy_s(context->dll, dll.c_str());
                    wcscpy_s(context->result, debuggerResult.c_str());
                    wcscpy_s(context->package, package.c_str());
                }
                // Package debugging limits its command line; keep per-run paths in the owned mapping.
                const auto command = profiler::Quote(profiler::ShortPath(profiler::ModulePath(nullptr))) +
                    L" --debugger --context " + profiler::Quote(contextName);
                if (command.size() >= MAX_PATH)
                {
                    hr = HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
                }
                if (SUCCEEDED(hr))
                {
                    stage = L"package_enable_debugging";
                    hr = debugging->EnableDebugging(package.c_str(), command.c_str(), environment.data());
                    packageState.enabled = SUCCEEDED(hr);
                }
                if (SUCCEEDED(hr))
                {
                    stage = L"package_activation_manager";
                    hr = CoCreateInstance(CLSID_ApplicationActivationManager, nullptr, CLSCTX_LOCAL_SERVER,
                        IID_PPV_ARGS(&activation));
                }
                if (SUCCEEDED(hr))
                {
                    stage = L"package_activate";
                    hr = activation->ActivateApplication(aumid.c_str(), nullptr, AO_NOERRORUI, &pid);
                }
                if (FAILED(hr))
                {
                    error = static_cast<DWORD>(hr);
                }
                if (context)
                {
                    if (!pid && context->claimedPid > 0)
                    {
                        pid = static_cast<DWORD>(context->claimedPid);
                    }
                    if (context->error != ERROR_SUCCESS && context->error != ERROR_IO_PENDING && !error)
                    {
                        error = static_cast<DWORD>(context->error);
                        stage = L"package_debugger";
                    }
                }
                if (pid)
                {
                    processInfo.hProcess = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION,
                        FALSE, pid);
                    if (!processInfo.hProcess && !error)
                    {
                        error = GetLastError();
                    }
                }
            }
        }

        profiler::Handle process(processInfo.hProcess);
        profiler::Handle thread(processInfo.hThread);
        if (!error && !process)
        {
            error = ERROR_PROCESS_ABORTED;
        }
        if (!error && process)
        {
            stage = L"helper_ready";
            const HANDLE handles[] = { ready.get(), process.get() };
            const DWORD result = WaitForMultipleObjects(ARRAYSIZE(handles), handles, FALSE,
                profiler::StartupTimeoutMs);
            if (result == WAIT_OBJECT_0)
            {
                stage = L"ready";
            }
            else if (result == WAIT_OBJECT_0 + 1)
            {
                if (!GetExitCodeProcess(process.get(), &error))
                {
                    error = GetLastError();
                }
                if (!error)
                {
                    error = ERROR_DLL_INIT_FAILED;
                }
            }
            else
            {
                error = result == WAIT_TIMEOUT ? ERROR_TIMEOUT : GetLastError();
            }
        }
        if (packageState.context && !error && packageState.context->error != ERROR_SUCCESS &&
            packageState.context->error != ERROR_IO_PENDING)
        {
            error = static_cast<DWORD>(packageState.context->error);
            stage = L"package_debugger";
        }
        // Activation may return before the debugger callback runs (notably under WOW64).
        const HRESULT cleanup = packageState.Close();
        if (FAILED(cleanup) && !error)
        {
            error = static_cast<DWORD>(cleanup);
            stage = L"package_cleanup";
        }
        std::wstring actualPackage;
        std::wstring actualAumid;
        if (!error && process)
        {
            UINT32 length = 0;
            LONG status = GetPackageFullName(process.get(), &length, nullptr);
            if (status == ERROR_INSUFFICIENT_BUFFER)
            {
                actualPackage.resize(length);
                status = GetPackageFullName(process.get(), &length, actualPackage.data());
                if (status == ERROR_SUCCESS)
                {
                    actualPackage.resize(length - 1);
                }
            }
            if (status != ERROR_SUCCESS && status != APPMODEL_ERROR_NO_PACKAGE)
            {
                error = static_cast<DWORD>(status);
            }
            length = 0;
            status = GetApplicationUserModelId(process.get(), &length, nullptr);
            if (status == ERROR_INSUFFICIENT_BUFFER)
            {
                actualAumid.resize(length);
                status = GetApplicationUserModelId(process.get(), &length, actualAumid.data());
                if (status == ERROR_SUCCESS)
                {
                    actualAumid.resize(length - 1);
                }
            }
            if (status != ERROR_SUCCESS && status != APPMODEL_ERROR_NO_APPLICATION &&
                status != APPMODEL_ERROR_NO_PACKAGE && !error)
            {
                error = static_cast<DWORD>(status);
            }
            const auto expectedPackage = Value(args, L"--package");
            const auto expectedAumid = Value(args, L"--expected-aumid");
            if ((!expectedPackage.empty() && actualPackage != expectedPackage) ||
                (!expectedAumid.empty() && actualAumid != expectedAumid))
            {
                error = ERROR_INVALID_DATA;
                stage = L"package_identity";
            }
        }
        FILETIME created{};
        if (process && !CreationTime(process.get(), created) && !error)
        {
            error = GetLastError();
            stage = L"process_times";
        }
        if (!SaveResult(output, pid, created, error, stage, actualPackage, actualAumid))
        {
            return GetLastError();
        }
        return error;
    }
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    int count = 0;
    wchar_t** words = CommandLineToArgvW(GetCommandLineW(), &count);
    if (!words)
    {
        return static_cast<int>(GetLastError());
    }
    Arguments args;
    bool valid = true;
    for (int index = 1; index < count; ++index)
    {
        const std::wstring key = words[index];
        if (key == L"--debugger")
        {
            valid = args.emplace(key, L"1").second && valid;
        }
        else if (index + 1 < count && (key == L"--exe" || key == L"--source" || key == L"--private" ||
            key == L"--controls" || key == L"--log" || key == L"--result" || key == L"--arguments" ||
            key == L"--aumid" || key == L"--package" || key == L"--expected-aumid" || key == L"--context" ||
            key == L"-p" || key == L"-tid"))
        {
            valid = args.emplace(key, words[++index]).second && valid;
        }
        else
        {
            valid = false;
        }
    }
    LocalFree(words);
    if (!valid)
    {
        return ERROR_INVALID_PARAMETER;
    }
    return static_cast<int>(args.contains(L"--debugger") ? DebuggerLaunch(args) : Launch(args));
}
