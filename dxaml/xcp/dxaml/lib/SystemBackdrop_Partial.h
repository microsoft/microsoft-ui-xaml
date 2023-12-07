// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SystemBackdrop.g.h"
#include "XamlRoot.g.h"
#include "EventCallbacks.h"
#include <Microsoft.UI.Composition.h>

namespace DirectUI
{
    PARTIAL_CLASS(SystemBackdrop)
    {
    public:
        static _Check_return_ HRESULT InvokeOnTargetConnectedFromCore(_In_ CDependencyObject* object, _In_ ixp::ICompositionSupportsSystemBackdrop* connectedTarget, _In_opt_ xaml::IXamlRoot* xamlRoot);
        static _Check_return_ HRESULT InvokeOnTargetDisconnectedFromCore(_In_ CDependencyObject* object, _In_ ixp::ICompositionSupportsSystemBackdrop* disconnectedTarget);

        _Check_return_ HRESULT InvokeOnTargetConnected(_In_ ixp::ICompositionSupportsSystemBackdrop* connectedTarget, _In_opt_ xaml::IXamlRoot* xamlRoot);
        _Check_return_ HRESULT InvokeOnTargetDisconnected(_In_ ixp::ICompositionSupportsSystemBackdrop* disconnectedTarget);
        _Check_return_ HRESULT InvokeOnDefaultSystemBackdropConfigurationChanged(_In_ ixp::ICompositionSupportsSystemBackdrop* target, _In_opt_ xaml::IXamlRoot* xamlRoot);

        _Check_return_ HRESULT OnTargetConnectedImpl(_In_ ixp::ICompositionSupportsSystemBackdrop* connectedTarget, _In_opt_ xaml::IXamlRoot* xamlRoot);
        _Check_return_ HRESULT OnTargetDisconnectedImpl(_In_ ixp::ICompositionSupportsSystemBackdrop* disconnectedTarget);
        _Check_return_ HRESULT OnDefaultSystemBackdropConfigurationChangedImpl(_In_ ixp::ICompositionSupportsSystemBackdrop* target, _In_opt_ xaml::IXamlRoot* xamlRoot);

        _Check_return_ HRESULT GetDefaultSystemBackdropConfigurationImpl(_In_ ixp::ICompositionSupportsSystemBackdrop* pTarget, _In_ xaml::IXamlRoot* pXamlRoot, _Outptr_ ixp::SystemBackdrops::ISystemBackdropConfiguration** ppResult);

    private:
        //
        // SystemBackdrop attaches event handlers to the tree to respond to theme, high contrast, and input activation
        // changes. There's a separate set of event handlers for each place where this SystemBackdrop is attached, which
        // we keep in a vector. Note that SystemBackdrop can also be attached to FlyoutBase, but that just forwards the
        // object to the underlying popup. Cascading menus will all share the same SystemBackdrop object (set on the
        // top-level menu).
        //
        // This class keeps track of those event handlers, and more importantly, keeps and updates the IXP configuration
        // object that controls the backdrop effect's theme, high contrast fallback, and whether it's enabled.
        //
        class PerTargetConfigurationEntry
        {
        public:
            PerTargetConfigurationEntry(_In_ SystemBackdrop* systemBackdrop, _In_ ixp::ICompositionSupportsSystemBackdrop* connectedTarget, _In_ xaml::IXamlRoot* xamlRoot);
            ~PerTargetConfigurationEntry();

            // Block copy and assignment. This class is meant to be constructed in-place in the vector.
            PerTargetConfigurationEntry(const PerTargetConfigurationEntry& other) = delete;
            PerTargetConfigurationEntry& operator=(const PerTargetConfigurationEntry& other) = delete;

            void AttachXamlRootChanged();
            void RemoveXamlRootChanged();
            _Check_return_ HRESULT OnXamlRootChanged(_In_ xaml::IXamlRoot* sender, _In_ xaml::IXamlRootChangedEventArgs* args);

            bool AttachActualThemeChanged();
            void RemoveActualThemeChanged();
            _Check_return_ HRESULT OnActualThemeChanged(_In_ xaml::IFrameworkElement* sender, _In_ IInspectable* args);
            void SetTheme(xaml::ElementTheme xamlTheme);

            bool AttachHighContrastChanged();
            void RemoveHighContrastChanged();
            _Check_return_ HRESULT OnHighContrastChanged(_In_ xaml::IFrameworkElement* sender, _In_ IInspectable* args);
            void SetHighContrast(bool isHighContrast);

            bool AttachInputActivationChanged();
            void RemoveInputActivationChanged();
            _Check_return_ HRESULT OnInputActivationChanged(_In_ xaml::IXamlRoot* sender, _In_ xaml::IXamlRootChangedEventArgs* args);
            void SetInputActivation(bool isInputActive);

            // The key of the map. These entries are actually stored in a vector to avoid heap allocations of std::map,
            // so bundle the key in the entry and do a linear lookup.
            ixp::ICompositionSupportsSystemBackdrop* m_key;

            // The IXP configuration object responsible for turning the system backdrop on/off in response to theme/high contrast changes.
            ctl::ComPtr<ixp::SystemBackdrops::ISystemBackdropConfiguration> m_configuration;

            // This entry lives in the SystemBackdrop's event handler table and its lifetime is entirely dependent on
            // the SystemBackdrop being alive.
            SystemBackdrop* m_systemBackdropNoRef { nullptr };

            // The Xaml object that implements ixp::ICompositionSupportsSystemBackdrop (window/island/popup)
            ctl::WeakRefPtr m_weakTargetICSB;

            ctl::WeakRefPtr m_weakXamlRoot;

            // Used for Theme/high contrast changes
            ctl::WeakRefPtr m_weakThemeSourceFE;

            EventRegistrationToken m_xamlRootChangedToken{};
            EventRegistrationToken m_actualThemeChangedToken{};
            EventRegistrationToken m_highContrastChangedToken{};
            EventRegistrationToken m_inputActivationChangedToken{};
        };

        //
        // A list of all targets that we're connected to. This list is expected to be small (SystemBackdrop is used in
        // windows, islands, and popups/flyouts, so <10 targets are expected at any time), so we'll avoid the heap
        // allocations of std::map and just search through it linearly. Note that this is a list of std::unique_ptr
        // because PerTargetConfigurationEntry blocks assignment and copy, which std::vector::erase needs. If we allow
        // those operations then PerTargetConfigurationEntry can be inlined into this vector directly and save the heap
        // allocations of std::unique_ptr.
        //
        // Note: There's a corresponding list of controller objects in the derived classes (e.g. for multiple IXP
        // MicaControllers). They are also keyed by the target.
        //
        std::vector<std::unique_ptr<PerTargetConfigurationEntry>> m_perTargetConfigurationList;
    };
}
