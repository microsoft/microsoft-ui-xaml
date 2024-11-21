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

#include "EasingFunctionBase.g.h"

#define __BackEase_GUID "75780e85-dd59-4b56-ad01-78a18ec799e1"

namespace DirectUI
{
    class BackEase;

    class __declspec(novtable) __declspec(uuid(__BackEase_GUID)) BackEase:
        public DirectUI::EasingFunctionBase
        , public ABI::Microsoft::UI::Xaml::Media::Animation::IBackEase
    {

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Media.Animation.BackEase");

        BEGIN_INTERFACE_MAP(BackEase, DirectUI::EasingFunctionBase)
            INTERFACE_ENTRY(BackEase, ABI::Microsoft::UI::Xaml::Media::Animation::IBackEase)
        END_INTERFACE_MAP(BackEase, DirectUI::EasingFunctionBase)

    public:
        BackEase();
        ~BackEase() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::BackEase;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::BackEase;
        }

        // Properties.
        IFACEMETHOD(get_Amplitude)(_Out_ DOUBLE* pValue) override;
        IFACEMETHOD(put_Amplitude)(DOUBLE value) override;

        // Events.

        // Methods.


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:
        // Customized properties.

        // Customized methods.

        // Fields.
    };
}


namespace DirectUI
{
    // Note that the ordering of the base types here is important - the base factory comes first, followed by all the
    // interfaces specific to this type.  By doing this, we allow every Factory's CreateInstance method to be more
    // COMDAT-folding-friendly.  Because this ensures that the first vfptr contains GetTypeIndex, it means that all
    // CreateInstance functions with the same base factory generate the same assembly instructions and thus will
    // fold together.  This is significant for binary size in Microsoft.UI.Xaml.dll so change this only with great
    // care.
    class __declspec(novtable) BackEaseFactory:
       public ctl::BetterCoreObjectActivationFactory
        , public ABI::Microsoft::UI::Xaml::Media::Animation::IBackEaseStatics
    {
        BEGIN_INTERFACE_MAP(BackEaseFactory, ctl::BetterCoreObjectActivationFactory)
            INTERFACE_ENTRY(BackEaseFactory, ABI::Microsoft::UI::Xaml::Media::Animation::IBackEaseStatics)
        END_INTERFACE_MAP(BackEaseFactory, ctl::BetterCoreObjectActivationFactory)

    public:
        // Factory methods.

        // Static properties.

        // Dependency properties.
        IFACEMETHOD(get_AmplitudeProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;

        // Attached properties.

        // Static methods.

        // Static events.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::BackEase;
        }


    private:

        // Customized static properties.

        // Customized static  methods.
    };
}
