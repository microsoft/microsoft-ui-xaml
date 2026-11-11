#pragma once

#include "NativeSupport.h"
#include <appmodel.h>
#include <detours.h>

namespace profiler
{
    // EVENT_FILTER_TYPE_PID accepts at most eight processes in one ETW session.
    constexpr size_t MaxStartupProcesses = 8;
    constexpr DWORD NoParent = MAXDWORD;
    constexpr GUID StartupPayloadId{
        0x9fe499dc, 0x506c, 0x4ab5, { 0x9a, 0x74, 0x5c, 0x0d, 0x28, 0xe7, 0x57, 0x41 }
    };

    struct StartupPayload
    {
        DWORD version;
        wchar_t mapping[128];
    };

    struct StartupProcess
    {
        volatile LONG pid;
        volatile LONG initialized;
        DWORD parentIndex;
        FILETIME created;
        ULONG_PTR controllerHandle;
        volatile LONG helperReady;
        volatile LONG runtimeLoaded;
        volatile LONG error;
    };

    struct StartupData
    {
        DWORD size;
        DWORD version;
        DWORD sessionId;
        FILETIME notBefore;
        DWORD launcherPid;
        FILETIME launcherCreated;
        volatile LONG active;
        volatile LONG claimedPid;
        volatile LONG debuggerComplete;
        volatile LONG error;
        DWORD redirectControls;
        wchar_t executable[MaxPathCharacters];
        wchar_t dll[MaxPathCharacters];
        wchar_t source[MaxPathCharacters];
        wchar_t target[MaxPathCharacters];
        wchar_t log[MaxPathCharacters];
        wchar_t result[MaxPathCharacters];
        wchar_t package[MaxPathCharacters];
        wchar_t event[128];
        StartupProcess processes[MaxStartupProcesses];
    };

    inline LONG ReadState(const volatile LONG& value)
    {
        return InterlockedCompareExchange(const_cast<volatile LONG*>(&value), 0, 0);
    }

    class StartupSession
    {
    public:
        StartupSession() = default;
        StartupSession(const StartupSession&) = delete;
        StartupSession& operator=(const StartupSession&) = delete;
        ~StartupSession()
        {
            if (data)
            {
                if (data->launcherPid == GetCurrentProcessId())
                {
                    for (const auto& entry : data->processes)
                    {
                        if (ReadState(entry.initialized))
                        {
                            CloseHandle(reinterpret_cast<HANDLE>(entry.controllerHandle));
                        }
                    }
                }
                UnmapViewOfFile(data);
            }
        }

        bool Create(const std::wstring& name)
        {
            mapping.reset(CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
                0, sizeof(StartupData), name.c_str()));
            if (!mapping) return false;
            if (GetLastError() == ERROR_ALREADY_EXISTS)
            {
                SetLastError(ERROR_ALREADY_EXISTS);
                return false;
            }
            data = static_cast<StartupData*>(MapViewOfFile(mapping.get(),
                FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, sizeof(StartupData)));
            if (!data) return false;
            payload.version = ProtocolVersion;
            wcscpy_s(payload.mapping, name.c_str());
            data->size = sizeof(StartupData);
            data->version = ProtocolVersion;
            data->active = 1;
            GetSystemTimeAsFileTime(&data->notBefore);
            data->launcherPid = GetCurrentProcessId();
            FILETIME exited{}, kernel{}, user{};
            if (!GetProcessTimes(GetCurrentProcess(), &data->launcherCreated, &exited, &kernel, &user)) return false;
            return ProcessIdToSessionId(GetCurrentProcessId(), &data->sessionId) != FALSE;
        }

        bool Open(const StartupPayload& value)
        {
            if (value.version != ProtocolVersion || !wmemchr(value.mapping, 0, ARRAYSIZE(value.mapping)) ||
                !std::wstring_view(value.mapping).starts_with(L"Local\\WinUIProfiler.Redirect."))
            {
                SetLastError(ERROR_INVALID_DATA);
                return false;
            }
            mapping.reset(OpenFileMappingW(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, value.mapping));
            if (!mapping) return false;
            data = static_cast<StartupData*>(MapViewOfFile(mapping.get(),
                FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, sizeof(StartupData)));
            if (!data) return false;
            if (data->size != sizeof(StartupData) || data->version != ProtocolVersion ||
                !wmemchr(data->executable, 0, ARRAYSIZE(data->executable)) ||
                !wmemchr(data->dll, 0, ARRAYSIZE(data->dll)) ||
                !wmemchr(data->source, 0, ARRAYSIZE(data->source)) ||
                !wmemchr(data->target, 0, ARRAYSIZE(data->target)) ||
                !wmemchr(data->log, 0, ARRAYSIZE(data->log)) ||
                !wmemchr(data->result, 0, ARRAYSIZE(data->result)) ||
                !wmemchr(data->package, 0, ARRAYSIZE(data->package)) ||
                !wmemchr(data->event, 0, ARRAYSIZE(data->event)))
            {
                UnmapViewOfFile(data);
                data = nullptr;
                SetLastError(ERROR_INVALID_DATA);
                return false;
            }
            payload = value;
            return true;
        }

        bool OpenPayload()
        {
            DWORD size = 0;
            const auto value = static_cast<const StartupPayload*>(DetourFindPayloadEx(StartupPayloadId, &size));
            if (!value || size != sizeof(StartupPayload))
            {
                SetLastError(ERROR_INVALID_DATA);
                return false;
            }
            return Open(*value);
        }

        bool CopyPayload(HANDLE process) const
        {
            return DetourCopyPayloadToProcess(process, StartupPayloadId, &payload, sizeof(payload)) != FALSE;
        }

