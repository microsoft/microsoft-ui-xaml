// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Hyperlink.h"
#include "HyperlinkAutomationPeer.g.h"
#include "HyperLink.g.h"
#include "AutomationProperties.h"
#include "AccessKeyStringBuilder.h"
#include <XamlOneCoreTransforms.h>
#include "RootScale.h"
#include "TextAdapter.h"
#include "UIElement.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

IFACEMETHODIMP HyperlinkAutomationPeer::GetPatternCore(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue)
{
    *ppReturnValue = nullptr;

    if (patternInterface == xaml_automation_peers::PatternInterface_Invoke)
    {
        *ppReturnValue = ctl::as_iinspectable(this);
        ctl::addref_interface(this);
    }
    else
    {
        IFC_RETURN(HyperlinkAutomationPeerGenerated::GetPatternCore(patternInterface, ppReturnValue));
    }

    return S_OK;
}

IFACEMETHODIMP HyperlinkAutomationPeer::GetClassNameCore(_Out_ HSTRING* pReturnValue)
{

    return (wrl_wrappers::HStringReference(STR_LEN_PAIR(L"Hyperlink")).CopyTo(pReturnValue));
}

IFACEMETHODIMP HyperlinkAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue)
{
    *pReturnValue = xaml_automation_peers::AutomationControlType_Hyperlink;

    return S_OK;
}

IFACEMETHODIMP HyperlinkAutomationPeer::IsContentElementCore(_Out_ BOOLEAN* returnValue)
{
    xaml_automation_peers::AccessibilityView accessibilityView = xaml_automation_peers::AccessibilityView_Content;
    ctl::ComPtr<xaml_docs::IHyperlink> spOwner;
    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    IFC_RETURN(AutomationProperties::GetAccessibilityViewStatic(spOwner.Cast<Hyperlink>(), &accessibilityView));

    *returnValue = accessibilityView == xaml_automation_peers::AccessibilityView_Content;

    return S_OK;
}

IFACEMETHODIMP HyperlinkAutomationPeer::IsControlElementCore(_Out_ BOOLEAN* returnValue)
{
    *returnValue = TRUE;

    return S_OK;
}

IFACEMETHODIMP HyperlinkAutomationPeer::GetNameCore(_Out_ HSTRING* pReturnValue)
{
    XUINT32 length = 0;
    ctl::ComPtr<xaml_docs::IHyperlink> spOwner;
    ctl::ComPtr<wf::IUriRuntimeClass> spUri;

    IFCPTR_RETURN(pReturnValue);
    *pReturnValue = nullptr;

    IFC_RETURN(get_Owner(&spOwner));

    // P1: Get AutomationProperties.Name
    IFC_RETURN(AutomationProperties::GetNameStatic(spOwner.Cast<Hyperlink>(), pReturnValue));

    length = (pReturnValue != nullptr) ? WindowsGetStringLen(*pReturnValue) : 0;

    // P2: Get Hyperlink Content
    if (length == 0)
    {
        CHyperlink *cCore = static_cast<CHyperlink*>(spOwner.Cast<Hyperlink>()->GetHandle());
        IFCPTR_RETURN(cCore);
        IFC_RETURN(cCore->GetContentText(pReturnValue));
        IFCPTR_RETURN(pReturnValue);
    }

    length = (pReturnValue != nullptr) ? WindowsGetStringLen(*pReturnValue) : 0;

    // P3: Get URI
    if(length == 0)
    {
        DELETE_STRING(*pReturnValue);
        *pReturnValue = nullptr;

        IFC_RETURN(spOwner.Cast<Hyperlink>()->get_NavigateUri(&spUri));
        if (spUri)
        {
            IFC_RETURN(spUri->get_DisplayUri(pReturnValue));
        }
    }

    return S_OK;
}

IFACEMETHODIMP HyperlinkAutomationPeer::IsEnabledCore(_Out_ BOOLEAN* pReturnValue)
{
    *pReturnValue = TRUE;
    return S_OK;
}

IFACEMETHODIMP HyperlinkAutomationPeer::GetAcceleratorKeyCore(_Out_ HSTRING* returnValue)
{
    ctl::ComPtr<xaml_docs::IHyperlink> spOwner;
    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    IFC_RETURN(AutomationProperties::GetAcceleratorKeyStatic(spOwner.Cast<Hyperlink>(), returnValue));
    return S_OK;
}

