// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
//------------------------------------------------------------------------
//
//  Abstract:
//
//      XAML types.
//      NOTE: This file was generated by a tool.
//
//------------------------------------------------------------------------

#pragma once

#include "ContentControl.g.h"

#define __WindowChrome_GUID "b2e03930-7cb2-4feb-a4b9-a38498b9d75e"

namespace DirectUI
{
    class WindowChrome;

    class __declspec(novtable) WindowChromeGenerated:
        public DirectUI::ContentControl
        , public ABI::Microsoft::UI::Xaml::IWindowChrome
    {
        friend class DirectUI::WindowChrome;

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.WindowChrome");

        BEGIN_INTERFACE_MAP(WindowChromeGenerated, DirectUI::ContentControl)
            INTERFACE_ENTRY(WindowChromeGenerated, ABI::Microsoft::UI::Xaml::IWindowChrome)
        END_INTERFACE_MAP(WindowChromeGenerated, DirectUI::ContentControl)

    public:
        WindowChromeGenerated();
        ~WindowChromeGenerated() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::WindowChrome;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::WindowChrome;
        }

        // Properties.
        IFACEMETHOD(get_CaptionVisibility)(_Out_ ABI::Microsoft::UI::Xaml::Visibility* pValue) override;
        _Check_return_ HRESULT put_CaptionVisibility(ABI::Microsoft::UI::Xaml::Visibility value);

        // Events.

        // Methods.

        IFACEMETHOD(OnApplyTemplate)() override;

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:

        // Fields.
        TrackerPtr<ABI::Microsoft::UI::Xaml::Controls::Primitives::IButtonBase> m_tpCloseButtonPart;
        TrackerPtr<ABI::Microsoft::UI::Xaml::IUIElement> m_tpLayoutRootPart;
        TrackerPtr<ABI::Microsoft::UI::Xaml::Controls::Primitives::IButtonBase> m_tpMaximizeButtonPart;
        TrackerPtr<ABI::Microsoft::UI::Xaml::Controls::Primitives::IButtonBase> m_tpMinimizeButtonPart;
        TrackerPtr<ABI::Microsoft::UI::Xaml::IUIElement> m_tpMinMaxCloseContainerPart;
        TrackerPtr<ABI::Microsoft::UI::Xaml::IUIElement> m_tpTitleBarMinMaxCloseContainerPart;
    };
}

#include "WindowChrome_Partial.h"

namespace DirectUI
{
    // Note that the ordering of the base types here is important - the base factory comes first, followed by all the
    // interfaces specific to this type.  By doing this, we allow every Factory's CreateInstance method to be more
    // COMDAT-folding-friendly.  Because this ensures that the first vfptr contains GetTypeIndex, it means that all
    // CreateInstance functions with the same base factory generate the same assembly instructions and thus will
    // fold together.  This is significant for binary size in Microsoft.UI.Xaml.dll so change this only with great
    // care.
    class __declspec(novtable) WindowChromeFactory:
       public ctl::AbstractActivationFactory
        , public ABI::Microsoft::UI::Xaml::IWindowChromeFactory
        , public ABI::Microsoft::UI::Xaml::IWindowChromeStatics
    {
        BEGIN_INTERFACE_MAP(WindowChromeFactory, ctl::AbstractActivationFactory)
            INTERFACE_ENTRY(WindowChromeFactory, ABI::Microsoft::UI::Xaml::IWindowChromeFactory)
            INTERFACE_ENTRY(WindowChromeFactory, ABI::Microsoft::UI::Xaml::IWindowChromeStatics)
        END_INTERFACE_MAP(WindowChromeFactory, ctl::AbstractActivationFactory)

    public:
        // Factory methods.
        IFACEMETHOD(CreateInstance)(_In_ ABI::Microsoft::UI::Xaml::IWindow* pParent, _Outptr_ ABI::Microsoft::UI::Xaml::IWindowChrome** ppInstance);

        // Static properties.

        // Dependency properties.
        IFACEMETHOD(get_CaptionVisibilityProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;

        // Attached properties.

        // Static methods.

        // Static events.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::WindowChrome;
        }


    private:
        _Check_return_ HRESULT CreateInstanceImpl(_In_ ABI::Microsoft::UI::Xaml::IWindow* pParent, _Outptr_ ABI::Microsoft::UI::Xaml::IWindowChrome** ppInstance);

        // Customized static properties.

        // Customized static  methods.
    };
}
