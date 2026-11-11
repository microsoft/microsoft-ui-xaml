#include "..\NativeSupport.h"
#include <shellapi.h>
#include <winmeta.h>
#include <TraceLoggingProvider.h>

TRACELOGGING_DEFINE_PROVIDER(StartupProvider, "WinUIProfiler-StartupFixture",
    (0xb468d34b, 0xf13d, 0x422b, 0x9a, 0x7f, 0x9f, 0xb0, 0xb1, 0x54, 0x7f, 0xb7));

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    int count = 0;
    wchar_t** words = CommandLineToArgvW(GetCommandLineW(), &count);
    if (!words || count != 3) return ERROR_INVALID_PARAMETER;
    const std::wstring output(words[1]);
    const std::wstring mode(words[2]);
    LocalFree(words);
    const auto directory = profiler::ParentPath(profiler::ModulePath(nullptr));
    const wchar_t* role = mode == L"handoff-etw" ? L"parent" :
        mode == L"etw-child" ? L"child" : mode == L"etw-unrelated" ? L"unrelated" : nullptr;
    if (role)
    {
        const HRESULT registered = TraceLoggingRegister(StartupProvider);
        if (FAILED(registered)) return static_cast<int>(registered);
        if (!TraceLoggingProviderEnabled(StartupProvider, WINEVENT_LEVEL_INFO, 0)) return ERROR_INVALID_STATE;
        TraceLoggingWrite(StartupProvider, "Startup", TraceLoggingWideString(role, "Role"));
    }

    if (mode.starts_with(L"handoff") || mode == L"ambiguous" || mode == L"overflow" ||
        mode == L"other-child" || mode == L"create-failure")
    {
        const bool unrelated = mode == L"other-child";
        const bool invalid = mode == L"create-failure";
        const bool ansi = mode.find(L"ansi") != std::wstring::npos;
        const bool asUser = mode.find(L"as-user") != std::wstring::npos;
        const auto executable = directory + (invalid ? L"\\missing.exe" :
            unrelated ? L"\\OtherFixture.exe" : L"\\Fixture.exe");
        const std::wstring childMode = mode == L"handoff-chain" ? L"handoff" :
            mode == L"handoff-etw" ? L"etw-child" : L"runtime";
        const int children = mode == L"overflow" ? 8 : mode == L"ambiguous" ? 2 : 1;
        std::vector<PROCESS_INFORMATION> processes(static_cast<size_t>(children));
        DWORD error = ERROR_SUCCESS;
        for (int index = 0; index < children; ++index)
        {
            std::wstring command = profiler::Quote(executable) + L" " +
                profiler::Quote(unrelated || children > 1 ? output + L"." + std::to_wstring(index) : output) + L" " + childMode;
            DWORD flags = mode == L"handoff-suspended" || children > 1 ? CREATE_SUSPENDED : 0;
            std::vector<wchar_t> environment;
            if (mode == L"handoff-environment" || asUser)
            {
                const auto entry = L"SystemRoot=" + profiler::Environment(L"SystemRoot");
                environment.assign(entry.begin(), entry.end());
                environment.push_back(0);
                environment.push_back(0);
                flags |= CREATE_UNICODE_ENVIRONMENT;
            }
            profiler::Handle token;
            if (asUser)
            {
                HANDLE handle = nullptr;
                if (!OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE | TOKEN_QUERY | TOKEN_ASSIGN_PRIMARY, &handle))
                    return static_cast<int>(GetLastError());
                profiler::Handle sourceToken(handle);
                if (!CreateRestrictedToken(sourceToken.get(), DISABLE_MAX_PRIVILEGE, 0, nullptr, 0, nullptr,
                    0, nullptr, &handle)) return static_cast<int>(GetLastError());
                token.reset(handle);
            }
            BOOL success;
            void* block = environment.empty() ? nullptr : environment.data();
            if (ansi)
            {
                const auto text = L"Fixture.exe " + profiler::Quote(profiler::ShortPath(profiler::ParentPath(output)) +
                    output.substr(output.find_last_of(L'\\'))) + L" " + childMode;
                std::string narrow;
                for (wchar_t ch : text)
                {
                    if (ch > 127) return ERROR_NO_UNICODE_TRANSLATION;
                    narrow += static_cast<char>(ch);
                }
                STARTUPINFOA startup{ sizeof(startup) };
                success = asUser ?
                    CreateProcessAsUserA(token.get(), "Fixture.exe", narrow.data(), nullptr, nullptr, FALSE,
                        flags, block, nullptr, &startup, &processes[index]) :
                    CreateProcessA("Fixture.exe", narrow.data(), nullptr, nullptr, FALSE,
                        flags, block, nullptr, &startup, &processes[index]);
            }
            else
            {
                STARTUPINFOW startup{ sizeof(startup) };
                success = asUser ?
                    CreateProcessAsUserW(token.get(), executable.c_str(), command.data(), nullptr, nullptr, FALSE,
                        flags, block, nullptr, &startup, &processes[index]) :
                    CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, FALSE,
                        flags, block, nullptr, &startup, &processes[index]);
            }
            if (!success)
            {
                error = GetLastError();
                break;
            }
        }
        for (auto& child : processes)
        {
            profiler::Handle process(child.hProcess);
            profiler::Handle thread(child.hThread);
            if (!process) continue;
            if (error) TerminateProcess(process.get(), error);
            else if ((mode == L"handoff-suspended" || children > 1) && ResumeThread(thread.get()) != 1)
            {
                error = ERROR_INVALID_STATE;
                TerminateProcess(process.get(), error);
            }
            if (unrelated && WaitForSingleObject(process.get(), 5000) != WAIT_OBJECT_0) error = ERROR_TIMEOUT;
        }
        if (mode == L"overflow")
        {
            return error == ERROR_BUFFER_OVERFLOW ? 0 : ERROR_INVALID_DATA;
        }
        if (invalid)
        {
            if (error != ERROR_FILE_NOT_FOUND) return ERROR_INVALID_DATA;
        }
        else
        {
            if (error) return static_cast<int>(error);
            if (!unrelated) return 0;
        }
    }

    profiler::Handle selectionSnapshot;
    profiler::Handle childCreated;
    if (mode == L"selection-race")
    {
        const auto barrier = L"Local\\WinUIProfiler.SelectionTest." + std::to_wstring(GetCurrentProcessId());
        selectionSnapshot.reset(CreateEventW(nullptr, TRUE, FALSE, (barrier + L".snapshot").c_str()));
        childCreated.reset(CreateEventW(nullptr, TRUE, FALSE, (barrier + L".created").c_str()));
        if (!selectionSnapshot || !childCreated) return static_cast<int>(GetLastError());
    }

    const auto original = directory + L"\\original\\";
    const DWORD flags = LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS;
    HMODULE mux = LoadLibraryExW((original + L"Microsoft.UI.Xaml.dll").c_str(), nullptr, flags);
    HMODULE muxc = LoadLibraryExW((original + L"Microsoft.UI.Xaml.Controls.dll").c_str(), nullptr, flags);
    if (!mux || !muxc) return static_cast<int>(GetLastError());
    const auto coreBuild = reinterpret_cast<int (*)()>(GetProcAddress(mux, "ProfilerBuildId"));
    const auto controlsBuild = reinterpret_cast<int (*)()>(GetProcAddress(muxc, "ProfilerBuildId"));
    if (!coreBuild || !controlsBuild) return ERROR_PROC_NOT_FOUND;
    const auto text = "{\"pid\":" + std::to_string(GetCurrentProcessId()) +
        ",\"mux\":" + std::to_string(coreBuild()) + ",\"muxc\":" + std::to_string(controlsBuild()) +
        ",\"muxPath\":" + profiler::Json(profiler::ModulePath(mux)) +
        ",\"muxcPath\":" + profiler::Json(profiler::ModulePath(muxc)) + "}\n";
    const auto result = mode == L"selection-race" ? output + L".parent" : output;
    {
        profiler::Handle file(CreateFileW((result + L".tmp").c_str(), GENERIC_WRITE, 0, nullptr,
            CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
        if (!file || !profiler::WriteAll(file.get(), text)) return static_cast<int>(GetLastError());
    }
    if (!MoveFileExW((result + L".tmp").c_str(), result.c_str(), MOVEFILE_WRITE_THROUGH))
        return static_cast<int>(GetLastError());
    if (mode == L"selection-race")
    {
        const DWORD snapshot = WaitForSingleObject(selectionSnapshot.get(), 10000);
        if (snapshot != WAIT_OBJECT_0)
            return static_cast<int>(snapshot == WAIT_TIMEOUT ? ERROR_TIMEOUT : GetLastError());
        const auto executable = directory + L"\\Fixture.exe";
        std::wstring command = profiler::Quote(executable) + L" " + profiler::Quote(output) + L" runtime";
        STARTUPINFOW startup{ sizeof(startup) };
        PROCESS_INFORMATION child{};
        if (!CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, FALSE, 0,
            nullptr, nullptr, &startup, &child)) return static_cast<int>(GetLastError());
        profiler::Handle process(child.hProcess);
        profiler::Handle thread(child.hThread);
        if (!SetEvent(childCreated.get())) return static_cast<int>(GetLastError());
        if (WaitForSingleObject(process.get(), 5000) != WAIT_OBJECT_0)
        {
            TerminateProcess(process.get(), ERROR_TIMEOUT);
            return ERROR_TIMEOUT;
        }
        DWORD error = ERROR_SUCCESS;
        if (!GetExitCodeProcess(process.get(), &error)) return static_cast<int>(GetLastError());
        return static_cast<int>(error);
    }
    if (mode == L"late-child")
    {
        const ULONGLONG started = GetTickCount64();
        while (GetFileAttributesW((output + L".continue").c_str()) == INVALID_FILE_ATTRIBUTES)
        {
            if (GetTickCount64() - started > 10000) return ERROR_TIMEOUT;
            Sleep(10);
        }
        const auto executable = directory + L"\\Fixture.exe";
        std::wstring command = profiler::Quote(executable) + L" " + profiler::Quote(output + L".late") + L" runtime";
        STARTUPINFOW startup{ sizeof(startup) };
        PROCESS_INFORMATION child{};
        if (!CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, FALSE, 0,
            nullptr, nullptr, &startup, &child)) return static_cast<int>(GetLastError());
        profiler::Handle process(child.hProcess);
        profiler::Handle thread(child.hThread);
        if (WaitForSingleObject(process.get(), 5000) != WAIT_OBJECT_0)
        {
            TerminateProcess(process.get(), ERROR_TIMEOUT);
            return ERROR_TIMEOUT;
        }
        DWORD error = ERROR_SUCCESS;
        if (!GetExitCodeProcess(process.get(), &error)) return static_cast<int>(GetLastError());
        return static_cast<int>(error);
    }
    Sleep(1000);
    if (role)
    {
        TraceLoggingWrite(StartupProvider, "AfterSelection", TraceLoggingWideString(role, "Role"));
        TraceLoggingUnregister(StartupProvider);
    }
    Sleep(1000);
    return 0;
}
