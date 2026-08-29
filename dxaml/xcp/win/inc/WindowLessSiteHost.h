// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Use of this source code is subject to the terms of the Microsoft
// premium shared source license agreement under which you licensed
// this source code. If you did not accept the terms of the license
// agreement, you are not authorized to use this source code.
// For the terms of the license, please see the license agreement
// signed by you and Microsoft.
// THE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
//  Abstract:
//
//      Define CWindowLessSiteHost, which hosts an WindowLess Site.
//      This class implements IRawElementProviderWindowlessSite to enable communication between WindowLess
//      Controls' IRawElementProviderSimple and the host control. Ideally we should have been implementing
//      this interface on TextBoxBase which is actually the host control for windowless RichEdit. Here we
//      had to use the Pal abstraction layer to allow separation between uicore and win layers. This design
//      by using this class as the adapter for TextBoxBase to provide IRawElementProviderWindowlessSite
//      implementation serves the purpose. RichEdit when needed any support can communicate with host control
//      in Jupiter for Siblings, parent, runtimeIds by using IRawElementProviderWindowlessSite.
//
//------------------------------------------------------------------------

class CWindowlessSiteHost final :
    public IPALWindowlessHost,
    public IRawElementProviderWindowlessSite,
    public IRichEditUiaInformation,
    public IRicheditUiaOverrides,
    public CInterlockedReferenceCount

{
public:
    CWindowlessSiteHost();

    _Check_return_ HRESULT Initialize(_In_ IXcpHostSite* pHostSite, _In_ CDependencyObject* pParentEditBox, _In_ XUINT32 uRuntimeId) override;
    void Destroy(_In_ void* pRichEditWindowlessAcc) override;

    // IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;
    HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID riid, _Out_ void**) override;

    // IRawElementProviderWindowlessSite methods
    HRESULT STDMETHODCALLTYPE GetAdjacentFragment(
        _In_ NavigateDirection direction,
        _Out_ IRawElementProviderFragment **ppParent) override;

    HRESULT STDMETHODCALLTYPE GetRuntimeIdPrefix(
        _Out_ SAFEARRAY **pRetVal) override;

    // IRichEditUiaInformation methods
    HRESULT STDMETHODCALLTYPE GetBoundaryRectangle(_Out_ UiaRect *pUiaRect) override;

    HRESULT STDMETHODCALLTYPE IsVisible() override;

    // IRicheditUiaOverrides method
    HRESULT STDMETHODCALLTYPE GetPropertyOverrideValue(
        _In_ PROPERTYID propertyId,
        _Out_ VARIANT *pRetValue) override;

    _Check_return_ HRESULT GetChildRawElementProviderSimple(
        _In_ void* pRichEditWindowlessAcc,
        _Outptr_ void** pProvider) override;

    _Check_return_ HRESULT GetUnwrappedPattern(
        _In_ void* pRichEditWindowlessAcc,
        _In_ XINT32 patternID,
        _In_ bool isRichEdit,
        _Outptr_result_maybenull_ void** ppPattern) override;

    _Check_return_ HRESULT GetParentAutomationPeer(_Outptr_ CAutomationPeer** ppParentAutomationPeer);
private:
    _Check_return_ HRESULT EnsureProvider(_In_ void* pRichEditWindowlessAcc);
    _Check_return_ HRESULT ConvertToVariantAsScreenRect(_In_ UiaRect rect, _Out_ VARIANT* pResult);
    ~CWindowlessSiteHost() override;

private:
    IRawElementProviderSimple* m_pProvider;
    CDependencyObject* m_pParent;
    IXcpHostSite* m_pHostSite;
    XUINT32 m_uRuntimeId;

};
