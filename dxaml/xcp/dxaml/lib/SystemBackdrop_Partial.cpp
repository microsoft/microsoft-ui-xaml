// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <Microsoft.UI.Composition.h>
#include "FrameworkElement.g.h"
#include "SystemBackdrop.g.h"
#include "SystemBackdrop.h"
#include "XamlRoot.g.h"
#include "FrameworkTheming.h"
#include "theming\inc\Theme.h"

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

/* static */ _Check_return_ HRESULT SystemBackdrop::InvokeOnTargetConnectedFromCore(_In_ CDependencyObject* object, _In_ ixp::ICompositionSupportsSystemBackdrop* connectedTarget, _In_opt_ xaml::IXamlRoot* xamlRoot)
{
    ctl::ComPtr<SystemBackdrop> thisPeer;
    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer<SystemBackdrop>(object, &thisPeer));
    IFC_RETURN(thisPeer->InvokeOnTargetConnected(connectedTarget, xamlRoot));
    return S_OK;
}

 _Check_return_ HRESULT SystemBackdrop::InvokeOnTargetConnected(_In_ ixp::ICompositionSupportsSystemBackdrop* target, _In_opt_ xaml::IXamlRoot* xamlRoot)
{
    IFC_RETURN(OnTargetConnectedProtected(target, xamlRoot));

    return S_OK;
}

// Default implementation
_Check_return_ HRESULT SystemBackdrop::OnTargetConnectedImpl(_In_ ixp::ICompositionSupportsSystemBackdrop* connectedTarget, _In_opt_ xaml::IXamlRoot* xamlRoot)
{
    auto entryIterator = std::find_if(
        m_perTargetConfigurationList.begin(),
        m_perTargetConfigurationList.end(),
        [connectedTarget](const std::unique_ptr<PerTargetConfigurationEntry>& entry){ return entry->m_key == connectedTarget; });
    FAIL_FAST_ASSERT(entryIterator == m_perTargetConfigurationList.end());

    m_perTargetConfigurationList.emplace_back(std::make_unique<PerTargetConfigurationEntry>(this, connectedTarget, xamlRoot));

    return S_OK;
}

/* static */ _Check_return_ HRESULT SystemBackdrop::InvokeOnTargetDisconnectedFromCore(_In_ CDependencyObject* object, _In_ ixp::ICompositionSupportsSystemBackdrop* disconnectedTarget)
{
    ctl::ComPtr<SystemBackdrop> thisPeer;
    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer<SystemBackdrop>(object, &thisPeer));
    IFC_RETURN(thisPeer->InvokeOnTargetDisconnected(disconnectedTarget));

    return S_OK;
}

_Check_return_ HRESULT SystemBackdrop::InvokeOnTargetDisconnected(_In_ ixp::ICompositionSupportsSystemBackdrop* target)
{
    IFC_RETURN(OnTargetDisconnectedProtected(target));

    return S_OK;
}

// Default implementation
_Check_return_ HRESULT SystemBackdrop::OnTargetDisconnectedImpl(_In_ ixp::ICompositionSupportsSystemBackdrop* disconnectedTarget)
{
    auto entryIterator = std::find_if(
        m_perTargetConfigurationList.begin(),
        m_perTargetConfigurationList.end(),
        [disconnectedTarget](const std::unique_ptr<PerTargetConfigurationEntry>& entry){ return entry->m_key == disconnectedTarget; });
    ASSERT(entryIterator != m_perTargetConfigurationList.end());
    m_perTargetConfigurationList.erase(entryIterator);

    return S_OK;
}

_Check_return_ HRESULT
SystemBackdrop::GetDefaultSystemBackdropConfigurationImpl(_In_ ixp::ICompositionSupportsSystemBackdrop* target, _In_opt_ xaml::IXamlRoot* xamlRoot, _Outptr_ ixp::SystemBackdrops::ISystemBackdropConfiguration** ppResult)
{
    UNREFERENCED_PARAMETER(xamlRoot);
    *ppResult = nullptr;

    auto entryIterator = std::find_if(
        m_perTargetConfigurationList.begin(),
        m_perTargetConfigurationList.end(),
        [target](const std::unique_ptr<PerTargetConfigurationEntry>& entry){ return entry->m_key == target; });
    if (entryIterator != m_perTargetConfigurationList.end())
    {
        IFC_RETURN((*entryIterator)->m_configuration.CopyTo(ppResult));
    }

    return S_OK;
}

