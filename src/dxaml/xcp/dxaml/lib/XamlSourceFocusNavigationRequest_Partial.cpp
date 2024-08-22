// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlSourceFocusNavigationRequest.g.h"

using namespace DirectUI;

XamlSourceFocusNavigationRequest::XamlSourceFocusNavigationRequest()
{
    if (UuidCreate(&m_correlationId) != RPC_S_OK)
    {
        IFCFAILFAST(E_FAIL);
    }
}

_Check_return_ HRESULT XamlSourceFocusNavigationRequest::Initialize(
    xaml_hosting::XamlSourceFocusNavigationReason reason,
    wf::Rect origin,
    GUID correlationId)
{
    m_reason = reason;
    m_origin = origin;
    m_correlationId = correlationId;
    return S_OK;
}

_Check_return_ HRESULT XamlSourceFocusNavigationRequest::Initialize(
    xaml_hosting::XamlSourceFocusNavigationReason reason,
    wf::Rect origin)
{
    m_reason = reason;
    m_origin = origin;
    return S_OK;
}

_Check_return_ HRESULT XamlSourceFocusNavigationRequest::Initialize(
    xaml_hosting::XamlSourceFocusNavigationReason reason)
{
    m_reason = reason;
    return S_OK;
}

XamlSourceFocusNavigationRequest::~XamlSourceFocusNavigationRequest()
{
}

_Check_return_ HRESULT
XamlSourceFocusNavigationRequest::get_CorrelationIdImpl(_Out_ GUID* pResult)
{
    *pResult = m_correlationId;
    return S_OK;
}

_Check_return_ HRESULT
XamlSourceFocusNavigationRequest::get_HintRectImpl(_Out_ wf::Rect* pResult)
{
    *pResult = m_origin;
    return S_OK;
}

_Check_return_ HRESULT
XamlSourceFocusNavigationRequest::get_ReasonImpl(_Out_ xaml_hosting::XamlSourceFocusNavigationReason* pResult)
{
    *pResult = m_reason;
    return S_OK;
}

_Check_return_ HRESULT
XamlSourceFocusNavigationRequestFactory::CreateInstanceImpl(
    _In_ xaml_hosting::XamlSourceFocusNavigationReason reason,
    _Outptr_ xaml_hosting::IXamlSourceFocusNavigationRequest** ppResult)
{
    ctl::ComPtr<XamlSourceFocusNavigationRequest> request;
    IFC_RETURN(ctl::make<XamlSourceFocusNavigationRequest>(reason, &request));
    IFC_RETURN(request.CopyTo(ppResult));
    return S_OK;
}

_Check_return_ HRESULT
XamlSourceFocusNavigationRequestFactory::CreateInstanceWithHintRectImpl(
    _In_ xaml_hosting::XamlSourceFocusNavigationReason reason,
    _In_ wf::Rect origin,
    _Outptr_ xaml_hosting::IXamlSourceFocusNavigationRequest** ppResult)
{
    ctl::ComPtr<XamlSourceFocusNavigationRequest> request;
    IFC_RETURN(ctl::make<XamlSourceFocusNavigationRequest>(reason, origin, &request));
    IFC_RETURN(request.CopyTo(ppResult));
    return S_OK;
}

_Check_return_ HRESULT
XamlSourceFocusNavigationRequestFactory::CreateInstanceWithHintRectAndCorrelationIdImpl(
    _In_ xaml_hosting::XamlSourceFocusNavigationReason reason,
    _In_ wf::Rect origin,
    _In_ GUID correlationId,
    _Outptr_ xaml_hosting::IXamlSourceFocusNavigationRequest** ppResult)
{
    ctl::ComPtr<XamlSourceFocusNavigationRequest> request;
    IFC_RETURN(ctl::make<XamlSourceFocusNavigationRequest>(reason, origin, correlationId, &request));
    IFC_RETURN(request.CopyTo(ppResult));
    return S_OK;
}

