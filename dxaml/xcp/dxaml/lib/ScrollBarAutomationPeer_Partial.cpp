// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ScrollBarAutomationPeer.g.h"
#include "ScrollBar.g.h"
#include "localizedResource.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT ScrollBarAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_primitives::IScrollBar* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IScrollBarAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IScrollBarAutomationPeer* pInstance = NULL;
    xaml::IUIElement* ownerAsUIE = NULL;
    IInspectable* pInner = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(owner);
    IFC(ctl::do_query_interface(ownerAsUIE, owner));
    
    IFC(ActivateInstance(pOuter,
            static_cast<ScrollBar*>(owner)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<ScrollBarAutomationPeer*>(pInstance)->put_Owner(ownerAsUIE));

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

// Initializes a new instance of the ScrollBarAutomationPeer class.
ScrollBarAutomationPeer::ScrollBarAutomationPeer()
{
}

// Deconstructor
ScrollBarAutomationPeer::~ScrollBarAutomationPeer()
{
}

IFACEMETHODIMP ScrollBarAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"ScrollBar")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ScrollBarAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue)
{
    *pReturnValue = xaml_automation_peers::AutomationControlType_ScrollBar;
    RRETURN(S_OK);
}

IFACEMETHODIMP ScrollBarAutomationPeer::GetClickablePointCore(_Out_ wf::Point* pReturnValue)
{
    pReturnValue->X = static_cast<FLOAT>(DoubleUtil::NaN);
    pReturnValue->Y = static_cast<FLOAT>(DoubleUtil::NaN);
    RRETURN(S_OK);
}

IFACEMETHODIMP ScrollBarAutomationPeer::GetOrientationCore(_Out_ xaml_automation_peers::AutomationOrientation* pReturnValue)
{
    HRESULT hr = S_OK;
    xaml::IUIElement* pOwner = NULL;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;

    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);
    IFC((static_cast<ScrollBar*>(pOwner))->get_Orientation(&orientation));
    
    if(orientation == xaml_controls::Orientation_Horizontal)
    {
        *pReturnValue = xaml_automation_peers::AutomationOrientation_Horizontal;
    }
    else
    {
        *pReturnValue = xaml_automation_peers::AutomationOrientation_Vertical;
    }

Cleanup:
    ReleaseInterface(pOwner);
    RRETURN(hr);
}

IFACEMETHODIMP ScrollBarAutomationPeer::IsContentElementCore(_Out_ BOOLEAN* pReturnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pReturnValue);
    *pReturnValue = FALSE;

Cleanup:
    RRETURN(hr);
}

// Override for the ScrollBarAutomationPeer doesn't allow creating 
// elements from collapsed vertical or horizontal template.
_Check_return_ HRESULT ScrollBarAutomationPeer::ChildIsAcceptable(
    _In_ xaml::IUIElement* pElement,
    _Out_ BOOLEAN* bchildIsAcceptable)
{
    HRESULT hr = S_OK;
    xaml::IUIElement* pOwner = NULL;
    xaml::IUIElement* pElementHorizontalTemplate = NULL;
    xaml::IUIElement* pElementVerticalTemplate = NULL;
    xaml::Visibility visibility = xaml::Visibility_Collapsed;

    IFC(ScrollBarAutomationPeerGenerated::ChildIsAcceptable(pElement, bchildIsAcceptable));

    if (*bchildIsAcceptable)
    {
        IFC(get_Owner(&pOwner));
        IFCPTR(pOwner);

        IFC((static_cast<ScrollBar*>(pOwner))->get_ElementHorizontalTemplate(&pElementHorizontalTemplate));
        IFC((static_cast<ScrollBar*>(pOwner))->get_ElementVerticalTemplate(&pElementVerticalTemplate));
        
        if (pElement == pElementHorizontalTemplate || pElement == pElementVerticalTemplate)
        {
            IFC((static_cast<UIElement*>(pElement))->get_Visibility(&visibility));
            *bchildIsAcceptable = visibility == xaml::Visibility_Visible;
        }
    }

Cleanup:
    ReleaseInterface(pElementHorizontalTemplate);
    ReleaseInterface(pElementVerticalTemplate);
    ReleaseInterface(pOwner);
    RRETURN(hr);
}

IFACEMETHODIMP ScrollBarAutomationPeer::GetNameCore(_Out_ HSTRING* returnValue)
{
    XUINT32 pLength = 0;

    IFC_RETURN(ScrollBarAutomationPeerGenerated::GetNameCore(returnValue));
    
    pLength = WindowsGetStringLen(*returnValue);
    if(pLength == 0)
    {
        ctl::ComPtr<xaml::IUIElement> spOwnerAsUIE;
        IFC_RETURN(get_Owner(&spOwnerAsUIE));

        xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;
        IFC_RETURN(spOwnerAsUIE.Cast<ScrollBar>()->get_Orientation(&orientation));

        DELETE_STRING(*returnValue);
        *returnValue = nullptr;

        if (orientation == xaml_controls::Orientation_Horizontal)
        {
            IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_SCROLLBAR_HORIZONTAL, returnValue));
        }
        else
        {
            IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_SCROLLBAR_VERTICAL, returnValue));
        }
    }
    
    return S_OK;
}