        StartupProcess* Find(DWORD pid, const FILETIME& created) const
        {
            for (auto& entry : data->processes)
            {
                if (ReadState(entry.initialized) && static_cast<DWORD>(ReadState(entry.pid)) == pid &&
                    CompareFileTime(&entry.created, &created) == 0) return &entry;
            }
            return nullptr;
        }

        StartupProcess* Register(HANDLE process, DWORD parentIndex)
        {
            const DWORD pid = GetProcessId(process);
            FILETIME created{}, exited{}, kernel{}, user{};
            if (!pid || pid > MAXLONG || !GetProcessTimes(process, &created, &exited, &kernel, &user)) return nullptr;
            for (auto& entry : data->processes)
            {
                const LONG previous = InterlockedCompareExchange(&entry.pid, static_cast<LONG>(pid), 0);
                if (!previous)
                {
                    // Pin the process object in the controller before resuming the child. Even an immediately
                    // exiting bootstrap cannot have its PID reused before the managed consumer binds its handle.
                    Handle controller(OpenProcess(PROCESS_DUP_HANDLE | PROCESS_QUERY_LIMITED_INFORMATION,
                        FALSE, data->launcherPid));
                    FILETIME controllerCreated{};
                    HANDLE retained = nullptr;
                    if (!controller ||
                        !GetProcessTimes(controller.get(), &controllerCreated, &exited, &kernel, &user) ||
                        CompareFileTime(&controllerCreated, &data->launcherCreated) != 0 ||
                        !DuplicateHandle(GetCurrentProcess(), process, controller.get(), &retained,
                            PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, 0))
                    {
                        const DWORD error = GetLastError();
                        InterlockedExchange(&entry.pid, 0);
                        SetLastError(error ? error : ERROR_INVALID_DATA);
                        return nullptr;
                    }
                    entry.parentIndex = parentIndex;
                    entry.created = created;
                    entry.controllerHandle = reinterpret_cast<ULONG_PTR>(retained);
                    InterlockedExchange(&entry.initialized, 1);
                    return &entry;
                }
                if (previous == static_cast<LONG>(pid))
                {
                    const ULONGLONG started = GetTickCount64();
                    while (!ReadState(entry.initialized))
                    {
                        if (GetTickCount64() - started >= StartupTimeoutMs)
                        {
                            SetLastError(ERROR_TIMEOUT);
                            return nullptr;
                        }
                        Sleep(0);
                    }
                    if (CompareFileTime(&entry.created, &created) == 0) return &entry;
                }
            }
            SetLastError(ERROR_BUFFER_OVERFLOW);
            return nullptr;
        }

        DWORD Index(const StartupProcess& process) const
        {
            return static_cast<DWORD>(&process - data->processes);
        }

        StartupData* data = nullptr;
        StartupPayload payload{};

    private:
        Handle mapping;
    };

    inline std::wstring RedirectLog(const StartupData& data, const StartupProcess& process)
    {
        std::wstring path(data.log);
        if (process.parentIndex != NoParent)
        {
            const auto extension = path.find_last_of(L'.');
            path.insert(extension == std::wstring::npos ? path.size() : extension,
                L"." + std::to_wstring(ReadState(process.pid)));
        }
        return path;
    }

    inline bool MatchesStartupIdentity(HANDLE process, const StartupData& data)
    {
        std::wstring path(MaxPathCharacters, L'\0');
        DWORD length = static_cast<DWORD>(path.size());
        DWORD session = 0;
        FILETIME created{}, exited{}, kernel{}, user{};
        if (!QueryFullProcessImageNameW(process, 0, path.data(), &length) ||
            !ProcessIdToSessionId(GetProcessId(process), &session) ||
            !GetProcessTimes(process, &created, &exited, &kernel, &user)) return false;
        path.resize(length);
        if (!EqualPath(path, data.executable) || session != data.sessionId ||
            CompareFileTime(&created, &data.notBefore) < 0) return false;
        UINT32 packageLength = 0;
        LONG status = GetPackageFullName(process, &packageLength, nullptr);
        if (status == APPMODEL_ERROR_NO_PACKAGE) return !data.package[0];
        if (status != ERROR_INSUFFICIENT_BUFFER) return false;
        std::wstring package(packageLength, L'\0');
        status = GetPackageFullName(process, &packageLength, package.data());
        if (status != ERROR_SUCCESS) return false;
        package.resize(packageLength - 1);
        return package == data.package;
    }

    inline bool SameProcessUser(HANDLE process)
    {
        Handle currentToken;
        Handle childToken;
        HANDLE value = nullptr;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &value)) return false;
        currentToken.reset(value);
        if (!OpenProcessToken(process, TOKEN_QUERY, &value)) return false;
        childToken.reset(value);
        alignas(TOKEN_USER) BYTE current[sizeof(TOKEN_USER) + SECURITY_MAX_SID_SIZE]{};
        alignas(TOKEN_USER) BYTE child[sizeof(TOKEN_USER) + SECURITY_MAX_SID_SIZE]{};
        DWORD length = 0;
        DWORD appContainer = 0;
        if (!GetTokenInformation(currentToken.get(), TokenUser, current, sizeof(current), &length) ||
            !GetTokenInformation(childToken.get(), TokenUser, child, sizeof(child), &length) ||
            !GetTokenInformation(childToken.get(), TokenIsAppContainer, &appContainer, sizeof(appContainer), &length))
            return false;
        return !appContainer && EqualSid(reinterpret_cast<TOKEN_USER*>(current)->User.Sid,
            reinterpret_cast<TOKEN_USER*>(child)->User.Sid);
    }
}
