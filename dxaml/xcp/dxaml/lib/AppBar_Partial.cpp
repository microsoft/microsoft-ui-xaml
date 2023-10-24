// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "AppBar.g.h"
#include "AppBarAutomationPeer.g.h"
#include "AppBarTemplateSettings.g.h"
#include "Button.g.h"
#include "CompositeTransform.g.h"
#include "FullWindowMediaRoot.g.h"
#include "Grid.g.h"
#include "KeyRoutedEventArgs.g.h"
#include "Page.g.h"
#include "Popup.g.h"
#include "PopupRoot.g.h"
#include "Rectangle.g.h"
#include "SolidColorBrush.g.h"
#include "Storyboard.g.h"
#include "Window.g.h"
#include <FrameworkUdk/BackButtonIntegration.h>
#include "VisualTreeHelper.h"
#include <LightDismissOverlayHelper.h>
#include "AutomationProperties.h"
#include "localizedResource.h"
#include "ApplicationBarService.g.h"
#include "XamlRoot.g.h"
#include "ElementSoundPlayerService_Partial.h"

using namespace DirectUI;
using namespace std::placeholders;

// Index table as follows: [ClosedDisplayMode][DoesOpenUp][IsOpen]
static const wchar_t* g_displayModeVisualStateTable[3][2][2] =
{
    // Compact
    {
        { L"CompactClosed", L"CompactOpenDown" },
        { L"CompactClosed", L"CompactOpenUp" }
    },

    // Minimal
    {
        { L"MinimalClosed", L"MinimalOpenDown" },
        { L"MinimalClosed", L"MinimalOpenUp" }
    },

    // Hidden
    {
        { L"HiddenClosed", L"HiddenOpenDown" },
        { L"HiddenClosed", L"HiddenOpenUp" }
    },
};

AppBar::AppBar()
    : m_Mode(AppBarMode_Inline)
    , m_onLoadFocusState(xaml::FocusState_Unfocused)
    , m_savedFocusState(xaml::FocusState_Unfocused)
    , m_isInOverlayState(false)
    , m_isChangingOpenedState(false)
    , m_hasUpdatedTemplateSettings(false)
    , m_compactHeight(0.0)
    , m_minimalHeight(0.0)
    , m_openedWithExpandButton(false)
    , m_contentHeight(0.0)
    , m_isOverlayVisible(false)
{
}

AppBar::~AppBar()
{
    auto xamlRoot = XamlRoot::GetForElementStatic(this);
    if (m_xamlRootChangedEventHandler && xamlRoot)
    {
        VERIFYHR(m_xamlRootChangedEventHandler.DetachEventHandler(xamlRoot.Get()));
    }

    if (DXamlCore::GetCurrent() != nullptr)
    {
        VERIFYHR(BackButtonIntegration_UnregisterListener(this));
    }
}

// Initialize the AppBar by setting up its OnLoaded handler.
_Check_return_ HRESULT
AppBar::PrepareState()
{
    IFC_RETURN(__super::PrepareState());

    IFC_RETURN(m_loadedEventHandler.AttachEventHandler(this, std::bind(&AppBar::OnLoaded, this, _1, _2)));
    IFC_RETURN(m_unloadedEventHandler.AttachEventHandler(this, std::bind(&AppBar::OnUnloaded, this, _1, _2)));
    IFC_RETURN(m_sizeChangedEventHandler.AttachEventHandler(this, std::bind(&AppBar::OnSizeChanged, this, _1, _2)));

    ctl::ComPtr<AppBarTemplateSettings> templateSettings;
    IFC_RETURN(ctl::make(&templateSettings));
    IFC_RETURN(put_TemplateSettings(templateSettings.Get()));

    return S_OK;
}

// Note that we need to wait for OnLoaded event to set focus.
// When we get the on opened event children of AppBar will not be populated
// yet which will prevent them from getting focus.
_Check_return_ HRESULT
AppBar::OnLoaded(_In_ IInspectable* pSender, _In_ xaml::IRoutedEventArgs* pArgs)
{
    if (!m_layoutUpdatedEventHandler)
    {
        IFC_RETURN(m_layoutUpdatedEventHandler.AttachEventHandler(this, std::bind(&AppBar::OnLayoutUpdated, this, _1, _2)));
    }

    //register for XamlRoot.Changed events
    auto xamlRoot = XamlRoot::GetForElementStatic(this);
    if (!m_xamlRootChangedEventHandler && xamlRoot)
    {
        IFC_RETURN(m_xamlRootChangedEventHandler.AttachEventHandler(xamlRoot.Get(), std::bind(&AppBar::OnXamlRootChanged, this, _1, _2)));
    }

    // register the app bar if it is floating
    if (m_Mode == AppBarMode_Floating)
    {
        ctl::ComPtr<IApplicationBarService> applicationBarService;
        if (const auto xamlRootImpl = xamlRoot.AsOrNull<XamlRoot>())
        {
            IFC_RETURN(xamlRootImpl->GetApplicationBarService(applicationBarService));
        }
        ASSERT(applicationBarService);
        IFC_RETURN(applicationBarService->RegisterApplicationBar(this, m_Mode));
    }

    // If it's a top or bottom bar, make sure the bounds are correct if we haven't set them yet
    if (m_Mode == AppBarMode_Top || m_Mode == AppBarMode_Bottom)
    {
        ctl::ComPtr<IApplicationBarService> applicationBarService;
        if (const auto xamlRootImpl = xamlRoot.AsOrNull<XamlRoot>())
        {
            IFC_RETURN(xamlRootImpl->GetApplicationBarService(applicationBarService));
        }
        ASSERT(applicationBarService);
        IFC_RETURN(applicationBarService->OnBoundsChanged());
    }

    // OnIsOpenChanged handles focus and other changes
    BOOLEAN isOpen = FALSE;
    IFC_RETURN(get_IsOpen(&isOpen));
    if (isOpen)
    {
        IFC_RETURN(OnIsOpenChanged(true));
    }

    // Update the visual state to make sure our calculations for what
    // direction to open in are correct.
    IFC_RETURN(UpdateVisualState());

    return S_OK;
}


_Check_return_ HRESULT
AppBar::OnUnloaded(_In_ IInspectable* /*pSender*/, _In_ xaml::IRoutedEventArgs* /*pArgs*/)
{
    if (m_layoutUpdatedEventHandler)
    {
        IFC_RETURN(m_layoutUpdatedEventHandler.DetachEventHandler(ctl::iinspectable_cast(this)));
    }

    if (m_isInOverlayState)
    {
        IFC_RETURN(TeardownOverlayState());
    }

    if (m_Mode == AppBarMode_Floating)
    {
        if (const auto xamlRoot = XamlRoot::GetImplementationForElementStatic(this))
        {
            ctl::ComPtr<IApplicationBarService> applicationBarService;
            IFC_RETURN(xamlRoot->GetApplicationBarService(applicationBarService));
            IFC_RETURN(applicationBarService->UnregisterApplicationBar(this));
        }
    }

    // Make sure we're not still registered for back button events when no longer
    // in the tree.
    IFC_RETURN(BackButtonIntegration_UnregisterListener(this));

    return S_OK;
}

_Check_return_ HRESULT
AppBar::OnLayoutUpdated(_In_ IInspectable* /*sender*/, _In_ IInspectable* /*args*/)
{
    if (m_layoutTransitionElement)
    {
        IFC_RETURN(PositionLTEs());
    }

    return S_OK;
}

_Check_return_ HRESULT
AppBar::OnSizeChanged(_In_ IInspectable* /*sender*/, _In_ xaml::ISizeChangedEventArgs* /*args*/)
{
    IFC_RETURN(RefreshContentHeight(nullptr /*didChange*/));
    IFC_RETURN(UpdateTemplateSettings());

    ctl::ComPtr<Page> pageOwner;
    if (SUCCEEDED(GetOwner(&pageOwner)) && pageOwner)
    {
        IFC_RETURN(pageOwner->AppBarClosedSizeChanged());
    }

    return S_OK;
}

_Check_return_ HRESULT
AppBar::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    IFC_RETURN(__super::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::AppBar_IsOpen:
        {
            bool isOpen = !!args.m_pNewValue->AsBool();
            IFC_RETURN(OnIsOpenChanged(isOpen));

            if (EventEnabledAppBarOpenBegin() && isOpen)
            {
                TraceAppBarOpenBegin(static_cast<unsigned int>(m_Mode));
            }
            if (EventEnabledAppBarClosedBegin() && !isOpen)
            {
                TraceAppBarClosedBegin(static_cast<unsigned int>(m_Mode));
            }

            IFC_RETURN(OnIsOpenChangedForAutomation(args));

            if (EventEnabledAppBarOpenEnd() && isOpen)
            {
                TraceAppBarOpenEnd();
            }
            if (EventEnabledAppBarClosedEnd() && !isOpen)
            {
                TraceAppBarClosedEnd();
            }

            IFC_RETURN(UpdateVisualState());
            break;
        }

        case KnownPropertyIndex::AppBar_IsSticky:
        {
            IFC_RETURN(OnIsStickyChanged());
            break;
        }

        case KnownPropertyIndex::AppBar_ClosedDisplayMode:
        {
            if (m_Mode != AppBarMode_Inline)
            {
                ctl::ComPtr<IApplicationBarService> applicationBarService;
                if (const auto xamlRoot = XamlRoot::GetImplementationForElementStatic(this))
                {
                    IFC_RETURN(xamlRoot->GetApplicationBarService(applicationBarService));
                    IFC_RETURN(applicationBarService->HandleApplicationBarClosedDisplayModeChange(this, m_Mode));
                }
            }

            IFC_RETURN(InvalidateMeasure());
            IFC_RETURN(UpdateVisualState());
            break;
        }

        case KnownPropertyIndex::AppBar_LightDismissOverlayMode:
        {
            IFC_RETURN(ReevaluateIsOverlayVisible());
            break;
        }

        case KnownPropertyIndex::Control_IsEnabled:
        {
            IFC_RETURN(UpdateVisualState());
            break;
        }

        default:
            break;
    }

    return S_OK;
}

