// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

enum XamlBehavior
{
    // On Windows Phone, FlyoutBase should not present its content with any margin from the edge of the screen.
    FlyoutBase_NoMargins,

    // On Windows Phone, flyout will not be light dismissed in Full mode
    FlyoutBase_NoLightDismissOnFull,

    // Phone doesn't have all HWND features, like non-fullscreen
    AllHwndFeatures_NotSupported,

    // Enable ListViewBase.ReorderMode on Phone
    ListViewBase_EnableListViewReorderMode,

    // OneCore devices do not receive WM_SETFOCUS/WM_KILLFOCUS messages. Instead we
    // listen to CoreWindow's OnActivated event and set plugin focus accordingly.
    JupiterWindow_PluginFocusFromActivated,

    // Devices with TSF3 require KeyEventServiceImpl to properly handle Key Events.
    JupiterWindow_UseTextInputProducer,

    // Use the OneCore version of FrameworkInputPane.
    FrameworkInputPaneOneCore_Enable,
};

// Deprecated.  Try hard to converge your feature's behavior!
bool IsXamlBehaviorEnabledForCurrentSku(enum XamlBehavior behavior);
