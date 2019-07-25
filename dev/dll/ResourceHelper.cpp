// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceHelper.h"
#include <languagelists.h>
#include <muiload.h>
#include <windowsstringp.h>

extern HINSTANCE g_hInstance;

void EnsureLibraryLoaded();
void SetProcessMuiLanguages();

void LoadStringResource(
    HINSTANCE module,
    UINT stringId,
    HSTRING *pString
    );

HRSRC FindResourceExWithFallback(
    HINSTANCE hModule,
    LPCTSTR lpType,
    LPCTSTR lpName,
    LANGID wLanguage);

HINSTANCE g_hResource{ nullptr };

winrt::hstring GetLocalizedStringResourceFromMui(int resourceId)
{
    EnsureLibraryLoaded();
    winrt::hstring localizedString;

    LoadStringResource(g_hResource, resourceId, reinterpret_cast<HSTRING*>(winrt::put_abi(localizedString)));

    return localizedString;
}

winrt::array_view<const byte> GetImageBytesFromDll(int assetId)
{
    HRSRC imageResource = FindResource(g_hInstance, MAKEINTRESOURCE(assetId), RT_RCDATA);
    HGLOBAL imageHandle = LoadResource(g_hInstance, imageResource);

    byte *imageBytes = static_cast<byte*>(LockResource(imageHandle));
    DWORD imageSize = SizeofResource(g_hInstance, imageResource);

    if (imageBytes == nullptr || imageSize == 0)
    {
        return winrt::array_view<const byte>();
    }

    return winrt::array_view<const byte>(imageBytes, imageBytes + imageSize);
}

void EnsureLibraryLoaded()
{
    if (!g_hResource)
    {
        SetProcessMuiLanguages();
        g_hResource = LoadMUILibrary(L"windows.ui.xaml.controls.dll", MUI_LANGUAGE_NAME, 0);

        if (!g_hResource)
        {
            winrt::throw_last_error();
        }
    }
}

void SetProcessMuiLanguages()
{
    wchar_t delimiter = L';';
    Windows::Internal::String languages;
    Windows::Internal::String languagesAsMui;
    
    HRESULT hr = GetApplicationLanguages(NULL, delimiter, languages.ReleaseAndGetAddressOf());
    if (FAILED(hr))
    {
        // ignore failure of GetApplicationLanguages() -
        // this will fail when we're hosted and don't have application identity
        return;
    }
    
    winrt::check_hresult(LanguageListAsMuiForm(delimiter, languages.Get(), L'\0', languagesAsMui.ReleaseAndGetAddressOf()));

    BOOL succeeded = SetProcessPreferredUILanguages(MUI_LANGUAGE_NAME, languagesAsMui.GetRawBuffer(NULL), NULL);

    if (!succeeded)
    {
        winrt::throw_last_error();
    }
}

void LoadStringResource(
    HINSTANCE module,
    UINT stringId,
    HSTRING *pString
    )
{
    HRESULT hr = E_FAIL; // Initialize to E_FAIL so that's our default if we didn't find the string.
    LANGID langId = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
    //strings are packed in blocks of 16
    LPTSTR block = MAKEINTRESOURCE(stringId / 16 + 1);
    int offset = stringId % 16;
    HRSRC hResInfo = FindResourceExWithFallback(module, RT_STRING, block, langId);

    if (hResInfo)
    {
        WCHAR *strBlock = static_cast<WCHAR*>(LoadResource(module, hResInfo));
        if (strBlock)
        {
            // Find the string. First character of each string is
            // the string length.
            WCHAR *curr = strBlock;
            int i = 0;
            while (i++ < offset)
            {
                // Skip to next string
                curr += (*curr + 1);
            }

            int len = *curr;
            curr++;

            // Copy string
            hr = WindowsCreateString(curr, len, pString);
        }
    }
    else
    {
        winrt::throw_last_error();
    }

    winrt::check_hresult(hr);
}

HRSRC FindResourceExWithFallback(
    HINSTANCE hModule,
    LPCTSTR lpType,
    LPCTSTR lpName,
    LANGID wLanguage)
{
    HRSRC hIncreaseDialogReference = NULL;

    hIncreaseDialogReference = FindResourceEx(hModule, lpType, lpName, wLanguage);

    if (hIncreaseDialogReference == NULL)
    {
        hIncreaseDialogReference = FindResourceEx(hModule, lpType, lpName, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
    }

    return hIncreaseDialogReference;
}

extern "C"
void
XamlTestHookFreeControlsResourceLibrary()
{
    if (g_hResource)
    {
        FreeMUILibrary(g_hResource);
        g_hResource = nullptr;
    }
}
