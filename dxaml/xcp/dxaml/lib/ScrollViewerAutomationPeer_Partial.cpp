// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ScrollViewerAutomationPeer.g.h"
#include "ScrollViewer.g.h"
#include "IScrollInfo.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

const DOUBLE ScrollViewerAutomationPeer::minimumPercent = 0.0f;
const DOUBLE ScrollViewerAutomationPeer::maximumPercent = 100.0f;
const DOUBLE ScrollViewerAutomationPeer::noScroll = -1.0f;

_Check_return_ HRESULT ScrollViewerAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::IScrollViewer* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IScrollViewerAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IScrollViewerAutomationPeer* pInstance = NULL;
    xaml::IUIElement* ownerAsUIE = NULL;
    IInspectable* pInner = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(owner);
    IFC(ctl::do_query_interface(ownerAsUIE, owner));

    IFC(ActivateInstance(pOuter,
            static_cast<ScrollViewer*>(owner)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<ScrollViewerAutomationPeer*>(pInstance)->put_Owner(ownerAsUIE));

    if (ppInner)
    {
        *ppInner = pInner;
        pInner = NULL;
    }

    *ppInstance = pInstance;
    pInstance = NULL;

Cleanup:
    ReleaseInterface(ownerAsUIE);
    ReleaseInterface(pInstance);
    ReleaseInterface(pInner);
    RRETURN(hr);
}

// Initializes a new instance of the ScrollViewerAutomationPeer class.
ScrollViewerAutomationPeer::ScrollViewerAutomationPeer()
{
}

// Deconstructor
ScrollViewerAutomationPeer::~ScrollViewerAutomationPeer()
{
}

IFACEMETHODIMP ScrollViewerAutomationPeer::GetPatternCore(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** returnValue)
{
    HRESULT hr = S_OK;

    if (patternInterface == xaml_automation_peers::PatternInterface_Scroll)
    {
        *returnValue = ctl::as_iinspectable(this);
        ctl::addref_interface(this);
    }
    else
    {
        IFC(ScrollViewerAutomationPeerGenerated::GetPatternCore(patternInterface, returnValue));
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ScrollViewerAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"ScrollViewer")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ScrollViewerAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* returnValue)
{
    *returnValue = xaml_automation_peers::AutomationControlType_Pane;
    RRETURN(S_OK);
}

//-------------------------------------------------------------------
//  IScrollProvider
//-------------------------------------------------------------------

// Request to scroll horizontally and vertically by the specified amount.
// The ability to call this method and simultaneously scroll horizontally
// and vertically provides simple panning support.
_Check_return_ HRESULT ScrollViewerAutomationPeer::ScrollImpl(_In_ xaml_automation::ScrollAmount horizontalAmount, _In_ xaml_automation::ScrollAmount verticalAmount)
{
    HRESULT hr = S_OK;
    xaml::IUIElement* pOwner = NULL;
    BOOLEAN bHorizontallyScrollable = FALSE;
    BOOLEAN bVerticallyScrollable = FALSE;
    BOOLEAN bIsEnabled;
    BOOLEAN scrollHorizontally = horizontalAmount != xaml_automation::ScrollAmount_NoAmount;
    BOOLEAN scrollVertically = verticalAmount != xaml_automation::ScrollAmount_NoAmount;

    IFC(IsEnabled(&bIsEnabled));
    if(!bIsEnabled)
    {
        IFC(static_cast<HRESULT>(UIA_E_ELEMENTNOTENABLED));
    }


    IFC(get_HorizontallyScrollable(&bHorizontallyScrollable));
    IFC(get_VerticallyScrollable(&bVerticallyScrollable));
    if(scrollHorizontally && !bHorizontallyScrollable || scrollVertically && !bVerticallyScrollable)
    {
        IFC(static_cast<HRESULT>(UIA_E_INVALIDOPERATION));
    }

    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);

    switch(horizontalAmount)
    {
        case xaml_automation::ScrollAmount_LargeDecrement:
            IFC((static_cast<ScrollViewer*>(pOwner))->PageLeft());
            break;
        case xaml_automation::ScrollAmount_SmallDecrement:
            IFC((static_cast<ScrollViewer*>(pOwner))->LineLeft());
            break;
        case xaml_automation::ScrollAmount_SmallIncrement:
            IFC((static_cast<ScrollViewer*>(pOwner))->LineRight());
            break;
        case xaml_automation::ScrollAmount_LargeIncrement:
            IFC((static_cast<ScrollViewer*>(pOwner))->PageRight());
            break;
        case xaml_automation::ScrollAmount_NoAmount:
            break;
        default:
            IFC(static_cast<HRESULT>(UIA_E_INVALIDOPERATION));
            break;
    }

    switch(verticalAmount)
    {
        case xaml_automation::ScrollAmount_LargeDecrement:
            IFC((static_cast<ScrollViewer*>(pOwner))->PageUp());
            break;
        case xaml_automation::ScrollAmount_SmallDecrement:
            IFC((static_cast<ScrollViewer*>(pOwner))->LineUp());
            break;
        case xaml_automation::ScrollAmount_SmallIncrement:
            IFC((static_cast<ScrollViewer*>(pOwner))->LineDown());
            break;
        case xaml_automation::ScrollAmount_LargeIncrement:
            IFC((static_cast<ScrollViewer*>(pOwner))->PageDown());
            break;
        case xaml_automation::ScrollAmount_NoAmount:
            break;
        default:
            IFC(static_cast<HRESULT>(UIA_E_INVALIDOPERATION));
            break;
    }

