// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <windows.h>

// Helper around GetModuleFileName that handles long paths and checks errors.
std::wstring GetModuleFileNameHelper(_In_ HMODULE hModule);

std::wstring GetMuxAbsPath(_In_ LPCWSTR lpLibFileName);

HMODULE WINAPI LoadLibraryExWAbs(
    _In_ LPCWSTR lpLibFileName,
    _Reserved_ HANDLE hFile,
    _In_ DWORD dwFlags
    );

BOOL WINAPI GetModuleHandleExWAbs(
    _In_ DWORD dwFlags,
    _In_opt_ LPCWSTR lpModuleName,
    _Out_ HMODULE* phModule
    );