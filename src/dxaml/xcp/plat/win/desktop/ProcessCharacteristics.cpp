// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//-----------------------------------------------------------------------------
//
// Returns TRUE if the current process has package identity.
//
//-----------------------------------------------------------------------------
bool CWindowsServices::IsProcessPackaged()
{
    UINT32 n = 0;
    return GetCurrentPackageId(&n, NULL) == ERROR_INSUFFICIENT_BUFFER;
}

//-----------------------------------------------------------------------------
//
// Returns TRUE if the current process is running in an AppContainer.
//
//-----------------------------------------------------------------------------
bool CWindowsServices::IsProcessAppContainer()
{
    HANDLE hToken = NULL;
    ULONG ulIsAppContainer = 0;
    DWORD unused;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        goto Cleanup;
    }

    if (!GetTokenInformation(hToken, TokenIsAppContainer, &ulIsAppContainer, sizeof(ulIsAppContainer), &unused))
    {
        ulIsAppContainer = 0;
    }

Cleanup:
    if (hToken)
    {
        CloseHandle(hToken);
    }

    return ulIsAppContainer != 0;
}

//-----------------------------------------------------------------------------
//
// Returns the package path of the current process.
//
// If the current process isn't packaged, succeeds and returns NULL for the out
// parameter.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT CWindowsServices::GetProcessPackagePath(_Out_ xstring_ptr* pstrPackagePath)
{
    HRESULT hr = S_OK;
    WCHAR wszPackagePath[MAX_PATH + 1];
    UINT32 cchPackagePath = ARRAY_SIZE(wszPackagePath);

    pstrPackagePath->Reset();

    LONG result = GetCurrentPackagePath(&cchPackagePath, wszPackagePath);

    if (ERROR_SUCCESS != result)
    {
        if (APPMODEL_ERROR_NO_PACKAGE == result)
        {
            // Process does not have package identity.
            // Succeed and return NULL for the out param.
            goto Cleanup;
        }

        IFC(HRESULT_FROM_WIN32(result));
    }

    IFC(xstring_ptr::CloneBuffer(wszPackagePath, cchPackagePath, pstrPackagePath));

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// Returns the package full name of the current process.
//
// If the current process isn't packaged, succeeds and returns NULL for the out
// parameter.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT CWindowsServices::GetProcessPackageFullName(_Out_ xstring_ptr* pstrPackageFullName)
{
    HRESULT hr = S_OK;
    WCHAR wszPackageFullName[PACKAGE_FULL_NAME_MAX_LENGTH + 1];
    UINT32 cchPackageFullName = ARRAY_SIZE(wszPackageFullName);

    pstrPackageFullName->Reset();

    LONG result = GetCurrentPackageFullName(&cchPackageFullName, wszPackageFullName);

    if (ERROR_SUCCESS != result)
    {
        if (APPMODEL_ERROR_NO_PACKAGE == result)
        {
            // Process does not have package identity.
            // Succeed and return NULL for the out param.
            goto Cleanup;
        }

        IFC(HRESULT_FROM_WIN32(result));
    }

    IFC(xstring_ptr::CloneBuffer(wszPackageFullName, cchPackageFullName, pstrPackageFullName));

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// Returns the modern application id for the current process.
//
// If the current process isn't packaged, succeeds and returns NULL for the out
// parameter.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT CWindowsServices::GetProcessModernAppId(_Out_ xstring_ptr* pstrAppId)
{
    HRESULT hr = S_OK;
    WCHAR wszAppId[APPLICATION_USER_MODEL_ID_MAX_LENGTH + 1];
    UINT32 cchAppId = ARRAY_SIZE(wszAppId);

    pstrAppId->Reset();

    LONG result = GetCurrentApplicationUserModelId(&cchAppId, wszAppId);

    if (ERROR_SUCCESS != result)
    {
        if (APPMODEL_ERROR_NO_PACKAGE == result ||
            APPMODEL_ERROR_NO_APPLICATION == result)
        {
            // Process does not have package identity.
            // Succeed and return NULL for the out param.
            goto Cleanup;
        }

        IFC(HRESULT_FROM_WIN32(result));
    }

    IFC(xstring_ptr::CloneBuffer(wszAppId, cchAppId, pstrAppId));

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// Returns the image name for the current process.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT CWindowsServices::GetProcessImageName(_Out_ xstring_ptr* pstrImageName)
{
    pstrImageName->Reset();

    // Get an actual handle to the current process rather than the pseudo current process
    // handle which does not support some of the queries we will make.
    auto hProcess = wil::unique_handle(OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId()));
    IFCCHECK_RETURN(hProcess.get());

    // Get the full process image name including the path: make sure it is terminated.
    WCHAR szFullImageName[MAX_PATH + 1];
    DWORD cchPath = ARRAYSIZE(szFullImageName);
    BOOL success = QueryFullProcessImageName(hProcess.get(), 0, szFullImageName, &cchPath);
    IFCCHECK_RETURN(success);
    ASSERT(cchPath < ARRAYSIZE(szFullImageName));
    ASSERT(szFullImageName[cchPath] == 0);

    // Trim any path information from the process name.
    PWSTR pFileName = szFullImageName + cchPath;
    for (; pFileName > szFullImageName; pFileName--)
    {
        WCHAR* pPreviousChar = pFileName - 1;
        ASSERT(pPreviousChar >= szFullImageName);
        if ((*pPreviousChar == L'\\') || (*pPreviousChar == L':'))
        {
            break;
        }
    }
    ASSERT(pFileName >= szFullImageName);

    IFC_RETURN(xstring_ptr::CloneBuffer(pFileName, xstrlen(pFileName), pstrImageName));

    return S_OK;
}
