// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "LoadLibraryAbs.h"

// Temporary wchar_t mangling compatibility shim for microsoft.internal.winuidetails NuGet package.
// The NuGet was compiled with /Zc:wchar_t- (wchar_t = unsigned short).
// This repo now uses /Zc:wchar_t (wchar_t = native type).
// These aliases let the NuGet's InputHelpers.obj resolve GetModuleHandleExWAbs.
// Remove once the NuGet is rebuilt with native wchar_t.
#  if defined(_M_X64) || defined(_M_ARM64EC) || defined(_M_ARM64)
#    pragma comment(linker, "/alternatename:?GetModuleHandleExWAbs@@YAHKPEBGPEAPEAUHINSTANCE__@@@Z=?GetModuleHandleExWAbs@@YAHKPEB_WPEAPEAUHINSTANCE__@@@Z")
#
#    if defined(_M_ARM64EC)
#      pragma comment(linker, "/alternatename:?GetModuleHandleExWAbs@@$$hYAHKPEBGPEAPEAUHINSTANCE__@@@Z=?GetModuleHandleExWAbs@@$$hYAHKPEB_WPEAPEAUHINSTANCE__@@@Z")
#    endif
#
#  elif defined(_M_IX86)
#    pragma comment(linker, "/alternatename:?GetModuleHandleExWAbs@@YGHKPBGPAPAUHINSTANCE__@@@Z=?GetModuleHandleExWAbs@@YGHKPB_WPAPAUHINSTANCE__@@@Z")
#  endif

#include <windows.h>
#include <string>
#include <minerror.h>
#include <shlwapi.h>
#include <strsafe.h>

std::wstring GetModuleFileNameHelper(_In_ HMODULE hModule)
{
    WCHAR buffer[MAX_PATH];
    DWORD len = GetModuleFileNameW(hModule, buffer, MAX_PATH);
    IFCW32FAILFAST(len);

    if (len < MAX_PATH)
    {
        return std::wstring(buffer, len);
    }

    std::wstring largeBuffer(UNICODE_STRING_MAX_CHARS, L'\0');
    len = GetModuleFileNameW(hModule, largeBuffer.data(), UNICODE_STRING_MAX_CHARS);
    if (len == 0 || len == UNICODE_STRING_MAX_CHARS)
    {
        IFCFAILFAST(HRESULT_FROM_WIN32(GetLastError()));
    }
    largeBuffer.resize(len);
    return largeBuffer;
}

static const std::wstring& GetMuxPath()
{
    static const std::wstring muxPath = [] {
        // Use GetModuleHandleEx with FROM_ADDRESS to avoid ambiguity when multiple
        // modules named Microsoft.UI.Xaml.dll are loaded (e.g. WinUI2 and WinUI3).
        HMODULE hModule = nullptr;

        IFCW32FAILFAST(GetModuleHandleExW(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(&GetMuxPath),
                &hModule));

        std::wstring result = GetModuleFileNameHelper(hModule);
        // Remove the filename to get the directory path.
        auto lastSlash = result.find_last_of(L'\\');
        FAIL_FAST_ASSERT(lastSlash != std::wstring::npos);
        result.resize(lastSlash + 1); // Keep trailing backslash — caller(s) append filenames directly.
        return result;
    }();

    return muxPath;
}

std::wstring GetMuxAbsPath(_In_ LPCWSTR lpLibFileName)
{
    FAIL_FAST_ASSERT(lpLibFileName != nullptr && L'\0' != lpLibFileName[0]);
    auto muxPath = GetMuxPath();
    FAIL_FAST_ASSERT(!muxPath.empty());
    
    if (PathIsRelativeW(lpLibFileName))
    {
        return muxPath + lpLibFileName;
    }
    else
    {
        // Absolute path, so ensure it's inside muxPath
        FAIL_FAST_ASSERT(_wcsnicmp(lpLibFileName, muxPath.c_str(), muxPath.size()) == 0);
        return lpLibFileName;
    }
}

HMODULE WINAPI LoadLibraryExWAbs(
    _In_ LPCWSTR lpLibFileName,
    _Reserved_ HANDLE hFile,
    _In_ DWORD dwFlags
    )
{
    // Ensure caller's flags are compatible with LOAD_WITH_ALTERED_SEARCH_PATH
    FAIL_FAST_ASSERT((dwFlags & (LOAD_LIBRARY_SEARCH_APPLICATION_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS|LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32|LOAD_LIBRARY_SEARCH_USER_DIRS)) == 0);
    return LoadLibraryExW(GetMuxAbsPath(lpLibFileName).c_str(), hFile, dwFlags | LOAD_WITH_ALTERED_SEARCH_PATH);
}

BOOL WINAPI GetModuleHandleExWAbs(
    _In_ DWORD dwFlags,
    _In_opt_ LPCWSTR lpModuleName,
    _Out_ HMODULE* phModule
    )
{
    return GetModuleHandleExW(dwFlags, GetMuxAbsPath(lpModuleName).c_str(), phModule);
}