// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#pragma once

#include "winrt/Microsoft.UI.Xaml.h"
#include "winrt/Microsoft.UI.Xaml.Markup.h"
#include "winrt/Microsoft.UI.Xaml.Controls.Primitives.h"
#include "CustomMediaPlayerElement.g.h"
#include "CustomMediaPlayerElementDragProvider.g.h"

namespace winrt::WinUICppDesktopSampleApp::implementation
{
    struct CustomMediaPlayerElement : CustomMediaPlayerElementT<CustomMediaPlayerElement>
    {
        CustomMediaPlayerElement();
    };

    struct CustomMediaPlayerElementDragProvider : CustomMediaPlayerElementDragProviderT<CustomMediaPlayerElementDragProvider>
    {
        CustomMediaPlayerElementDragProvider() = default;

        bool IsGrabbed() { throw hresult_not_implemented(); }
        hstring DropEffect() { throw hresult_not_implemented(); }
        com_array<hstring> DropEffects() { throw hresult_not_implemented(); }
        com_array<winrt::Microsoft::UI::Xaml::Automation::Provider::IRawElementProviderSimple> GetGrabbedItems() { throw hresult_not_implemented(); }
    };
}

namespace winrt::WinUICppDesktopSampleApp::factory_implementation
{
    struct CustomMediaPlayerElement : CustomMediaPlayerElementT<CustomMediaPlayerElement, implementation::CustomMediaPlayerElement>
    {
    };

    struct CustomMediaPlayerElementDragProvider : CustomMediaPlayerElementDragProviderT<CustomMediaPlayerElementDragProvider, implementation::CustomMediaPlayerElementDragProvider>
    {
    };
}
