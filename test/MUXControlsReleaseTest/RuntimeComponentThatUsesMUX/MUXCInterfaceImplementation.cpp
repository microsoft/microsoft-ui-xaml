#include "pch.h"
#include "MUXCInterfaceImplementation.h"
#if __has_include("MUXCInterfaceImplementation.g.cpp")
#include "MUXCInterfaceImplementation.g.cpp"
#endif

using namespace winrt;
using namespace Windows::UI::Xaml;

namespace winrt::RuntimeComponentThatUsesMUX::implementation
{
    MUXCInterfaceImplementation::MUXCInterfaceImplementation()
    {
    }

    winrt::Microsoft::UI::Xaml::Controls::IAnimatedVisual MUXCInterfaceImplementation::TryCreateAnimatedVisual(
        winrt::Windows::UI::Composition::Compositor const&,
        winrt::Windows::Foundation::IInspectable&)
    {
        return nullptr;
    }
}
