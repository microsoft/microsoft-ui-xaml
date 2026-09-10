// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

XAML_ABI_NAMESPACE_BEGIN
namespace Microsoft
{
    namespace UI
    {
        struct WindowId;

        namespace Content
        {
            interface IContentIsland;
            interface IDesktopChildSiteBridge;
        }
    }
}
XAML_ABI_NAMESPACE_END

// The transport package defines interfaces with the same API names but lifted
// DispatcherQueue parameters and therefore different IIDs. Use experiment-only
// interface names for the system ABI implemented by the paired OS branch.
namespace SystemContentAbi
{
    MIDL_INTERFACE("e4245e0b-ebec-5667-9c97-3896d2b1f4aa")
    IContentIslandRoot : public IInspectable
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetSystemVisualRoot(
            XAML_ABI_PARAMETER(Windows::UI::Composition::IVisual)* root) = 0;
    };

    MIDL_INTERFACE("0dbbd079-769c-5d62-9b31-f963cfe1bddb")
    IContentIslandStatics : public IInspectable
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateForSystemVisual(
            XAML_ABI_PARAMETER(Windows::System::IDispatcherQueue)* queue,
            XAML_ABI_PARAMETER(Windows::UI::Composition::IVisual)* root,
            XAML_ABI_PARAMETER(Microsoft::UI::Content::IContentIsland)** result) = 0;
    };

    MIDL_INTERFACE("2d785b66-79fa-5a24-9a4a-4da47348e655")
    IDesktopChildSiteBridgeStatics : public IInspectable
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateWithDispatcherQueue(
            XAML_ABI_PARAMETER(Windows::System::IDispatcherQueue)* queue,
            XAML_ABI_PARAMETER(Microsoft::UI::WindowId) parentWindowId,
            XAML_ABI_PARAMETER(Microsoft::UI::Content::IDesktopChildSiteBridge)** result) = 0;
    };
}
