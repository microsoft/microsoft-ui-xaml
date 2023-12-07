// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// IPALMemory implementation for memory mapped file
class FileMappedMemory final : public IPALMemory
{
    template <typename Ty, typename... Types> friend xref_ptr<Ty> make_xref(Types&&... Args);

public:
    static _Check_return_ HRESULT Create(_In_ const xstring_ptr& strFilePath, _Outptr_ IPALMemory** ppMemory);

    // IPALMemory implementation
    unsigned int AddRef() const override;
    unsigned int Release() const override;
    unsigned int GetSize() const override { return m_cSize; }
    void* GetAddress() const override { return m_pData; }

private:
    FileMappedMemory();
    ~FileMappedMemory();

    mutable unsigned int m_cRef;
    unsigned int m_cSize;
    HANDLE m_hFile;
    HANDLE m_hMapped;
    _Field_size_(m_cSize) unsigned char* m_pData;
};
