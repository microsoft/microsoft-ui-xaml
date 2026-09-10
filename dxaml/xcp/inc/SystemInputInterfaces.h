// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

XAML_ABI_NAMESPACE_BEGIN
namespace Microsoft
{
    namespace UI
    {
        namespace Input
        {
            interface IInputPointerSource;
        }
    }
}
XAML_ABI_NAMESPACE_END

// The transport package does not yet contain the system-visual overloads
// implemented by the paired OS experiment branch.
namespace SystemInputAbi
{
    MIDL_INTERFACE("d1ba3784-555e-5749-b2f5-cc8456cd3995")
    IInputPointerSourceStatics : public IInspectable
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetForSystemVisual(
            XAML_ABI_PARAMETER(Windows::UI::Composition::IVisual)* visual,
            XAML_ABI_PARAMETER(Microsoft::UI::Input::IInputPointerSource)** result) = 0;

        virtual HRESULT STDMETHODCALLTYPE RemoveForSystemVisual(
            XAML_ABI_PARAMETER(Windows::UI::Composition::IVisual)* visual) = 0;
    };
}