_Check_return_ HRESULT SystemBackdrop::InvokeOnDefaultSystemBackdropConfigurationChanged(_In_ ixp::ICompositionSupportsSystemBackdrop* target, _In_opt_ xaml::IXamlRoot* xamlRoot)
{
    IFC_RETURN(OnDefaultSystemBackdropConfigurationChangedProtected(target, xamlRoot));
    return S_OK;
}

// Default implementation
_Check_return_ HRESULT SystemBackdrop::OnDefaultSystemBackdropConfigurationChangedImpl(_In_ ixp::ICompositionSupportsSystemBackdrop* connectedTarget, _In_opt_ xaml::IXamlRoot* xamlRoot)
{
    return S_OK;
}

// Set up an entry in the event handler map (created in-place since copy and assignment are blocked). Hooks up the necessary handlers.
SystemBackdrop::PerTargetConfigurationEntry::PerTargetConfigurationEntry(_In_ SystemBackdrop* systemBackdrop, _In_ ixp::ICompositionSupportsSystemBackdrop* connectedTarget, _In_ xaml::IXamlRoot* xamlRoot)
{
    m_key = connectedTarget;

    m_systemBackdropNoRef = systemBackdrop;

    IFCFAILFAST(ctl::AsWeak(connectedTarget, &m_weakTargetICSB));

    IFCFAILFAST(ctl::AsWeak(xamlRoot, &m_weakXamlRoot));

    IFCFAILFAST(wf::ActivateInstance(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Composition_SystemBackdrops_SystemBackdropConfiguration).Get(),
        m_configuration.ReleaseAndGetAddressOf()));

    ctl::ComPtr<xaml_primitives::IPopup> associatedPopup;
    bool isPopup = SUCCEEDED(connectedTarget->QueryInterface(IID_PPV_ARGS(&associatedPopup)));

    // Hook up ActualThemeChanged event to "ThemeSource" element which is either <Popup> or DxamlRoot.Content (for DWXS/Window).
    ctl::ComPtr<xaml::IFrameworkElement> themeSourceFE;
    if (isPopup && associatedPopup.Get())
    {
        IFCFAILFAST(associatedPopup.As(&themeSourceFE));
    }
    else
    {
        ctl::ComPtr<xaml::IUIElement> contentElement;
        IFCFAILFAST(xamlRoot->get_Content(&contentElement));
        IFCFAILFAST(contentElement.As(&themeSourceFE));
    }

    IFCFAILFAST(ctl::AsWeak(themeSourceFE.Get(), &m_weakThemeSourceFE));

    if (themeSourceFE)
    {
        AttachActualThemeChanged();
        AttachHighContrastChanged();
    }

    ctl::ComPtr<XamlRoot> spXamlRoot;
    IFCFAILFAST(xamlRoot->QueryInterface(IID_PPV_ARGS(&spXamlRoot)));
    if (spXamlRoot)
    {
        AttachInputActivationChanged();
    }

    // For DWXS/Window, also hook up XamlRoot.Changed to re-wire policy when Content changes
    if (!isPopup)
    {
        AttachXamlRootChanged();
    }
}

// The event handler entry is going away (i.e. we've been disconnected from a target). Disconnect all event handlers.
SystemBackdrop::PerTargetConfigurationEntry::~PerTargetConfigurationEntry()
{
    RemoveXamlRootChanged();
    RemoveActualThemeChanged();
    RemoveHighContrastChanged();
    RemoveInputActivationChanged();

    m_weakThemeSourceFE.Reset();
    m_weakTargetICSB.Reset();
    m_weakXamlRoot.Reset();
}

