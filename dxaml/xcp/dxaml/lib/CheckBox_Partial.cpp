// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CheckBox.g.h"
#include "CheckBoxAutomationPeer.g.h"
#include "KeyboardNavigation.h"

using namespace DirectUI;

// Initializes a new instance of the CheckBox class.
CheckBox::CheckBox()
{
}

// Deconstructor
CheckBox::~CheckBox()
{
}

_Check_return_ HRESULT
CheckBox::Initialize()
{
    IFC_RETURN(CheckBoxGenerated::Initialize());

    // Ignore the ENTER key by default
    SetAcceptsReturn(false);

    return S_OK;
}


// Handles the KeyDown event for CheckBox.
_Check_return_ HRESULT CheckBox::OnKeyDownInternal(
    _In_ wsy::VirtualKey nKey,
    _Out_ BOOLEAN* pbHandled)
{
    HRESULT hr = S_OK;
    BOOLEAN bIsThreeState = FALSE;
    BOOLEAN bIsEnabled = FALSE;
    ctl::ComPtr<IInspectable> spNewValue;
    ctl::ComPtr<wf::IReference<bool>> spNewValueReference;
    
    IFC(CheckBoxGenerated::OnKeyDownInternal(nKey, pbHandled));
    IFC(get_IsThreeState(&bIsThreeState));
    IFC(get_IsEnabled(&bIsEnabled));
    
    if (!bIsThreeState && bIsEnabled)
    {
        if (nKey == wsy::VirtualKey_Add)
        {
            *pbHandled = TRUE;
            IFC(put_IsPressed(FALSE));

            IFC(PropertyValue::CreateFromBoolean(TRUE, &spNewValue));
            IFC(spNewValue.As(&spNewValueReference));
            IFC(put_IsChecked(spNewValueReference.Get()));
        }
        else if (nKey == wsy::VirtualKey_Subtract)
        {
            *pbHandled = TRUE;
            IFC(put_IsPressed(FALSE));
            
            IFC(PropertyValue::CreateFromBoolean(FALSE, &spNewValue));
            IFC(spNewValue.As(&spNewValueReference));
            IFC(put_IsChecked(spNewValueReference.Get()));
        }
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP CheckBox::OnApplyTemplate()
{
    ctl::ComPtr<xaml::IDependencyObject> checkGlyph;

    IFC_RETURN(__super::OnApplyTemplate());

    // If HighContrastAdjustment property hasn't been set for CheckGlyph set the initial value to None to avoid BackPlate drawing over CheckBox rectangle.
    // VSO: 10073971
    IFC_RETURN(GetTemplateChild(wrl_wrappers::HStringReference(L"CheckGlyph").Get(), &checkGlyph));

    if (checkGlyph != nullptr)
    {
        auto checkGlyphDO = static_cast<DependencyObject*>(checkGlyph.Get())->GetHandle();

        const auto elementHighContrastAdjustment = checkGlyphDO->GetPropertyByIndexInline(KnownPropertyIndex::UIElement_HighContrastAdjustment);
            
        if (checkGlyphDO->IsPropertyDefault(elementHighContrastAdjustment))
        {
            CValue highContrastAdjustment;
            highContrastAdjustment.Set(DirectUI::ElementHighContrastAdjustment::None);

            IFC_RETURN(checkGlyphDO->SetValueByIndex(KnownPropertyIndex::UIElement_HighContrastAdjustment, highContrastAdjustment));
        }
    }

    return S_OK;
}

// Create CheckBoxAutomationPeer to represent the CheckBox.
IFACEMETHODIMP CheckBox::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::ICheckBoxAutomationPeer> spCheckBoxAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::ICheckBoxAutomationPeerFactory> spCheckBoxAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;
    
    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::CheckBoxAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As(&spCheckBoxAPFactory));

    IFC(spCheckBoxAPFactory.Cast<CheckBoxAutomationPeerFactory>()->CreateInstanceWithOwner(this, 
        NULL, 
        &spInner, 
        &spCheckBoxAutomationPeer));
    IFC(spCheckBoxAutomationPeer.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}

// Change to the correct visual state for the CheckBox.
_Check_return_ HRESULT
CheckBox::ChangeVisualState(
    // true to use transitions when updating the visual state, false
    // to snap directly to the new visual state.
    _In_ bool bUseTransitions)
{
    HRESULT hr = S_OK;
    BOOLEAN bIsEnabled = FALSE;
    BOOLEAN bIsPressed = FALSE;
    BOOLEAN bIsPointerOver = FALSE;
    ctl::ComPtr<wf::IReference<bool>> spIsCheckedReference;
    BOOLEAN bIsChecked = FALSE;
    BOOLEAN bSucceed = FALSE;
    xaml::FocusState focusState = xaml::FocusState_Unfocused;

    IFC(get_IsEnabled(&bIsEnabled));
    IFC(get_IsPressed(&bIsPressed));
    IFC(get_IsPointerOver(&bIsPointerOver));
    IFC(get_FocusState(&focusState));

    IFC(this->get_IsChecked(&spIsCheckedReference));
    if (spIsCheckedReference)
    {
        IFC(spIsCheckedReference->get_Value(&bIsChecked));
    }

    //Update the Combined state group
    if (!spIsCheckedReference) 
    {
        if (!bIsEnabled) 
        {
            IFC(GoToState(bUseTransitions, L"IndeterminateDisabled", &bSucceed));
        }
        else if (bIsPressed)
        {
            IFC(GoToState(bUseTransitions, L"IndeterminatePressed", &bSucceed));
        }
        else if (bIsPointerOver) 
        {
            IFC(GoToState(bUseTransitions, L"IndeterminatePointerOver", &bSucceed));
        }
        else
        {
            IFC(GoToState(bUseTransitions, L"IndeterminateNormal", &bSucceed));
        }
    }
    else if (bIsChecked) 
    {
        if (!bIsEnabled) 
        {
            IFC(GoToState(bUseTransitions, L"CheckedDisabled", &bSucceed));
        }
        else if (bIsPressed)
        {
            IFC(GoToState(bUseTransitions, L"CheckedPressed", &bSucceed));
        }
        else if (bIsPointerOver) 
        {
            IFC(GoToState(bUseTransitions, L"CheckedPointerOver", &bSucceed));
        }
        else
        {
            IFC(GoToState(bUseTransitions, L"CheckedNormal", &bSucceed));
        }
    }
    else 
    {
        if (!bIsEnabled)
        {
            IFC(GoToState(bUseTransitions, L"UncheckedDisabled", &bSucceed));
        }
        else if (bIsPressed)
        {
            IFC(GoToState(bUseTransitions, L"UncheckedPressed", &bSucceed));
        }
        else if (bIsPointerOver) 
        {
            IFC(GoToState(bUseTransitions, L"UncheckedPointerOver", &bSucceed));
        }
        else
        {
            IFC(GoToState(bUseTransitions, L"UncheckedNormal", &bSucceed));
        }
    }

    // Go to an older state if a combined state isn't available. If a Blue app is upgraded to TH
    // without updating its custom templates, these states will be present in the template instead
    // of the newer ones.
    if (!bSucceed) 
    {
        // Update the Interaction state group
        if (!bIsEnabled)
        {
            IFC(GoToState(bUseTransitions, L"Disabled", &bSucceed));
        }
        else if (bIsPressed)
        {
            IFC(GoToState(bUseTransitions, L"Pressed", &bSucceed));
        }
        else if (bIsPointerOver)
        {
            IFC(GoToState(bUseTransitions, L"PointerOver", &bSucceed));
        }
        else
        {
            IFC(GoToState(bUseTransitions, L"Normal", &bSucceed));
        }

        // Update the Check state group
        if (!spIsCheckedReference)
        {
            // Indeterminate
            IFC(GoToState(bUseTransitions, L"Indeterminate", &bSucceed));
        }
        else if (bIsChecked)
        {
            // Checked
            IFC(GoToState(bUseTransitions, L"Checked", &bSucceed));
        }
        else
        {
            // Unchecked
            IFC(GoToState(bUseTransitions, L"Unchecked", &bSucceed));
        }
    }

    // Update the Focus group
    if (xaml::FocusState_Unfocused != focusState && bIsEnabled)
    {
        if (xaml::FocusState_Pointer == focusState) 
        {
            IFC(GoToState(bUseTransitions, L"PointerFocused", &bSucceed));
        }
        else
        {
            IFC(GoToState(bUseTransitions, L"Focused", &bSucceed));
        }
    }
    else
    {
        IFC(GoToState(bUseTransitions, L"Unfocused", &bSucceed));
    }

Cleanup:
    RRETURN(hr);
}

