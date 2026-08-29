// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"
#include "CustomMediaPlayerElement.h"
#if __has_include("CustomMediaPlayerElement.g.cpp")
#include "CustomMediaPlayerElement.g.cpp"
#endif
#if __has_include("CustomMediaPlayerElementDragProvider.g.cpp")
#include "CustomMediaPlayerElementDragProvider.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI, the WinUI project structure,
// and project templates, see the WinUI documentation.

namespace winrt::WinUICppDesktopSampleApp::implementation
{
    CustomMediaPlayerElement::CustomMediaPlayerElement()
    {
        DefaultStyleKey(winrt::box_value(L"WinUICppDesktopSampleApp.CustomMediaPlayerElement"));
    }
}