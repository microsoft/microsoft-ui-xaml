// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "SliderAutomationPeer.g.h"
#include "Slider.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT SliderAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::ISlider* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::ISliderAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::ISliderAutomationPeer* pInstance = NULL;
    IInspectable* pInner = NULL;
    xaml::IUIElement* ownerAsUIE = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(owner);
    IFC(ctl::do_query_interface(ownerAsUIE, owner));
    
    IFC(ActivateInstance(pOuter,
            static_cast<Slider*>(owner)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<SliderAutomationPeer*>(pInstance)->put_Owner(ownerAsUIE));

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

// Initializes a new instance of the SliderAutomationPeer class.
SliderAutomationPeer::SliderAutomationPeer()
{
}

// Deconstructor
SliderAutomationPeer::~SliderAutomationPeer()
{
}

IFACEMETHODIMP SliderAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"Slider")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP SliderAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue)
{
    *pReturnValue = xaml_automation_peers::AutomationControlType_Slider;
    RRETURN(S_OK);
}

IFACEMETHODIMP SliderAutomationPeer::GetClickablePointCore(_Out_ wf::Point* pReturnValue)
{
    pReturnValue->X = static_cast<FLOAT>(DoubleUtil::NaN);
    pReturnValue->Y = static_cast<FLOAT>(DoubleUtil::NaN);
    RRETURN(S_OK);
}

IFACEMETHODIMP SliderAutomationPeer::GetOrientationCore(_Out_ xaml_automation_peers::AutomationOrientation* pReturnValue)
{
    HRESULT hr = S_OK;
    xaml::IUIElement* pOwner = NULL;
    xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;

    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);
    IFC((static_cast<Slider*>(pOwner))->get_Orientation(&orientation));
    
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


// Override for the SliderAutomationPeer doesn't allow creating 
// elements from collapsed vertical or horizontal template.
_Check_return_ HRESULT SliderAutomationPeer::ChildIsAcceptable(
    _In_ xaml::IUIElement* pElement,
    _Out_ BOOLEAN* bchildIsAcceptable)
{
    HRESULT hr = S_OK;
    xaml::IUIElement* pOwner = NULL;
    xaml::IUIElement* pElementHorizontalTemplate = NULL;
    xaml::IUIElement* pElementVerticalTemplate = NULL;
    xaml::Visibility visibility = xaml::Visibility_Collapsed;

    IFC(SliderAutomationPeerGenerated::ChildIsAcceptable(pElement, bchildIsAcceptable));

    if (*bchildIsAcceptable)
    {
        IFC(get_Owner(&pOwner));
        IFCPTR(pOwner);

        IFC((static_cast<Slider*>(pOwner))->get_ElementHorizontalTemplate(&pElementHorizontalTemplate));
        IFC((static_cast<Slider*>(pOwner))->get_ElementVerticalTemplate(&pElementVerticalTemplate));
        
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
