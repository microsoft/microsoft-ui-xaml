// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "HubSection.g.h"
#include "Hub.g.h"
#include "Button.g.h"
#include "SemanticZoom.g.h"
#include "KeyRoutedEventArgs.g.h"
#include "XboxUtility.h"

using namespace DirectUI;
using namespace DirectUISynonyms;


// Override Arrange so that we update our chevron's visibility and direction.
// When effective FlowDirection (i.e. inherited FlowDirection) changes it causes layout to rerun, which is
// why we update the FlowDirection state of the header button here.
IFACEMETHODIMP
    HubSection::ArrangeOverride(
    _In_ wf::Size finalSize,
    _Out_ wf::Size* returnValue)
{
    HRESULT hr = S_OK;

    IFC(HubSectionGenerated::ArrangeOverride(finalSize, returnValue));
    IFC(UpdateHeaderButtonStyle());

Cleanup:
    return hr;
}


//update the button's visual state such that if Hub is not in a SemanticZoom it should look like a text block
_Check_return_ HRESULT
HubSection::UpdateHeaderButtonStyle()
{
    HRESULT hr = S_OK;
    BOOLEAN isHeaderInteractive = FALSE;

    IFC(get_IsHeaderInteractive(&isHeaderInteractive));

    if (m_tpHeaderButtonPart)
    {
        BOOLEAN bIgnored = FALSE;

        ctl::ComPtr<Hub> spParentHub;
        ctl::ComPtr<ISemanticZoom> semanticZoom;

        IFC(GetParentHub(&spParentHub));
        if (spParentHub)
        {
            ctl::ComPtr<ISemanticZoomInformation> spSemanticZoomInformation;

            IFC(spParentHub.As(&spSemanticZoomInformation));
            IFC(spSemanticZoomInformation->get_SemanticZoomOwner(&semanticZoom));
        }

        if (semanticZoom)
        {
            IFC(m_tpHeaderButtonPart.Cast<Button>()->put_IsHitTestVisible(true));
            IFC(m_tpHeaderButtonPart.Cast<Button>()->put_IsTabStop(true));
            IFC(VisualStateManager::GoToState(m_tpHeaderButtonPart.Cast<Button>(), wrl_wrappers::HStringReference(L"Normal").Get(), false, &bIgnored));
        }
        else
        {
            IFC(m_tpHeaderButtonPart.Cast<Button>()->put_IsHitTestVisible(false));
            IFC(m_tpHeaderButtonPart.Cast<Button>()->put_IsTabStop(false));
            IFC(VisualStateManager::GoToState(m_tpHeaderButtonPart.Cast<Button>(), wrl_wrappers::HStringReference(L"ImitatedTextBlock").Get(), false, &bIgnored));
        }
    }

    // Update the SeeMore button visibility according to IsHeaderInteractive state
    if (m_tpHeaderSeeMoreButtonPart)
    {
        IFC(m_tpHeaderSeeMoreButtonPart.Cast<Button>()->put_Visibility(isHeaderInteractive ? xaml::Visibility_Visible : xaml::Visibility_Collapsed));
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
HubSection::OnHeaderButtonClick()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<Hub> spParentHub;

    IFC(GetParentHub(&spParentHub));

    if (spParentHub)
    {
        ctl::ComPtr<ISemanticZoomInformation> spSemanticZoomInformation;
        ctl::ComPtr<ISemanticZoom> spSemanticZoom;

        IFC(spParentHub.As(&spSemanticZoomInformation));
        IFC(spSemanticZoomInformation->get_SemanticZoomOwner(&spSemanticZoom));

        if (spSemanticZoom)
        {
            BOOLEAN isZoomedInView = FALSE;

            IFC(spSemanticZoomInformation->get_IsZoomedInView(&isZoomedInView));

            if (isZoomedInView)
            {
                ctl::ComPtr<ISemanticZoomInformation> spZoomedOutView;

                IFC(spSemanticZoom->get_ZoomedOutView(&spZoomedOutView));

                if (spZoomedOutView)
                {
                    IFC(spSemanticZoom.Cast<SemanticZoom>()->ToggleActiveView());
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
HubSection::OnHeaderButtonKeyDown(
    _In_ IKeyRoutedEventArgs* pArgs)
{
    auto originalKey = wsy::VirtualKey::VirtualKey_None;
    IFC_RETURN(static_cast<KeyRoutedEventArgs*>(pArgs)->get_OriginalKey(&originalKey));
    if (XboxUtility::IsGamepadNavigationInput(originalKey))
    {
        return S_OK;
    }

    auto key = wsy::VirtualKey::VirtualKey_None;
    IFC_RETURN(pArgs->get_Key(&key));
    switch (key)
    {
    case wsy::VirtualKey::VirtualKey_Right:
    case wsy::VirtualKey::VirtualKey_Left:
    case wsy::VirtualKey::VirtualKey_Up:
    case wsy::VirtualKey::VirtualKey_Down:
    case wsy::VirtualKey::VirtualKey_PageUp:
    case wsy::VirtualKey::VirtualKey_PageDown:
        {
            ctl::ComPtr<Hub> spParentHub;

            IFC_RETURN(GetParentHub(&spParentHub));
            if (spParentHub)
            {
                BOOLEAN wasHandled = FALSE;

                IFC_RETURN(spParentHub->HandleNavigationKey(this, key, &wasHandled));
                if (wasHandled)
                {
                    IFC_RETURN(pArgs->put_Handled(TRUE));
                }
            }
        }
        break;
    }

    return S_OK;
}

// Attempts to focus the HeaderButton, and if that fails then attempts to focus the HubSection itself.
//
// semanticZoomMode param means that if this HubSection fails to take focus, we call
// back to the Hub and tell it to try to focus the next HubSection so that focus does transfer
// from the SeZo to the Hub.
_Check_return_ HRESULT
HubSection::TakeFocus(
    _In_ xaml::FocusState focusState,
    _In_ BOOLEAN semanticZoomMode)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<Hub> spParentHub;
    CDependencyObject* pLastFocusableElement = nullptr;

    IFC(GetParentHub(&spParentHub));
    ASSERT(spParentHub);
    if (spParentHub)
    {
        BOOLEAN focusLastElement = FALSE;
        BOOLEAN wasFocused = FALSE;

        if (semanticZoomMode)
        {
            // For the case where we are semantically zooming into a HubSection and we cannot focus it
            // or any sections after it, we try to focus the last focusable element of the previous HubSection
            // and keep working backwards through the remaining HubSections.
            IFC(spParentHub->SectionComesBeforeDestinationForSemanticZoom(this, &focusLastElement));
        }

        if (focusLastElement)
        {
            IFC(CoreImports::FocusManager_GetLastFocusableElement(GetHandle(), &pLastFocusableElement));
            if (pLastFocusableElement)
            {
                ctl::ComPtr<DependencyObject> spLastFocusableElement;

                IFC(DXamlCore::GetCurrent()->TryGetPeer(pLastFocusableElement, &spLastFocusableElement));
                if (spLastFocusableElement)
                {
                    IFC(DependencyObject::SetFocusedElement(spLastFocusableElement.Get(), focusState, FALSE /*animateIfBringIntoView*/, &wasFocused));
                }
            }

            if (!wasFocused)
            {
                IFC(FocusHeaderButton(focusState, &wasFocused));
            }
        }
        else
        {
            IFC(FocusHeaderButton(focusState, &wasFocused));
            if (!wasFocused)
            {
                IFC(Focus(focusState, &wasFocused));
            }
        }

        if (semanticZoomMode)
        {
            if (wasFocused)
            {
                IFC(spParentHub->DelayScrollToSeZoDestination());
            }
            else
            {
                // If we failed to focus the destination HubSection after completing a view change, then tell the Hub
                // to try to transfer it to a nearby HubSection rather than letting it remain in the ZoomedOutView.
                IFC(spParentHub->TransferSemanticZoomFocus(this));
            }
        }
    }

Cleanup:
    ReleaseInterface(pLastFocusableElement);
    RRETURN(hr);
}

// Calls TakeFocus() with the provided arguments if the HubSection is already loaded,
// otherwise assumes the HubSection is expected to be loaded and caches the arguments
// and then calls TakeFocus() on the HubSection once it is loaded.
//
// semanticZoomMode param means that if this HubSection fails to take focus, we call
// back to the Hub and tell it to try to focus the next HubSection so that focus does transfer
// from the SeZo to the Hub.
_Check_return_ HRESULT
HubSection::TakeFocusOnLoaded(
    _In_ xaml::FocusState focusState,
    _In_ BOOLEAN semanticZoomMode)
{
    HRESULT hr = S_OK;

    ASSERT(focusState != xaml::FocusState_Unfocused);

    if (m_isLoaded)
    {
        IFC(TakeFocus(focusState, semanticZoomMode));
    }
    else
    {
        m_takeFocusWhenLoaded = TRUE;
        m_focusStateToUseWhenLoaded = focusState;
        m_transferFocusForSemanticZoomMode = semanticZoomMode;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
HubSection::FocusHeaderButton(
    _In_ xaml::FocusState focusState,
    _Out_ BOOLEAN* pWasFocused)
{
    HRESULT hr = S_OK;
    BOOLEAN wasFocused = FALSE;

    if (m_tpHeaderButtonPart)
    {
        ctl::ComPtr<IInspectable> spHeader;
        ctl::ComPtr<IDataTemplate> spHeaderTemplate;

        IFC(get_Header(&spHeader));
        IFC(get_HeaderTemplate(&spHeaderTemplate));

        if (spHeader || spHeaderTemplate)
        {
            IFC(m_tpHeaderButtonPart.Cast<Button>()->Focus(focusState, &wasFocused));
        }
    }

    *pWasFocused = wasFocused;

Cleanup:
    RRETURN(hr);
}