Cleanup:
    ReleaseInterface(pOwner);
    if(hr == UIA_E_INVALIDOPERATION)
    {
        IGNOREHR(ErrorHelper::OriginateError(AgError(UIA_OPERATION_CANNOT_BE_PERFORMED)));
    }
    RRETURN(hr);
}

// Request to set the current horizontal and Vertical scroll position by percent (0-100).
// Passing in the value of "-1", represented by the constant "NoScroll", will indicate that scrolling
// in that direction should be ignored.
// The ability to call this method and simultaneously scroll horizontally and vertically provides simple panning support.
_Check_return_ HRESULT ScrollViewerAutomationPeer::SetScrollPercentImpl(_In_ DOUBLE horizontalPercent, _In_ DOUBLE verticalPercent)
{
    HRESULT hr = S_OK;
    xaml::IUIElement* pOwner = NULL;
    DOUBLE extentWidth = 0.0;
    DOUBLE extentHeight = 0.0;
    DOUBLE viewportWidth = 0.0;
    DOUBLE viewportHeight = 0.0;
    DOUBLE minHorizontalOffset = 0.0;
    DOUBLE minVerticalOffset = 0.0;
    DOUBLE nettAvailable = 0.0;
    BOOLEAN bHorizontallyScrollable = FALSE;
    BOOLEAN bVerticallyScrollable = FALSE;
    BOOLEAN bIsEnabled;
    BOOLEAN scrollHorizontally = horizontalPercent != noScroll;
    BOOLEAN scrollVertically = verticalPercent != noScroll;

    IFC(IsEnabled(&bIsEnabled));
    if(!bIsEnabled)
    {
        IFC(static_cast<HRESULT>(UIA_E_ELEMENTNOTENABLED));
    }

    IFC(get_HorizontallyScrollable(&bHorizontallyScrollable));
    IFC(get_VerticallyScrollable(&bVerticallyScrollable));
    if(scrollHorizontally && !bHorizontallyScrollable || scrollVertically && !bVerticallyScrollable)
    {
        IFC(static_cast<HRESULT>(UIA_E_INVALIDOPERATION));
    }

    if((scrollHorizontally && (horizontalPercent < minimumPercent || horizontalPercent > maximumPercent))
        || (scrollVertically && (verticalPercent < minimumPercent || verticalPercent > maximumPercent)))
    {
        IFC(E_INVALIDARG);
    }

    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);

    // Make sure to take minimum offset into account in either dimension to add to actual offset,
    // minimum offset is new concept introduced for headered controls.
    if(scrollHorizontally)
    {
        IFC((static_cast<ScrollViewer*>(pOwner))->get_ExtentWidth(&extentWidth));
        IFC((static_cast<ScrollViewer*>(pOwner))->get_ViewportWidth(&viewportWidth));
        IFC((static_cast<ScrollViewer*>(pOwner))->get_MinHorizontalOffset(&minHorizontalOffset));
        nettAvailable = DoubleUtil::Max(0.0, (extentWidth - viewportWidth - minHorizontalOffset));
        IFC((static_cast<ScrollViewer*>(pOwner))->ScrollToHorizontalOffsetInternal((nettAvailable * (DOUBLE)(horizontalPercent * 0.01)) + minHorizontalOffset));
    }

    if(scrollVertically)
    {
        IFC((static_cast<ScrollViewer*>(pOwner))->get_ExtentHeight(&extentHeight));
        IFC((static_cast<ScrollViewer*>(pOwner))->get_ViewportHeight(&viewportHeight));
        IFC((static_cast<ScrollViewer*>(pOwner))->get_MinVerticalOffset(&minVerticalOffset));
        nettAvailable = DoubleUtil::Max(0.0, (extentHeight - viewportHeight - minVerticalOffset));
        IFC((static_cast<ScrollViewer*>(pOwner))->ScrollToVerticalOffsetInternal((nettAvailable * (DOUBLE)(verticalPercent * 0.01)) + minVerticalOffset));
    }