IFACEMETHODIMP HyperlinkAutomationPeer::GetAccessKeyCore(_Out_ HSTRING* returnValue)
{
    ctl::ComPtr<IInspectable> value;
    ctl::ComPtr<xaml_docs::IHyperlink> spOwner;
    xaml::IDependencyProperty* prop = nullptr;
    BOOLEAN isUnset = FALSE;

    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    //Check to see if the value is unset
    MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AutomationProperties_AccessKey, &prop);
    IFC_RETURN(spOwner.Cast<Hyperlink>()->ReadLocalValue(prop, &value));
    IFC_RETURN(DependencyPropertyFactory::IsUnsetValue(value.Get(), isUnset));

    //If value is unset, then fallback to the AccessKey property on TextElement
    if (isUnset)
    {
        ctl::ComPtr<DependencyObject> spOwnerAsDO(spOwner.Cast<Hyperlink>());
        IFC_RETURN(AccessKeyStringBuilder::GetAccessKeyMessageFromElement(spOwnerAsDO, returnValue));
    }
    else
    {
        //Find the value normally
        IFC_RETURN(AutomationProperties::GetAccessKeyStatic(spOwner.Cast<Hyperlink>(), returnValue));
    }

    return S_OK;
}

IFACEMETHODIMP HyperlinkAutomationPeer::GetAutomationIdCore(_Out_ HSTRING* returnValue)
{
    ctl::ComPtr<xaml_docs::IHyperlink> spOwner;
    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    IFC_RETURN(AutomationProperties::GetAutomationIdStatic(spOwner.Cast<Hyperlink>(), returnValue));

    XUINT32 length = WindowsGetStringLen(*returnValue);
    if (length == 0)
    {
        xstring_ptr strAutomationId;
        xruntime_string_ptr strAutomationIdRuntime;
        IFC_RETURN(static_cast<CAutomationPeer*>(GetHandle())->GetAutomationIdHelper(&strAutomationId));

        IFC_RETURN(strAutomationId.Promote(&strAutomationIdRuntime));
        *returnValue = strAutomationIdRuntime.DetachHSTRING();
    }

    return S_OK;
}

IFACEMETHODIMP HyperlinkAutomationPeer::GetHelpTextCore(_Out_ HSTRING* returnValue)
{
    ctl::ComPtr<xaml_docs::IHyperlink> spOwner;
    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    IFC_RETURN(AutomationProperties::GetHelpTextStatic(spOwner.Cast<Hyperlink>(), returnValue));

    return S_OK;
}

IFACEMETHODIMP HyperlinkAutomationPeer::GetItemStatusCore(_Out_ HSTRING* returnValue)
{
    ctl::ComPtr<xaml_docs::IHyperlink> spOwner;
    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    IFC_RETURN(AutomationProperties::GetItemStatusStatic(spOwner.Cast<Hyperlink>(), returnValue));

    return S_OK;
}

IFACEMETHODIMP HyperlinkAutomationPeer::GetItemTypeCore(_Out_ HSTRING* returnValue)
{
    ctl::ComPtr<xaml_docs::IHyperlink> spOwner;
    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    IFC_RETURN(AutomationProperties::GetItemTypeStatic(spOwner.Cast<Hyperlink>(), returnValue));

    return S_OK;
}

IFACEMETHODIMP HyperlinkAutomationPeer::GetLabeledByCore(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue)
{
    ctl::ComPtr<xaml_docs::IHyperlink> spOwner;
    ctl::ComPtr<IUIElement> spLabeledByUie;
    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    IFC_RETURN(AutomationProperties::GetLabeledByStatic(spOwner.Cast<Hyperlink>(), &spLabeledByUie));
    if (spLabeledByUie)
    {
        IFC_RETURN(spLabeledByUie.Cast<UIElement>()->GetOrCreateAutomationPeer(returnValue));
    }

    return S_OK;
}

IFACEMETHODIMP HyperlinkAutomationPeer::GetLiveSettingCore(_Out_ xaml_automation_peers::AutomationLiveSetting* returnValue)
{
    ctl::ComPtr<xaml_docs::IHyperlink> spOwner;
    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    IFC_RETURN(AutomationProperties::GetLiveSettingStatic(spOwner.Cast<Hyperlink>(), returnValue));

    return S_OK;
}


// Set the owner of AutomationPeer
_Check_return_ HRESULT HyperlinkAutomationPeer::put_Owner(
    _In_ xaml_docs::IHyperlink* pOwner)
{
    return ctl::AsWeak(pOwner, &m_wpOwner);
}

_Check_return_ HRESULT HyperlinkAutomationPeer::get_Owner(
    _Outptr_ xaml_docs::IHyperlink** pValue)
{
    auto spOwner = m_wpOwner.AsOrNull<xaml_docs::IHyperlink>();

    IFCPTR_RETURN(pValue);
    *pValue = nullptr;

    if (!spOwner)
    {
        IFC_RETURN(static_cast<HRESULT>(UIA_E_ELEMENTNOTAVAILABLE));
    }

    *pValue = spOwner.Detach();
    return S_OK;
}

// Support the IInvokeProvider interface.
_Check_return_ HRESULT HyperlinkAutomationPeer::InvokeImpl()
{
    ctl::ComPtr<xaml_docs::IHyperlink> spOwner;

    IFC_RETURN(get_Owner(&spOwner));
    IFC_RETURN(spOwner.Cast<Hyperlink>()->AutomationHyperlinkClick());

    return S_OK;
}

