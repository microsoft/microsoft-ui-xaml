#include "..\NativeSupport.h"
#include <shellapi.h>
#include <thread>

#ifdef PROFILER_STATIC_IMPORT
extern "C" __declspec(dllimport) int ProfilerBuildId();
#endif

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
#ifdef PROFILER_STATIC_IMPORT
    if (ProfilerBuildId() != 1) return ERROR_INVALID_DATA;
#endif
    int count = 0;
    wchar_t** words = CommandLineToArgvW(GetCommandLineW(), &count);
    if (!words || count > 3) return ERROR_INVALID_PARAMETER;
    const auto directory = profiler::ParentPath(profiler::ModulePath(nullptr));
    const std::wstring output = count > 1 ? words[1] : directory + L"\\fixture-result.json";
    const std::wstring mode = count > 2 ? words[2] : L"full";
    LocalFree(words);
    const auto original = directory + L"\\original\\";
    std::wstring core = original + L"Microsoft.UI.Xaml.dll";
    std::wstring controls = original + L"Microsoft.UI.Xaml.Controls.dll";
    auto load = LoadLibraryExW;
    if (mode == L"kernelbase")
        load = reinterpret_cast<decltype(load)>(GetProcAddress(GetModuleHandleW(L"KernelBase.dll"), "LoadLibraryExW"));
    if (!load) return ERROR_PROC_NOT_FOUND;
    if (mode == L"bare")
    {
        if (!SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS) || !AddDllDirectory(original.c_str()))
            return static_cast<int>(GetLastError());
        core = L"Microsoft.UI.Xaml.dll";
        controls = L"Microsoft.UI.Xaml.Controls.dll";
    }
    else if (mode == L"extended")
    {
        core = L"\\\\?\\" + core;
        controls = L"\\\\?\\" + controls;
    }
    const DWORD flags = mode == L"bare" ? LOAD_LIBRARY_SEARCH_DEFAULT_DIRS :
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS;
    HMODULE mux = nullptr;
    HMODULE muxc = nullptr;
    DWORD loadError = ERROR_SUCCESS;
    const auto loadRuntime = [&]()
    {
        mux = mode == L"loadlibrary" ? LoadLibraryW(core.c_str()) : load(core.c_str(), nullptr, flags);
        if (!mux) { loadError = GetLastError(); return; }
        muxc = mode == L"loadlibrary" ? LoadLibraryW(controls.c_str()) : load(controls.c_str(), nullptr, flags);
        if (!muxc) loadError = GetLastError();
    };
    if (mode == L"threaded") std::thread(loadRuntime).join();
    else loadRuntime();
    if (loadError) return static_cast<int>(loadError);
    const auto coreBuild = reinterpret_cast<int (*)()>(GetProcAddress(mux, "ProfilerBuildId"));
    const auto controlsBuild = reinterpret_cast<int (*)()>(GetProcAddress(muxc, "ProfilerBuildId"));
    if (!coreBuild || !controlsBuild) return ERROR_PROC_NOT_FOUND;
    int otherBuild = 0;
    if (mode == L"unrelated")
    {
        HMODULE other = load((directory + L"\\other\\Microsoft.UI.Xaml.dll").c_str(), nullptr, flags);
        if (!other) return static_cast<int>(GetLastError());
        auto build = reinterpret_cast<int (*)()>(GetProcAddress(other, "ProfilerBuildId"));
        if (!build) return ERROR_PROC_NOT_FOUND;
        otherBuild = build();
    }
    const auto result = "{\"mux\":" + std::to_string(coreBuild()) + ",\"muxc\":" +
        std::to_string(controlsBuild()) + ",\"unrelated\":" + std::to_string(otherBuild) +
        ",\"muxPath\":" + profiler::Json(profiler::ModulePath(mux)) +
        ",\"muxcPath\":" + profiler::Json(profiler::ModulePath(muxc)) + "}\n";
    const auto temporary = output + L".tmp";
    {
        profiler::Handle file(CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr,
            CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
        if (!file) return static_cast<int>(GetLastError());
        if (!profiler::WriteAll(file.get(), result)) return static_cast<int>(GetLastError());
    }
    const DWORD error = MoveFileExW(temporary.c_str(), output.c_str(), MOVEFILE_WRITE_THROUGH) ?
        ERROR_SUCCESS : GetLastError();
    Sleep(750);
    return static_cast<int>(error);
}
