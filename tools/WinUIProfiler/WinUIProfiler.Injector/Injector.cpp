#include <windows.h>
#include <objbase.h>
#include <psapi.h>
#include "..\WinUIProfiler.Tap\Protocol.h"
#include "..\WinUIProfiler.Tap\Tap.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <cwctype>
#include <string>
#include <stdexcept>
#include <vector>

namespace
{
#ifdef _M_ARM64
    constexpr USHORT Machine = IMAGE_FILE_MACHINE_ARM64;
#else
    constexpr USHORT Machine = IMAGE_FILE_MACHINE_AMD64;
#endif
    using WinUIProfiler::Tap::Handle;

    HRESULT LocalPath(const wchar_t* input, std::wstring& path)
    {
        if (!input || wcsnlen_s(input, 32768) < 3 || wcsnlen_s(input, 32768) >= 32768 ||
            !((input[0] >= L'A' && input[0] <= L'Z') || (input[0] >= L'a' && input[0] <= L'z')) ||
            input[1] != L':' || input[2] != L'\\') return E_INVALIDARG;
        std::vector<wchar_t> buffer(32768);
        const DWORD length = GetFullPathNameW(input, static_cast<DWORD>(buffer.size()), buffer.data(), nullptr);
        if (!length) return HRESULT_FROM_WIN32(GetLastError());
        if (length >= buffer.size()) return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
        path.assign(buffer.data(), length);
        return S_OK;
    }

    HRESULT ReadExact(HANDLE file, void* buffer, DWORD length) noexcept
    {
        DWORD read{};
        if (!ReadFile(file, buffer, length, &read, nullptr)) return HRESULT_FROM_WIN32(GetLastError());
        return read == length ? S_OK : HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    }

