// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ErrorInfo.h"
    
// not currently published
static GUID CLSID_RestrictedErrorObject = { 0x00000352, 0x0, 0x0, { 0xc0, 0x0,  0x0,  0x0,  0x0,  0x0,  0x0,  0x46 } };  

using namespace DirectUI;

void ErrorInfo::Clear()
{
    m_spErrorInfo = nullptr;
    m_ErrorInfoInitialized = TRUE;
    
    m_spRestrictedErrorInfo = nullptr;
    m_RestrictedErrorInfoInitialized = TRUE;

    m_description.Free();
    m_restrictedDescription.Free();
    m_ErrorCode = 0;
    m_ErrorDetailsInitialized = TRUE;
}

void ErrorInfo::SetErrorInfo(_In_opt_ IErrorInfo* pErrorInfo)
{
    Clear();

    if (pErrorInfo)
    {
        m_spErrorInfo = pErrorInfo;
        m_ErrorInfoInitialized = TRUE;
        m_RestrictedErrorInfoInitialized = FALSE;
        m_ErrorDetailsInitialized = FALSE;
    }
}

void ErrorInfo::SetRestrictedErrorInfo(_In_opt_ IRestrictedErrorInfo* pRestrictedErrorInfo)
{
    Clear();

    if (pRestrictedErrorInfo)
    {
        m_spRestrictedErrorInfo = pRestrictedErrorInfo;
        m_RestrictedErrorInfoInitialized = TRUE;
        m_ErrorInfoInitialized = FALSE;
        m_ErrorDetailsInitialized = FALSE;
    }
}

_Check_return_ HRESULT ErrorInfo::GetFromThread()
{
    ctl::ComPtr<IErrorInfo> spErrorInfo;
    
    HRESULT hr = ::GetErrorInfo(0, &spErrorInfo);

    if (SUCCEEDED(hr))
    {
        // put the error info back on the thread
        if (spErrorInfo)
        {
            IGNOREHR(::SetErrorInfo(0, spErrorInfo.Get()));
        }

        // set the error info into this wrapper
        this->SetErrorInfo(spErrorInfo.Get());
    }

    // GetErrorInfo returns S_FALSE if there was no error info on the thread.
    // Convert to S_OK because propagating S_FALSE can cause subtle bugs.
    if (S_FALSE == hr)
    {
        hr = S_OK;
    }
    
    return hr;
}

_Check_return_ HRESULT ErrorInfo::SetOnThread()
{
    HRESULT hr = S_OK;

    if (HasRestrictedErrorInfo())
    {
        hr = ::SetRestrictedErrorInfo(GetRestrictedErrorInfoNoRef());
    }
    else
    {
        hr = ::SetErrorInfo(0, GetErrorInfoNoRef());
    }

    return hr;
}

BOOL ErrorInfo::HasErrorInfo()
{
    EnsureErrorInfoInitialized();
    
    return m_spErrorInfo.Get() != NULL;
}

BOOL ErrorInfo::HasRestrictedErrorInfo()
{
    EnsureRestrictedErrorInfoInitialized();

    return m_spRestrictedErrorInfo.Get() != NULL;
}

void ErrorInfo::EnsureErrorInfoInitialized()
{
    if (!m_ErrorInfoInitialized)
    {
        ASSERT(m_spRestrictedErrorInfo.Get() != NULL);
        IGNOREHR(m_spRestrictedErrorInfo.As(&m_spErrorInfo));
        m_ErrorInfoInitialized = TRUE;
    }
}

void ErrorInfo::EnsureRestrictedErrorInfoInitialized()
{
    if (!m_RestrictedErrorInfoInitialized)
    {
        ASSERT(m_spErrorInfo.Get() != NULL);
        IGNOREHR(m_spErrorInfo.As(&m_spRestrictedErrorInfo));
        m_RestrictedErrorInfoInitialized = TRUE;
    }
}

_Check_return_ HRESULT ErrorInfo::EnsureErrorDetailsInitialized()
{
    HRESULT hr = S_OK;

    if (!m_ErrorDetailsInitialized)
    {
        EnsureRestrictedErrorInfoInitialized();
        if (m_spRestrictedErrorInfo)
        {
            AutoBstr capabilitySid; // (unused)
            
            hr = m_spRestrictedErrorInfo->GetErrorDetails(
                m_description.FreeAndGetAddressOf(), 
                &m_ErrorCode, 
                m_restrictedDescription.FreeAndGetAddressOf(), 
                capabilitySid.FreeAndGetAddressOf());
        }
        else 
        {
            EnsureErrorInfoInitialized();
            if (m_spErrorInfo)
            {
                hr = m_spErrorInfo->GetDescription(m_description.FreeAndGetAddressOf());
            }
        }

        m_ErrorDetailsInitialized = TRUE;   
    }

    return hr;
}