IFACEMETHODIMP HyperlinkAutomationPeer::GetBoundingRectangleCore(_Out_ wf::Rect* returnValue)
{
    CCoreServices *pCore = static_cast<CCoreServices*>(DXamlCore::GetCurrent()->GetHandle());
    ctl::ComPtr<xaml_docs::IHyperlink> spOwner;
    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    XRECTF rect;
    IFC_RETURN(pCore->GetTextElementBoundingRect(spOwner.Cast<Hyperlink>()->GetHandle(), &rect));

    if (XamlOneCoreTransforms::IsEnabled())
    {
        // In OneCoreTransforms mode, GetTextElementBoundingRect returns logical pixels so we must convert to RasterizedClient
        const float scale = RootScale::GetRasterizationScaleForElement(static_cast<CAutomationPeer*>(GetHandle())->GetRootNoRef());
        const auto logicalRect = rect;
        const auto physicalRect = logicalRect * scale;
        rect = physicalRect;
    }

    returnValue->X = rect.X;
    returnValue->Y = rect.Y;
    returnValue->Width = rect.Width;
    returnValue->Height = rect.Height;

    return S_OK;
}

IFACEMETHODIMP HyperlinkAutomationPeer::IsKeyboardFocusableCore(_Out_ BOOLEAN* returnValue)
{
    *returnValue = TRUE;
    return S_OK;
}

IFACEMETHODIMP HyperlinkAutomationPeer::GetClickablePointCore(_Out_ wf::Point* pReturnValue)
{
    ctl::ComPtr<xaml_docs::IHyperlink> spOwner;

    IFC_RETURN(get_Owner(&spOwner));
    CHyperlink *hyperlink = static_cast<CHyperlink*>(spOwner.Cast<Hyperlink>()->GetHandle());
    CFrameworkElement *element = hyperlink->GetContainingFrameworkElement();

    ITextView* textView = CTextAdapter::GetTextView(element);

    if (textView != nullptr)
    {
        xref_ptr<CTextPointerWrapper> contentStart;
        xref_ptr<CTextPointerWrapper> contentEnd;
        int contentStartOffset = 0;
        int contentEndOffset = 0;
        uint32_t cBoundRects = 0;
        XRECTF *pBoundRects = nullptr;
        auto scopeExit = wil::scope_exit([&]
        {
            delete pBoundRects;
            pBoundRects = nullptr;
        });

        IFC_RETURN(hyperlink->GetTextContentStart(contentStart.ReleaseAndGetAddressOf()));
        IFC_RETURN(hyperlink->GetTextContentEnd(contentEnd.ReleaseAndGetAddressOf()));
        if (contentStart == nullptr || contentEnd == nullptr)
        {
            return S_OK;
        }
        IFC_RETURN(contentStart->GetOffset(&contentStartOffset));
        IFC_RETURN(contentEnd->GetOffset(&contentEndOffset));

        textView->TextRangeToTextBounds(contentStartOffset, contentEndOffset, &cBoundRects, &pBoundRects);

        if (cBoundRects > 0)
        {
            // We're looking for the point at the start of the link, so We only care about the first rectangle
            XRECTF pRectBound = pBoundRects[0];

            XRECTF_RB rtGlobalBounds = {};
            IFC_RETURN(element->TransformToWorldSpace(&(ToXRectFRB(pRectBound)), &rtGlobalBounds, false /*ignoreClip*/, false /* ignoreClippingOnScrollContentPresenters */, false /* useTargetInformation */));
            pRectBound = ToXRectF(rtGlobalBounds);

            // Return the top left corner of the Hyperlink because we determine the length from there.
            // Round up the Y pixel so we don't get the previous line when there is more than one line.
            pReturnValue->X = pRectBound.X;
            pReturnValue->Y = std::ceil(pRectBound.Y);
        }
    }

    return S_OK;
}

IFACEMETHODIMP HyperlinkAutomationPeer::IsOffscreenCore(_Out_ BOOLEAN* returnValue)
{
    // IsOffscreen for a hyperlink should match the value of its containing text element
    ctl::ComPtr<xaml_docs::IHyperlink> spOwner;
    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));
    CHyperlink *pHyperlink = static_cast<CHyperlink*>(spOwner.Cast<Hyperlink>()->GetHandle());
    CFrameworkElement *pContainingTextElement = pHyperlink->GetContainingFrameworkElement();

    if (pContainingTextElement)
    {
        CAutomationPeer* pContainerAP = pContainingTextElement->OnCreateAutomationPeer();
        IFC_RETURN(static_cast<CFrameworkElementAutomationPeer*>(pContainerAP)->IsOffscreenHelper(false /* ignoreClippingOnScrollContentPresenters */, returnValue));
    }
    else
    {
        *returnValue = TRUE;
    }

    return S_OK;
}
