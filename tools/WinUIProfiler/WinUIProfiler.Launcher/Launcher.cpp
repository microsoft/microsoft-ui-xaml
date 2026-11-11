#include "StartupSession.h"
#include <shellapi.h>
#include <shobjidl.h>
#include <appmodel.h>
#include <wrl/client.h>
#include <format>
#include <map>
#include <array>

using Microsoft::WRL::ComPtr;

namespace
{
    using Arguments = std::map<std::wstring, std::wstring>;

    struct PackageLaunchState
    {
        bool initialized = false;
        bool enabled = false;
        std::wstring package;
        ComPtr<IPackageDebugSettings> debugging;
        ComPtr<IApplicationActivationManager> activation;

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
        std::wstring_view stage, std::wstring_view package = {}, std::wstring_view aumid = {},
        const profiler::StartupSession* session = nullptr)
    {
        const auto creation = (static_cast<ULONGLONG>(created.dwHighDateTime) << 32) | created.dwLowDateTime;
        std::string details;
        if (session)
        {
            details = ",\"activationPid\":" + std::to_string(profiler::ReadState(session->data->claimedPid)) +
                ",\"processes\":[";
            bool first = true;
            for (const auto& entry : session->data->processes)
            {
                if (!profiler::ReadState(entry.initialized)) continue;
                if (!first) details += ',';
                first = false;
                const DWORD parentPid = entry.parentIndex < profiler::MaxStartupProcesses ?
                    static_cast<DWORD>(profiler::ReadState(session->data->processes[entry.parentIndex].pid)) : 0;
                const auto entryCreated = (static_cast<ULONGLONG>(entry.created.dwHighDateTime) << 32) |
                    entry.created.dwLowDateTime;
                details += std::format(
                    R"({{"pid":{},"parentPid":{},"creationTimeFileTime":{},"helperReady":{},)"
                    R"("runtimeLoaded":{},"error":{}}})",
                    profiler::ReadState(entry.pid), parentPid, entryCreated,
                    profiler::ReadState(entry.helperReady) != 0, profiler::ReadState(entry.runtimeLoaded) != 0,
                    static_cast<DWORD>(profiler::ReadState(entry.error)));
            }
            details += ']';
        }
        const auto line = std::format(
            R"({{"schemaVersion":{},"pid":{},"creationTimeFileTime":{},"error":{},)"
            R"("stage":{},"packageFullName":{},"aumid":{}{}}})" "\n",
            session ? 2 : 1, pid, creation, error, profiler::Json(stage), profiler::Json(package), profiler::Json(aumid), details);
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
        if (!pid || !tid || !contextName.starts_with(L"Local\\WinUIProfiler.Redirect."))
        {
            return ERROR_INVALID_PARAMETER;
        }
        profiler::StartupSession context;
        profiler::StartupPayload payload{};
        payload.version = profiler::StartupProtocolVersion;
        if (contextName.size() >= ARRAYSIZE(payload.mapping)) return ERROR_INVALID_PARAMETER;
        wcscpy_s(payload.mapping, contextName.c_str());
        if (!context.Open(payload))
        {
            const DWORD error = GetLastError();
            // The launch can be cancelled before this callback starts. Resume the callback's thread, without injection.
            const DWORD resumed = ResumeDebugThread(pid, tid);
            OutputDebugStringW(L"WinUI profiler: launch context expired; startup helper was not inserted.\n");
            return resumed ? resumed : error;
        }
        profiler::Handle process(OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
            PROCESS_VM_READ | PROCESS_VM_WRITE | SYNCHRONIZE, FALSE, pid));
        const DWORD processError = process ? ERROR_SUCCESS : GetLastError();
        profiler::Handle thread(OpenThread(THREAD_SUSPEND_RESUME | THREAD_QUERY_INFORMATION, FALSE, tid));
        const DWORD threadError = thread ? ERROR_SUCCESS : GetLastError();
        DWORD error = ERROR_SUCCESS;
        bool claimed = false;
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
        else if (CompareFileTime(&created, &context.data->notBefore) < 0 || session != context.data->sessionId)
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
            if (!profiler::EqualPath(actual, context.data->executable) || !profiler::SameProcessUser(process.get()))
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
                    if (package != context.data->package)
                    {
                        error = ERROR_INVALID_DATA;
                    }
                }
            }
            if (!error && InterlockedCompareExchange(&context.data->claimedPid, static_cast<LONG>(pid), 0) != 0)
            {
                error = ERROR_ALREADY_EXISTS;
            }
            if (!error)
            {
                claimed = true;
                if (!context.Register(process.get(), profiler::NoParent) || !context.CopyPayload(process.get()))
                {
                    error = GetLastError();
                }
            }
            if (!error)
            {
                const auto dll = profiler::ImportPath(context.data->dll);
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
        if (claimed)
        {
            if (!SaveResult(context.data->result, pid, created, error, L"package_debugger") && !error)
            {
                error = GetLastError();
            }
            if (error) InterlockedCompareExchange(&context.data->error, static_cast<LONG>(error), 0);
            InterlockedExchange(&context.data->debuggerComplete, 1);
        }
        return error;
    }

    DWORD WaitForRuntime(profiler::StartupSession& session, HANDLE changed,
        PackageLaunchState& packageState, DWORD& pid, std::wstring& stage)
    {
        const ULONGLONG started = GetTickCount64();
        while (GetTickCount64() - started < profiler::StartupTimeoutMs)
        {
            const LONG64 activity = profiler::ReadState(session.data->active);
            if (const LONG error = profiler::ReadState(session.data->error)) return static_cast<DWORD>(error);
            std::array<bool, profiler::MaxStartupProcesses> alive{};
            bool anyAlive = false;
            bool anyRegistered = false;
            DWORD exitError = ERROR_DLL_INIT_FAILED;
            for (size_t index = 0; index < alive.size(); ++index)
            {
                auto& entry = session.data->processes[index];
                if (!profiler::ReadState(entry.initialized)) continue;
                anyRegistered = true;
                if (packageState.enabled && entry.parentIndex == profiler::NoParent &&
                    profiler::ReadState(entry.helperReady) && profiler::ReadState(session.data->debuggerComplete))
                {
                    const HRESULT result = packageState.Close();
                    if (FAILED(result))
                    {
                        stage = L"package_cleanup";
                        return static_cast<DWORD>(result);
                    }
                }
                const auto process = reinterpret_cast<HANDLE>(entry.controllerHandle);
                const DWORD state = WaitForSingleObject(process, 0);
                if (state == WAIT_TIMEOUT) anyAlive = alive[index] = true;
                else if (state == WAIT_FAILED) return GetLastError();
                else
                {
                    DWORD code = 0;
                    if (!GetExitCodeProcess(process, &code)) return GetLastError();
                    if (code) exitError = code;
                }
            }
            DWORD selected = 0;
            size_t liveLeaves = 0;
            for (size_t index = 0; index < alive.size(); ++index)
            {
                const auto& entry = session.data->processes[index];
                if (!alive[index]) continue;
                bool hasDescendant = false;
                for (size_t child = 0; child < alive.size(); ++child)
                {
                    if (!alive[child]) continue;
                    DWORD parent = session.data->processes[child].parentIndex;
                    for (size_t depth = 0; parent < alive.size() && depth < alive.size(); ++depth)
                    {
                        if (parent == index) hasDescendant = true;
                        parent = session.data->processes[parent].parentIndex;
                    }
                }
                if (hasDescendant) continue;
                ++liveLeaves;
                if (!profiler::ReadState(entry.runtimeLoaded)) continue;
                if (selected)
                {
                    stage = L"runtime_ambiguous";
                    return ERROR_MORE_DATA;
                }
                selected = static_cast<DWORD>(profiler::ReadState(entry.pid));
            }
            // A completed creation must invalidate the snapshot, even after its activity reference is released.
            if (selected && liveLeaves == 1 && !packageState.enabled)
            {
#if defined(PROFILER_STARTUP_SELECTION_TEST)
                const auto barrier = L"Local\\WinUIProfiler.SelectionTest." +
                    std::to_wstring(profiler::ReadState(session.data->claimedPid));
                profiler::Handle snapshot(OpenEventW(EVENT_MODIFY_STATE, FALSE, (barrier + L".snapshot").c_str()));
                if (snapshot)
                {
                    profiler::Handle created(OpenEventW(SYNCHRONIZE, FALSE, (barrier + L".created").c_str()));
                    if (!created || !SetEvent(snapshot.get())) return GetLastError();
                    const DWORD ready = WaitForSingleObject(created.get(), 10000);
                    if (ready != WAIT_OBJECT_0) return ready == WAIT_TIMEOUT ? ERROR_TIMEOUT : GetLastError();
                }
#endif
                if (profiler::StartupActivityCount(activity) == 1 &&
                    InterlockedCompareExchange64(&session.data->active, 0, activity) == activity)
                {
                    if (const LONG error = profiler::ReadState(session.data->error)) return static_cast<DWORD>(error);
                    pid = selected;
                    return ERROR_SUCCESS;
                }
            }
            if (anyRegistered && !anyAlive) return exitError;
            if (WaitForSingleObject(changed, 50) == WAIT_FAILED) return GetLastError();
        }
        return ERROR_TIMEOUT;
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
            log.size() + 16 >= profiler::MaxPathCharacters || dll.size() >= profiler::MaxPathCharacters ||
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
        const std::wstring eventName = std::wstring(L"Local\\WinUIProfiler.Redirect.") + guid;
        profiler::Handle ready(CreateEventW(nullptr, FALSE, FALSE, eventName.c_str()));
        if (!ready)
        {
            return GetLastError();
        }
        profiler::StartupSession session;
        const auto contextName = eventName + L".Context";
        if (!session.Create(contextName))
        {
            error = GetLastError();
            SaveResult(output, 0, {}, error, L"configuration");
            return error;
        }
        wcscpy_s(session.data->executable, exe.c_str());
        wcscpy_s(session.data->dll, dll.c_str());
        wcscpy_s(session.data->source, source.c_str());
        wcscpy_s(session.data->target, target.c_str());
        wcscpy_s(session.data->log, log.c_str());
        wcscpy_s(session.data->result, (output + L".debugger.json").c_str());
        wcscpy_s(session.data->package, Value(args, L"--package").c_str());
        wcscpy_s(session.data->event, eventName.c_str());
        session.data->redirectControls = Value(args, L"--controls") == L"1";
        PROCESS_INFORMATION processInfo{};
        const auto aumid = Value(args, L"--aumid");
        PackageLaunchState packageState;
        if (aumid.empty())
        {
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
                InterlockedExchange(&session.data->claimedPid, static_cast<LONG>(pid));
                if (!session.Register(processInfo.hProcess, profiler::NoParent) || !session.CopyPayload(processInfo.hProcess))
                {
                    error = GetLastError();
                    stage = L"insert_helper";
                    TerminateProcess(processInfo.hProcess, error);
                }
                else if (ResumeThread(processInfo.hThread) == static_cast<DWORD>(-1))
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
                stage = L"package_debug_settings";
                hr = CoCreateInstance(CLSID_PackageDebugSettings, nullptr, CLSCTX_INPROC_SERVER,
                    IID_PPV_ARGS(&debugging));
                packageState.package = Value(args, L"--package");
                const auto& package = packageState.package;
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
                    hr = debugging->EnableDebugging(package.c_str(), command.c_str(), nullptr);
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
                if (session.data)
                {
                    if (!pid && profiler::ReadState(session.data->claimedPid) > 0)
                    {
                        pid = static_cast<DWORD>(profiler::ReadState(session.data->claimedPid));
                    }
                    if (profiler::ReadState(session.data->error) && !error)
                    {
                        error = static_cast<DWORD>(profiler::ReadState(session.data->error));
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
            stage = L"runtime_ready";
            error = WaitForRuntime(session, ready.get(), packageState, pid, stage);
            if (!error)
            {
                stage = L"ready";
                process.reset(OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid));
                if (!process) error = GetLastError();
            }
        }
        const auto closing = GetTickCount64();
        while (profiler::StartupActivityCount(profiler::ReadState(session.data->active)) > 1 &&
            GetTickCount64() - closing < 1000)
        {
            Sleep(1);
        }
        InterlockedExchange64(&session.data->active, 0);
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
        if (!SaveResult(output, pid, created, error, stage, actualPackage, actualAumid, &session))
        {
            return GetLastError();
        }
        // Keep the registered process objects, even exited bootstraps, until the consumer owns matching handles.
        // A missing/crashed consumer cannot leave a permanent native launcher behind.
        if (!error)
        {
            const auto accepted = output + L".accepted";
            const auto published = GetTickCount64();
            while (GetFileAttributesW(accepted.c_str()) == INVALID_FILE_ATTRIBUTES &&
                GetTickCount64() - published < 10000)
            {
                Sleep(20);
            }
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