Cleanup:
    ReleaseInterface(pOwner);
    if(hr == UIA_E_INVALIDOPERATION)
    {
        IGNOREHR(ErrorHelper::OriginateError(AgError(UIA_OPERATION_CANNOT_BE_PERFORMED)));
    }
    RRETURN(hr);
}

// True if control can scroll horizontally
_Check_return_ HRESULT ScrollViewerAutomationPeer::get_HorizontallyScrollableImpl(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    DOUBLE extentWidth = 0.0;
    DOUBLE viewportWidth = 0.0;
    DOUBLE minHorizontalOffset = 0.0;
    xaml::IUIElement* pOwner = NULL;
    IScrollInfo* pScrollInfo = NULL;

    IFCPTR(pValue);
    *pValue = FALSE;
    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);

    IFC((static_cast<ScrollViewer*>(pOwner))->get_ScrollInfo(&pScrollInfo));
    if(pScrollInfo)
    {
        IFC((static_cast<ScrollViewer*>(pOwner))->get_ExtentWidth(&extentWidth));
        IFC((static_cast<ScrollViewer*>(pOwner))->get_ViewportWidth(&viewportWidth));
        IFC((static_cast<ScrollViewer*>(pOwner))->get_MinHorizontalOffset(&minHorizontalOffset));
        *pValue = AutomationIsScrollable(extentWidth, viewportWidth, minHorizontalOffset);
    }

Cleanup:
    ReleaseInterface(pOwner);
    ReleaseInterface(pScrollInfo);
    RRETURN(hr);
}

// Get the current horizontal scroll position
_Check_return_ HRESULT ScrollViewerAutomationPeer::get_HorizontalScrollPercentImpl(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;
    DOUBLE extentWidth = 0.0;
    DOUBLE viewportWidth = 0.0;
    DOUBLE horizontalOffset = 0.0;
    DOUBLE minHorizontalOffset = 0.0;
    xaml::IUIElement* pOwner = NULL;
    IScrollInfo* pScrollInfo = NULL;

    IFCPTR(pValue);
    *pValue = noScroll;
    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);

    IFC((static_cast<ScrollViewer*>(pOwner))->get_ScrollInfo(&pScrollInfo));
    if(pScrollInfo)
    {
        // Make sure to take minimum offset into account in horizontal dimension to subtract from actual offset,
        // minimum offset is new concept introduced for headered controls.
        IFC((static_cast<ScrollViewer*>(pOwner))->get_ExtentWidth(&extentWidth));
        IFC((static_cast<ScrollViewer*>(pOwner))->get_ViewportWidth(&viewportWidth));
        IFC((static_cast<ScrollViewer*>(pOwner))->get_HorizontalOffset(&horizontalOffset));
        IFC((static_cast<ScrollViewer*>(pOwner))->get_MinHorizontalOffset(&minHorizontalOffset));
        *pValue = AutomationGetScrollPercent(extentWidth, viewportWidth, horizontalOffset, minHorizontalOffset);
    }

Cleanup:
    ReleaseInterface(pOwner);
    ReleaseInterface(pScrollInfo);
    RRETURN(hr);
}

