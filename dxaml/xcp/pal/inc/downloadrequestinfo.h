// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CDownloadRequestInfo final : public CXcpObjectBase<IPALDownloadRequest, CXcpObjectThreadSafeAddRefPolicy>
{
protected:
    CDownloadRequestInfo();
    ~CDownloadRequestInfo() override;
public:
    static _Check_return_ HRESULT Create(
        _In_ const xstring_ptr& strUrl,
        _In_ IPALDownloadResponseCallback* pCallback,
        _Outptr_ CDownloadRequestInfo** ppDownloadReq
        );

    static _Check_return_ HRESULT Create(
        _In_ const xstring_ptr& strUrl,
        _In_opt_ IPALUri* pAbsoluteUri,
        _In_ IPALDownloadResponseCallback* pCallback,
        _Outptr_ CDownloadRequestInfo** ppDownloadReq
        );

    _Check_return_ HRESULT GetUrl(_Out_ xstring_ptr* pstrUrl) override;
    _Check_return_ HRESULT GetAbsoluteUri(_Outptr_result_maybenull_ IPALUri** ppAbsoluteUri) override;
    _Check_return_ HRESULT GetCallback(_Outptr_ IPALDownloadResponseCallback** ppCallback) override;
    _Check_return_ HRESULT GetOptionFlags(_Out_ XUINT32* pDownloadOptions) override;
    virtual _Check_return_ HRESULT SetOptionFlags(_In_ XUINT32 downloadOptions);
    _Check_return_ HRESULT GetUseDefaultCredentials(_Out_ bool* pbUseDefaultCredentials) override;
    virtual _Check_return_ HRESULT SetCredentials(_In_ const xstring_ptr& strUsername, _In_ const xstring_ptr& strPassword, _In_ const xstring_ptr& strAuthDomain);
    _Check_return_ HRESULT GetUsername(_Out_ xstring_ptr* pstrUsername) override;
    _Check_return_ HRESULT GetPassword(_Out_ xstring_ptr* pstrPassword) override;
    _Check_return_ HRESULT GetAuthDomain(_Out_ xstring_ptr* pstrAuthDomain) override;
private:

    _Check_return_ HRESULT Initialize(
        _In_ const xstring_ptr& strUrl,
        _In_opt_ IPALUri* pAbsoluteUri,
        _In_ IPALDownloadResponseCallback* pCallback
        );

private:
    XUINT32 m_cRef;

    xstring_ptr m_strUrl;
    IPALUri* m_pAbsoluteUri; // Cached to avoid redoing expensive URI combine operations.
    IPALDownloadResponseCallback* m_pCallback;
    XUINT32 m_flags;
    bool m_bUseDefaultCreds;
    xstring_ptr m_strUsername;
    xstring_ptr m_strPassword;
    xstring_ptr m_strAuthDomain;

    wil::critical_section m_Lock;
};