void SystemBackdrop::PerTargetConfigurationEntry::AttachXamlRootChanged()
{
    FAIL_FAST_ASSERT(m_xamlRootChangedToken.value == 0);

    ctl::ComPtr<xaml::IXamlRoot> xamlRoot = m_weakXamlRoot.AsOrNull<xaml::IXamlRoot>();
    FAIL_FAST_ASSERT(xamlRoot);

    auto callback = wrl::Callback<wf::ITypedEventHandler<xaml::XamlRoot*, xaml::XamlRootChangedEventArgs*>>(
        this, &SystemBackdrop::PerTargetConfigurationEntry::OnXamlRootChanged);
    IFCFAILFAST(xamlRoot.Cast<XamlRoot>()->add_Changed(callback.Get(), &m_xamlRootChangedToken));
}

void SystemBackdrop::PerTargetConfigurationEntry::RemoveXamlRootChanged()
{
    ctl::ComPtr<xaml::IXamlRoot> source = m_weakXamlRoot.AsOrNull<xaml::IXamlRoot>();
    if (m_xamlRootChangedToken.value != 0 && source.Get())
    {
        IFCFAILFAST(source.Cast<XamlRoot>()->remove_Changed(m_xamlRootChangedToken));
        m_xamlRootChangedToken.value = 0;
    }
}

_Check_return_ HRESULT SystemBackdrop::PerTargetConfigurationEntry::OnXamlRootChanged(_In_ xaml::IXamlRoot* sender, _In_ xaml::IXamlRootChangedEventArgs* args)
{
    ctl::ComPtr<xaml::IFrameworkElement> currentContentFE = m_weakThemeSourceFE.AsOrNull<xaml::IFrameworkElement>();

    ctl::ComPtr<xaml::IUIElement> newContent;
    IFC_RETURN(sender->get_Content(&newContent));
    ctl::ComPtr<xaml::IFrameworkElement> newContentFE = newContent.AsOrNull<xaml::IFrameworkElement>();

    if (currentContentFE != newContentFE)
    {
        RemoveActualThemeChanged();
        RemoveHighContrastChanged();
        RemoveInputActivationChanged();

        IFC_RETURN(ctl::AsWeak(newContentFE.Get(), &m_weakThemeSourceFE));

        if (m_weakThemeSourceFE)
        {
            bool actualThemeChanged = AttachActualThemeChanged();
            bool highContrastChanged = AttachHighContrastChanged();
            bool inputActivationChanged = AttachInputActivationChanged();

            if (actualThemeChanged || highContrastChanged || inputActivationChanged)
            {
                m_systemBackdropNoRef->InvokeOnDefaultSystemBackdropConfigurationChanged(m_weakTargetICSB.AsOrNull<ixp::ICompositionSupportsSystemBackdrop>().Get(), m_weakXamlRoot.AsOrNull<xaml::IXamlRoot>().Get());
            }
        }
    }

    return S_OK;
}

bool SystemBackdrop::PerTargetConfigurationEntry::AttachActualThemeChanged()
{
    bool actualThemeChanged = false;

    // When we're attaching the event handler we expect the element to still exist.
    ctl::ComPtr<xaml::IFrameworkElement> themeSourceFE = m_weakThemeSourceFE.AsOrNull<xaml::IFrameworkElement>();
    FAIL_FAST_ASSERT(themeSourceFE);

    ASSERT(m_configuration.Get());
    ixp::SystemBackdrops::SystemBackdropTheme oldTheme;
    m_configuration->get_Theme(&oldTheme);

    xaml::ElementTheme newTheme;
    IFCFAILFAST(themeSourceFE->get_ActualTheme(&newTheme));

    if (static_cast<ixp::SystemBackdrops::SystemBackdropTheme>(newTheme) != oldTheme)
    {
        actualThemeChanged = true;
        SetTheme(newTheme);
    }

    FAIL_FAST_ASSERT(m_actualThemeChangedToken.value == 0);
    auto callback = wrl::Callback<wf::ITypedEventHandler<xaml::FrameworkElement*, IInspectable*>>(
        this, &SystemBackdrop::PerTargetConfigurationEntry::OnActualThemeChanged);
    IFCFAILFAST(themeSourceFE.Cast<FrameworkElement>()->add_ActualThemeChanged(callback.Get(), &m_actualThemeChangedToken));

    return actualThemeChanged;
}

