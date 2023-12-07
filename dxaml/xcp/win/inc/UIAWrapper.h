// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#pragma warning(push)
#pragma warning(disable:4996) // use of apis marked as [[deprecated("PrivateAPI")]]

class CDOCollection; // Forward declaration for ConvertToVariant(CDOCollection, ...)
class CAutomationPeerAnnotationCollection;

class __declspec(uuid("1d1e1a89-f7af-4587-af21-13f33476f481")) CUIAWrapper final : public IRawElementProviderSimple2,
                               public IRawElementProviderFragment,
                               public IRawElementProviderAdviseEvents,
                               public CInterlockedReferenceCount,
                               public IUIAWrapper,
                               public IRawElementProviderVisualRelative
{
public:
    CUIAWrapper(
        _In_ const UIAHostEnvironmentInfo& uiaHostEnvironmentInfo,
        _In_ IXcpHostSite *pHost,
        _In_ CUIAWindow *pWindow,
        _In_ CAutomationPeer *pAP,
        _In_ UIAIdentifiers *UIAIds);
    ~CUIAWrapper() override;

    static HRESULT Create(
        _In_ const UIAHostEnvironmentInfo& uiaHostEnvironmentInfo,
        _In_ IXcpHostSite *pHost,
        _In_ CUIAWindow *pWindow,
        _In_ CAutomationPeer *pAP,
        _In_ UIAIdentifiers *UIAIds,
        _Outptr_ CUIAWrapper** ppUIAWrapper);
    HRESULT Deinit();

    // IUnknown methods
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;
    HRESULT STDMETHODCALLTYPE QueryInterface(_In_ REFIID riid, _Out_ void**) override;

    // IRawElementProviderSimple methods
    HRESULT STDMETHODCALLTYPE get_ProviderOptions(_Out_ ProviderOptions * pRetVal) override;
    HRESULT STDMETHODCALLTYPE GetPatternProvider(_In_ PATTERNID patternId, _Out_ IUnknown ** pRetVal) override;
    HRESULT STDMETHODCALLTYPE GetPatternProviderImpl(_In_ PATTERNID patternId, _Out_ IUnknown ** pRetVal);
    HRESULT STDMETHODCALLTYPE GetPropertyValue(_In_ PROPERTYID propertyId, _Out_ VARIANT * pRetVal) override;
    HRESULT STDMETHODCALLTYPE GetPropertyValueImpl(_In_ PROPERTYID propertyId, _Out_ VARIANT * pRetVal);
    HRESULT STDMETHODCALLTYPE get_HostRawElementProvider(_Out_ IRawElementProviderSimple ** pRetVal) override;

    // IRawElementProviderSimple2 methods
    HRESULT STDMETHODCALLTYPE ShowContextMenu() override;
    HRESULT STDMETHODCALLTYPE ShowContextMenuImpl();

    // IRawElementProviderFragment methods
    HRESULT STDMETHODCALLTYPE get_BoundingRectangle(_Out_ UiaRect * pRetVal) override;
    HRESULT STDMETHODCALLTYPE get_BoundingRectangleImpl(_Out_ UiaRect * pRetVal);
    HRESULT STDMETHODCALLTYPE get_FragmentRoot(_Out_ IRawElementProviderFragmentRoot** pRetVal) override;
    HRESULT STDMETHODCALLTYPE GetEmbeddedFragmentRoots(_Out_ SAFEARRAY **pRetVal) override;
    HRESULT STDMETHODCALLTYPE GetRuntimeId(_Out_ SAFEARRAY ** pRetVal) override;
    HRESULT STDMETHODCALLTYPE GetRuntimeIdImpl(_Out_ SAFEARRAY ** pRetVal);
    HRESULT STDMETHODCALLTYPE Navigate(_In_ NavigateDirection direction, _Out_ IRawElementProviderFragment ** pRetVal) override;
    HRESULT STDMETHODCALLTYPE NavigateImpl(_In_ NavigateDirection direction, _Outptr_ IRawElementProviderFragment ** pRetVal);
    HRESULT STDMETHODCALLTYPE SetFocus() override;
    HRESULT STDMETHODCALLTYPE SetFocusImpl();

    // IRawElementProviderVisualRelative methods
    _Check_return_ STDMETHOD(GetVisualRelativeBoundingRectangle)(_Out_ UiaVisualRelativeRectangle* visualRelativeRect) final;
    _Check_return_ STDMETHOD(GetVisualRelativeCenterPoint)(_Out_ UiaVisualRelativePoint* visualRelativePoint) final;
    _Check_return_ STDMETHOD(GetVisualRelativeClickablePoint)(_Out_ UiaVisualRelativePoint* visualRelativePoint) final;

    // IRawElementProviderAdviseEvents methods
    HRESULT STDMETHODCALLTYPE AdviseEventAdded(_In_ EVENTID eventId, _Out_ SAFEARRAY *propertyIDs) override;
    HRESULT STDMETHODCALLTYPE AdviseEventRemoved(_In_ EVENTID eventId, _Out_ SAFEARRAY *propertyIDs) override;

    HRESULT ConvertToVariant(_In_ XINT32 iValue, _Out_ VARIANT* pResult);
    HRESULT ConvertToVariant(_In_ XUINT32 uValue, _Out_ VARIANT* pResult);
    HRESULT ConvertToVariant(_In_ const xstring_ptr_view& strString, _Out_ VARIANT* pResult);
    HRESULT ConvertToVariant(_In_ CAutomationPeer *pAP, _Out_ VARIANT* pResult);
    HRESULT ConvertToVariant(_In_ bool fValue, _Out_ VARIANT* pResult);
    HRESULT ConvertToVariant(_In_ UIAXcp::APAutomationControlType eValue, _Out_ VARIANT* pResult);
    HRESULT ConvertToVariant(_In_ XPOINTF *pPoint, _Out_ VARIANT* pResult);
    HRESULT ConvertToVariant(_In_ CDOCollection* pDos, _Out_ VARIANT* pResult);
    HRESULT ConvertToVariantArr(_In_ CAutomationPeer *pAP, _Out_ VARIANT* pResult);
    HRESULT ConvertToVariant(_In_opt_ CAutomationPeerAnnotationCollection* pAnnotations, _In_ PROPERTYID propertyId, _Out_ VARIANT* pResult);
    HRESULT ConvertToVariant(_In_ UIAXcp::AutomationLandmarkType eValue, _Out_ VARIANT* pResult);
    HRESULT ConvertToVariant(_In_ UIAXcp::AutomationHeadingLevel eValue, _Out_ VARIANT* pResult);

    void Invalidate() override;

    CAutomationPeer* GetAP()
    {
        return m_pAP;
    }

    CUIAWindow* GetUIAWindow()
    {
        return m_pWindow;
    }

private:
    UIAHostEnvironmentInfo m_uiaHostEnvironmentInfo;

    IXcpHostSite *m_pHost;
    CUIAWindow *m_pWindow;
    CAutomationPeer *m_pAP;
    UIAIdentifiers *pUIAIds;
    IUIAWindowValidator *m_pUIAWindowValidator;
    wrl::ComPtr<IRawElementProviderSimple> m_providerSimpleOverrider;
};

#pragma warning(pop)