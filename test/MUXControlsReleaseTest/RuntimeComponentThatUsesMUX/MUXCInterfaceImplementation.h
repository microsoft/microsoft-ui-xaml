#pragma once

#include "winrt/Windows.UI.Xaml.h"
#include "winrt/Windows.UI.Xaml.Markup.h"
#include "winrt/Windows.UI.Xaml.Controls.Primitives.h"
#include "winrt/Microsoft.UI.Xaml.Controls.h"
#include "MUXCInterfaceImplementation.g.h"

namespace winrt::RuntimeComponentThatUsesMUX::implementation
{
    struct MUXCInterfaceImplementation : MUXCInterfaceImplementationT<MUXCInterfaceImplementation>
    {
        MUXCInterfaceImplementation();

        winrt::Microsoft::UI::Xaml::Controls::IAnimatedVisual TryCreateAnimatedVisual(
            winrt::Windows::UI::Composition::Compositor const& compositor,
            winrt::Windows::Foundation::IInspectable& diagnostics);
    };
}

namespace winrt::RuntimeComponentThatUsesMUX::factory_implementation
{
    struct MUXCInterfaceImplementation : MUXCInterfaceImplementationT<MUXCInterfaceImplementation, implementation::MUXCInterfaceImplementation>
    {
    };
}