void SystemBackdrop::PerTargetConfigurationEntry::RemoveActualThemeChanged()
{
    ctl::ComPtr<xaml::IFrameworkElement> source = m_weakThemeSourceFE.AsOrNull<xaml::IFrameworkElement>();
    if (m_actualThemeChangedToken.value != 0 && source.Get())
    {
        IFCFAILFAST(source.Cast<FrameworkElement>()->remove_ActualThemeChanged(m_actualThemeChangedToken));
    }

    m_actualThemeChangedToken.value = 0;
}

_Check_return_ HRESULT SystemBackdrop::PerTargetConfigurationEntry::OnActualThemeChanged(_In_ xaml::IFrameworkElement* sender, _In_ IInspectable* args)
{
    wrl::ComPtr<xaml::IFrameworkElement> senderAsFE;
    IFC_RETURN(sender->QueryInterface<xaml::IFrameworkElement>(&senderAsFE));

    xaml::ElementTheme actualTheme;
    IFC_RETURN(senderAsFE->get_ActualTheme(&actualTheme));
    SetTheme(actualTheme);

    IFC_RETURN(m_systemBackdropNoRef->InvokeOnDefaultSystemBackdropConfigurationChanged(m_weakTargetICSB.AsOrNull<ixp::ICompositionSupportsSystemBackdrop>().Get(), m_weakXamlRoot.AsOrNull<xaml::IXamlRoot>().Get()));

    return S_OK;
}

void SystemBackdrop::PerTargetConfigurationEntry::SetTheme(xaml::ElementTheme xamlTheme)
{
    static_assert(xaml::ElementTheme_Default == ixp::SystemBackdrops::SystemBackdropTheme_Default, "xaml::ElementTheme and ixp::SystemBackdrops::SystemBackdropTheme enums mismatched for Default");
    static_assert(xaml::ElementTheme_Light == ixp::SystemBackdrops::SystemBackdropTheme_Light, "xaml::ElementTheme and ixp::SystemBackdrops::SystemBackdropTheme enums mismatched for Light");
    static_assert(xaml::ElementTheme_Dark == ixp::SystemBackdrops::SystemBackdropTheme_Dark, "xaml::ElementTheme and ixp::SystemBackdrops::SystemBackdropTheme enums mismatched for Dark");

    ASSERT(m_configuration.Get());
    m_configuration->put_Theme(static_cast<ixp::SystemBackdrops::SystemBackdropTheme>(xamlTheme));
}

bool SystemBackdrop::PerTargetConfigurationEntry::AttachHighContrastChanged()
{
    bool highContrastChanged = false;

    // In the future, we plan on adding a global Accessibility event for HighContrast. In the meantime, we are getting
    // HighContrast from FrameworkElement.

    // When we're attaching the event handler we expect the element to still exist.
    ctl::ComPtr<xaml::IFrameworkElement> themeSourceFE = m_weakThemeSourceFE.AsOrNull<xaml::IFrameworkElement>();
    FAIL_FAST_ASSERT(themeSourceFE);
    ASSERT(m_configuration.Get());

    BOOLEAN oldIsHighContrast;
    m_configuration->get_IsHighContrast(&oldIsHighContrast);

    BOOLEAN newIsHighContrast = DXamlCore::GetCurrent()->GetHandle()->GetFrameworkTheming()->HasHighContrastTheme();
    if (newIsHighContrast != oldIsHighContrast)
    {
        highContrastChanged = true;
        SetHighContrast(newIsHighContrast);
    }

    FAIL_FAST_ASSERT(m_highContrastChangedToken.value == 0);
    auto callback = wrl::Callback<wf::ITypedEventHandler<xaml::FrameworkElement*, IInspectable*>>(
        this, &SystemBackdrop::PerTargetConfigurationEntry::OnHighContrastChanged);
    IFCFAILFAST(themeSourceFE.Cast<FrameworkElement>()->add_HighContrastChanged(callback.Get(), &m_highContrastChangedToken));

    return highContrastChanged;
}

void SystemBackdrop::PerTargetConfigurationEntry::RemoveHighContrastChanged()
{
    ctl::ComPtr<xaml::IFrameworkElement> source = m_weakThemeSourceFE.AsOrNull<xaml::IFrameworkElement>();
    if (m_highContrastChangedToken.value != 0 && source.Get())
    {
        IFCFAILFAST(source.Cast<FrameworkElement>()->remove_HighContrastChanged(m_highContrastChangedToken));
    }

    m_highContrastChangedToken.value = 0;
}

