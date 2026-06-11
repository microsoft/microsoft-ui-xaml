// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

// This needs to be exported in defn file of msftedit.dll so as we can use GetProc ideally. It will be removed from here once fixed.
static const IID IID_IRichEditUiaInformationInternal =
{ 0x23969a9d, 0x8546, 0x4032, { 0xa1, 0xbb, 0x73, 0x75, 0xc, 0xbf, 0x33, 0x33 } };

// This needs to be exported in defn file of msftedit.dll so as we can use GetProc ideally. It will be removed from here once fixed.
static const IID IID_IRicheditUiaOverridesInternal =
{ 0xf2fb5cc0, 0xb5a9, 0x437f, { 0x9b, 0xa2, 0x47, 0x63, 0x20, 0x82, 0x26, 0x9f } };

//------------------------------------------------------------------------
//
//  Method: CWindowlessSiteHost ctor
//
//------------------------------------------------------------------------

CWindowlessSiteHost::CWindowlessSiteHost()
{
    m_pProvider = NULL;
    XCP_WEAK(&m_pParent);
    m_pParent = NULL;
    XCP_WEAK(&m_pHostSite);
    m_pHostSite = NULL;
    m_uRuntimeId = -1;
}
CWindowlessSiteHost::~CWindowlessSiteHost()
{
    ReleaseInterface(m_pProvider);
}

_Check_return_ HRESULT CWindowlessSiteHost::Initialize(_In_ IXcpHostSite* pHostSite, _In_ CDependencyObject* pParentEditBox, _In_ XUINT32 uRuntimeId)
{
    HRESULT hr = S_OK;

    m_pHostSite = pHostSite;
    m_pParent = pParentEditBox;
    m_uRuntimeId = uRuntimeId;

    RRETURN(hr);
}

void CWindowlessSiteHost::Destroy(_In_ void* pRichEditWindowlessAcc)
{
    m_pParent = NULL;
    m_pHostSite = NULL;
    IRicheditWindowlessAccessibility* pRichEditWindowlessProvider = NULL;
    pRichEditWindowlessProvider = reinterpret_cast<IRicheditWindowlessAccessibility*>(pRichEditWindowlessAcc);
    if(pRichEditWindowlessProvider)
    {
        // This is to clear IRawElementProviderWindowlessSite's ref from provider by RichEdit.
        IGNOREHR((reinterpret_cast<IRicheditWindowlessAccessibility*>(pRichEditWindowlessProvider))->CreateProvider(NULL, &m_pProvider));
    }
    Release();
}