// Equal to the horizontal percentage of the entire control that is currently viewable.
_Check_return_ HRESULT ScrollViewerAutomationPeer::get_HorizontalViewSizeImpl(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;
    DOUBLE extentWidth = 0.0;
    DOUBLE viewportWidth = 0.0;
    DOUBLE minHorizontalOffset = 0.0;
    xaml::IUIElement* pOwner = NULL;
    IScrollInfo* pScrollInfo = NULL;

    IFCPTR(pValue);
    *pValue = 100.0;
    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);

    IFC((static_cast<ScrollViewer*>(pOwner))->get_ScrollInfo(&pScrollInfo));
    if(pScrollInfo)
    {
        IFC((static_cast<ScrollViewer*>(pOwner))->get_ExtentWidth(&extentWidth));
        IFC((static_cast<ScrollViewer*>(pOwner))->get_ViewportWidth(&viewportWidth));
        IFC((static_cast<ScrollViewer*>(pOwner))->get_MinHorizontalOffset(&minHorizontalOffset));
        *pValue = AutomationGetViewSize(extentWidth, viewportWidth, minHorizontalOffset);
    }

Cleanup:
    ReleaseInterface(pOwner);
    ReleaseInterface(pScrollInfo);
    RRETURN(hr);
}

// True if control can scroll vertically
_Check_return_ HRESULT ScrollViewerAutomationPeer::get_VerticallyScrollableImpl(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    DOUBLE extentHeight = 0.0;
    DOUBLE viewportHeight = 0.0;
    DOUBLE minVerticalOffset = 0.0;
    xaml::IUIElement* pOwner = NULL;
    IScrollInfo* pScrollInfo = NULL;

    IFCPTR(pValue);
    *pValue = FALSE;
    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);

    IFC((static_cast<ScrollViewer*>(pOwner))->get_ScrollInfo(&pScrollInfo));
    if(pScrollInfo)
    {
        IFC((static_cast<ScrollViewer*>(pOwner))->get_ExtentHeight(&extentHeight));
        IFC((static_cast<ScrollViewer*>(pOwner))->get_ViewportHeight(&viewportHeight));
        IFC((static_cast<ScrollViewer*>(pOwner))->get_MinVerticalOffset(&minVerticalOffset));
        *pValue = AutomationIsScrollable(extentHeight, viewportHeight, minVerticalOffset);
    }

Cleanup:
    ReleaseInterface(pOwner);
    ReleaseInterface(pScrollInfo);
    RRETURN(hr);
}

_Check_return_ HRESULT ScrollViewerAutomationPeer::get_VerticalScrollPercentImpl(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;
    DOUBLE extentHeight = 0.0;
    DOUBLE viewportHeight = 0.0;
    DOUBLE verticalOffset = 0.0;
    DOUBLE minVerticalOffset = 0.0;
    xaml::IUIElement* pOwner = NULL;
    IScrollInfo* pScrollInfo = NULL;

    IFCPTR(pValue);
    *pValue = noScroll;
    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);
    IFC((static_cast<ScrollViewer*>(pOwner))->get_ScrollInfo(&pScrollInfo));

    if(pScrollInfo)
    {
        // Make sure to take minimum offset into account in veritcal dimension to subtract from actual offset,
        // minimum offset is new concept introduced for headered controls.
        IFC((static_cast<ScrollViewer*>(pOwner))->get_ExtentHeight(&extentHeight));
        IFC((static_cast<ScrollViewer*>(pOwner))->get_ViewportHeight(&viewportHeight));
        IFC((static_cast<ScrollViewer*>(pOwner))->get_VerticalOffset(&verticalOffset));
        IFC((static_cast<ScrollViewer*>(pOwner))->get_MinVerticalOffset(&minVerticalOffset));

        *pValue = AutomationGetScrollPercent(extentHeight, viewportHeight, verticalOffset, minVerticalOffset);
    }

Cleanup:
    ReleaseInterface(pOwner);
    ReleaseInterface(pScrollInfo);
    RRETURN(hr);
}