_Check_return_ HRESULT
AppBar::OnVisibilityChanged()
{
    ctl::ComPtr<Page> pageOwner;
    if (SUCCEEDED(GetOwner(&pageOwner)) && pageOwner)
    {
        IFC_RETURN(pageOwner->AppBarClosedSizeChanged());
    }

    return S_OK;
}

IFACEMETHODIMP
AppBar::OnApplyTemplate()
{
    if (m_tpContentRoot)
    {
        IFC_RETURN(DetachHandler(m_contentRootSizeChangedEventHandler, m_tpContentRoot));
    }

    if (m_tpExpandButton)
    {
        IFC_RETURN(DetachHandler(m_expandButtonClickEventHandler, m_tpExpandButton));
    }

    if (m_tpDisplayModesStateGroup)
    {
        IFC_RETURN(DetachHandler(m_displayModeStateChangedEventHandler, m_tpDisplayModesStateGroup));
    }

    // Clear our previous template parts.
    m_tpLayoutRoot.Clear();
    m_tpContentRoot.Clear();
    m_tpExpandButton.Clear();
    m_tpDisplayModesStateGroup.Clear();

    IFC_RETURN(__super::OnApplyTemplate());

    ctl::ComPtr<xaml_controls::IGrid> layoutRoot;
    IFC_RETURN(GetTemplatePart<xaml_controls::IGrid>(STR_LEN_PAIR(L"LayoutRoot"), layoutRoot.ReleaseAndGetAddressOf()));
    SetPtrValue(m_tpLayoutRoot, layoutRoot.Get());

    ctl::ComPtr<xaml::IFrameworkElement> contentRoot;
    IFC_RETURN(GetTemplatePart<xaml::IFrameworkElement>(STR_LEN_PAIR(L"ContentRoot"), contentRoot.ReleaseAndGetAddressOf()));
    SetPtrValue(m_tpContentRoot, contentRoot.Get());

    if (m_tpContentRoot)
    {
        IFC_RETURN(m_contentRootSizeChangedEventHandler.AttachEventHandler(m_tpContentRoot.Get(), std::bind(&AppBar::OnContentRootSizeChanged, this, _1, _2)));
    }

    ctl::ComPtr<xaml_primitives::IButtonBase> expandButton;
    IFC_RETURN(GetTemplatePart<xaml_primitives::IButtonBase>(STR_LEN_PAIR(L"ExpandButton"), expandButton.ReleaseAndGetAddressOf()));
    if (!expandButton)
    {
        // The previous CommandBar template used "MoreButton" for this template part's name,
        // so now we're stuck with it, as much as I'd like to converge them..
        IFC_RETURN(GetTemplatePart<xaml_primitives::IButtonBase>(STR_LEN_PAIR(L"MoreButton"), expandButton.ReleaseAndGetAddressOf()));
    }
    SetPtrValue(m_tpExpandButton, expandButton.Get());

    if (m_tpExpandButton)
    {
        IFC_RETURN(m_expandButtonClickEventHandler.AttachEventHandler(m_tpExpandButton.Get(), std::bind(&AppBar::OnExpandButtonClick, this, _1, _2)));

        // Set a tooltip on the expand button.
        wrl_wrappers::HString toolTipText;
        IFC_RETURN(DXamlCore::GetCurrent()->GetLocalizedResourceString(TEXT_HUB_SEE_MORE, toolTipText.ReleaseAndGetAddressOf()));

        ctl::ComPtr<IInspectable> boxedToolTipText;
        IFC_RETURN(PropertyValue::CreateFromString(toolTipText.Get(), &boxedToolTipText));

        IFC_RETURN(m_tpExpandButton.Cast<ButtonBase>()->SetValue(
            MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ToolTipService_ToolTip),
            boxedToolTipText.Get())
            );

        // Provide default localized accessibility name for expand button for appbar and commandbars.
        wrl_wrappers::HString automationName;
        IFC_RETURN(AutomationProperties::GetNameStatic(m_tpExpandButton.Cast<Button>(), automationName.ReleaseAndGetAddressOf()));
        if(automationName.Get() == nullptr)
        {
            IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_MORE_BUTTON, automationName.ReleaseAndGetAddressOf()));
            IFC_RETURN(AutomationProperties::SetNameStatic(m_tpExpandButton.Cast<Button>(), automationName.Get()));
        }
    }

    // Query compact & minimal height from resource dictionary.
    {
        ctl::ComPtr<xaml::IResourceDictionary> resources;
        ctl::ComPtr<wfc::IMap<IInspectable*, IInspectable*>> resourcesMap;
        ctl::ComPtr<IInspectable> boxedResourceKey;
        ctl::ComPtr<IInspectable> boxedResource;
        BOOLEAN doesResourceExist = FALSE;

        IFC_RETURN(get_Resources(&resources));
        IFC_RETURN(resources.As(&resourcesMap));

        IFC_RETURN(PropertyValue::CreateFromString(wrl_wrappers::HStringReference(L"AppBarThemeCompactHeight").Get(), &boxedResourceKey));
        IFC_RETURN(resourcesMap->HasKey(boxedResourceKey.Get(), &doesResourceExist));

        if (doesResourceExist)
        {
            IFC_RETURN(resourcesMap->Lookup(boxedResourceKey.Get(), &boxedResource));
            IFCPTR_RETURN(boxedResource);

            auto doubleReference = ctl::query_interface_cast<wf::IReference<double>>(boxedResource.Get());
            IFC_RETURN(doubleReference->get_Value(&m_compactHeight));
        }

        IFC_RETURN(PropertyValue::CreateFromString(wrl_wrappers::HStringReference(L"AppBarThemeMinimalHeight").Get(), &boxedResourceKey));
        IFC_RETURN(resourcesMap->HasKey(boxedResourceKey.Get(), &doesResourceExist));

        if (doesResourceExist)
        {
            IFC_RETURN(resourcesMap->Lookup(boxedResourceKey.Get(), &boxedResource));
            IFCPTR_RETURN(boxedResource);

            auto doubleReference = ctl::query_interface_cast<wf::IReference<double>>(boxedResource.Get());
            IFC_RETURN(doubleReference->get_Value(&m_minimalHeight));
        }
    }

    // Lookup the animations to use for the window overlay.
    if (m_tpLayoutRoot)
    {
        ctl::ComPtr<xaml::IResourceDictionary> resources;
        IFC_RETURN(m_tpLayoutRoot.Cast<Grid>()->get_Resources(&resources));

        ctl::ComPtr<wfc::IMap<IInspectable*, IInspectable*>> resourcesMap;
        IFC_RETURN(resources.As(&resourcesMap));

        ctl::ComPtr<IInspectable> boxedResourceKey;
        IFC_RETURN(PropertyValue::CreateFromString(wrl_wrappers::HStringReference(L"OverlayOpeningAnimation").Get(), boxedResourceKey.ReleaseAndGetAddressOf()));

        BOOLEAN hasKey = FALSE;
        IFC_RETURN(resourcesMap->HasKey(boxedResourceKey.Get(), &hasKey));
        if (hasKey)
        {
            ctl::ComPtr<IInspectable> boxedResource;
            IFC_RETURN(resourcesMap->Lookup(boxedResourceKey.Get(), &boxedResource));

            ctl::ComPtr<xaml_animation::IStoryboard> windowOverlayOpeningStoryboard;
            IFC_RETURN(boxedResource.As(&windowOverlayOpeningStoryboard));

            SetPtrValue(m_overlayOpeningStoryboard, windowOverlayOpeningStoryboard.Get());
        }

        IFC_RETURN(PropertyValue::CreateFromString(wrl_wrappers::HStringReference(L"OverlayClosingAnimation").Get(), boxedResourceKey.ReleaseAndGetAddressOf()));
        IFC_RETURN(resourcesMap->HasKey(boxedResourceKey.Get(), &hasKey));
        if (hasKey)
        {
            ctl::ComPtr<IInspectable> boxedResource;
            IFC_RETURN(resourcesMap->Lookup(boxedResourceKey.Get(), &boxedResource));

            ctl::ComPtr<xaml_animation::IStoryboard> windowOverlayOpeningStoryboard;
            IFC_RETURN(boxedResource.As(&windowOverlayOpeningStoryboard));

            SetPtrValue(m_overlayClosingStoryboard, windowOverlayOpeningStoryboard.Get());
        }
    }

    IFC_RETURN(ReevaluateIsOverlayVisible());

    return S_OK;
}

IFACEMETHODIMP
AppBar::MeasureOverride(_In_ wf::Size availableSize, _Out_ wf::Size* returnValue)
{
    IFC_RETURN(__super::MeasureOverride(availableSize, returnValue));

    if (m_Mode == AppBarMode_Top || m_Mode == AppBarMode_Bottom)
    {
        // regardless of what we desire, settings of alignment or fixed size content, we will always take up full width
        returnValue->Width = availableSize.Width;
    }

    // Make sure our returned height matches the configured state.
    auto closedDisplayMode = xaml_controls::AppBarClosedDisplayMode_Hidden;
    IFC_RETURN(get_ClosedDisplayMode(&closedDisplayMode));

    switch (closedDisplayMode)
    {
        case xaml_controls::AppBarClosedDisplayMode_Compact:
            returnValue->Height = static_cast<float>(m_compactHeight);
            break;

        case xaml_controls::AppBarClosedDisplayMode_Minimal:
            returnValue->Height = static_cast<float>(m_minimalHeight);
            break;

        default:
        case xaml_controls::AppBarClosedDisplayMode_Hidden:
            returnValue->Height = 0.f;
            break;
    }

    return S_OK;
}

