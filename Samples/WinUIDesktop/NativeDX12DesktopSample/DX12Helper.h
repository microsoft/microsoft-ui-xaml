// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winrt/Windows.Storage.h>

// This header contains DX12 helper functions and is available at: https://github.com/Microsoft/DirectX-Graphics-Samples/tree/master/Libraries/D3DX12
//
// It uses WRL which appears to be incompatible with a WINUI3 application.  So it is included in this cpp file rather than any header.
// This also means that we can't use it anywhere exposed to XAML headers, but since the helper structs inherit from and use
// the real struct as a base, we can just use the helper structs for thier added functionality (e.g. initialization).
#include "d3dx12.h"

// Assign a name to the object to aid with debugging.
// Naming helper for ComPtr<T>.
// Assigns the name of the variable as the name of the object.
// The indexed variant will include the index in the name of the object.
#define NAME_D3D12_OBJECT(x) SetName((x).get(), L#x)
#define NAME_D3D12_OBJECT_INDEXED(x, n) SetNameIndexed((x)[n].get(), L#x, n)

#if defined(_DEBUG) || defined(DBG)
inline void SetName(ID3D12Object* pObject, LPCWSTR name)
{
    pObject->SetName(name);
}
inline void SetNameIndexed(ID3D12Object* pObject, LPCWSTR name, UINT index)
{
    WCHAR fullName[50];
    if (swprintf_s(fullName, L"%s[%u]", name, index) > 0)
    {
        pObject->SetName(fullName);
    }
}
#else
inline void SetName(ID3D12Object*, LPCWSTR) {}
inline void SetNameIndexed(ID3D12Object*, LPCWSTR, UINT) {}
#endif

inline std::wstring GetAssetFullPath(const std::wstring& filename)
{
    std::wstring fullFilename = winrt::Windows::ApplicationModel::Package::Current().InstalledLocation().Path().c_str();
    fullFilename.append(L"\\");
    fullFilename.append(filename);
    return fullFilename;
}