_Check_return_ HRESULT CWindowlessSiteHost::GetParentAutomationPeer(_Outptr_ CAutomationPeer** ppParentAutomationPeer)
{
    HRESULT hr = S_OK;

    IFCEXPECT(m_pParent);
    IFCPTR(ppParentAutomationPeer);
    *ppParentAutomationPeer = m_pParent->OnCreateAutomationPeer();

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Implementation of IRawElementProviderWindowlessSite::GetAdjacentFragment
//      It's responsible for providing Parent UIA object or Siblings to the Windowless
//      UIA object (IRawElementProviderSimple). As we have decided RichEdit's
//      IRawElementProviderSimple.To be the first child of the parent, Previous Sibling
//      always returns NULL while calling this API.
//
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CWindowlessSiteHost::GetAdjacentFragment(
        _In_ NavigateDirection direction,
        _Out_ IRawElementProviderFragment **ppRetVal)
{
    HRESULT hr = S_OK;
    CUIAWindow* pWindowNoRef = nullptr;
    xref_ptr<IRawElementProviderFragment> spFrag;
    xref_ptr<CAutomationPeer> spAP;
    CAutomationPeer* pAPParentNoRef = nullptr;

    IFCPTR(ppRetVal);
    *ppRetVal = nullptr;
    IFCEXPECT_NOTRACE(m_pHostSite);
    IFC(m_pHostSite->GetUIAWindow(m_pParent, m_pHostSite->GetXcpControlWindow(), false /*onlyGet*/, &pWindowNoRef));
    IFCEXPECT(pWindowNoRef);
    IFC(GetParentAutomationPeer(&pAPParentNoRef));
    switch(direction)
    {
        case NavigateDirection_Parent:
        {
            spAP = pAPParentNoRef;
            break;
        }
        case NavigateDirection_NextSibling:
        {
            // Windowless control is the first child always, so when it asks for next sibling to itself,
            // it will be the first child of the host control from Visual tree (which has corresponding AP).
            if (pAPParentNoRef)
            {
                xref_ptr<IUnknown> spUnkNativeNode;
                IFC(pAPParentNoRef->Navigate(UIAXcp::AutomationNavigationDirection_FirstChild, spAP.ReleaseAndGetAddressOf(), spUnkNativeNode.ReleaseAndGetAddressOf()));
                ASSERT(!spUnkNativeNode);
            }
            break;
        }
        case NavigateDirection_PreviousSibling:
            // Windowless control is the first child always, hence previous sibling will always be null.
            break;
        default:
            IFC(E_INVALIDARG);
            break;
    }

    if(spAP)
    {
        xref_ptr<CUIAWrapper> spWrapper;
        IFC(pWindowNoRef->CreateProviderForAP(spAP, spWrapper.ReleaseAndGetAddressOf()));
        IFC(spWrapper->QueryInterface(__uuidof(IRawElementProviderFragment), reinterpret_cast<void**>(spFrag.ReleaseAndGetAddressOf())));
    }

    *ppRetVal = spFrag.detach();

Cleanup:
    return hr;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Implementation of IRawElementProviderWindowlessSite::GetRuntimeIdPrefix
//      It's responsible for providing runtime id prefix to the Windowless
//      UIA object (IRAwElementProviderSimple). so as allowing them to generate
//      unique runtime Ids for their own objects.
//
//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE CWindowlessSiteHost::GetRuntimeIdPrefix(
        _Out_ SAFEARRAY **ppRetVal)
{
    IFCEXPECT_NOTRACE_RETURN(m_pHostSite);
    IFCPTR_RETURN(ppRetVal);
    *ppRetVal = NULL;

    // using 0 as the unique identifier since we're the root element
    int rId[] = { UiaAppendRuntimeId, m_uRuntimeId };
    SAFEARRAY *psa = SafeArrayCreateVector(VT_I4, 0, 2);
    IFCOOMFAILFAST(psa);
    for (LONG i = 0; i < 2; i++)
    {
        IFC_RETURN(SafeArrayPutElement(psa, &i, (void*)&(rId[i])));
    }
    *ppRetVal = psa;

    return S_OK;
}

// IRichEditUiaInformation methods
HRESULT STDMETHODCALLTYPE CWindowlessSiteHost::GetBoundaryRectangle(_Out_ UiaRect *pUiaRect)
{
    HRESULT hr = S_OK;
    CUIAWindow *pWindow = NULL;
    CUIAWrapper* pWrapper = NULL;
    CAutomationPeer *pAPParent = NULL;

    IFCPTR(pUiaRect);
    IFCEXPECT(m_pHostSite);
    IFC(m_pHostSite->GetUIAWindow(m_pParent, m_pHostSite->GetXcpControlWindow(), false /*onlyGet*/, &pWindow));
    IFCEXPECT(pWindow);

    IFC(GetParentAutomationPeer(&pAPParent));
    if(pAPParent)
    {
        IFC(pWindow->CreateProviderForAP(pAPParent, &pWrapper));
        IFC(pWrapper->get_BoundingRectangleImpl(pUiaRect));
    }

Cleanup:
    ReleaseInterface(pWrapper);
    RRETURN(hr);
}

HRESULT STDMETHODCALLTYPE CWindowlessSiteHost::IsVisible()
{
    HRESULT hr = S_OK;

    RRETURN(hr);
}

// IRicheditUiaOverrides method
HRESULT STDMETHODCALLTYPE CWindowlessSiteHost::GetPropertyOverrideValue(
                                            _In_ PROPERTYID propertyId,
                                            _Out_ VARIANT *pRetVal)
{
    HRESULT hr = S_OK;
    CUIAWindow *pWindow = NULL;
    CUIAWrapper* pWrapper = NULL;
    UIAIdentifiers *pUIAIds = NULL;
    CAutomationPeer *pAPParent = NULL;

    if (pRetVal == NULL) return E_INVALIDARG;
    IFCEXPECT(m_pHostSite);
    IFC(m_pHostSite->GetUIAWindow(m_pParent, m_pHostSite->GetXcpControlWindow(), false /*onlyGet*/, &pWindow));
    IFCEXPECT(pWindow);
    IFC(GetParentAutomationPeer(&pAPParent));
    if(!pAPParent)
    {
        goto Cleanup;
    }
    IFC(pWindow->CreateProviderForAP(pAPParent, &pWrapper));
    pUIAIds = pWindow->GetUIAds();

    IFCEXPECT(pUIAIds);

    if (propertyId == pUIAIds->IsOffscreen_Property)
    {
        IFC(pWrapper->GetPropertyValueImpl(propertyId, pRetVal));
    }
    else if (propertyId == pUIAIds->IsControlElement_Property
        || propertyId == pUIAIds->IsContentElement_Property)
    {
        pRetVal->vt = VT_BOOL;
        pRetVal->boolVal = VARIANT_FALSE;
    }
    else if (propertyId == pUIAIds->IsOffscreen_Property
        || propertyId == pUIAIds->IsPassword_Property
        || propertyId == pUIAIds->IsEnabled_Property
        || propertyId == pUIAIds->Orientation_Property
        || propertyId == pUIAIds->ClickablePoint_Property
        || propertyId == pUIAIds->ControlType_Property
        || propertyId == pUIAIds->LocalizedControlType_Property
        || propertyId == pUIAIds->Name_Property)
    {
        IFC(pWrapper->GetPropertyValueImpl(propertyId, pRetVal));
    }
    else if (propertyId == pUIAIds->BoundingRectangle_Property)
    {
        // Ideally it should be fetched via IRichEditUiaInformation::GetBoundaryRectangle, but RichEdit is fetching Bounding Rectangle as well through
        // IRichEditUiaPropertyOverrides.
        UiaRect uiaRect;
        IFC(CWindowlessSiteHost::GetBoundaryRectangle(&uiaRect));
        IFC(ConvertToVariantAsScreenRect(uiaRect, pRetVal));
    }

Cleanup:
    ReleaseInterface(pWrapper);
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      RichEdit team introduced IRicheditWindowlessAccessibility using which we can provide
//      RichEdit with IRawElementProviderWindowlessSite and retrieve IRawElementProviderSimple
//      and return it to UIA client, so as UIA client can directly communicate with RichEdit
//      without going through UIA provider in Jupiter.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CWindowlessSiteHost::GetChildRawElementProviderSimple(
        _In_ void* pRichEditWindowlessAcc,
        _Outptr_ void** ppProvider)
{
    HRESULT hr = S_OK;
    IFCPTR(ppProvider);

    IFC(EnsureProvider(pRichEditWindowlessAcc));
    *ppProvider = (void*)m_pProvider;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT CWindowlessSiteHost::GetUnwrappedPattern(
        _In_ void* pRichEditWindowlessAcc,
        _In_ XINT32 patternID,
        _In_ bool isRichEdit,
        _Outptr_result_maybenull_ void** ppPattern)
{
    HRESULT hr = S_OK;
    IUnknown *pUnk = NULL;
    CUIAWindow *pWindow = NULL;
    UIAIdentifiers *pUIAIds = NULL;

    IFCPTR(ppPattern);

    IFC(EnsureProvider(pRichEditWindowlessAcc));

    if(m_pProvider)
    {
        if (isRichEdit)
        {
            IFCEXPECT(m_pHostSite);
            IFC(m_pHostSite->GetUIAWindow(m_pParent, m_pHostSite->GetXcpControlWindow(), false /*onlyGet*/, &pWindow));
            IFCEXPECT(pWindow);
            pUIAIds = pWindow->GetUIAds();
            IFCEXPECT(pUIAIds);
        }

        // Restrict RichEditBox from supporting Value pattern. RichEditBox provides complex content where value pattern would result in loss of data
        // also we dont raise value propertychange event in case of RichEditBox.
        if(!isRichEdit || patternID != pUIAIds->Value_Pattern)
        {
            IFC(m_pProvider->GetPatternProvider(patternID, &pUnk));
            *ppPattern = (void*) pUnk;
        }
    }
    else
    {
        *ppPattern = NULL;
    }

    pUnk = NULL;

Cleanup:
    ReleaseInterface(pUnk);
    RRETURN(hr);
}

_Check_return_ HRESULT CWindowlessSiteHost::EnsureProvider(_In_ void* pRichEditWindowlessAcc)
{
    HRESULT hr = S_OK;
    IRicheditWindowlessAccessibility* pRichEditWindowlessProvider = NULL;
    if(!m_pProvider)
    {
        pRichEditWindowlessProvider = reinterpret_cast<IRicheditWindowlessAccessibility*>(pRichEditWindowlessAcc);
        IFCEXPECT(pRichEditWindowlessProvider);
        IFC(pRichEditWindowlessProvider->CreateProvider(this, &m_pProvider));
    }

Cleanup:
    RRETURN(hr);
}

// IUnknown implementation.

ULONG STDMETHODCALLTYPE CWindowlessSiteHost::AddRef()
{
    return CInterlockedReferenceCount::AddRef();
}

ULONG STDMETHODCALLTYPE CWindowlessSiteHost::Release()
{
    return CInterlockedReferenceCount::Release();
}

HRESULT STDMETHODCALLTYPE CWindowlessSiteHost::QueryInterface(_In_ REFIID riid, _Out_ void** ppInterface)
{
    IUnknown *pResult = NULL;
    if (riid == __uuidof(IUnknown))
    {
        pResult = static_cast<IUnknown*>(static_cast<IRawElementProviderWindowlessSite*>(this));
    }
    else if (riid == __uuidof(IRawElementProviderWindowlessSite))
    {
        pResult = static_cast<IRawElementProviderWindowlessSite*>(this);
    }
    else if (riid == IID_IRichEditUiaInformationInternal)
    {
        pResult = static_cast<IRichEditUiaInformation*>(this);
    }
    else if (riid == IID_IRicheditUiaOverridesInternal)
    {
        pResult = static_cast<IRicheditUiaOverrides*>(this);
    }
    else
    {
        pResult = NULL;
        return E_NOINTERFACE;
    }

    pResult->AddRef();
    *ppInterface = (void*)pResult;

    return S_OK;
}

// This converts the UiaRect to Variant, but it also changes format from left top width height to left top right bottom as
// that's the format RichEdit is expecting this rect in. It's not ideal as Uia always expects it in earlier format. But
// we do have to adjust it for RichEdit.
_Check_return_ HRESULT CWindowlessSiteHost::ConvertToVariantAsScreenRect(_In_ UiaRect rect, _Out_ VARIANT* pResult)
{
    HRESULT hr = S_OK;

    LONG lLbound = 0;
    double ptValue = 0;
    SAFEARRAY *pSafeArray = NULL;

    ASSERT(pResult);

    pResult->vt = VT_R8| VT_ARRAY;

    pSafeArray = SafeArrayCreateVector(VT_R8, 0, 4);
    IFCOOMFAILFAST(pSafeArray);

    // Add left value
    ptValue = static_cast<XDOUBLE>(rect.left);
    IFC(SafeArrayPutElement(pSafeArray, &lLbound, (void*)&ptValue));

    // Add top value
    lLbound = 1;
    ptValue = static_cast<XDOUBLE>(rect.top);
    IFC(SafeArrayPutElement(pSafeArray, &lLbound, (void*)&ptValue));

    // Add width value
    lLbound = 2;
    ptValue = static_cast<XDOUBLE>(rect.left) + static_cast<XDOUBLE>(rect.width);
    IFC(SafeArrayPutElement(pSafeArray, &lLbound, (void*)&ptValue));

    // Add height value
    lLbound = 3;
    ptValue = static_cast<XDOUBLE>(rect.top) + static_cast<XDOUBLE>(rect.height);
    IFC(SafeArrayPutElement(pSafeArray, &lLbound, (void*)&ptValue));

    pResult->parray = pSafeArray;
    pSafeArray = NULL;

Cleanup:
    if(pSafeArray)
    {
        IGNOREHR(SafeArrayDestroy(pSafeArray));
    }
    RRETURN(hr);
}
