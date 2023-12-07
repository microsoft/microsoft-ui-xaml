// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "HubSection.g.h"
#include "HubSectionAutomationPeer.g.h"
#include "Hub.g.h"
#include "Button.g.h"
#include "localizedResource.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Work around disruptive max/min macros
#undef max
#undef min

HubSection::HubSection()
    : m_isLoaded(FALSE)
    , m_takeFocusWhenLoaded(FALSE)
    , m_transferFocusForSemanticZoomMode(FALSE)
    , m_focusStateToUseWhenLoaded(xaml::FocusState::FocusState_Unfocused)
{
}

_Check_return_ HRESULT
    HubSection::Initialize()
{
    HRESULT hr = S_OK;

    IFC(HubSectionGenerated::Initialize());

    IFC(m_epLoadedHandler.AttachEventHandler(this,
        [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
    {
        RRETURN(OnLoaded());
    }));

    IFC(m_epUnloadedHandler.AttachEventHandler(this,
        [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
    {
        m_isLoaded = FALSE;
        RRETURN(S_OK);
    }));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
HubSection::OnLoaded()
{
    HRESULT hr = S_OK;

    m_isLoaded = TRUE;

    if (m_takeFocusWhenLoaded)
    {
        IFC(TakeFocus(m_focusStateToUseWhenLoaded, m_transferFocusForSemanticZoomMode));
    }

Cleanup:
    m_takeFocusWhenLoaded = FALSE;
    m_transferFocusForSemanticZoomMode = FALSE;
    m_focusStateToUseWhenLoaded = xaml::FocusState::FocusState_Unfocused;

    RRETURN(hr);
}

IFACEMETHODIMP HubSection::OnApplyTemplate()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IFrameworkElement> spHubHeaderPlaceHolderPart;
    ctl::ComPtr<IButton> spHeaderButtonPart;
    ctl::ComPtr<IButton> spHeaderSeeMoreButtonPart;
    ctl::ComPtr<IContentPresenter> spContentPresenter;
    ctl::ComPtr<IFrameworkElement> spHeaderPart;

    IFC(DetachHandler(m_epHeaderButtonClickHandler, m_tpHeaderButtonPart));
    IFC(DetachHandler(m_epHeaderButtonKeyDownHandler, m_tpHeaderButtonPart));
    IFC(DetachHandler(m_epHeaderButtonLoadedHandler, m_tpHeaderButtonPart));
    IFC(DetachHandler(m_epHeaderSeeMoreButtonClickHandler, m_tpHeaderSeeMoreButtonPart));

    m_tpHubHeaderPlaceHolderPart.Clear();
    m_tpHeaderButtonPart.Clear();
    m_tpHeaderSeeMoreButtonPart.Clear();
    m_tpContentPresenterPart.Clear();

    IFC(HubSectionGenerated::OnApplyTemplate());

    IFC(GetTemplatePart<IFrameworkElement>(STR_LEN_PAIR(L"HubHeaderPlaceholder"), spHubHeaderPlaceHolderPart.ReleaseAndGetAddressOf()));
    SetPtrValueWithQIOrNull(m_tpHubHeaderPlaceHolderPart, spHubHeaderPlaceHolderPart.Get());
    IFC(RefreshHubHeaderPlaceholderHeight());

    IFC(GetTemplatePart<IButton>(STR_LEN_PAIR(L"HeaderButton"), spHeaderButtonPart.ReleaseAndGetAddressOf()));
    SetPtrValueWithQIOrNull(m_tpHeaderButtonPart, spHeaderButtonPart.Get());
    if (m_tpHeaderButtonPart)
    {
        IFC(m_epHeaderButtonClickHandler.AttachEventHandler(m_tpHeaderButtonPart.Cast<Button>(),
            [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
        {
            RRETURN(OnHeaderButtonClick());
        }));

        IFC(m_epHeaderButtonKeyDownHandler.AttachEventHandler(m_tpHeaderButtonPart.Cast<Button>(),
            [this](IInspectable* pSender, IKeyRoutedEventArgs* pArgs)
        {
            RRETURN(OnHeaderButtonKeyDown(pArgs));
        }));

        IFC(m_epHeaderButtonLoadedHandler.AttachEventHandler(m_tpHeaderButtonPart.Cast<Button>(),
            [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
        {
            RRETURN(UpdateHeaderButtonStyle());
        }));

    }

    IFC(GetTemplatePart<IButton>(STR_LEN_PAIR(L"SeeMoreButton"), spHeaderSeeMoreButtonPart.ReleaseAndGetAddressOf()));
    SetPtrValueWithQIOrNull(m_tpHeaderSeeMoreButtonPart, spHeaderSeeMoreButtonPart.Get());
    if (m_tpHeaderSeeMoreButtonPart)
    {
        wrl_wrappers::HString strSeeMoreButtonContentText;
        ctl::ComPtr<IInspectable> spTextAsInspectable;

        // Update the SeeMore button content with the localized text string
        IFC(DXamlCore::GetCurrent()->GetLocalizedResourceString(TEXT_HUB_SEE_MORE, strSeeMoreButtonContentText.ReleaseAndGetAddressOf()));
        IFC(PropertyValue::CreateFromString(strSeeMoreButtonContentText.Get(), &spTextAsInspectable));
        IFC(m_tpHeaderSeeMoreButtonPart.Cast<Button>()->put_Content(spTextAsInspectable.Get()));

        IFC(m_epHeaderSeeMoreButtonClickHandler.AttachEventHandler(m_tpHeaderSeeMoreButtonPart.Cast<Button>(),
            [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
        {
            RRETURN(OnHeaderSeeMoreButtonClick());
        }));
    }

    IFC(GetTemplatePart<IContentPresenter>(STR_LEN_PAIR(L"ContentPresenter"), spContentPresenter.ReleaseAndGetAddressOf()));
    SetPtrValueWithQIOrNull(m_tpContentPresenterPart, spContentPresenter.Get());

    IFC(UpdateHeaderPresenterVisibility());

Cleanup:
    RRETURN(hr);
}

// Fetches the Hub.Header height from the parent hub and sets this value on m_tpHubHeaderPlaceHolderPart.
_Check_return_ HRESULT HubSection::RefreshHubHeaderPlaceholderHeight()
{
    HRESULT hr = S_OK;

    if (m_tpHubHeaderPlaceHolderPart)
    {
        ctl::ComPtr<Hub> spParentHub;
        DOUBLE placeholderHeight = 0;

        IFC(GetParentHub(&spParentHub));
        if (spParentHub)
        {
            xaml_controls::Orientation orientation = xaml_controls::Orientation_Vertical;

            IFC(spParentHub->get_Orientation(&orientation));

            if (orientation == xaml_controls::Orientation_Horizontal)
            {
                placeholderHeight = spParentHub->GetHubHeaderHeight();
            }
        }

        IFC(m_tpHubHeaderPlaceHolderPart->put_Height(placeholderHeight));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT HubSection::SetParentHub(_In_opt_ Hub* pHub)
{
    HRESULT hr = S_OK;

    IFC(ctl::AsWeak(ctl::as_iinspectable(pHub), &m_wrParentHub));

    // Since the Hub may have changed, recalculate the space to reserve for Hub.Header height.
    // Currently, this can be hit when a section is Prepared, Cleared, and then re-Prepared.
    IFC(RefreshHubHeaderPlaceholderHeight());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT HubSection::GetParentHub(_Outptr_result_maybenull_ Hub** ppHub)
{

    HRESULT hr = S_OK;
    ctl::ComPtr<IHub> spParentHubAsI;

    IFCPTR(ppHub);
    *ppHub = nullptr;

    IFC(m_wrParentHub.As(&spParentHubAsI));
    *ppHub = static_cast<Hub*>(spParentHubAsI.Detach());

Cleanup:
    RRETURN(hr);
}

// Handle the custom property changed event and call the OnPropertyChanged2 methods.
_Check_return_ HRESULT
HubSection::OnPropertyChanged2(
    _In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;

    IFC(HubSectionGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::HubSection_IsHeaderInteractive:
        // Make sure ArrangeOverride() runs to take us to the correct IsHeaderInteractive visual state.
        IFC(InvalidateArrange());
        break;

    case KnownPropertyIndex::HubSection_Header:
        IFC(OnHeaderChanged());
        break;

    case KnownPropertyIndex::HubSection_HeaderTemplate:
        IFC(UpdateHeaderPresenterVisibility());
        break;
    }

Cleanup:
    RRETURN(hr);
}

// Returns the HubSection's ContentPresenter template part.
_Check_return_ HRESULT HubSection::GetContentPresenterPart(_Outptr_result_maybenull_ xaml_controls::IContentPresenter** ppContentPresenter)
{
    HRESULT hr = S_OK;

    IFCPTR(ppContentPresenter);
    *ppContentPresenter = NULL;

    if (m_tpContentPresenterPart)
    {
        IFC(m_tpContentPresenterPart.CopyTo(ppContentPresenter));
    }

Cleanup:
    RRETURN(hr);
}

// Returns the HubSections's HeaderButton template part.
_Check_return_ HRESULT HubSection::GetHeaderButtonPart(_Outptr_result_maybenull_ xaml_controls::IButton** ppHeaderButton)
{
    HRESULT hr = S_OK;

    IFCPTR(ppHeaderButton);
    *ppHeaderButton = NULL;

    if (m_tpHeaderButtonPart)
    {
        IFC(m_tpHeaderButtonPart.CopyTo(ppHeaderButton));
    }

Cleanup:
    RRETURN(hr);
}

// Returns the HubSections's SeeMoreButton template part.
_Check_return_ HRESULT HubSection::GetSeeMoreButtonPart(_Outptr_result_maybenull_ xaml_controls::IButton** ppSeeMoreButton)
{
    *ppSeeMoreButton = NULL;

    if (m_tpHeaderSeeMoreButtonPart)
    {
        IFC_RETURN(m_tpHeaderSeeMoreButtonPart.CopyTo(ppSeeMoreButton));
    }

    return S_OK;
}

// Create HubSectionAutomationPeer to represent the HubSection.
IFACEMETHODIMP HubSection::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IHubSectionAutomationPeer> spHubSectionAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IHubSectionAutomationPeerFactory> spHubSectionAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;

    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::HubSectionAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As(&spHubSectionAPFactory));

    IFC(spHubSectionAPFactory.Cast<HubSectionAutomationPeerFactory>()->CreateInstanceWithOwner(this,
        NULL,
        &spInner,
        &spHubSectionAutomationPeer));
    IFC(spHubSectionAutomationPeer.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}

// Callback for when the HubSection.Header property changes.
_Check_return_ HRESULT
HubSection::OnHeaderChanged()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<Hub> spParentHub;

    IFC(GetParentHub(&spParentHub));
    if (spParentHub)
    {
        IFC(spParentHub->OnSectionHeaderChanged(this));
    }

    IFC(UpdateHeaderPresenterVisibility());

Cleanup:
    RRETURN(hr);
}

// Evaluates whether or not to collapse the header area based on the Header and HeaderTemplate properties.
_Check_return_ HRESULT
HubSection::UpdateHeaderPresenterVisibility()
{
    HRESULT hr = S_OK;

    if (m_tpHeaderButtonPart.Get())
    {
        ctl::ComPtr<xaml::IDataTemplate> spHeaderTemplateValue;
        ctl::ComPtr<IInspectable> spHeaderValue;
        xaml::Visibility visibility = xaml::Visibility_Collapsed;

        IFC(get_HeaderTemplate(&spHeaderTemplateValue));
        IFC(get_Header(&spHeaderValue));

        visibility = (spHeaderTemplateValue || spHeaderValue) ?
            xaml::Visibility_Visible : xaml::Visibility_Collapsed;

        IFC(m_tpHeaderButtonPart.Cast<Button>()->put_Visibility(visibility));

        if (visibility == xaml::Visibility_Visible)
        {
            // Make sure ArrangeOverride() runs to take us to the correct IsHeaderInteractive visual state.
            IFC(InvalidateArrange());
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
HubSection::OnHeaderSeeMoreButtonClick()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<Hub> spParentHub;

    IFC(GetParentHub(&spParentHub));

    if (spParentHub)
    {
        BOOLEAN isHeaderInteractive = FALSE;

        IFC(get_IsHeaderInteractive(&isHeaderInteractive));

        if (isHeaderInteractive)
        {
            IFC(spParentHub->RaiseSectionHeaderClick(this));
        }
    }

Cleanup:
    RRETURN(hr);
}
