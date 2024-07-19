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

#include "DoubleKeyFrame.g.h"

#define __EasingDoubleKeyFrame_GUID "d296df57-dd91-4ae7-98fc-58b6eaba7f66"

namespace DirectUI
{
    class EasingDoubleKeyFrame;
    class EasingFunctionBase;

    class __declspec(novtable) __declspec(uuid(__EasingDoubleKeyFrame_GUID)) EasingDoubleKeyFrame:
        public DirectUI::DoubleKeyFrame
        , public ABI::Microsoft::UI::Xaml::Media::Animation::IEasingDoubleKeyFrame
    {

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Media.Animation.EasingDoubleKeyFrame");

        BEGIN_INTERFACE_MAP(EasingDoubleKeyFrame, DirectUI::DoubleKeyFrame)
            INTERFACE_ENTRY(EasingDoubleKeyFrame, ABI::Microsoft::UI::Xaml::Media::Animation::IEasingDoubleKeyFrame)
        END_INTERFACE_MAP(EasingDoubleKeyFrame, DirectUI::DoubleKeyFrame)

    public:
        EasingDoubleKeyFrame();
        ~EasingDoubleKeyFrame() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::EasingDoubleKeyFrame;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::EasingDoubleKeyFrame;
        }

        // Properties.
        IFACEMETHOD(get_EasingFunction)(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::Media::Animation::IEasingFunctionBase** ppValue) override;
        IFACEMETHOD(put_EasingFunction)(_In_opt_ ABI::Microsoft::UI::Xaml::Media::Animation::IEasingFunctionBase* pValue) override;

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
    class __declspec(novtable) EasingDoubleKeyFrameFactory:
       public ctl::BetterCoreObjectActivationFactory
        , public ABI::Microsoft::UI::Xaml::Media::Animation::IEasingDoubleKeyFrameStatics
    {
        BEGIN_INTERFACE_MAP(EasingDoubleKeyFrameFactory, ctl::BetterCoreObjectActivationFactory)
            INTERFACE_ENTRY(EasingDoubleKeyFrameFactory, ABI::Microsoft::UI::Xaml::Media::Animation::IEasingDoubleKeyFrameStatics)
        END_INTERFACE_MAP(EasingDoubleKeyFrameFactory, ctl::BetterCoreObjectActivationFactory)

    public:
        // Factory methods.

        // Static properties.

        // Dependency properties.
        IFACEMETHOD(get_EasingFunctionProperty)(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue) override;

        // Attached properties.

        // Static methods.

        // Static events.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::EasingDoubleKeyFrame;
        }


    private:

        // Customized static properties.

        // Customized static  methods.
    };
}