IErrorInfo* ErrorInfo::GetErrorInfoNoRef()
{
    EnsureErrorInfoInitialized();

    return m_spErrorInfo.Get();
}

IRestrictedErrorInfo* ErrorInfo::GetRestrictedErrorInfoNoRef()
{
    EnsureRestrictedErrorInfoInitialized();

    return m_spRestrictedErrorInfo.Get();
}

BOOL ErrorInfo::HasDescription()
{
    return SUCCEEDED(EnsureErrorDetailsInitialized()) && !m_description.IsEmpty();
}

_Check_return_ HRESULT ErrorInfo::GetDescription(_Out_ xstring_ptr* pstrDescription)
{
    HRESULT hr;

    pstrDescription->Reset();

    hr = EnsureErrorDetailsInitialized();
    if (SUCCEEDED(hr))
    {
        hr = m_description.ToXStringPtr(pstrDescription);
    }
    return hr;
}

BOOL ErrorInfo::HasRestrictedDescription()
{
    return SUCCEEDED(EnsureErrorDetailsInitialized()) && !m_restrictedDescription.IsEmpty();
}

_Check_return_ HRESULT ErrorInfo::GetRestrictedDescription(_Out_ xstring_ptr* pstrRestrictedDescription)
{
    HRESULT hr;

    pstrRestrictedDescription->Reset();

    hr = EnsureErrorDetailsInitialized();
    if (SUCCEEDED(hr))
    {
        hr = m_restrictedDescription.ToXStringPtr(pstrRestrictedDescription);
    }
    return hr;
}

BOOL ErrorInfo::HasMessage()
{
    return HasRestrictedDescription() || HasDescription();
}

_Check_return_ HRESULT ErrorInfo::GetMessage(_Out_ xstring_ptr* pstrMessage)
{
    HRESULT hr = S_OK;

    pstrMessage->Reset();

    if (HasRestrictedDescription())
    {
        hr = GetRestrictedDescription(pstrMessage);
    }
    else if (HasDescription())
    {
        hr = GetDescription(pstrMessage);
    }
    
    return hr;
}

BOOL ErrorInfo::HasErrorCode()
{
    // This might look a little weird. We want to return TRUE only when our error code
    // is really a failure code. If for some reason m_ErrorCode is S_FALSE or something,
    // we want to return FALSE.
    
    return SUCCEEDED(EnsureErrorDetailsInitialized()) && (FAILED(m_ErrorCode));
}

// Note: intentionally no _Check_return_. This function is returning an error code
// as a piece of data, not as the success/failure of calling this function.
HRESULT ErrorInfo::GetErrorCode()
{
    if (SUCCEEDED(EnsureErrorDetailsInitialized()))
    {
        return m_ErrorCode;
    }
    return S_OK;
}

// This special LanguageException is used by CreateRestrictedErrorInfo. The StackBackTrace
// this provides is specifically empty, which RoFailFastWithErrorContextInternal2 will ignore,
// allowing any other stowed exception to be the blame (hopefully at the source of the error).
class NoStackLanguageException : public wrl::RuntimeClass<wrl::RuntimeClassFlags<wrl::ClassicCom>, ILanguageExceptionStackBackTrace>
{
public:
    NoStackLanguageException() {}

    IFACEMETHOD(GetStackBackTrace)(ULONG maxFramesToCapture, UINT_PTR stackBackTrace[], ULONG* framesCaptured) override
    {
        *framesCaptured = 0;
        return S_OK;
    }
};

_Check_return_ HRESULT ErrorInfo::CreateRestrictedErrorInfo(HRESULT hrErrorCode, _In_ const xstring_ptr& strRestrictedDescription)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<IRestrictedErrorInfo> spRestrictedErrorInfo;

    XUINT32 size;
    const WCHAR* description = strRestrictedDescription.GetBufferAndCount(&size);

    wrl::ComPtr<NoStackLanguageException> languageException = wrl::Make<NoStackLanguageException>();
    ::RoOriginateLanguageException(hrErrorCode, wrl_wrappers::HStringReference(description).Get(), languageException.Get());
    IFC_NOTRACE(GetRestrictedErrorInfo(&spRestrictedErrorInfo));

    this->SetRestrictedErrorInfo(spRestrictedErrorInfo.Get());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ErrorInfo::CreateErrorInfo(_In_ const xstring_ptr& strDescription)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ICreateErrorInfo> spCreateErrorInfo;
    ctl::ComPtr<IErrorInfo> spErrorInfo;

    IFC_NOTRACE(::CreateErrorInfo(&spCreateErrorInfo));

    if (!strDescription.IsNull())
    {
        IFC_NOTRACE(spCreateErrorInfo->SetDescription(const_cast<WCHAR*>(strDescription.GetBuffer())));
    }
    
    IFC_NOTRACE(spCreateErrorInfo.As(&spErrorInfo));
    this->SetErrorInfo(spErrorInfo.Get());

Cleanup:
    RRETURN(hr);
}