_Check_return_ HRESULT SystemBackdrop::PerTargetConfigurationEntry::OnHighContrastChanged(_In_ xaml::IFrameworkElement* sender, _In_ IInspectable* args)
{
    bool isHighContrast = DXamlCore::GetCurrent()->GetHandle()->GetFrameworkTheming()->HasHighContrastTheme();
    SetHighContrast(isHighContrast);

    IFC_RETURN(m_systemBackdropNoRef->InvokeOnDefaultSystemBackdropConfigurationChanged(m_weakTargetICSB.AsOrNull<ixp::ICompositionSupportsSystemBackdrop>().Get(), m_weakXamlRoot.AsOrNull<xaml::IXamlRoot>().Get()));

    return S_OK;
}

void SystemBackdrop::PerTargetConfigurationEntry::SetHighContrast(bool isHighContrast)
{
    ASSERT(m_configuration.Get());
    m_configuration->put_IsHighContrast(isHighContrast);
}

bool SystemBackdrop::PerTargetConfigurationEntry::AttachInputActivationChanged()
{
    bool inputActivationChanged = false;

    ctl::ComPtr<xaml::IXamlRoot> xamlRoot = m_weakXamlRoot.AsOrNull<xaml::IXamlRoot>();
    FAIL_FAST_ASSERT(xamlRoot);

    ASSERT(m_configuration.Get());
    BOOLEAN oldIsInputActive;
    m_configuration->get_IsInputActive(&oldIsInputActive);

    BOOLEAN newIsInputActive;
    xamlRoot.Cast<XamlRoot>()->get_IsInputActive(&newIsInputActive);

    if (newIsInputActive != oldIsInputActive)
    {
        inputActivationChanged = true;
        SetInputActivation(newIsInputActive);
    }

    FAIL_FAST_ASSERT(m_inputActivationChangedToken.value == 0);
    auto callback = wrl::Callback<wf::ITypedEventHandler<xaml::XamlRoot*, xaml::XamlRootChangedEventArgs*>>(
        this, &SystemBackdrop::PerTargetConfigurationEntry::OnInputActivationChanged);
    IFCFAILFAST(xamlRoot.Cast<XamlRoot>()->add_InputActivationChanged(callback.Get(), &m_inputActivationChangedToken));

    return inputActivationChanged;
}

void SystemBackdrop::PerTargetConfigurationEntry::RemoveInputActivationChanged()
{
    ctl::ComPtr<xaml::IXamlRoot> source = m_weakXamlRoot.AsOrNull<xaml::IXamlRoot>();
    if (m_inputActivationChangedToken.value != 0 && source.Get())
    {
        IFCFAILFAST(source.Cast<XamlRoot>()->remove_InputActivationChanged(m_inputActivationChangedToken));
    }

    m_inputActivationChangedToken.value = 0;
}

_Check_return_ HRESULT SystemBackdrop::PerTargetConfigurationEntry::OnInputActivationChanged(_In_ xaml::IXamlRoot* sender, _In_ xaml::IXamlRootChangedEventArgs* args)
{
    ctl::ComPtr<XamlRoot> spXamlRoot;
    IFCFAILFAST(sender->QueryInterface(IID_PPV_ARGS(&spXamlRoot)));

    BOOLEAN isInputActive;
    spXamlRoot->get_IsInputActive(&isInputActive);
    SetInputActivation(isInputActive);

    IFC_RETURN(m_systemBackdropNoRef->InvokeOnDefaultSystemBackdropConfigurationChanged(m_weakTargetICSB.AsOrNull<ixp::ICompositionSupportsSystemBackdrop>().Get(), m_weakXamlRoot.AsOrNull<xaml::IXamlRoot>().Get()));

    return S_OK;
}

void SystemBackdrop::PerTargetConfigurationEntry::SetInputActivation(bool isInputActive)
{
    ASSERT(m_configuration.Get());
    m_configuration->put_IsInputActive(isInputActive);
}