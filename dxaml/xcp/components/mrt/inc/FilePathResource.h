// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <BasePALResource.h>

// IPALResource implementation that represents a file path.
class CFilePathResource final : public CBasePALResource
{
    template <typename Ty, typename... Types> friend xref_ptr<Ty> make_xref(Types&&... Args);

public:
    static _Check_return_ HRESULT Create(_In_ IPALUri* pResourceUri, _In_ const xstring_ptr& strFilePath, _Outptr_ IPALResource** ppResource);
    static _Check_return_ HRESULT Create(_In_ IPALUri* pResourceUri, _In_reads_(cchFilePath) WCHAR* wszFilePath, unsigned int cchFilePath, _Outptr_ IPALResource** ppResource);

    // CBasePALResource
    _Check_return_ HRESULT Load(_Outptr_ IPALMemory** ppMemory) override;
    const WCHAR* ToString() override;
    _Check_return_ HRESULT Exists(_Out_ bool* pfExists) override;
    _Check_return_ HRESULT TryGetFilePath(_Out_ xstring_ptr* pstrFilePath) override;

private:
    CFilePathResource(_In_ IPALUri* pResourceUri);
    ~CFilePathResource() override;

    xstring_ptr m_strFilePath;
    xref_ptr<IPALMemory> m_memory;
};