// Equal to the vertical percentage of the entire control that is currently viewable.
_Check_return_ HRESULT ScrollViewerAutomationPeer::get_VerticalViewSizeImpl(_Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;
    DOUBLE extentHeight = 0.0;
    DOUBLE viewportHeight = 0.0;
    DOUBLE minVerticalOffset = 0.0;
    xaml::IUIElement* pOwner = NULL;
    IScrollInfo* pScrollInfo = NULL;

    IFCPTR(pValue);
    *pValue = 100.0;
    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);

    IFC((static_cast<ScrollViewer*>(pOwner))->get_ScrollInfo(&pScrollInfo));
    if(pScrollInfo)
    {
        IFC((static_cast<ScrollViewer*>(pOwner))->get_ExtentHeight(&extentHeight));
        IFC((static_cast<ScrollViewer*>(pOwner))->get_ViewportHeight(&viewportHeight));
        IFC((static_cast<ScrollViewer*>(pOwner))->get_MinVerticalOffset(&minVerticalOffset));
        *pValue = AutomationGetViewSize(extentHeight, viewportHeight, minVerticalOffset);
    }

Cleanup:
    ReleaseInterface(pOwner);
    ReleaseInterface(pScrollInfo);
    RRETURN(hr);
}

// Raise Relevant Scroll Pattern related events to UIAutomation Clients for various changes related to scrolling.
_Check_return_ HRESULT ScrollViewerAutomationPeer::RaiseAutomationEvents(DOUBLE extentX,
                                                                        DOUBLE extentY,
                                                                        DOUBLE viewportX,
                                                                        DOUBLE viewportY,
                                                                        DOUBLE minOffsetX,
                                                                        DOUBLE minOffsetY,
                                                                        DOUBLE offsetX,
                                                                        DOUBLE offsetY)
{
    HRESULT hr = S_OK;
    BOOLEAN oldScrollable = FALSE;
    BOOLEAN newScrollable = FALSE;
    DOUBLE oldViewSize = 0.0;
    DOUBLE newViewSize = 0.0;
    DOUBLE oldScrollPercent = 0.0;
    DOUBLE newScrollPercent = 0.0;
    CValue valueOld;
    CValue valueNew;

    CAutomationPeer* pHandle = static_cast<CAutomationPeer*>(GetHandle());

    oldScrollable = AutomationIsScrollable(extentX, viewportX, minOffsetX);
    IFC(get_HorizontallyScrollable(&newScrollable));
    if(oldScrollable != newScrollable)
    {
        IFC(CValueBoxer::BoxValue(&valueOld, oldScrollable));
        IFC(CValueBoxer::BoxValue(&valueNew, newScrollable));
        IFC(CoreImports::AutomationRaiseAutomationPropertyChanged(pHandle, UIAXcp::APAutomationProperties::APHorizontallyScrollableProperty, valueOld, valueNew));
    }

    oldScrollable = AutomationIsScrollable(extentY, viewportY, minOffsetY);
    IFC(get_VerticallyScrollable(&newScrollable));
    if(oldScrollable != newScrollable)
    {
        IFC(CValueBoxer::BoxValue(&valueOld, oldScrollable));
        IFC(CValueBoxer::BoxValue(&valueNew, newScrollable));
        IFC(CoreImports::AutomationRaiseAutomationPropertyChanged(pHandle, UIAXcp::APAutomationProperties::APVerticallyScrollableProperty, valueOld, valueNew));
    }

    oldViewSize = AutomationGetViewSize(extentX, viewportX, minOffsetX);
    IFC(get_HorizontalViewSize(&newViewSize));
    if(oldViewSize != newViewSize)
    {
        IFC(CValueBoxer::BoxValue(&valueOld, oldViewSize));
        IFC(CValueBoxer::BoxValue(&valueNew, newViewSize));
        IFC(CoreImports::AutomationRaiseAutomationPropertyChanged(pHandle, UIAXcp::APAutomationProperties::APHorizontalViewSizeProperty, valueOld, valueNew));
    }

    oldViewSize = AutomationGetViewSize(extentY, viewportY, minOffsetY);
    IFC(get_VerticalViewSize(&newViewSize));
    if(oldViewSize != newViewSize)
    {
        IFC(CValueBoxer::BoxValue(&valueOld, oldViewSize));
        IFC(CValueBoxer::BoxValue(&valueNew, newViewSize));
        IFC(CoreImports::AutomationRaiseAutomationPropertyChanged(pHandle, UIAXcp::APAutomationProperties::APVerticalViewSizeProperty, valueOld, valueNew));
    }

    oldScrollPercent = AutomationGetScrollPercent(extentX, viewportX, offsetX, minOffsetX);
    IFC(get_HorizontalScrollPercent(&newScrollPercent));
    if(oldScrollPercent != newScrollPercent)
    {
        IFC(CValueBoxer::BoxValue(&valueOld, oldScrollPercent));
        IFC(CValueBoxer::BoxValue(&valueNew, newScrollPercent));
        IFC(CoreImports::AutomationRaiseAutomationPropertyChanged(pHandle, UIAXcp::APAutomationProperties::APHorizontalScrollPercentProperty, valueOld, valueNew));
    }

    oldScrollPercent = AutomationGetScrollPercent(extentY, viewportY, offsetY, minOffsetY);
    IFC(get_VerticalScrollPercent(&newScrollPercent));
    if(oldScrollPercent != newScrollPercent)
    {
        IFC(CValueBoxer::BoxValue(&valueOld, oldScrollPercent));
        IFC(CValueBoxer::BoxValue(&valueNew, newScrollPercent));
        IFC(CoreImports::AutomationRaiseAutomationPropertyChanged(pHandle, UIAXcp::APAutomationProperties::APVerticalScrollPercentProperty, valueOld, valueNew));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_
BOOLEAN
ScrollViewerAutomationPeer::AutomationIsScrollable(_In_ DOUBLE extent, _In_ DOUBLE viewport, _In_ DOUBLE minOffset)
{
    return DoubleUtil::GreaterThan(extent, viewport + minOffset);
}

_Check_return_
DOUBLE
ScrollViewerAutomationPeer::AutomationGetScrollPercent(_In_ DOUBLE extent, _In_ DOUBLE viewport, _In_ DOUBLE actualOffset, _In_ DOUBLE minOffset)
{
    HRESULT hr = S_OK;
    DOUBLE retVal = noScroll;
    xaml::IUIElement* pOwner = NULL;

    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);

    if (!AutomationIsScrollable(extent, viewport, minOffset))
    {
        goto Cleanup;
    }

    actualOffset = DoubleUtil::Max(0.0, (actualOffset - minOffset));
    retVal = (actualOffset * 100.0 / (extent - viewport - minOffset));

Cleanup:
    ReleaseInterface(pOwner);
    return retVal;
}

_Check_return_
DOUBLE
ScrollViewerAutomationPeer::AutomationGetViewSize(_In_ DOUBLE extent, _In_ DOUBLE viewport, _In_ DOUBLE minOffset)
{
    DOUBLE nettExtent = 0.0;

    nettExtent = DoubleUtil::Max(0.0, (extent - minOffset));
    if (DoubleUtil::IsZero(nettExtent)) { return 100.0; }
    return DoubleUtil::Min(100.0, (viewport * 100.0 / nettExtent));
}


// Override for the ScrollViewerAutomationPeer doesn't allow creating
// elements from collapsed vertical or horizontal template.
_Check_return_ HRESULT ScrollViewerAutomationPeer::ChildIsAcceptable(
    _In_ xaml::IUIElement* pElement,
    _Out_ BOOLEAN* bchildIsAcceptable)
{
    HRESULT hr = S_OK;
    xaml::IUIElement* pOwner = NULL;
    xaml::IUIElement* pElementHorizontalScrollBar = NULL;
    xaml::IUIElement* pElementVerticalScrollBar = NULL;
    xaml::Visibility visibility = xaml::Visibility_Collapsed;

    IFC(ScrollViewerAutomationPeerGenerated::ChildIsAcceptable(pElement, bchildIsAcceptable));

    if (*bchildIsAcceptable)
    {
        IFC(get_Owner(&pOwner));
        IFCPTR(pOwner);

        IFC((static_cast<ScrollViewer*>(pOwner))->get_ElementHorizontalScrollBar(&pElementHorizontalScrollBar));
        IFC((static_cast<ScrollViewer*>(pOwner))->get_ElementVerticalScrollBar(&pElementVerticalScrollBar));

        if (pElement == pElementHorizontalScrollBar || pElement == pElementVerticalScrollBar)
        {
            IFC((static_cast<UIElement*>(pElement))->get_Visibility(&visibility));
            *bchildIsAcceptable = visibility == xaml::Visibility_Visible;
        }
    }

Cleanup:
    ReleaseInterface(pElementHorizontalScrollBar);
    ReleaseInterface(pElementVerticalScrollBar);
    ReleaseInterface(pOwner);
    RRETURN(hr);
}