IFACEMETHODIMP
AppBar::ArrangeOverride(_In_ wf::Size arrangeSize, _Out_ wf::Size* returnValue)
{
    // Make sure our returned height matches the configured state.
    wf::Size layoutRootDesiredSize = {};
    if (m_tpLayoutRoot)
    {
        IFC_RETURN(m_tpLayoutRoot.Cast<Grid>()->get_DesiredSize(&layoutRootDesiredSize));
    }
    else
    {
        layoutRootDesiredSize = arrangeSize;
    }

    IFC_RETURN(__super::ArrangeOverride({ arrangeSize.Width, layoutRootDesiredSize.Height }, returnValue));

    returnValue->Height = arrangeSize.Height;

    return S_OK;
}

_Check_return_ HRESULT
AppBar::OnOpeningImpl(_In_ IInspectable* pArgs)
{
    IFC_RETURN(TryQueryDisplayModesStatesGroup());

    if (m_Mode == AppBarMode_Inline)
    {
        // If we're in a popup that is light-dismissable, then we don't want to set up
        // a light-dismiss layer - the popup will have its own light-dismiss layer,
        // and it can interfere with ours.
        auto popupAncestor = CPopup::GetClosestPopupAncestor(GetHandle());
        if (!popupAncestor || !popupAncestor->IsSelfOrAncestorLightDismiss())
        {
            if (!m_isInOverlayState)
            {
                if (IsInLiveTree())
                {
                    // Setup our LTEs and light-dismiss layer.
                    IFC_RETURN(SetupOverlayState());

                    if (m_isOverlayVisible)
                    {
                        IFC_RETURN(PlayOverlayOpeningAnimation());
                    }
                }
            }
        }

        BOOLEAN isSticky = FALSE;
        IFC_RETURN(get_IsSticky(&isSticky));
        if (!isSticky)
        {
            IFC_RETURN(SetFocusOnAppBar());
        }
    }
    else
    {
        // Pre-Threshold AppBars were hidden and would get added to the tree upon opening which
        // would invoke their loaded handlers to set focus.
        // In threshold, hidden appbars are always in the tree, so we have to simulate the same
        // behavior by focusing the appbar whenever it opens.
        auto closedDisplayMode = xaml_controls::AppBarClosedDisplayMode_Hidden;
        IFC_RETURN(get_ClosedDisplayMode(&closedDisplayMode));
        if (closedDisplayMode == xaml_controls::AppBarClosedDisplayMode_Hidden)
        {
            ctl::ComPtr<IApplicationBarService> applicationBarService;
            if (const auto xamlRoot = XamlRoot::GetImplementationForElementStatic(this))
            {
                IFC_RETURN(xamlRoot->GetApplicationBarService(applicationBarService));
            }
            ASSERT(applicationBarService);

            // Determine the focus state
            auto focusState = (m_onLoadFocusState != xaml::FocusState_Unfocused ? m_onLoadFocusState : xaml::FocusState_Programmatic);
            IFC_RETURN(applicationBarService->FocusApplicationBar(this, focusState));

            // Reset the saved focus state
            m_onLoadFocusState = xaml::FocusState_Unfocused;
        }
    }

    if (m_tpExpandButton)
    {
        // Set a tooltip with "See Less" for the expand button.
        wrl_wrappers::HString toolTipText;
        IFC_RETURN(DXamlCore::GetCurrent()->GetLocalizedResourceString(TEXT_HUB_SEE_LESS, toolTipText.ReleaseAndGetAddressOf()));

        ctl::ComPtr<IInspectable> boxedToolTipText;
        IFC_RETURN(PropertyValue::CreateFromString(toolTipText.Get(), &boxedToolTipText));

        IFC_RETURN(m_tpExpandButton.Cast<ButtonBase>()->SetValue(
            MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ToolTipService_ToolTip),
            boxedToolTipText.Get())
            );

        // Update the localized accessibility name for expand button with the less app bar button.
        wrl_wrappers::HString automationName;

        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_LESS_BUTTON, automationName.GetAddressOf()));
        IFC_RETURN(AutomationProperties::SetNameStatic(m_tpExpandButton.Cast<Button>(), automationName.Get()));
    }

    // Request a play show sound for opened AppBar
    IFC_RETURN(DirectUI::ElementSoundPlayerService::RequestInteractionSoundForElementStatic(xaml::ElementSoundKind_Show, this));

    // Raise the event
    OpeningEventSourceType* eventSource;
    IFC_RETURN(GetOpeningEventSourceNoRef(&eventSource));
    IFC_RETURN(eventSource->Raise(ctl::as_iinspectable(this), pArgs));

    return S_OK;
}

_Check_return_ HRESULT
AppBar::OnOpenedImpl(_In_ IInspectable* pArgs)
{
    // Raise the event
    OpenedEventSourceType* eventSource;
    IFC_RETURN(GetOpenedEventSourceNoRef(&eventSource));
    IFC_RETURN(eventSource->Raise(ctl::as_iinspectable(this), pArgs));

    if (DXamlCore::GetCurrent()->GetHandle()->BackButtonSupported())
    {
        IFC_RETURN(BackButtonIntegration_RegisterListener(this));
    }

    return S_OK;
}

_Check_return_ HRESULT
AppBar::OnClosingImpl(_In_ IInspectable* pArgs)
{
    m_openedWithExpandButton = false;

    if (m_Mode == AppBarMode_Inline)
    {
        // Only restore focus if this AppBar isn't in a flyout - if it is,
        // then focus will be restored when the flyout closes.
        // We'll interfere with that if we restore focus before that time.
        auto popupAncestor = CPopup::GetClosestPopupAncestor(GetHandle());
        if (!popupAncestor || !popupAncestor->IsFlyout())
        {
            IFC_RETURN(RestoreSavedFocus());
        }

        if (m_isOverlayVisible && m_isInOverlayState)
        {
            IFC_RETURN(PlayOverlayClosingAnimation());
        }
    }

    if (m_tpExpandButton)
    {
        // Set a tooltip with "More options" for the expand button.
        wrl_wrappers::HString toolTipText;
        IFC_RETURN(DXamlCore::GetCurrent()->GetLocalizedResourceString(TEXT_HUB_SEE_MORE, toolTipText.ReleaseAndGetAddressOf()));

        ctl::ComPtr<IInspectable> boxedToolTipText;
        IFC_RETURN(PropertyValue::CreateFromString(toolTipText.Get(), &boxedToolTipText));

        IFC_RETURN(m_tpExpandButton.Cast<ButtonBase>()->SetValue(
            MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ToolTipService_ToolTip),
            boxedToolTipText.Get())
            );

        // Update the localized accessibility name for expand button with the more app bar button.
        wrl_wrappers::HString automationName;

        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_MORE_BUTTON, automationName.GetAddressOf()));
        IFC_RETURN(AutomationProperties::SetNameStatic(m_tpExpandButton.Cast<Button>(), automationName.Get()));
    }

    // Request a play hide sound for closed
    IFC_RETURN(DirectUI::ElementSoundPlayerService::RequestInteractionSoundForElementStatic(xaml::ElementSoundKind_Hide, this));

    // Raise the event
    ClosingEventSourceType* eventSource;
    IFC_RETURN(GetClosingEventSourceNoRef(&eventSource));
    IFC_RETURN(eventSource->Raise(ctl::as_iinspectable(this), pArgs));

    return S_OK;
}

_Check_return_ HRESULT
AppBar::OnClosedImpl(_In_ IInspectable* pArgs)
{
    if (m_Mode == AppBarMode_Inline && m_isInOverlayState)
    {
        IFC_RETURN(TeardownOverlayState());
    }

    // Raise the event
    ClosedEventSourceType* eventSource;
    IFC_RETURN(GetClosedEventSourceNoRef(&eventSource));
    IFC_RETURN(eventSource->Raise(ctl::as_iinspectable(this), pArgs));

    IFC_RETURN(BackButtonIntegration_UnregisterListener(this));

    return S_OK;
}

