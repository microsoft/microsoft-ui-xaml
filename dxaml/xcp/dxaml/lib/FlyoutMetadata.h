// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//    FlyoutMetadata - Storage class in DXamlcore to enforce one Flyout
//      at a time requirement.

#pragma once

namespace DirectUI
{
    class FlyoutBase;

    // FlyoutMetadata is used to keep track of currently open flyout as well
    // as stage flyouts while waiting for an open one to close.
    // The instance on DXamlCore tracks the root flyout, of which there can
    // be only 1 open at a time.
    // However, another flyout can be opened if its placement target is within
    // an already opened flyout presenter's tree.
    // In those cases, instances of FlyoutMetadata are used to track the child
    // and staged child flyouts for any open flyout.
    class FlyoutMetadata
        : public ctl::WeakReferenceSource
    {
    public:
        _Check_return_ HRESULT GetOpenFlyout(
            _Out_ xaml_primitives::IFlyoutBase** ppFlyout,
            _Out_opt_ xaml::IFrameworkElement** ppPlacementTarget);

    private:
        // Grant access to Flyout storage methods.
        friend class DirectUI::FlyoutBase;

        void SetOpenFlyout(
            _In_ xaml_primitives::IFlyoutBase* pFlyout,
            _In_ xaml::IFrameworkElement* pPlacementTarget);

        void SetStagedFlyout(
            _In_ xaml_primitives::IFlyoutBase* pFlyout,
            _In_ xaml::IFrameworkElement* pPlacementTarget);

        _Check_return_ HRESULT GetStagedFlyout(
            _Out_ xaml_primitives::IFlyoutBase** ppFlyout,
            _Out_opt_ xaml::IFrameworkElement** ppPlacementTarget);

        TrackerPtr<xaml_primitives::IFlyoutBase> m_tpOpenFlyout;
        TrackerPtr<xaml::IFrameworkElement> m_tpOpenFlyoutPlacementTarget;

        TrackerPtr<xaml_primitives::IFlyoutBase> m_tpStagedFlyout;
        TrackerPtr<xaml::IFrameworkElement> m_tpStagedFlyoutTarget;
    };
}
