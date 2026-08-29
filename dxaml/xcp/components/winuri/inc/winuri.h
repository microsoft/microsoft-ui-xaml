// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Synopsis:
//      The Windows implementation of the PAL URI object.
// Read WinUri.cpp for details about this factory and the
// CWinUri* implementation

#pragma once

#include "ConcurrencySal.h"

struct IUri;

class CWinUriFactory final
{
    CWinUriFactory() = delete;

public:
    // Creation method for Uri representing Resource
    static _Check_return_ HRESULT Create(
        _In_ XUINT32 cString,
        _In_reads_(cString) const WCHAR *pString,
        _Out_ IPALUri **ppUri);
};

class CWinUriImpl final : public IPALUri, public CInterlockedReferenceCount
{
    // To grant access to the Create methods
    friend HRESULT CWinUriFactory::Create(
        _In_ XUINT32 cString,
        _In_reads_(cString) const WCHAR *pString,
        _Out_ IPALUri **ppUri);

    friend xref_ptr<CWinUriImpl> make_xref<CWinUriImpl>();

    CWinUriImpl() = default;
    ~CWinUriImpl() override;

    // Creation method for Uri representing Resource
    static _Check_return_ HRESULT Create(
        _In_ XUINT32 cString,
        _In_reads_(cString) const WCHAR *pString,
        _Out_ IPALUri **ppUri);

public:

    // Reference count implementation
    XUINT32 AddRef() override  {return CInterlockedReferenceCount::AddRef();}
    XUINT32 Release() override {return CInterlockedReferenceCount::Release();}

    // IPALUri methods
    _Check_return_ HRESULT Clone(_Out_ IPALUri **ppUri) const override;
    _Check_return_ HRESULT Combine(
        _In_ XUINT32 cUri,
        _In_reads_(cUri) const WCHAR* pUri,
        _Outptr_ IPALUri** ppUriCombine
        ) override;
    _Check_return_ HRESULT CreateBaseURI(_Out_ IPALUri **ppBaseUri) override;
    _Check_return_ HRESULT GetCanonical(_Inout_ XUINT32 * pBufferLength,
                                 _Out_writes_to_opt_(*pBufferLength, *pBufferLength) WCHAR * pszBuffer) const override;
    _Check_return_ HRESULT GetCanonical(_Out_ xstring_ptr* pstrCanonical) const override;
    _Check_return_ HRESULT GetExtension(_Inout_ XUINT32 * pBufferLength,
                                 _Out_writes_(*pBufferLength) WCHAR * pszBuffer) override;
    _Check_return_ HRESULT GetFileName(_Inout_ XUINT32 * pBufferLength,
                                 _Out_writes_(*pBufferLength) WCHAR * pszBuffer) override;
    _Check_return_ HRESULT GetHost(_Inout_ XUINT32 * pBufferLength,
                            _Out_writes_opt_(*pBufferLength) WCHAR * pszBuffer) const override;
    _Check_return_ HRESULT GetPassword(_Inout_ XUINT32 * pBufferLength,
                                _Out_writes_(*pBufferLength) WCHAR * pszBuffer) override;
    _Check_return_ HRESULT GetPath(_Inout_ XUINT32 * pBufferLength,
                            _Out_writes_opt_(*pBufferLength) WCHAR * pszBuffer) const override;
    _Check_return_ HRESULT GetScheme(_Inout_ XUINT32 * pBufferLength,
                              _Out_writes_opt_(*pBufferLength) WCHAR * pszBuffer) const override;
    _Check_return_ HRESULT GetUsername(_Inout_ XUINT32 * pBufferLength,
                                _Out_writes_opt_(*pBufferLength) WCHAR * pszBuffer) override;
    _Check_return_ HRESULT GetQueryString(_Inout_ XUINT32 * pBufferLength,
                                _Out_writes_opt_(*pBufferLength) WCHAR * pszBuffer) override;
    _Check_return_ HRESULT GetPortNumber(_Out_ XUINT32 *pPortNumber) override;

    _Check_return_ HRESULT GetFilePath(
        _Inout_ XUINT32 * pBufferLength,
        _Out_writes_opt_(*pBufferLength) WCHAR * pszBuffer) const override;

    _Check_return_ HRESULT TransformToMsResourceUri(_Outptr_ IPALUri **ppUri) const override;
    void SetComponentResourceLocation(_In_ ComponentResourceLocation resourceLocation) override;
    ComponentResourceLocation GetComponentResourceLocation() const override;

private:
    IUri* m_pUri = nullptr;
    ComponentResourceLocation m_componentResourceLocation = ComponentResourceLocation::Application;
};