    HRESULT VerifyImage(const std::wstring& path) noexcept
    {
        Handle file(CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr));
        if (file.Get() == INVALID_HANDLE_VALUE) return HRESULT_FROM_WIN32(GetLastError());
        IMAGE_DOS_HEADER dos{};
        HRESULT result = ReadExact(file.Get(), &dos, sizeof(dos));
        if (FAILED(result)) return result;
        if (dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew < sizeof(dos) || dos.e_lfanew > 1024 * 1024)
            return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
        LARGE_INTEGER offset{};
        offset.QuadPart = dos.e_lfanew;
        if (!SetFilePointerEx(file.Get(), offset, nullptr, FILE_BEGIN)) return HRESULT_FROM_WIN32(GetLastError());
        DWORD signature{};
        IMAGE_FILE_HEADER header{};
        result = ReadExact(file.Get(), &signature, sizeof(signature));
        if (SUCCEEDED(result)) result = ReadExact(file.Get(), &header, sizeof(header));
        if (FAILED(result)) return result;
        return signature == IMAGE_NT_SIGNATURE && header.Machine == Machine ? S_OK :
            HRESULT_FROM_WIN32(ERROR_EXE_MACHINE_TYPE_MISMATCH);
    }

    HRESULT VerifyCreation(HANDLE process, uint64_t expected) noexcept
    {
        FILETIME creation{}, exit{}, kernel{}, user{};
        if (!GetProcessTimes(process, &creation, &exit, &kernel, &user)) return HRESULT_FROM_WIN32(GetLastError());
        const auto actual = (static_cast<uint64_t>(creation.dwHighDateTime) << 32) | creation.dwLowDateTime;
        return actual == expected ? S_OK : E_ACCESSDENIED;
    }

    HRESULT TokenUser(HANDLE process, std::vector<uint8_t>& bytes)
    {
        HANDLE raw{};
        if (!OpenProcessToken(process, TOKEN_QUERY, &raw)) return HRESULT_FROM_WIN32(GetLastError());
        Handle token(raw);
        DWORD length{};
        if (GetTokenInformation(token.Get(), ::TokenUser, nullptr, 0, &length) ||
            GetLastError() != ERROR_INSUFFICIENT_BUFFER || length > 65536) return E_UNEXPECTED;
        bytes.resize(length);
        return GetTokenInformation(token.Get(), ::TokenUser, bytes.data(), length, &length) ? S_OK :
            HRESULT_FROM_WIN32(GetLastError());
    }

    HRESULT VerifyUser(HANDLE process)
    {
        std::vector<uint8_t> current, target;
        HRESULT result = TokenUser(GetCurrentProcess(), current);
        if (SUCCEEDED(result)) result = TokenUser(process, target);
        if (FAILED(result)) return result;
        return EqualSid(reinterpret_cast<TOKEN_USER*>(current.data())->User.Sid,
            reinterpret_cast<TOKEN_USER*>(target.data())->User.Sid) ? S_OK : E_ACCESSDENIED;
    }

    HRESULT VerifyLoaded(HANDLE process, const std::wstring& runtime, const std::wstring& udk)
    {
        std::array<HMODULE, 1024> modules{};
        DWORD required{};
        if (!EnumProcessModulesEx(process, modules.data(), static_cast<DWORD>(sizeof(modules)),
            &required, LIST_MODULES_ALL)) return HRESULT_FROM_WIN32(GetLastError());
        if (required > sizeof(modules) || required % sizeof(HMODULE)) return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
        std::vector<wchar_t> path(32768);
        bool hasRuntime = false, hasUdk = false;
        for (size_t index = 0; index < required / sizeof(HMODULE); ++index)
        {
            const DWORD length = GetModuleFileNameExW(process, modules[index], path.data(), static_cast<DWORD>(path.size()));
            if (!length) return HRESULT_FROM_WIN32(GetLastError());
            if (length >= path.size()) return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
            hasRuntime |= _wcsicmp(path.data(), runtime.c_str()) == 0;
            hasUdk |= _wcsicmp(path.data(), udk.c_str()) == 0;
        }
        return hasRuntime && hasUdk ? S_OK : HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
    }

    HRESULT Inject(int argc, wchar_t** argv, const char*& phase)
    {
        phase = "arguments/trust";
        if (argc != 6 || wcscmp(argv[5], L"--trust-local-binaries")) return E_INVALIDARG;
        std::wstring runtime, udk, tap;
        HRESULT result = LocalPath(argv[1], runtime);
        if (SUCCEEDED(result)) result = LocalPath(argv[2], udk);
        if (SUCCEEDED(result)) result = LocalPath(argv[3], tap);
        if (FAILED(result)) return result;
        if (tap.size() >= MAX_PATH) return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
        if (_wcsicmp(runtime.substr(runtime.find_last_of(L'\\') + 1).c_str(), L"Microsoft.UI.Xaml.dll") ||
            _wcsicmp(udk.substr(udk.find_last_of(L'\\') + 1).c_str(), L"Microsoft.Internal.FrameworkUdk.dll"))
            return E_INVALIDARG;
        uint32_t endpoint{};
        for (size_t index = 0; argv[4][index]; ++index)
        {
            const wchar_t digit = argv[4][index];
            if (index >= 5 || digit < L'0' || digit > L'9') return E_INVALIDARG;
            endpoint = endpoint * 10 + static_cast<uint32_t>(digit - L'0');
        }
        if (!endpoint || endpoint > 65535) return E_INVALIDARG;
        phase = "bootstrap";
        wchar_t initialization[1026]{};
        if (!fgetws(initialization, ARRAYSIZE(initialization), stdin)) return E_INVALIDARG;
        size_t length = wcsnlen_s(initialization, ARRAYSIZE(initialization));
        if (length && initialization[length - 1] == L'\n') initialization[--length] = 0;
        if (length && initialization[length - 1] == L'\r') initialization[--length] = 0;
        WinUIProfiler::Tap::Bootstrap bootstrap{};
        result = WinUIProfiler::Tap::ParseBootstrap(initialization, bootstrap);
        if (FAILED(result)) return result;
        phase = "target identity/architecture/user";
        Handle process(OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, bootstrap.Target.Pid));
        if (!process.Get()) return HRESULT_FROM_WIN32(GetLastError());
        result = VerifyCreation(process.Get(), bootstrap.Target.CreationTime);
        if (SUCCEEDED(result)) result = VerifyUser(process.Get());
        if (FAILED(result)) return result;
        USHORT processMachine{}, nativeMachine{};
        if (!IsWow64Process2(process.Get(), &processMachine, &nativeMachine)) return HRESULT_FROM_WIN32(GetLastError());
        if ((processMachine ? processMachine : nativeMachine) != Machine)
            return HRESULT_FROM_WIN32(ERROR_EXE_MACHINE_TYPE_MISMATCH);
        phase = "trusted native images/loaded runtime";
        for (const auto* path : {&runtime, &udk, &tap})
        {
            result = VerifyImage(*path);
            if (FAILED(result)) return result;
        }
        result = VerifyLoaded(process.Get(), runtime, udk);
        if (FAILED(result)) return result;
        phase = "FrameworkUdk diagnostics initializer";
        const auto compact = WinUIProfiler::Tap::EncodeBootstrap(bootstrap);
        HMODULE module = LoadLibraryExW(udk.c_str(), nullptr,
            LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!module) return HRESULT_FROM_WIN32(GetLastError());
        const FARPROC address = GetProcAddress(module, "InitializeXamlDiagnosticsEx");
        using Initialize = HRESULT(WINAPI*)(LPCWSTR, DWORD, LPCWSTR, LPCWSTR, CLSID, LPCWSTR);
        Initialize initialize{};
        static_assert(sizeof(initialize) == sizeof(address));
        std::memcpy(&initialize, &address, sizeof(initialize));
        if (!initialize) { FreeLibrary(module); return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND); }
        wchar_t port[64]{};
        swprintf_s(port, L"WinUIVisualDiagConnection%u", endpoint);
        result = initialize(port, bootstrap.Target.Pid, L"", tap.c_str(), WinUIProfiler::Tap::ClassId, compact.c_str());
        FreeLibrary(module);
        return result;
    }
}

int wmain(int argc, wchar_t** argv)
{
    const char* phase = "COM initialization";
    const HRESULT apartment = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    HRESULT result = apartment;
    if (SUCCEEDED(apartment))
    {
        try { result = Inject(argc, argv, phase); }
        catch (const std::bad_alloc&) { result = E_OUTOFMEMORY; }
        catch (const std::invalid_argument&) { result = E_INVALIDARG; }
        CoUninitialize();
    }
    if (FAILED(result))
        std::fprintf(stderr, "WinUIProfiler injector %s failed: HRESULT 0x%08lX.\n",
            phase, static_cast<unsigned long>(result));
    std::printf("{\"schemaVersion\":1,\"hresult\":%lu,\"injectionAccepted\":%s}\n",
        static_cast<unsigned long>(result), SUCCEEDED(result) ? "true" : "false");
    return static_cast<int>(result);
}