_Check_return_ HRESULT
AppBar::ProcessTabStopOverride(
    _In_opt_ DependencyObject* pFocusedElement,
    _In_opt_ DependencyObject* pCandidateTabStopElement,
    const bool isBackward,
    const bool /*didCycleFocusAtRootVisualScope*/,
    _Outptr_ DependencyObject** ppNewTabStop,
    _Out_ BOOLEAN* pIsTabStopOverridden
    )
{
    *ppNewTabStop = nullptr;
    *pIsTabStopOverridden = FALSE;

    // The ApplicationBarService manages focus for non-inline appbars.
    if (m_Mode == AppBarMode_Inline)
    {
        BOOLEAN isOpen = FALSE;
        IFC_RETURN(get_IsOpen(&isOpen));

        BOOLEAN isSticky = FALSE;
        IFC_RETURN(get_IsSticky(&isSticky));

        // We don't override tab-stop behavior for closed or sticky appbars.
        if (!isOpen || isSticky)
        {
            return S_OK;
        }

        BOOLEAN isAncestorOfFocusedElement = FALSE;
        IFC_RETURN(IsAncestorOf(pFocusedElement, &isAncestorOfFocusedElement));

        BOOLEAN isAncestorOfCandidateElement = FALSE;
        IFC_RETURN(IsAncestorOf(pCandidateTabStopElement, &isAncestorOfCandidateElement));

        // If the element losing focus is a child of the appbar and the element
        // we're losing focus to is not, then we override tab-stop to keep the
        // focus within the appbar.
        if (isAncestorOfFocusedElement && !isAncestorOfCandidateElement)
        {
            xref_ptr<CDependencyObject> newTabStop;

            IFC_RETURN(isBackward ?
                CoreImports::FocusManager_GetLastFocusableElement(GetHandle(), newTabStop.ReleaseAndGetAddressOf()) :
                CoreImports::FocusManager_GetFirstFocusableElement(GetHandle(), newTabStop.ReleaseAndGetAddressOf())
                );

            // Check to see if we overrode the tab stop candidate.
            if (newTabStop)
            {
                IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(newTabStop, ppNewTabStop));
                *pIsTabStopOverridden = TRUE;
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
AppBar::OnContentRootSizeChanged(_In_ IInspectable* /*sender*/, _In_ xaml::ISizeChangedEventArgs* /*args*/)
{
    bool didChange = false;
    IFC_RETURN(RefreshContentHeight(&didChange));

    if (didChange)
    {
        IFC_RETURN(UpdateTemplateSettings());
    }

    return S_OK;
}

_Check_return_ HRESULT
AppBar::OnXamlRootChanged(_In_ xaml::IXamlRoot* pSender, _In_ xaml::IXamlRootChangedEventArgs* pArgs)
{
    if (m_Mode == AppBarMode_Inline && !m_isChangingOpenedState)
    {
        IFC_RETURN(TryDismissInlineAppBar());
    }

    return S_OK;
}

// floating appbars are managed through vsm. System appbars (as set by page) use
// transitions that are triggered by layout to load, unload and move around.
_Check_return_ HRESULT
AppBar::ChangeVisualState(bool useTransitions)
{
    IFC_RETURN(__super::ChangeVisualState(useTransitions));

    BOOLEAN ignored = FALSE;
    BOOLEAN isEnabled = FALSE;
    BOOLEAN isOpen = FALSE;

    auto closedDisplayMode = xaml_controls::AppBarClosedDisplayMode_Hidden;
    bool shouldOpenUp = false;

    IFC_RETURN(get_IsEnabled(&isEnabled));
    IFC_RETURN(get_IsOpen(&isOpen));
    IFC_RETURN(get_ClosedDisplayMode(&closedDisplayMode));

    // We only need to check this if we're going to an opened state.
    if (isOpen)
    {
        IFC_RETURN(GetShouldOpenUp(&shouldOpenUp));
    }

    // CommonStates
    IFC_RETURN(GoToState(useTransitions, isEnabled ? L"Normal" : L"Disabled", &ignored));

    // FloatingStates
    if (m_Mode == AppBarMode_Floating)
    {
        IFC_RETURN(GoToState(useTransitions, isOpen ? L"FloatingVisible" : L"FloatingHidden", &ignored));
    }

    // DockPositions
    switch (m_Mode)
    {
        case AppBarMode_Top:
            IFC_RETURN(GoToState(useTransitions, L"Top", &ignored));
            break;

        case AppBarMode_Bottom:
            IFC_RETURN(GoToState(useTransitions, L"Bottom", &ignored));
            break;

        default:
            IFC_RETURN(GoToState(useTransitions, L"Undocked", &ignored));
            break;
    }

    // DisplayModeStates
    auto visualStateName = g_displayModeVisualStateTable[static_cast<size_t>(closedDisplayMode)][static_cast<size_t>(shouldOpenUp)][static_cast<size_t>(isOpen)];
    IFC_RETURN(GoToState(useTransitions, visualStateName, &ignored));

    return S_OK;
}

IFACEMETHODIMP
AppBar::OnPointerPressed(_In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    IFC_RETURN(__super::OnPointerPressed(pArgs));

    BOOLEAN bIsOpen = FALSE;
    IFC_RETURN(get_IsOpen(&bIsOpen));

    if (bIsOpen)
    {
        BOOLEAN isSticky = FALSE;
        IFC_RETURN(get_IsSticky(&isSticky));

        if (!isSticky)
        {
            // If the app bar is in a modal-like state, then don't propagate pointer
            // events.
            IFC_RETURN(pArgs->put_Handled(TRUE));
        }
    }
    else
    {
        // For closed minimal appbars, clicking on the bar will open it.
        auto closedDisplayMode = xaml_controls::AppBarClosedDisplayMode_Hidden;
        IFC_RETURN(get_ClosedDisplayMode(&closedDisplayMode));
        if (closedDisplayMode == xaml_controls::AppBarClosedDisplayMode_Minimal)
        {
            IFC_RETURN(put_IsOpen(TRUE));
            IFC_RETURN(pArgs->put_Handled(TRUE));
        }
    }

    return S_OK;
}

// When right tapped on the application bar toggle all application bars
IFACEMETHODIMP
AppBar::OnRightTapped(_In_ xaml_input::IRightTappedRoutedEventArgs* pArgs)
{
    IFC_RETURN(__super::OnRightTapped(pArgs));

    if (m_Mode != AppBarMode_Inline)
    {
        auto pointerDeviceType = mui::PointerDeviceType_Touch;
        IFC_RETURN(pArgs->get_PointerDeviceType(&pointerDeviceType));
        if (pointerDeviceType != mui::PointerDeviceType_Mouse)
        {
            return S_OK;
        }

        BOOLEAN isOpen = FALSE;
        IFC_RETURN(get_IsOpen(&isOpen));

        BOOLEAN isHandled = FALSE;
        IFC_RETURN(pArgs->get_Handled(&isHandled));

        if (isOpen && !isHandled)
        {
            ctl::ComPtr<IApplicationBarService> applicationBarService;
            if (const auto xamlRoot = XamlRoot::GetImplementationForElementStatic(this))
            {
                IFC_RETURN(xamlRoot->GetApplicationBarService(applicationBarService));
            }
            ASSERT(applicationBarService);

            applicationBarService->SetFocusReturnState(xaml::FocusState_Pointer);
            IFC_RETURN(applicationBarService->ToggleApplicationBars());

            applicationBarService->ResetFocusReturnState();
            IFC_RETURN(pArgs->put_Handled(TRUE));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
AppBar::OnIsStickyChanged()
{
    if (m_Mode != AppBarMode_Inline)
    {
        ctl::ComPtr<IApplicationBarService> applicationBarService;
        if (const auto xamlRoot = XamlRoot::GetImplementationForElementStatic(this))
        {
            IFC_RETURN(xamlRoot->GetApplicationBarService(applicationBarService));
        }

        // this function can be called before OnLoaded and therefore
        // it is possible (and OK) not to have appbarservice at this time
        if (applicationBarService)
        {
            IFC_RETURN(applicationBarService->UpdateDismissLayer());
        }
    }

    if (m_overlayElement)
    {
        BOOLEAN isSticky = FALSE;
        IFC_RETURN(get_IsSticky(&isSticky));
        IFC_RETURN(m_overlayElement.Cast<FrameworkElement>()->put_IsHitTestVisible(!isSticky));
    }

    return S_OK;
}

_Check_return_ HRESULT
AppBar::OnIsOpenChanged(bool isOpen)
{
    // If the AppBar is not live, then wait until it's loaded before
    // responding to changes to opened state and firing our Opening/Opened events.
    if (!IsInLiveTree())
    {
        return S_OK;
    }

    if (m_Mode != AppBarMode_Inline)
    {
        ctl::ComPtr<IApplicationBarService> applicationBarService;
        if (const auto xamlRoot = XamlRoot::GetImplementationForElementStatic(this))
        {
            IFC_RETURN(xamlRoot->GetApplicationBarService(applicationBarService));
        }
        ASSERT(applicationBarService);

        BOOLEAN hasFocus = FALSE;
        IFC_RETURN(HasFocus(&hasFocus));

        if (isOpen)
        {
            IFC_RETURN(applicationBarService->SaveCurrentFocusedElement(this));
            IFC_RETURN(applicationBarService->OpenApplicationBar(this, m_Mode));

            // If the AppBar does not already have focus (i.e. it was opened programmatically),
            // then focus the AppBar.
            if (!hasFocus)
            {
                IFC_RETURN(applicationBarService->FocusApplicationBar(this, xaml::FocusState_Programmatic));
            }
        }
        else
        {
            IFC_RETURN(applicationBarService->CloseApplicationBar(this, m_Mode));

            // Only restore the focus to the saved element if we have the focus just before closing.
            // For CommandBar, we also check if the Overflow has focus in the override method "HasFocus"
            if (hasFocus)
            {
                IFC_RETURN(applicationBarService->FocusSavedElement(this));
            }
        }

        IFC_RETURN(applicationBarService->UpdateDismissLayer());
    }

    // Flag that we're transitions between opened & closed states.
    m_isChangingOpenedState = true;

    // Fire our Opening/Closing events.  If we're a legacy app or a badly
    // re-templated app, then fire the Opened/Closed events as well.
    {
        ctl::ComPtr<RoutedEventArgs> routedEventArgs;
        IFC_RETURN(ctl::make(&routedEventArgs));
        IFC_RETURN(routedEventArgs->put_OriginalSource(ctl::as_iinspectable(this)));
        auto inspArgs = ctl::as_iinspectable(routedEventArgs.Get());

        IFC_RETURN(isOpen ? OnOpeningProtected(inspArgs) : OnClosingProtected(inspArgs));

        // We only query the display modes visual state group for post-WinBlue AppBars
        // so in cases where we don't have it (either via re-templating or legacy apps)
        // fire the Opening/Closing & Opened/Closed events immediately.
        // For WinBlue apps, firing the Opening/Closing events as well doesn't
        // matter because Blue apps wouldn't have had access to them.
        // For post-WinBlue AppBars, we fire the Opening/Closing & Opened/Closed
        // events based on our display mode state transitions.
        if (!m_tpDisplayModesStateGroup)
        {
            IFC_RETURN(isOpen ? OnOpenedProtected(inspArgs) : OnClosedProtected(inspArgs));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
AppBar::OnIsOpenChangedForAutomation(_In_ const PropertyChangedParams& args)
{
    bool isOpen = !!args.m_pNewValue->AsBool();
    BOOLEAN bAutomationListener = FALSE;

    if (isOpen)
    {
        IFC_RETURN(AutomationPeer::RaiseEventIfListener(this, xaml_automation_peers::AutomationEvents_MenuOpened));
    }
    else
    {
        IFC_RETURN(AutomationPeer::RaiseEventIfListener(this, xaml_automation_peers::AutomationEvents_MenuClosed));
    }

    // Raise ToggleState Property change event for Automation clients if they are listening for property changed events.
    IFC_RETURN(AutomationPeer::ListenerExistsHelper(xaml_automation_peers::AutomationEvents_PropertyChanged, &bAutomationListener));
    if (bAutomationListener)
    {
        ctl::ComPtr<xaml_automation_peers::IAutomationPeer> automationPeer;

        IFC_RETURN(GetOrCreateAutomationPeer(&automationPeer));
        if (automationPeer)
        {
            ctl::ComPtr<xaml_automation_peers::IAppBarAutomationPeer>
                applicationBarAutomationPeer(automationPeer.AsOrNull<xaml_automation_peers::IAppBarAutomationPeer>());

            if (applicationBarAutomationPeer)
            {
                ctl::ComPtr<IInspectable> oldValue;
                ctl::ComPtr<IInspectable> newValue;

                IFC_RETURN(CValueBoxer::UnboxObjectValue(args.m_pOldValue, args.m_pDP->GetPropertyType(), &oldValue));
                IFC_RETURN(CValueBoxer::UnboxObjectValue(args.m_pNewValue, args.m_pDP->GetPropertyType(), &newValue));

                IFC_RETURN(applicationBarAutomationPeer.Cast<AppBarAutomationPeer>()->RaiseToggleStatePropertyChangedEvent(oldValue.Get(), newValue.Get()));
                IFC_RETURN(applicationBarAutomationPeer.Cast<AppBarAutomationPeer>()->RaiseExpandCollapseAutomationEvent(isOpen));
            }
        }
    }

    return S_OK;
}

IFACEMETHODIMP
AppBar::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    ctl::ComPtr<xaml_automation_peers::IAppBarAutomationPeer> applicationBarAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IAppBarAutomationPeerFactory> applicationBarAPFactory;
    ctl::ComPtr<IActivationFactory> activationFactory;
    ctl::ComPtr<IInspectable> inner;

    activationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::AppBarAutomationPeerFactory>::CreateActivationFactory());
    IFC_RETURN(activationFactory.As(&applicationBarAPFactory));

    IFC_RETURN(applicationBarAPFactory.Cast<AppBarAutomationPeerFactory>()->CreateInstanceWithOwner(this,
        nullptr,
        &inner,
        &applicationBarAutomationPeer));
    IFC_RETURN(applicationBarAutomationPeer.CopyTo(ppAutomationPeer));

    return S_OK;
}

IFACEMETHODIMP
AppBar::OnKeyDown(_In_ xaml_input::IKeyRoutedEventArgs* pArgs)
{
    IFC_RETURN(__super::OnKeyDown(pArgs));

    //Ignore already handled events
    BOOLEAN isHandled = FALSE;
    IFC_RETURN(pArgs->get_Handled(&isHandled));
    if (isHandled)
    {
        return S_OK;
    }

    auto key = wsy::VirtualKey_None;
    IFC_RETURN(pArgs->get_Key(&key));
    if (key == wsy::VirtualKey_Escape)
    {
        bool isAnyAppBarClosed = false;

        if (m_Mode == AppBarMode_Inline)
        {
            IFC_RETURN(TryDismissInlineAppBar(&isAnyAppBarClosed));
        }
        else
        {
            BOOLEAN isSticky = FALSE;
            IFC_RETURN(get_IsSticky(&isSticky));

            // If we have focus and the app bar is not sticky close all light-dismiss app bars on ESC
            ctl::ComPtr<IApplicationBarService> applicationBarService;
            if (const auto xamlRoot = XamlRoot::GetImplementationForElementStatic(this))
            {
                IFC_RETURN(xamlRoot->GetApplicationBarService(applicationBarService));
            }
            ASSERT(applicationBarService);

            BOOLEAN hasFocus = FALSE;
            IFC_RETURN(HasFocus(&hasFocus));
            if (hasFocus)
            {
                IFC_RETURN(applicationBarService->CloseAllNonStickyAppBars(&isAnyAppBarClosed));

                if (isSticky)
                {
                    // If the appbar is sticky restore focus to the saved element without closing the appbar
                    applicationBarService->SetFocusReturnState(xaml::FocusState_Keyboard);
                    IFC_RETURN(applicationBarService->FocusSavedElement(this));
                    applicationBarService->ResetFocusReturnState();
                }
            }
        }

        IFC_RETURN(pArgs->put_Handled(isAnyAppBarClosed));
    }

    return S_OK;
}

_Check_return_ HRESULT AppBar::SetOwner(_In_opt_ Page* pOwner)
{
    return ctl::AsWeak(pOwner, &m_wpOwner);
}

_Check_return_ HRESULT AppBar::GetOwner(_Outptr_result_maybenull_ Page **ppOwner)
{
    ctl::ComPtr<Page> owner;
    owner = m_wpOwner.AsOrNull<xaml_controls::IPage>().Cast<Page>();
    *ppOwner = owner.Detach();

    return S_OK;
}

_Check_return_ HRESULT
AppBar::ContainsElement(_In_ DependencyObject* pElement, _Out_ bool *pContainsElement)
{
    BOOLEAN isAncestorOfElement = FALSE;

    *pContainsElement = false;

    // For AppBar, ContainsElement is equivalent to IsAncestorOf.
    // However, ContainsElement is a virtual method, and CommandBar's
    // implementation of it also checks the overflow popup separately from
    // IsAncestorOf since the popup isn't part of the same visual tree.
    IFC_RETURN(IsAncestorOf(pElement, &isAncestorOfElement));

    *pContainsElement = !!isAncestorOfElement;

    return S_OK;
}

bool
AppBar::IsExpandButton(_In_ UIElement* element)
{
    return m_tpExpandButton && ctl::are_equal(element, m_tpExpandButton.Get());
}

_Check_return_ HRESULT
AppBar::OnExpandButtonClick(_In_ IInspectable* /* pSender */, _In_ xaml::IRoutedEventArgs* /* pArgs */)
{
    BOOLEAN bIsOpen = FALSE;
    IFC_RETURN(get_IsOpen(&bIsOpen));

    if (!bIsOpen)
    {
        m_openedWithExpandButton = true;
    }
    
    IFC_RETURN(put_IsOpen(!bIsOpen));

    return S_OK;
}

_Check_return_ HRESULT
AppBar::OnDisplayModesStateChanged(_In_ IInspectable* pSender, xaml::IVisualStateChangedEventArgs* pArgs)
{
    // We only fire the opened/closed events if we're changing our opened state (either
    // from open to closed or closed to open).  We don't fire the event if we changed
    // between 2 opened states or 2 closed states such as might happen when changing
    // closed display mode.
    if (m_isChangingOpenedState)
    {
        // Create the event args we'll use for our Opened/Closed events.
        ctl::ComPtr<RoutedEventArgs> routedEventArgs;
        IFC_RETURN(ctl::make(&routedEventArgs));
        IFC_RETURN(routedEventArgs->put_OriginalSource(ctl::as_iinspectable(this)));
        auto inspArgs = ctl::as_iinspectable(routedEventArgs.Get());

        BOOLEAN isOpen = FALSE;
        IFC_RETURN(get_IsOpen(&isOpen));

        IFC_RETURN(isOpen ? OnOpenedProtected(inspArgs) : OnClosedProtected(inspArgs));

        m_isChangingOpenedState = false;
    }

    return S_OK;
}

_Check_return_ HRESULT
AppBar::UpdateTemplateSettings()
{
    //
    // AppBar/CommandBar TemplateSettings and how they're used.
    //
    // The template settings are core to acheiving the desired experience
    // for AppBar/CommandBar at least to how it relates to the various
    // ClosedDisplayModes.
    //
    // This comment block will describe how the code uses TemplateSettings
    // to achieve the desired bar interation experience which is controlled
    // via the ClosedDisplayMode property.
    //
    // Anatomy of the bar component of an AppBar/CommandBar:
    //
    //  !==================================================!
    //  !                  Clip Rectangle                  !
    //  !                                                  !
    //  ! |----------------------------------------------| !
    //  ! |                                              | !
    //  ! |                 Content Root                 | !
    //  ! |                                              | !
    //  !=|==============================================|=!
    //    |::::::::::::::::::::::::::::::::::::::::::::::|
    //    |::::::::::::::::::::::::::::::::::::::::::::::|
    //    |----------------------------------------------|
    //
    // * The region covered in '::' is clipped away.
    //
    // ** The diagram shows the clip rect wider than the content, but
    //    that is just done to make it more readable.  In reality, they
    //    are the same width.
    //
    // When we measure and arrange an AppBar/CommandBar, the size we return
    // as our desired sized (in the case of measure) and the final size
    // (in the case of arrange) depends on the closed display mode.  We
    // measure our sub-tree normally but we modify the returned height to
    // make it match our closed display mode.
    //
    // This causes the control to get arranged such that the top portion
    // of the content root that is within our closed display mode height
    // will be visible, while the rest that is below will get covered up
    // by other content.  It's similar to if we had a negative margin on
    // the bottom.
    //
    // The clip rectangle is then used to make sure this bottom portion does
    // not get rendered; so we are left with just the top portion representing
    // our closed display mode.
    //
    // This is where the template settings start to play a part.  We need
    // to make sure to translate the clip rectangle up by a value that is equal
    // to the difference between the content's height and our closed display
    // mode height.  Since we want to translate up, we have to make that value
    // negative, which results in this equation:
    //
    //      VerticalDelta = ClosedDisplayMode_Height - ContentHeight
    //
    // This value is calculated for each of our supported ClosedDisplayModes
    // and is then used in our template & VSM to create the Closed/OpenUp/OpenDown
    // experiences.
    //
    // We apply it in the following ways to achieve our various states:
    //
    //     Closed:
    //      - Clip Rectangle translated by VerticalDelta (essentially translated up).
    //      - Content Root not translated.
    //
    //     OpenUp:
    //      - Clip Rectangle translated by VerticalDelta (essentially translated up).
    //      - Content Root translated by VerticalDelta (essentially translated up).
    //
    //     OpenDown:
    //      - Clip Rectangle not translated.
    //      - Content Root not translated.
    //

    ctl::ComPtr<xaml_primitives::IAppBarTemplateSettings> templateSettings;
    IFC_RETURN(get_TemplateSettings(&templateSettings));

    double actualWidth = 0.0;
    IFC_RETURN(get_ActualWidth(&actualWidth));

    double contentHeight = GetContentHeight();

    IFC_RETURN(templateSettings.Cast<AppBarTemplateSettings>()->put_ClipRect({ 0, 0, static_cast<float>(actualWidth), static_cast<float>(contentHeight) }));

    double compactVerticalDelta = m_compactHeight - contentHeight;
    IFC_RETURN(templateSettings.Cast<AppBarTemplateSettings>()->put_CompactVerticalDelta(compactVerticalDelta));
    IFC_RETURN(templateSettings.Cast<AppBarTemplateSettings>()->put_NegativeCompactVerticalDelta(-compactVerticalDelta));

    double minimalVerticalDelta = m_minimalHeight - contentHeight;
    IFC_RETURN(templateSettings.Cast<AppBarTemplateSettings>()->put_MinimalVerticalDelta(minimalVerticalDelta));
    IFC_RETURN(templateSettings.Cast<AppBarTemplateSettings>()->put_NegativeMinimalVerticalDelta(-minimalVerticalDelta));

    IFC_RETURN(templateSettings.Cast<AppBarTemplateSettings>()->put_HiddenVerticalDelta(-contentHeight));
    IFC_RETURN(templateSettings.Cast<AppBarTemplateSettings>()->put_NegativeHiddenVerticalDelta(contentHeight));

    if (m_hasUpdatedTemplateSettings)
    {
        // We wait until after the first call to update template settings to query DisplayModesStates VSG
        // to to prevent a performance hit on app startup (see VSO 2362425)
        IFC_RETURN(TryQueryDisplayModesStatesGroup());

        // Force animations that reference our template settings in the current visual state
        // to update their bindings.
        if (m_tpDisplayModesStateGroup)
        {
            ctl::ComPtr<xaml::IVisualState> currentState;
            IFC_RETURN(m_tpDisplayModesStateGroup->get_CurrentState(&currentState));
            if (currentState)
            {
                ctl::ComPtr<xaml_animation::IStoryboard> storyboard;
                IFC_RETURN(currentState->get_Storyboard(&storyboard));
                if (storyboard)
                {
                    IFC_RETURN(storyboard->SkipToFill());
                }
            }
        }
    }
    m_hasUpdatedTemplateSettings = true;

    return S_OK;
}

_Check_return_ HRESULT
AppBar::GetShouldOpenUp(bool* shouldOpenUp)
{
    // Bottom appbars always open up.  All other appbars by default open down
    *shouldOpenUp = (m_Mode == AppBarMode_Bottom);

    // If the appbar is inline, check to see if opening in the default direction would cause
    // the appbar to appear partially offscreen and if so switch to opening the other way instead.
    if (m_Mode == AppBarMode_Inline)
    {
        ctl::ComPtr<xaml_media::IGeneralTransform> transform;
        IFC_RETURN(TransformToVisual(nullptr, &transform));

        double offsetNeeded = 0;
        bool opensWindowed = false;
        IFC_RETURN(GetVerticalOffsetNeededToOpenInDefaultDirection(&offsetNeeded, &opensWindowed));

        // Subtract layout bounds to avoid using the System Tray area to open the AppBar.
        wf::Point offsetFromRootOpened = {};
        IFC_RETURN(transform->TransformPoint({ 0, (float)(offsetNeeded) }, &offsetFromRootOpened));

        wf::Rect layoutBounds = {};

        if (opensWindowed)
        {
            wf::Point topLeftPoint = { 0, 0 };
            IFC_RETURN(transform->TransformPoint(topLeftPoint, &topLeftPoint));

            IFC_RETURN(DXamlCore::GetCurrent()->CalculateAvailableMonitorRect(this, topLeftPoint, &layoutBounds));
        }
        else
        {
            wf::Rect windowBounds = {};
            IFC_RETURN(DXamlCore::GetCurrent()->GetContentBoundsForElement(GetHandle(), &windowBounds));
            IFC_RETURN(DXamlCore::GetCurrent()->GetContentLayoutBoundsForElement(GetHandle(), &layoutBounds));

            // Convert the layout bounds X/Y offsets from screen coorindates into window coordinates.
            layoutBounds.X -= windowBounds.X;
            layoutBounds.Y -= windowBounds.Y;
        }

        // Since we open down by default, we'll open up only if we *don't* have space in the down direction.
        *shouldOpenUp = (offsetFromRootOpened.Y > layoutBounds.Y + layoutBounds.Height);
    }

    return S_OK;
}

_Check_return_ HRESULT
AppBar::GetVerticalOffsetNeededToOpenInDefaultDirection(_Out_ double* neededOffset, _Out_ bool* opensWindowed)
{
    double verticalDelta = 0.0;
    ctl::ComPtr<xaml_primitives::IAppBarTemplateSettings> templateSettings;
    IFC_RETURN(get_TemplateSettings(&templateSettings));

    auto closedDisplayMode = xaml_controls::AppBarClosedDisplayMode_Compact;
    IFC_RETURN(get_ClosedDisplayMode(&closedDisplayMode));
    switch (closedDisplayMode)
    {
        case xaml_controls::AppBarClosedDisplayMode_Compact:
            IFC_RETURN(templateSettings->get_CompactVerticalDelta(&verticalDelta));
            break;

        case xaml_controls::AppBarClosedDisplayMode_Minimal:
            IFC_RETURN(templateSettings->get_MinimalVerticalDelta(&verticalDelta));
            break;

        default:
        case xaml_controls::AppBarClosedDisplayMode_Hidden:
            IFC_RETURN(templateSettings->get_HiddenVerticalDelta(&verticalDelta));
            break;
    }

    *neededOffset = -verticalDelta;
    *opensWindowed = false;

    return S_OK;
}

_Check_return_ HRESULT
AppBar::TryDismissInlineAppBar()
{
    bool isAppBarDismissed = false;
    IFC_RETURN(TryDismissInlineAppBar(&isAppBarDismissed));

    return S_OK;
}

_Check_return_ HRESULT
AppBar::TryDismissInlineAppBar(_Out_ bool* isAppBarDismissed)
{
    ASSERT(m_Mode == AppBarMode_Inline);

    *isAppBarDismissed = false;

    BOOLEAN isSticky = FALSE;
    IFC_RETURN(get_IsSticky(&isSticky));
    if (!isSticky)
    {
        BOOLEAN isOpen = FALSE;
        IFC_RETURN(get_IsOpen(&isOpen));
        if (isOpen) {
            *isAppBarDismissed = true;
            IFC_RETURN(put_IsOpen(FALSE));
        }

    }

    return S_OK;
}

_Check_return_ HRESULT
AppBar::SetFocusOnAppBar()
{
    // The ApplicationBarService handles focus for non-inline appbars.
    ASSERT(m_Mode == AppBarMode_Inline);

    ctl::ComPtr<DependencyObject> focusedElement;
    IFC_RETURN(GetFocusedElement(&focusedElement));

    BOOLEAN isAncestorOf = FALSE;
    IFC_RETURN(IsAncestorOf(focusedElement.Get(), &isAncestorOf));

    // Only steal focus if focus isn't already within the appbar.
    if (!isAncestorOf)
    {
        IFC_RETURN(ctl::AsWeak(focusedElement.Get(), &m_savedFocusedElementWeakRef));

        auto focusedElementAsControl = focusedElement.AsOrNull<Control>();
        if (focusedElementAsControl)
        {
            IFC_RETURN(focusedElementAsControl->get_FocusState(&m_savedFocusState));
        }
        else
        {
            m_savedFocusState = xaml::FocusState_Programmatic;
        }

        // Now focus the first-focusable element in the appbar.
        xref_ptr<CDependencyObject> firstFocusableElement;
        IFC_RETURN(CoreImports::FocusManager_GetFirstFocusableElement(GetHandle(), firstFocusableElement.ReleaseAndGetAddressOf()));
        if (firstFocusableElement)
        {
            ctl::ComPtr<DependencyObject> firstFocusableElementPeer;
            IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(firstFocusableElement, &firstFocusableElementPeer));

            BOOLEAN wasFocused = FALSE;
            IFC_RETURN(DependencyObject::SetFocusedElement(firstFocusableElementPeer.Get(), m_savedFocusState, FALSE /*animateIfBringIntoView*/, &wasFocused));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
AppBar::RestoreSavedFocus()
{
    // The ApplicationBarService handles focus for non-inline appbars.
    ASSERT(m_Mode == AppBarMode_Inline);

    ctl::ComPtr<DependencyObject> savedFocusedElement;
    if (m_savedFocusedElementWeakRef)
    {
        savedFocusedElement = m_savedFocusedElementWeakRef.AsOrNull<DependencyObject>();
    }

    IFC_RETURN(RestoreSavedFocusImpl(savedFocusedElement.Get(), m_savedFocusState));

    m_savedFocusedElementWeakRef.Reset();
    m_savedFocusState = xaml::FocusState_Unfocused;

    return S_OK;
}

_Check_return_ HRESULT
AppBar::RestoreSavedFocusImpl(_In_opt_ DependencyObject* savedFocusedElement, xaml::FocusState savedFocusState)
{
    if (savedFocusedElement)
    {
        BOOLEAN ignored = FALSE;
        IFC_RETURN(DependencyObject::SetFocusedElement(savedFocusedElement, m_savedFocusState, FALSE /*animateIfBringIntoView*/, &ignored));
    }

    return S_OK;
}

_Check_return_ HRESULT
AppBar::RefreshContentHeight(_Out_opt_ bool *didChange)
{
    double oldHeight = m_contentHeight;

    if (m_tpContentRoot)
    {
        IFC_RETURN(m_tpContentRoot.Cast<FrameworkElement>()->get_ActualHeight(&m_contentHeight));
    }

    if (didChange != nullptr)
    {
        *didChange = (oldHeight != m_contentHeight);
    }

    return S_OK;
}

_Check_return_ HRESULT
AppBar::SetupOverlayState()
{
    ASSERT(m_Mode == AppBarMode_Inline);
    ASSERT(!m_isInOverlayState);

    // The approach used to achieve light-dismiss is to create a 1x1 element that is added
    // as the first child of our layout root panel.  Adding it as the first child ensures that
    // it is below our actual content and will therefore not affect the content area's hit-testing.
    // We then use a scale transform to scale up an LTE targeted to the element to match the
    // dimensions of our window.  Finally, we translate that same LTE to make sure it's upper-left
    // corner aligns with the window's upper left corner, causing it to cover the entire window.
    // A pointer pressed handler is attached to the element to intercept any pointer
    // input that is not directed at the actual content.  The value of AppBar.IsSticky controls
    // whether the light-dismiss element is hit-testable (IsSticky=True -> hit-testable=False).
    // The pointer pressed handler simply closes the appbar and marks the routed event args
    // message as handled.
    if (m_tpLayoutRoot)
    {
        // Create our overlay element if necessary.
        if (!m_overlayElement)
        {
            ctl::ComPtr<Rectangle> rectangle;
            IFC_RETURN(ctl::make(&rectangle));
            IFC_RETURN(rectangle->put_Width(1));
            IFC_RETURN(rectangle->put_Height(1));
            IFC_RETURN(rectangle->put_UseLayoutRounding(FALSE));

            BOOLEAN isSticky = FALSE;
            IFC_RETURN(get_IsSticky(&isSticky));
            IFC_RETURN(rectangle->put_IsHitTestVisible(!isSticky));

            IFC_RETURN(m_overlayElementPointerPressedEventHandler.AttachEventHandler(
                rectangle.Get(),
                std::bind(&AppBar::OnOverlayElementPointerPressed, this, _1, _2))
                );

            IFC_RETURN(SetPtrValueWithQI(m_overlayElement, rectangle.Get()));

            IFC_RETURN(UpdateOverlayElementBrush());
        }

        // Add our overlay element to our layout root panel.
        ctl::ComPtr<wfc::IVector<xaml::UIElement*>> layoutRootChildren;
        IFC_RETURN(m_tpLayoutRoot.Cast<Grid>()->get_Children(&layoutRootChildren));
        IFC_RETURN(layoutRootChildren->InsertAt(0, m_overlayElement.Cast<FrameworkElement>()));
    }

    IFC_RETURN(CreateLTEs());

    // Update the animations to target the newly created overlay element LTE.
    if (m_isOverlayVisible)
    {
        IFC_RETURN(UpdateTargetForOverlayAnimations());
    }

    m_isInOverlayState = true;

    return S_OK;
}

_Check_return_ HRESULT
AppBar::TeardownOverlayState()
{
    ASSERT(m_Mode == AppBarMode_Inline);
    ASSERT(m_isInOverlayState);

    IFC_RETURN(DestroyLTEs());

    // Remove our light-dismiss element from our layout root panel.
    if (m_tpLayoutRoot)
    {
        ctl::ComPtr<wfc::IVector<xaml::UIElement*>> layoutRootChildren;
        IFC_RETURN(m_tpLayoutRoot.Cast<Grid>()->get_Children(&layoutRootChildren));

        UINT32 indexOfOverlayElement = 0;
        BOOLEAN wasFound = FALSE;
        IFC_RETURN(layoutRootChildren->IndexOf(m_overlayElement.Cast<FrameworkElement>(), &indexOfOverlayElement, &wasFound));
        ASSERT(wasFound);
        IFC_RETURN(layoutRootChildren->RemoveAt(indexOfOverlayElement));
    }

    m_isInOverlayState = false;

    return S_OK;
}

_Check_return_ HRESULT
AppBar::CreateLTEs()
{
    ASSERT(!m_layoutTransitionElement);
    ASSERT(!m_overlayLayoutTransitionElement);
    ASSERT(!m_parentElementForLTEs);

    // If we're under the PopupRoot or FullWindowMediaRoot, then we'll explicitly set
    // our LTE's parent to make sure the LTE doesn't get placed under the TransitionRoot,
    // which is lower in z-order than these other roots.
    if (ShouldUseParentedLTE())
    {
        ctl::ComPtr<xaml::IDependencyObject> parent;
        IFC_RETURN(VisualTreeHelper::GetParentStatic(this, &parent));
        IFCEXPECT_RETURN(parent);

        IFC_RETURN(SetPtrValueWithQI(m_parentElementForLTEs, parent.Get()));
    }

    if (m_overlayElement)
    {
        // Create an LTE for our overlay element.
        IFC_RETURN(CoreImports::LayoutTransitionElement_Create(
            DXamlCore::GetCurrent()->GetHandle(),
            m_overlayElement.Cast<FrameworkElement>()->GetHandle(),
            m_parentElementForLTEs ? m_parentElementForLTEs.Cast<UIElement>()->GetHandle() : nullptr,
            false /*isAbsolutelyPositioned*/,
            m_overlayLayoutTransitionElement.ReleaseAndGetAddressOf()
        ));

        // Configure the overlay LTE.
        {
            ctl::ComPtr<DependencyObject> overlayLTEPeer;
            IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(m_overlayLayoutTransitionElement.get(), &overlayLTEPeer));

            wf::Rect windowBounds = {};
            IFC_RETURN(DXamlCore::GetCurrent()->GetContentBoundsForElement(GetHandle(), &windowBounds));

            ctl::ComPtr<CompositeTransform> compositeTransform;
            IFC_RETURN(ctl::make(&compositeTransform));

            IFC_RETURN(compositeTransform->put_ScaleX(windowBounds.Width));
            IFC_RETURN(compositeTransform->put_ScaleY(windowBounds.Height));

            IFC_RETURN(overlayLTEPeer.Cast<UIElement>()->put_RenderTransform(compositeTransform.Get()));

            ctl::ComPtr<xaml_media::IGeneralTransform> transformToVisual;
            IFC_RETURN(m_overlayElement.Cast<FrameworkElement>()->TransformToVisual(nullptr, &transformToVisual));

            wf::Point offsetFromRoot = {};
            IFC_RETURN(transformToVisual->TransformPoint({ 0, 0 }, &offsetFromRoot));

            auto flowDirection = xaml::FlowDirection_LeftToRight;
            IFC_RETURN(get_FlowDirection(&flowDirection));

            // Translate the light-dismiss layer so that it is positioned at the top-left corner of the window (for LTR cases)
            // or the top-right corner of the window (for RTL cases).
            // TransformToVisual(nullptr) will return an offset relative to the top-left corner of the window regardless of
            // flow direction, so for RTL cases subtract the window width from the returned offset.x value to make it relative
            // to the right edge of the window.
            IFC_RETURN(compositeTransform->put_TranslateX(flowDirection == xaml::FlowDirection_LeftToRight ? -offsetFromRoot.X : offsetFromRoot.X - windowBounds.Width));
            IFC_RETURN(compositeTransform->put_TranslateY(-offsetFromRoot.Y));
        }
    }

    IFC_RETURN(CoreImports::LayoutTransitionElement_Create(
        DXamlCore::GetCurrent()->GetHandle(),
        GetHandle(),
        m_parentElementForLTEs ? m_parentElementForLTEs.Cast<UIElement>()->GetHandle() : nullptr,
        false /*isAbsolutelyPositioned*/,
        m_layoutTransitionElement.ReleaseAndGetAddressOf()
    ));

    // Forward our control's opacity to the LTE since it doesn't happen automatically.
    {
        double opacity = 0.0;
        IFC_RETURN(get_Opacity(&opacity));
        IFC_RETURN(m_layoutTransitionElement->SetValueByKnownIndex(KnownPropertyIndex::UIElement_Opacity, static_cast<float>(opacity)));
    }

    IFC_RETURN(PositionLTEs());

    return S_OK;
}

_Check_return_ HRESULT
AppBar::PositionLTEs()
{
    ASSERT(m_layoutTransitionElement);

    ctl::ComPtr<xaml::IDependencyObject> parentDO;
    ctl::ComPtr<xaml::IUIElement> parent;

    IFC_RETURN(VisualTreeHelper::GetParentStatic(this, &parentDO));

    // If we don't have a parent, then there's nothing for us to do.
    if (parentDO)
    {
        IFC_RETURN(parentDO.As(&parent));

        ctl::ComPtr<xaml_media::IGeneralTransform> transform;
        IFC_RETURN(TransformToVisual(parent.Cast<UIElement>(), &transform));

        wf::Point offset = {};
        IFC_RETURN(transform->TransformPoint({ 0, 0 }, &offset));

        IFC_RETURN(CoreImports::LayoutTransitionElement_SetDestinationOffset(m_layoutTransitionElement, offset.X, offset.Y));
    }

    return S_OK;
}

_Check_return_ HRESULT
AppBar::DestroyLTEs()
{
    if (m_layoutTransitionElement)
    {
        IFC_RETURN(CoreImports::LayoutTransitionElement_Destroy(
            DXamlCore::GetCurrent()->GetHandle(),
            GetHandle(),
            m_parentElementForLTEs ? m_parentElementForLTEs.Cast<UIElement>()->GetHandle() : nullptr,
            m_layoutTransitionElement.get()
        ));

        m_layoutTransitionElement.reset();
    }

    if (m_overlayLayoutTransitionElement)
    {
        // Destroy our light-dismiss element's LTE.
        IFC_RETURN(CoreImports::LayoutTransitionElement_Destroy(
            DXamlCore::GetCurrent()->GetHandle(),
            m_overlayElement.Cast<FrameworkElement>()->GetHandle(),
            m_parentElementForLTEs ? m_parentElementForLTEs.Cast<UIElement>()->GetHandle() : nullptr,
            m_overlayLayoutTransitionElement.get()
            ));

        m_overlayLayoutTransitionElement.reset();
    }

    m_parentElementForLTEs.Clear();

    return S_OK;
}

_Check_return_ HRESULT
AppBar::OnOverlayElementPointerPressed(IInspectable* /*pSender*/, xaml_input::IPointerRoutedEventArgs* pArgs)
{
    ASSERT(m_Mode == AppBarMode_Inline);

    IFC_RETURN(TryDismissInlineAppBar());
    IFC_RETURN(pArgs->put_Handled(TRUE));

    return S_OK;
}

_Check_return_ HRESULT
AppBar::TryQueryDisplayModesStatesGroup()
{
    if (!m_tpDisplayModesStateGroup)
    {
        ctl::ComPtr<xaml::IVisualStateGroup> displayModesStateGroup;
        IFC_RETURN(GetTemplatePart<xaml::IVisualStateGroup>(STR_LEN_PAIR(L"DisplayModeStates"), displayModesStateGroup.ReleaseAndGetAddressOf()));
        SetPtrValue(m_tpDisplayModesStateGroup, displayModesStateGroup.Get());

        if (m_tpDisplayModesStateGroup)
        {
            IFC_RETURN(m_displayModeStateChangedEventHandler.AttachEventHandler(m_tpDisplayModesStateGroup.Get(), std::bind(&AppBar::OnDisplayModesStateChanged, this, _1, _2)));
        }
    }

    return S_OK;
}

bool
AppBar::ShouldUseParentedLTE()
{
    ctl::ComPtr<xaml::IDependencyObject> rootDO;
    if (SUCCEEDED(VisualTreeHelper::GetRootStatic(this, &rootDO)) && rootDO)
    {
        ctl::ComPtr<PopupRoot> popupRoot;
        ctl::ComPtr<FullWindowMediaRoot> mediaRoot;

        if (SUCCEEDED(rootDO.As(&popupRoot)) && popupRoot)
        {
            return true;
        }
        else if (SUCCEEDED(rootDO.As(&mediaRoot)) && mediaRoot)
        {
            return true;
        }
    }

    return false;
}

_Check_return_ HRESULT
AppBar::OnBackButtonPressedImpl(_Out_ BOOLEAN* pHandled)
{
    BOOLEAN isOpen = FALSE;
    BOOLEAN isSticky = FALSE;

    IFCPTR_RETURN(pHandled);

    IFC_RETURN(get_IsOpen(&isOpen));
    IFC_RETURN(get_IsSticky(&isSticky));
    if (isOpen && !isSticky)
    {
        IFC_RETURN(put_IsOpen(FALSE));
        *pHandled = TRUE;

        if (m_Mode != AppBarMode_Inline)
        {
            ctl::ComPtr<IApplicationBarService> applicationBarService;
            if (const auto xamlRoot = XamlRoot::GetImplementationForElementStatic(this))
            {
                IFC_RETURN(xamlRoot->GetApplicationBarService(applicationBarService));
            }
            ASSERT(applicationBarService);
            IFC_RETURN(applicationBarService->CloseAllNonStickyAppBars());
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
AppBar::ReevaluateIsOverlayVisible()
{
    bool isOverlayVisible = false;
    IFC_RETURN(LightDismissOverlayHelper::ResolveIsOverlayVisibleForControl(this, &isOverlayVisible));

    // Only inline app bars can enable their overlays.  Top/Bottom/Floating will use
    // the overlay from the ApplicationBarService.
    isOverlayVisible &= (m_Mode == AppBarMode_Inline);

    if (isOverlayVisible != m_isOverlayVisible)
    {
        m_isOverlayVisible = isOverlayVisible;

        if (m_isOverlayVisible)
        {
            if (m_isInOverlayState)
            {
                IFC_RETURN(UpdateTargetForOverlayAnimations());
            }
        }
        else
        {
            // Make sure we've stopped our animations.
            if (m_overlayOpeningStoryboard)
            {
                IFC_RETURN(m_overlayOpeningStoryboard->Stop());
            }

            if (m_overlayClosingStoryboard)
            {
                IFC_RETURN(m_overlayClosingStoryboard->Stop());
            }
        }

        if (m_overlayElement)
        {
            IFC_RETURN(UpdateOverlayElementBrush());
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
AppBar::UpdateOverlayElementBrush()
{
    ASSERT(m_overlayElement);

    if (m_isOverlayVisible)
    {
        // Create a theme resource for the overlay brush.
        auto core = DXamlCore::GetCurrent()->GetHandle();
        auto dictionary = core->GetThemeResources();

        xstring_ptr themeBrush;
        IFC_RETURN(xstring_ptr::CloneBuffer(L"AppBarLightDismissOverlayBackground", &themeBrush));

        CDependencyObject* initialValueNoRef = nullptr;
        IFC_RETURN(dictionary->GetKeyNoRef(themeBrush, &initialValueNoRef));

        CREATEPARAMETERS cp(core);
        xref_ptr<CThemeResourceExtension> themeResourceExtension;
        IFC_RETURN(CThemeResourceExtension::Create(
            reinterpret_cast<CDependencyObject **>(themeResourceExtension.ReleaseAndGetAddressOf()),
            &cp));

        themeResourceExtension->m_strResourceKey = themeBrush;

        IFC_RETURN(themeResourceExtension->SetInitialValueAndTargetDictionary(initialValueNoRef, dictionary));

        IFC_RETURN(themeResourceExtension->SetThemeResourceBinding(
            m_overlayElement.Cast<FrameworkElement>()->GetHandle(),
            DirectUI::MetadataAPI::GetPropertyByIndex(KnownPropertyIndex::Shape_Fill))
            );
    }
    else
    {
        ctl::ComPtr<SolidColorBrush> transparentBrush;
        IFC_RETURN(ctl::make(&transparentBrush));
        IFC_RETURN(transparentBrush->put_Color({ 0, 0, 0, 0 }));

        ctl::ComPtr<Rectangle> overlayAsRect;
        IFC_RETURN(m_overlayElement.As(&overlayAsRect));
        IFC_RETURN(overlayAsRect.Cast<Rectangle>()->put_Fill(transparentBrush.Get()));
    }

    return S_OK;
}

_Check_return_ HRESULT
AppBar::UpdateTargetForOverlayAnimations()
{
    ASSERT(m_layoutTransitionElement);
    ASSERT(m_isOverlayVisible);

    if (m_overlayOpeningStoryboard)
    {
        IFC_RETURN(m_overlayOpeningStoryboard->Stop());

        IFC_RETURN(CoreImports::Storyboard_SetTarget(
            static_cast<CTimeline*>(m_overlayOpeningStoryboard.Cast<Storyboard>()->GetHandle()),
            m_overlayLayoutTransitionElement.get())
            );
    }

    if (m_overlayClosingStoryboard)
    {
        IFC_RETURN(m_overlayClosingStoryboard->Stop());

        IFC_RETURN(CoreImports::Storyboard_SetTarget(
            static_cast<CTimeline*>(m_overlayClosingStoryboard.Cast<Storyboard>()->GetHandle()),
            m_overlayLayoutTransitionElement.get())
            );
    }

    return S_OK;
}

_Check_return_ HRESULT
AppBar::PlayOverlayOpeningAnimation()
{
    ASSERT(m_isInOverlayState);
    ASSERT(m_isOverlayVisible);

    if (m_overlayClosingStoryboard)
    {
        IFC_RETURN(m_overlayClosingStoryboard->Stop());
    }

    if (m_overlayOpeningStoryboard)
    {
        IFC_RETURN(m_overlayOpeningStoryboard->Begin());
    }

    return S_OK;
}

_Check_return_ HRESULT
AppBar::PlayOverlayClosingAnimation()
{
    ASSERT(m_isInOverlayState);
    ASSERT(m_isOverlayVisible);

    if (m_overlayOpeningStoryboard)
    {
        IFC_RETURN(m_overlayOpeningStoryboard->Stop());
    }

    if (m_overlayClosingStoryboard)
    {
        IFC_RETURN(m_overlayClosingStoryboard->Begin());
    }

    return S_OK;
}
